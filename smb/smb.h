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
#define PACKET_MAX  1400  

/**
 * Print an error message and exit the program.
 * @param msg The error message to print.
 */
static inline void die(const char *msg) {
    perror(msg);
    exit(EXIT_FAILURE);
}

/**
 * Resolve a hostname to an IPv4 address.
 * If `host` is already a numeric IPv4 address, 
 * `inet_pton` succeeds and it returns without doing DNS Lookup.
 * @param host The hostname or dotted-decimal IPv4 address.
 * @param out Pointer to sockaddr_in structure to fill.
 * @param port The port number to set in the sockaddr_in structure.
 * @return 0 on success, -1 on failure.
 */
static inline int resolve_host_ipv4(const char *host, struct sockaddr_in *out, int port) {
    memset(out, 0, sizeof(*out));
    out->sin_family = AF_INET;
    out->sin_port = htons((uint16_t)port);

    // Try dotted IPv4 first
    if (inet_pton(AF_INET, host, &out->sin_addr) == 1) return 0;

    // Prepare hints for getaddrinfo
    struct addrinfo hints, *res = NULL;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;

    // Then DNS lookup
    int rc = getaddrinfo(host, NULL, &hints, &res);
    if (rc != 0 || !res) return -1;

    // Extract the IPv4 address and save it in the out structure
    struct sockaddr_in *addr = (struct sockaddr_in *)res->ai_addr;
    out->sin_addr = addr->sin_addr;

    freeaddrinfo(res);
    return 0;
}

#endif