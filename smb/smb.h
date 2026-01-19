#ifndef SMB_H
#define SMB_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <unistd.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/types.h>

#define BROKER_PORT 8080

// Limits to keep parsing simple and safe
#define TOPIC_MAX   256
#define MESSAGE_MAX 1024
#define PACKET_MAX  1400  // fits typical MTU safely

static inline void die(const char *msg) {
    perror(msg);
    exit(EXIT_FAILURE);
}

static inline int resolve_host_ipv4(const char *host, struct sockaddr_in *out, int port) {
    memset(out, 0, sizeof(*out));
    out->sin_family = AF_INET;
    out->sin_port = htons((uint16_t)port);

    // Try dotted IPv4 first
    if (inet_pton(AF_INET, host, &out->sin_addr) == 1) return 0;

    // Then DNS lookup
    struct addrinfo hints, *res = NULL;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;

    int rc = getaddrinfo(host, NULL, &hints, &res);
    if (rc != 0 || !res) return -1;

    struct sockaddr_in *addr = (struct sockaddr_in *)res->ai_addr;
    out->sin_addr = addr->sin_addr;

    freeaddrinfo(res);
    return 0;
}

#endif