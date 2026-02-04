#include "smb.h"

typedef struct
{
    char topic[TOPIC_MAX];   // subscribed topic
    struct sockaddr_in addr; // subscriber IP
    uint16_t port;           // subscriber listening port (from SUB message)
} subscription_t;

static subscription_t *subs = NULL; // dynamic array of subscriptions
static size_t subs_len = 0; // current number of subscriptions
static size_t subs_cap = 0; // current capacity of subscription array

/**
 * Add a new subscription or update an existing one.
 * The subscription array is dynamically resized as needed.
 * @param topic The topic to subscribe to.
 * @param src_addr The subscriber's IP address.
 * @param port The subscriber's listening port.
 */
static void subs_add_or_update(const char *topic, const struct sockaddr_in *src_addr, uint16_t port)
{
    // If same IP:port + topic already exists, just update (idempotent subscribe)
    for (size_t i = 0; i < subs_len; i++)
    {
        if (subs[i].port == port &&
            subs[i].addr.sin_addr.s_addr == src_addr->sin_addr.s_addr &&
            strncmp(subs[i].topic, topic, TOPIC_MAX) == 0)
        {
            subs[i].addr = *src_addr;
            subs[i].port = port;
            return;
        }
    }
    
    // Otherwise,
    // Add new subscription

    // if there is no space, grow the subscription array
    if (subs_len == subs_cap)
    {
        // Double capacity or start with 16 if subscription array is empty
        size_t new_cap = (subs_cap == 0) ? 16 : subs_cap * 2;
        // Reallocate subscription array to larger size
        subscription_t *p = realloc(subs, new_cap * sizeof(*subs));
        if (!p)
            die("Error allocating memory for subscriptions");
        subs = p;
        subs_cap = new_cap;
    }

    // Add new subscription at the end
    memset(&subs[subs_len], 0, sizeof(subs[subs_len])); // After realloc, the new memory is uninitialized. Thus, we zero it.
    snprintf(subs[subs_len].topic, TOPIC_MAX, "%s", topic); // Copy topic
    subs[subs_len].addr = *src_addr;
    subs[subs_len].port = port;
    subs_len++;
}

/**
 * Remove a subscription matching topic + IP + port.
 * @return number of removed subscriptions.
 */
static int subs_remove(const char *topic, const struct sockaddr_in *src_addr, uint16_t port)
{
    int removed = 0;
    for (size_t i = 0; i < subs_len; )
    {
        if (subs[i].port == port &&
            subs[i].addr.sin_addr.s_addr == src_addr->sin_addr.s_addr &&
            strncmp(subs[i].topic, topic, TOPIC_MAX) == 0)
        {
            subs[i] = subs[subs_len - 1];
            subs_len--;
            removed++;
            continue;
        }
        i++;
    }
    return removed;
}

/**
 * Check if a published topic matches a subscribed topic.
 * Supports only '#' wildcard for "all topics".
 * @param sub_topic The subscribed topic (may contain '#').
 * @param pub_topic The published topic.
 * @return 1 if matches, 0 otherwise.
 */
static int topic_matches(const char *sub_topic, const char *pub_topic)
{
    if (strcmp(sub_topic, "#") == 0)
        return 1;

    const char *hash = strchr(sub_topic, '#');
    if (hash)
    {
        // Expect "prefix/#"
        if (hash[1] != '\0' || hash == sub_topic || hash[-1] != '/')
            return 0;

        size_t prefix_len = (size_t)(hash - sub_topic);
        size_t prefix_no_slash = prefix_len - 1; // exclude trailing '/'

        if (strncmp(pub_topic, sub_topic, prefix_no_slash) != 0)
            return 0;

        return pub_topic[prefix_no_slash] == '\0' || pub_topic[prefix_no_slash] == '/';
    }

    return strcmp(sub_topic, pub_topic) == 0;
}

/**
 * Print a sockaddr_in as "IP:port" into a buffer.
 * @param a The sockaddr_in to print.
 * @param buf The output buffer.
 * @param buflen The length of the output buffer.
 */
static void print_addr(const struct sockaddr_in *a, char *buf, size_t buflen)
{
    char ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &a->sin_addr, ip, sizeof(ip));
    snprintf(buf, buflen, "%s:%u", ip, (unsigned)ntohs(a->sin_port));
}

/**
 * Parse a PUB packet into topic and message.
 * @param packet The input packet string.
 * @param topic The output topic buffer.
 * @param message The output message buffer.
 * @param retFlag Output flag: 1=success, 3=error.
 */
void parsePackage(char packet[1400], char topic[256], char message[1024], int *retFlag)
{
    *retFlag = 1;
    // Parse topic first, then remainder as message (including spaces)
    const char *p = packet + 4;
    while (*p == ' ')
        p++;

    // Extract topic
    size_t i = 0;
    while (*p && *p != ' ' && i < TOPIC_MAX - 1)
    {
        topic[i++] = *p++;
    }
    topic[i] = '\0';

    while (*p == ' ')
        p++;

    // Remainder is message
    strncpy(message, p, MESSAGE_MAX - 1);
    message[MESSAGE_MAX - 1] = '\0';

    if (topic[0] == '\0' || message[0] == '\0')
    {
        fprintf(stderr, "Invalid PUB packet: %s\n", packet);
        {
            *retFlag = 3;
            return;
        };
    }
    // Enforce: publisher not allowed to use wildcard '#' and requires hierarchical topic
    if (!is_valid_pub_topic(topic))
    {
        fprintf(stderr, "Rejected PUB with invalid topic '%s'\n", topic);
        {
            *retFlag = 3;
            return;
        };
    }
}

