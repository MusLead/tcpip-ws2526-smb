#include "smb.h"

typedef struct {
    char topic[TOPIC_MAX];
    struct sockaddr_in addr; // subscriber IP
    uint16_t port;           // subscriber listening port (from SUB message)
} subscription_t;

static subscription_t *subs = NULL;
static size_t subs_len = 0;
static size_t subs_cap = 0;

static void subs_add_or_update(const char *topic, const struct sockaddr_in *src_addr, uint16_t port) {
    // If same IP:port + topic already exists, just update (idempotent subscribe)
    for (size_t i = 0; i < subs_len; i++) {
        if (subs[i].port == port &&
            subs[i].addr.sin_addr.s_addr == src_addr->sin_addr.s_addr &&
            strncmp(subs[i].topic, topic, TOPIC_MAX) == 0) {
            subs[i].addr = *src_addr;
            subs[i].port = port;
            return;
        }
    }

    if (subs_len == subs_cap) {
        size_t new_cap = (subs_cap == 0) ? 16 : subs_cap * 2;
        subscription_t *p = realloc(subs, new_cap * sizeof(*subs));
        if (!p) die("realloc");
        subs = p;
        subs_cap = new_cap;
    }

    memset(&subs[subs_len], 0, sizeof(subs[subs_len]));
    snprintf(subs[subs_len].topic, TOPIC_MAX, "%s", topic);
    subs[subs_len].addr = *src_addr;
    subs[subs_len].port = port;
    subs_len++;
}

static int topic_matches(const char *sub_topic, const char *pub_topic) {
    // Only feature required: '#' means "receive all"
    if (strcmp(sub_topic, "#") == 0) return 1;
    return strcmp(sub_topic, pub_topic) == 0;
}

static void print_addr(const struct sockaddr_in *a, char *buf, size_t buflen) {
    char ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &a->sin_addr, ip, sizeof(ip));
    snprintf(buf, buflen, "%s:%u", ip, (unsigned)ntohs(a->sin_port));
}

int main(int argc, char **argv) {
    int port = BROKER_PORT;
    if (argc == 2) {
        port = atoi(argv[1]);
        if (port <= 0 || port > 65535) {
            fprintf(stderr, "Usage: %s [port]\n", argv[0]);
            return EXIT_FAILURE;
        }
    } else if (argc != 1) {
        fprintf(stderr, "Usage: %s [port]\n", argv[0]);
        return EXIT_FAILURE;
    }

    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) die("socket");

    struct sockaddr_in bind_addr;
    memset(&bind_addr, 0, sizeof(bind_addr));
    bind_addr.sin_family = AF_INET;
    bind_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    bind_addr.sin_port = htons((uint16_t)port);

    if (bind(sock, (struct sockaddr *)&bind_addr, sizeof(bind_addr)) < 0) die("bind");

    printf("smbbroker listening on UDP port %d\n", port);
    fflush(stdout);

    for (;;) {
        char packet[PACKET_MAX];
        struct sockaddr_in src;
        socklen_t srclen = sizeof(src);

        ssize_t n = recvfrom(sock, packet, sizeof(packet) - 1, 0, (struct sockaddr *)&src, &srclen);
        if (n < 0) {
            perror("recvfrom");
            continue;
        }
        packet[n] = '\0';

        // Determine command
        if (strncmp(packet, "SUB ", 4) == 0) {
            char topic[TOPIC_MAX];
            unsigned int sub_port = 0;

            // "SUB <topic> <port>"
            if (sscanf(packet + 4, "%255s %u", topic, &sub_port) != 2 ||
                sub_port == 0 || sub_port > 65535) {
                fprintf(stderr, "Invalid SUB packet: %s\n", packet);
                continue;
            }

            char srcbuf[64];
            print_addr(&src, srcbuf, sizeof(srcbuf));
            printf("[SUB] from %s topic='%s' port=%u\n", srcbuf, topic, sub_port);
            fflush(stdout);

            subs_add_or_update(topic, &src, (uint16_t)sub_port);
            continue;
        }

        if (strncmp(packet, "PUB ", 4) == 0) {
            // "PUB <topic> <message...>"
            char topic[TOPIC_MAX];
            char message[MESSAGE_MAX];

            // Parse topic first, then remainder as message (including spaces)
            const char *p = packet + 4;
            while (*p == ' ') p++;

            // Extract topic
            size_t i = 0;
            while (*p && *p != ' ' && i < TOPIC_MAX - 1) {
                topic[i++] = *p++;
            }
            topic[i] = '\0';

            while (*p == ' ') p++;

            // Remainder is message
            strncpy(message, p, MESSAGE_MAX - 1);
            message[MESSAGE_MAX - 1] = '\0';

            if (topic[0] == '\0' || message[0] == '\0') {
                fprintf(stderr, "Invalid PUB packet: %s\n", packet);
                continue;
            }
            // Enforce: publisher not allowed to use wildcard '#'
            if (strcmp(topic, "#") == 0) {
                fprintf(stderr, "Rejected PUB with wildcard topic '#'\n");
                continue;
            }

            char srcbuf[64];
            print_addr(&src, srcbuf, sizeof(srcbuf));
            printf("[PUB] from %s topic='%s' message='%s'\n", srcbuf, topic, message);
            fflush(stdout);

            // Forward to matching subscribers
            char out[PACKET_MAX];
            snprintf(out, sizeof(out), "MSG %s %s", topic, message);

            int forwarded = 0;
            for (size_t s = 0; s < subs_len; s++) {
                if (!topic_matches(subs[s].topic, topic)) continue;

                struct sockaddr_in dst = subs[s].addr;
                dst.sin_port = htons(subs[s].port);

                ssize_t sent = sendto(sock, out, strlen(out), 0, (struct sockaddr *)&dst, sizeof(dst));
                if (sent < 0) {
                    perror("sendto");
                    continue;
                }
                forwarded++;
            }

            printf("        forwarded to %d subscriber(s)\n", forwarded);
            fflush(stdout);
            continue;
        }

        fprintf(stderr, "Unknown packet: %s\n", packet);
    }

    close(sock);
    return 0;
}