#include "smb.h"
#include "smb_secure.h"

/**
 * Trim trailing newline and carriage return characters from a string.
 * @param s The string to trim.
 */
static void trim_newline(char *s) {
    if (!s) return;
    size_t n = strlen(s);
    if (n > 0 && s[n - 1] == '\n') s[n - 1] = '\0';
    n = strlen(s);
    if (n > 0 && s[n - 1] == '\r') s[n - 1] = '\0';
}

int main(int argc, char **argv) {
    const char *key_path = NULL;
    char *pos[2];
    int pos_count = smb_parse_key_and_pos(argc, argv, &key_path, pos, 2);
    if (pos_count != 2) {
        fprintf(stderr, "Usage: %s broker topic [--key[=<path>]]\n", argv[0]);
        fprintf(stderr, "Example: %s localhost zimmer/temperatur --key\n", argv[0]);
        fprintf(stderr, "Commands: /topic <newtopic>, /help, /quit\n");
        return EXIT_FAILURE;
    }

    const char *broker = pos[0];
    char topic[TOPIC_MAX];
    if (strlen(pos[1]) >= TOPIC_MAX) {
        fprintf(stderr, "Topic too long (max %d)\n", TOPIC_MAX - 1);
        return EXIT_FAILURE;
    }
    snprintf(topic, sizeof(topic), "%s", pos[1]);

    if (!is_valid_pub_topic(topic)) {
        fprintf(stderr, "Error: invalid topic. Expected form oberthema/thema, wildcard '#' not allowed\n");
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

    printf("Interactive publisher connected to %s:%d\n", broker, BROKER_PORT);
    printf("Current topic: %s\n", topic);
    printf("Type messages to publish. Commands: /topic <newtopic>, /help, /quit\n");
    fflush(stdout);

    // Main loop: read lines from stdin and publish them
    char line[MESSAGE_MAX];
    while (fgets(line, sizeof(line), stdin)) {
        trim_newline(line);
        if (line[0] == '\0') continue;

        if (strncmp(line, "/quit", 5) == 0 || strncmp(line, "/exit", 5) == 0) {
            break;
        }
        if (strncmp(line, "/help", 5) == 0) {
            printf("Commands:\n");
            printf("  /topic <newtopic>   Change the publish topic\n");
            printf("  /quit               Exit\n");
            printf("Send any other line to publish it.\n");
            fflush(stdout);
            continue;
        }
        if (strncmp(line, "/topic ", 7) == 0) {
            const char *newtopic = line + 7;
            while (*newtopic == ' ') newtopic++;
            if (strlen(newtopic) >= TOPIC_MAX) {
                fprintf(stderr, "Topic too long (max %d)\n", TOPIC_MAX - 1);
                continue;
            }
            if (!is_valid_pub_topic(newtopic)) {
                fprintf(stderr, "Invalid topic. Use oberthema/thema (no wildcard)\n");
                continue;
            }
            snprintf(topic, sizeof(topic), "%s", newtopic);
            printf("Topic changed to: %s\n", topic);
            fflush(stdout);
            continue;
        }

        if (strlen(line) >= MESSAGE_MAX) {
            fprintf(stderr, "Message too long (max %d)\n", MESSAGE_MAX - 1);
            continue;
        }

        // Construct the PUB packet
        char plain[PACKET_MAX];
        snprintf(plain, sizeof(plain), "PUB %s %s", topic, line);
        uint8_t pkt[PACKET_MAX];
        size_t pkt_len = smb_secure_pack(key, (const uint8_t *)plain, strlen(plain), pkt, sizeof(pkt));
        if (pkt_len == 0) {
            fprintf(stderr, "Message too large to secure\n");
            continue;
        }

        // Send the PUB packet to the broker
        if (sendto(sock, pkt, pkt_len, 0, (struct sockaddr *)&broker_addr, sizeof(broker_addr)) < 0) {
            perror("sendto PUB");
        } else {
            printf("[PUB %s] %s\n", topic, line);
            fflush(stdout);
        }
    }

    close(sock);
    return 0;
}
