#include "smb.h"
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
    if (argc != 2 && argc != 3) {
        fprintf(stderr, "Usage: %s broker [topic]\n", argv[0]);
        fprintf(stderr, "Example: %s localhost zimmer/temperatur\n", argv[0]);
        fprintf(stderr, "Example: %s 192.168.1.13 zimmer/#\n", argv[0]);
        fprintf(stderr, "Example: %s localhost   (defaults to '#')\n", argv[0]);
        return EXIT_FAILURE;
    }

    const char *broker = argv[1];
    const char *topic = (argc == 3) ? argv[2] : "#";

    if (strlen(topic) >= TOPIC_MAX) {
        fprintf(stderr, "Topic too long (max %d)\n", TOPIC_MAX - 1);
        return EXIT_FAILURE;
    }
    if (!is_valid_sub_topic(topic)) {
        fprintf(stderr, "Invalid topic. Use oberthema/thema, or wildcard like oberthema/#, or '#'\n");
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

    char subpkt[PACKET_MAX];
    snprintf(subpkt, sizeof(subpkt), "SUB %s %u", topic, (unsigned)myport);

    if (sendto(sock, subpkt, strlen(subpkt), 0, (struct sockaddr *)&broker_addr, sizeof(broker_addr)) < 0) {
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
        char pkt[PACKET_MAX];
        struct sockaddr_in src;
        socklen_t srclen = sizeof(src);

        ssize_t n = recvfrom(sock, pkt, sizeof(pkt) - 1, 0, (struct sockaddr *)&src, &srclen);
        if (n < 0) {
            if (errno == EINTR && stop_requested) break;
            perror("recvfrom");
            continue;
        }
        pkt[n] = '\0';

        if (strncmp(pkt, "MSG ", 4) != 0) {
            fprintf(stderr, "Unknown packet: %s\n", pkt);
            continue;
        }

        // MSG <topic> <message...>
        const char *p = pkt + 4;
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
        char unpkt[PACKET_MAX];
        snprintf(unpkt, sizeof(unpkt), "UNSUB %s %u", topic, (unsigned)myport);
        if (sendto(sock, unpkt, strlen(unpkt), 0, (struct sockaddr *)&broker_addr, sizeof(broker_addr)) < 0) {
            perror("sendto UNSUB");
        } else {
            printf("Unsubscribed from topic '%s'\n", topic);
            fflush(stdout);
        }
    }

    close(sock);
    return 0;
}
