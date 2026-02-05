/*
 *  smbpublish.c
 *  Developed on: Feb 05, 2026
 *      Author: Agha Muhammad Aslam
 *  
 *  MAIN FEATURE
 *  Sends a single PUB <topic> <message> to the broker and exits.
 *  Rejects invalid topics (empty orwith wildcard).
 *  Usage: smbpublish <broker> <topic> <message>.
 * 
 *  ADDITIONAL FEATURE
 *  Supports levelled topics like "zimmer/temperatur" and "zimmer/luftfeuchte".
 */
#include "smb.h"

int main(int argc, char **argv) {
    if (argc < 4) {
        fprintf(stderr, "Usage: %s broker topic message...\n", argv[0]);
        fprintf(stderr, "Example: %s 127.0.0.1 zimmer/temperatur 08.02.2021\n", argv[0]);
        fprintf(stderr, "Example: %s smbserver zimmer/luftfeuchte \"hallo welt!\"\n", argv[0]);
        return EXIT_FAILURE;
    }

    const char *broker = argv[1];
    const char *topic  = argv[2];
    char msgbuf[MESSAGE_MAX];
    size_t used = 0;

    // Concatenate all message parts with spaces
    for (int i = 3; i < argc; i++) {
        const char *part = argv[i];
        size_t len = strlen(part);
        if (used != 0) {
            if (used + 1 >= MESSAGE_MAX) {
                fprintf(stderr, "Message too long (max %d)\n", MESSAGE_MAX - 1);
                return EXIT_FAILURE;
            }
            msgbuf[used++] = ' ';
        }
        if (used + len >= MESSAGE_MAX) {
            fprintf(stderr, "Message too long (max %d)\n", MESSAGE_MAX - 1);
            return EXIT_FAILURE;
        }
        memcpy(msgbuf + used, part, len);
        used += len;
    }
    msgbuf[used] = '\0';
    const char *msg = msgbuf;

    if (!is_valid_pub_topic(topic)) {
        fprintf(stderr, "Error: invalid topic. Expected form 'thema' or 'ober/thema', wildcard '#' not allowed\n");
        return EXIT_FAILURE;
    }

    if (strlen(topic) >= TOPIC_MAX) {
        fprintf(stderr, "Topic too long (max %d)\n", TOPIC_MAX - 1);
        return EXIT_FAILURE;
    }
    if (msg[0] == '\0') {
        fprintf(stderr, "Message must not be empty\n");
        return EXIT_FAILURE;
    }

    // Create UDP socket
    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) die("socket");

    // Resolve broker address
    struct sockaddr_in broker_addr;
    if (resolve_host_ipv4(broker, &broker_addr, BROKER_PORT) != 0) {
        fprintf(stderr, "Cannot resolve broker host: %s\n", broker);
        return EXIT_FAILURE;
    }

    // Prepare and send PUB packet
    char pkt[PACKET_MAX];
    snprintf(pkt, sizeof(pkt), "PUB %s %s", topic, msg);
    
    if (sendto(sock, pkt, strlen(pkt), 0, (struct sockaddr *)&broker_addr, sizeof(broker_addr)) < 0) {
        die("sendto PUB");
    }

    close(sock);
    return 0;
}
