#include "smb.h"
#include "smb_secure.h"

int main(int argc, char **argv) {
    const char *key_path = NULL;
    char *pos[3];
    int pos_count = smb_parse_key_and_pos(argc, argv, &key_path, pos, 3);
    if (pos_count != 3) {
        fprintf(stderr, "Usage: %s broker topic message [--key[=<path>]]\n", argv[0]);
        fprintf(stderr, "Example: %s 127.0.0.1 zimmer/temperatur 08.02.2021 --key\n", argv[0]);
        fprintf(stderr, "Example: %s smbserver zimmer/luftfeuchte \"hallo welt!\" --key=build/.key\n", argv[0]);
        return EXIT_FAILURE;
    }

    const char *broker = pos[0];
    const char *topic  = pos[1];
    const char *msg    = pos[2];

    if (!is_valid_pub_topic(topic)) {
        fprintf(stderr, "Error: invalid topic. Expected form oberthema/thema, wildcard '#' not allowed\n");
        return EXIT_FAILURE;
    }

    if (strlen(topic) >= TOPIC_MAX) {
        fprintf(stderr, "Topic too long (max %d)\n", TOPIC_MAX - 1);
        return EXIT_FAILURE;
    }
    if (strlen(msg) >= MESSAGE_MAX) {
        fprintf(stderr, "Message too long (max %d)\n", MESSAGE_MAX - 1);
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

    char plain[PACKET_MAX];
    snprintf(plain, sizeof(plain), "PUB %s %s", topic, msg);
    uint8_t pkt[PACKET_MAX];
    size_t pkt_len = smb_secure_pack(key, (const uint8_t *)plain, strlen(plain), pkt, sizeof(pkt));
    if (pkt_len == 0) {
        fprintf(stderr, "Message too large to secure\n");
        return EXIT_FAILURE;
    }

    if (sendto(sock, pkt, pkt_len, 0, (struct sockaddr *)&broker_addr, sizeof(broker_addr)) < 0) {
        die("sendto PUB");
    }

    close(sock);
    return 0;
}