int main(int argc, char **argv)
{
    int port = BROKER_PORT;
    if (argc == 2)
    {
        port = atoi(argv[1]);
        if (port <= 0 || port > 65535)
        {
            fprintf(stderr, "Invalid: Port should be between 1 and 65535\nUsage: %s [port]\n", argv[0]);
            return EXIT_FAILURE;
        }
    }
    else if (argc != 1)
    {
        fprintf(stderr, "Expect arguments only 1 or 2 (program name and optional port)\nUsage: %s [port]\n", argv[0]);
        return EXIT_FAILURE;
    }

    // Create UDP socket
    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0)
        die("Error creating socket");

    // Prepare bind address to all interfaces
    struct sockaddr_in bind_addr;
    memset(&bind_addr, 0, sizeof(bind_addr));
    bind_addr.sin_family = AF_INET;
    bind_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    bind_addr.sin_port = htons((uint16_t)port);

    // Bind to all interfaces on specified port
    if (bind(sock, (struct sockaddr *)&bind_addr, sizeof(bind_addr)) < 0)
        die("Error binding socket");

    printf("smbbroker listening on UDP port %d\n", port);
    fflush(stdout);

    // Unlimited loop
    for (;;)
    {
        char packet[PACKET_MAX]; // packet is topic+message
        // sender address
        struct sockaddr_in src;
        socklen_t srclen = sizeof(src);

        // Receive packet from any sender (Publisher or Subscriber)
        ssize_t n = recvfrom(sock, packet, sizeof(packet) - 1, 0, (struct sockaddr *)&src, &srclen);
        if (n < 0)
        {
            perror("Error receiving packet");
            continue;
        }
        packet[n] = '\0';

        // Determine command

        // if SUB comes, add to subscriber list or update existing
        if (strncmp(packet, "SUB ", 4) == 0)
        {
            char topic[TOPIC_MAX];
            unsigned int sub_port = 0;

            // "SUB <topic> <port>"
            if (sscanf(packet + 4, "%255s %u", topic, &sub_port) != 2 ||
                sub_port == 0 || sub_port > 65535)
            {
                fprintf(stderr, "Invalid SUB packet: %s\n", packet);
                continue;
            }
            if (!is_valid_sub_topic(topic))
            {
                fprintf(stderr, "Invalid SUB topic: %s\n", topic);
                continue;
            }

            // print Subscriber info
            char srcbuf[64];
            print_addr(&src, srcbuf, sizeof(srcbuf));
            printf("[SUB] from %s topic='%s' port=%u\n", srcbuf, topic, sub_port);
            fflush(stdout);

            subs_add_or_update(topic, &src, (uint16_t)sub_port);
            continue;
        }

        // if UNSUB comes, remove from subscriber list
        if (strncmp(packet, "UNSUB ", 6) == 0)
        {
            char topic[TOPIC_MAX];
            unsigned int sub_port = 0;

            // "UNSUB <topic> <port>"
            if (sscanf(packet + 6, "%255s %u", topic, &sub_port) != 2 ||
                sub_port == 0 || sub_port > 65535)
            {
                fprintf(stderr, "Invalid UNSUB packet: %s\n", packet);
                continue;
            }
            if (!is_valid_sub_topic(topic))
            {
                fprintf(stderr, "Invalid UNSUB topic: %s\n", topic);
                continue;
            }

            char srcbuf[64];
            print_addr(&src, srcbuf, sizeof(srcbuf));
            int removed = subs_remove(topic, &src, (uint16_t)sub_port);
            printf("[UNSUB] from %s topic='%s' port=%u removed=%d\n", srcbuf, topic, sub_port, removed);
            fflush(stdout);
            continue;
        }

        // if PUB comes, forward the message to matching subscribers
        if (strncmp(packet, "PUB ", 4) == 0)
        {
            // "PUB <topic> <message...>"
            char topic[TOPIC_MAX];
            char message[MESSAGE_MAX];

            int retFlag;
            parsePackage(packet, topic, message, &retFlag);
            if (retFlag == 3)
                continue;

            // print Publisher info
            char srcbuf[64];
            print_addr(&src, srcbuf, sizeof(srcbuf));
            printf("[PUB] from %s topic='%s' message='%s'\n", srcbuf, topic, message);
            fflush(stdout);

            // Forward to matching subscribers
            char out[PACKET_MAX]; 
            snprintf(out, sizeof(out), "MSG %s %s", topic, message);

            int forwarded = 0;
            for (size_t s = 0; s < subs_len; s++)
            {
                if (!topic_matches(subs[s].topic, topic))
                    continue;

                struct sockaddr_in dst = subs[s].addr;
                dst.sin_port = htons(subs[s].port);

                ssize_t sent = sendto(sock, out, strlen(out), 0, (struct sockaddr *)&dst, sizeof(dst));
                if (sent < 0)
                {
                    perror("Error sending packet to subscriber");
                    continue;
                }
                forwarded++;
            }

            printf("        forwarded to %d subscriber(s)\n", forwarded);
            fflush(stdout);
            continue;
        }

        // Otherwise, unknown packet
        fprintf(stderr, "Unknown packet: %s\n", packet);
    }

    close(sock);
    return 0;
}
