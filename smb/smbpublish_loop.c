#include "smb.h"
#include "smb_secure.h"
#include <signal.h>
#include <time.h>

static volatile sig_atomic_t stop_requested = 0;

static void handle_signal(int signo) {
    (void)signo;
    stop_requested = 1;
}

static int parse_interval(const char *s, unsigned int *out) {
    if (!s || *s == '\0') return -1;
    char *end = NULL;
    errno = 0;
    long v = strtol(s, &end, 10);
    if (errno != 0 || end == s || *end != '\0') return -1;
    if (v <= 0 || v > 86400) return -1;
    *out = (unsigned int)v;
    return 0;
}

int main(int argc, char **argv) {
    const char *key_path = NULL;
    char *pos[4];
    int pos_count = smb_parse_key_and_pos(argc, argv, &key_path, pos, 4);
    if (pos_count < 3 || pos_count > 4) {
        fprintf(stderr, "Usage: %s broker topic interval_seconds [prefix] [--key[=<path>]]\n", argv[0]);
        fprintf(stderr, "Example: %s localhost zimmer/temperatur 30 --key\n", argv[0]);
        fprintf(stderr, "Example: %s 127.0.0.1 zimmer/luftfeuchte 10 sensor1 --key=build/.key\n", argv[0]);
        return EXIT_FAILURE;
    }

    const char *broker = pos[0];
    const char *topic = pos[1];
    const char *interval_str = pos[2];
    const char *prefix = (pos_count == 4) ? pos[3] : NULL;

    if (strlen(topic) >= TOPIC_MAX) {
        fprintf(stderr, "Topic too long (max %d)\n", TOPIC_MAX - 1);
        return EXIT_FAILURE;
    }
    if (!is_valid_pub_topic(topic)) {
        fprintf(stderr, "Error: invalid topic. Expected form oberthema/thema, wildcard '#' not allowed\n");
        return EXIT_FAILURE;
    }

    unsigned int interval = 0;
    if (parse_interval(interval_str, &interval) != 0) {
        fprintf(stderr, "Invalid interval_seconds: %s (1..86400)\n", interval_str);
        return EXIT_FAILURE;
    }

    uint8_t key[SMB_KEY_LEN];
    if (smb_load_key(key, key_path) != 0) {
        fprintf(stderr, "Missing key. Use --key[=<path>], or set SMB_KEY, or create %s\n", SMB_DEFAULT_KEY_PATH);
        return EXIT_FAILURE;
    }

    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) die("socket");

    struct sockaddr_in broker_addr;
    if (resolve_host_ipv4(broker, &broker_addr, BROKER_PORT) != 0) {
        fprintf(stderr, "Cannot resolve broker host: %s\n", broker);
        return EXIT_FAILURE;
    }

    // Setup signal handlers for graceful termination
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = handle_signal;
    sigemptyset(&sa.sa_mask);
    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);

    // Main loop: publish messages at regular intervals until stopped
    unsigned long counter = 0;
    while (!stop_requested) {
        counter++;

        char ts[64];
        time_t now = time(NULL);
        struct tm tm_now;
        localtime_r(&now, &tm_now);
        strftime(ts, sizeof(ts), "%Y-%m-%d %H:%M:%S", &tm_now);

        char msg[MESSAGE_MAX];
        if (prefix && prefix[0] != '\0') {
            snprintf(msg, sizeof(msg), "%s %lu %s", prefix, counter, ts);
        } else {
            snprintf(msg, sizeof(msg), "%lu %s", counter, ts);
        }

        char plain[PACKET_MAX];
        snprintf(plain, sizeof(plain), "PUB %s %s", topic, msg);
        uint8_t pkt[PACKET_MAX];
        size_t pkt_len = smb_secure_pack(key, (const uint8_t *)plain, strlen(plain), pkt, sizeof(pkt));
        if (pkt_len == 0) {
            fprintf(stderr, "Message too large to secure\n");
            break;
        }

        if (sendto(sock, pkt, pkt_len, 0, (struct sockaddr *)&broker_addr, sizeof(broker_addr)) < 0) {
            perror("sendto PUB");
        } else {
            printf("[PUB] %s\n", msg);
            fflush(stdout);
        }

        // Sleep for the specified interval, but wake up on signal
        unsigned int slept = 0;
        while (slept < interval && !stop_requested) {
            unsigned int remaining = interval - slept;
            unsigned int s = sleep(remaining);
            if (s == 0) {
                slept = interval;
            } else {
                slept = interval - s;
            }
        }
    }

    close(sock);
    return 0;
}
