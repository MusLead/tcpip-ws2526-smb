#include "smb.h"

int main(int argc, char **argv) {
    if (argc != 4) {
        fprintf(stderr, "Usage: %s broker topic message\n", argv[0]);
        fprintf(stderr, "Example: %s 127.0.0.1 zimmer/temperatur 08.02.2021\n", argv[0]);
        fprintf(stderr, "Example: %s smbserver zimmer/luftfeuchte \"hallo welt!\"\n", argv[0]);
        return EXIT_FAILURE;
    }

    const char *broker = argv[1];
    const char *topic  = argv[2];
    const char *msg    = argv[3];

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

    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) die("socket");

    struct sockaddr_in broker_addr;
    if (resolve_host_ipv4(broker, &broker_addr, BROKER_PORT) != 0) {
        fprintf(stderr, "Cannot resolve broker host: %s\n", broker);
        return EXIT_FAILURE;
    }

    char pkt[PACKET_MAX];
    snprintf(pkt, sizeof(pkt), "PUB %s %s", topic, msg);

    if (sendto(sock, pkt, strlen(pkt), 0, (struct sockaddr *)&broker_addr, sizeof(broker_addr)) < 0) {
        die("sendto PUB");
    }

    close(sock);
    return 0;
}
