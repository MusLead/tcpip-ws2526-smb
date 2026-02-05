#include "smb.h"
#include "smb_secure.h"
#include <signal.h>

static volatile sig_atomic_t stop_requested = 0;

/**
 * Signal handler to set the stop_requested flag when SIGINT or SIGTERM is received.
 * This allows the main loop to exit gracefully and perform cleanup.
 * @param signo The signal number (ignored in this handler).
 */
static void handle_signal(int signo) {
    (void)signo;
    stop_requested = 1;
}

/**
 * Get the local port number that the socket is bound to.
 * This is used to find out which port the OS assigned when we bind to port 0.
 * @param sock The socket file descriptor.
 * @return The local port number in host byte order.
 */
static uint16_t get_bound_port(int sock) {
    struct sockaddr_in a;
    socklen_t alen = sizeof(a);
    if (getsockname(sock, (struct sockaddr *)&a, &alen) < 0) die("getsockname");
    return ntohs(a.sin_port);
}

int main(int argc, char **argv) {
    const char *key_path = NULL;
    char *pos[2];
    int pos_count = smb_parse_key_and_pos(argc, argv, &key_path, pos, 2);
    if (pos_count != 2) {
        fprintf(stderr, "Usage: %s broker topic [--key[=<path>]]\n", argv[0]);
        fprintf(stderr, "Example: %s localhost zimmer/temperatur --key\n", argv[0]);
        fprintf(stderr, "Example: %s 192.168.1.13 zimmer/# --key=build/.key\n", argv[0]);
        return EXIT_FAILURE;
    }

    const char *broker = pos[0];
    const char *topic = pos[1];

    if (strlen(topic) >= TOPIC_MAX) {
        fprintf(stderr, "Topic too long (max %d)\n", TOPIC_MAX - 1);
        return EXIT_FAILURE;
    }
    if (!is_valid_sub_topic(topic)) {
        fprintf(stderr, "Invalid topic. Use oberthema/thema, or wildcard like oberthema/#, or '#'\n");
        return EXIT_FAILURE;
    }

    uint8_t key[SMB_KEY_LEN];
    if (smb_load_key(key, key_path) != 0) {
        fprintf(stderr, "Missing key. Use --key[=<path>], or set SMB_KEY, or create %s\n", SMB_DEFAULT_KEY_PATH);
        return EXIT_FAILURE;
    }

    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) die("socket");

    // Bind to any local port so we can receive messages
    struct sockaddr_in local;
    memset(&local, 0, sizeof(local));
    local.sin_family = AF_INET;
    local.sin_addr.s_addr = htonl(INADDR_ANY);
    local.sin_port = htons(0);

    if (bind(sock, (struct sockaddr *)&local, sizeof(local)) < 0) die("bind");

    uint16_t myport = get_bound_port(sock);

    struct sockaddr_in broker_addr;
    if (resolve_host_ipv4(broker, &broker_addr, BROKER_PORT) != 0) {
        fprintf(stderr, "Cannot resolve broker host: %s\n", broker);
        return EXIT_FAILURE;
    }

    char subplain[PACKET_MAX];
    snprintf(subplain, sizeof(subplain), "SUB %s %u", topic, (unsigned)myport);
    uint8_t subpkt[PACKET_MAX];
    size_t subpkt_len = smb_secure_pack(key, (const uint8_t *)subplain, strlen(subplain), subpkt, sizeof(subpkt));
    if (subpkt_len == 0) {
        fprintf(stderr, "SUB packet too large to secure\n");
        return EXIT_FAILURE;
    }

    if (sendto(sock, subpkt, subpkt_len, 0, (struct sockaddr *)&broker_addr, sizeof(broker_addr)) < 0) {
        die("sendto SUB");
    }

    printf("Subscribed to topic '%s' (listening on UDP port %u)\n", topic, (unsigned)myport);
    fflush(stdout);

    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = handle_signal;
    sigemptyset(&sa.sa_mask);
    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);

    for (;;) {
        if (stop_requested) break;
        uint8_t pkt[PACKET_MAX];
        struct sockaddr_in src;
        socklen_t srclen = sizeof(src);

        ssize_t n = recvfrom(sock, pkt, sizeof(pkt), 0, (struct sockaddr *)&src, &srclen);
        if (n < 0) {
            if (errno == EINTR && stop_requested) break;
            perror("recvfrom");
            continue;
        }
        uint8_t plain[PACKET_MAX];
        size_t plain_len = 0;
        if (smb_secure_unpack(key, pkt, (size_t)n, plain, sizeof(plain) - 1, &plain_len) != 0) {
            fprintf(stderr, "Dropped packet: authentication/decryption failed\n");
            continue;
        }
        plain[plain_len] = '\0';

        if (strncmp((const char *)plain, "MSG ", 4) != 0) {
            fprintf(stderr, "Unknown packet: %s\n", (const char *)plain);
            continue;
        }

        // MSG <topic> <message...>
        const char *p = (const char *)plain + 4;
        while (*p == ' ') p++;

        char rtopic[TOPIC_MAX];
        size_t i = 0;
        while (*p && *p != ' ' && i < TOPIC_MAX - 1) {
            rtopic[i++] = *p++;
        }
        rtopic[i] = '\0';
        while (*p == ' ') p++;

        printf("[%s] %s\n", rtopic, p);
        fflush(stdout);
    }

    if (stop_requested) {
        char unplain[PACKET_MAX];
        snprintf(unplain, sizeof(unplain), "UNSUB %s %u", topic, (unsigned)myport);
        uint8_t unpkt[PACKET_MAX];
        size_t unpkt_len = smb_secure_pack(key, (const uint8_t *)unplain, strlen(unplain), unpkt, sizeof(unpkt));
        if (unpkt_len == 0) {
            fprintf(stderr, "UNSUB packet too large to secure\n");
        } else if (sendto(sock, unpkt, unpkt_len, 0, (struct sockaddr *)&broker_addr, sizeof(broker_addr)) < 0) {
            perror("sendto UNSUB");
        } else {
            printf("Unsubscribed from topic '%s'\n", topic);
            fflush(stdout);
        }
    }

    close(sock);
    return 0;
}
