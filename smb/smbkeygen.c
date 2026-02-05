#include "smb_secure.h"

#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

static int random_bytes(uint8_t *out, size_t len) {
    int fd = open("/dev/urandom", O_RDONLY);
    if (fd < 0) return -1;
    size_t off = 0;
    while (off < len) {
        ssize_t n = read(fd, out + off, len - off);
        if (n <= 0) {
            close(fd);
            return -1;
        }
        off += (size_t)n;
    }
    close(fd);
    return 0;
}

int main(int argc, char **argv) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <output_path>\n", argv[0]);
        return 1;
    }

    uint8_t key[SMB_KEY_LEN];
    if (random_bytes(key, sizeof(key)) != 0) {
        fprintf(stderr, "Failed to read randomness\n");
        return 1;
    }

    int fd = open(argv[1], O_WRONLY | O_CREAT | O_TRUNC, 0600);
    if (fd < 0) {
        perror("open");
        return 1;
    }

    char hex[SMB_KEY_LEN * 2 + 2];
    for (size_t i = 0; i < SMB_KEY_LEN; i++) {
        snprintf(hex + (i * 2), 3, "%02x", key[i]);
    }
    hex[SMB_KEY_LEN * 2] = '\n';
    hex[SMB_KEY_LEN * 2 + 1] = '\0';

    size_t len = strlen(hex);
    ssize_t n = write(fd, hex, len);
    close(fd);
    if (n != (ssize_t)len) {
        fprintf(stderr, "Failed to write key file\n");
        return 1;
    }

    return 0;
}
