#include "smb_secure.h"

#include <arpa/inet.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define ROTRIGHT(a, b) (((a) >> (b)) | ((a) << (32 - (b))))
#define CH(x, y, z) (((x) & (y)) ^ (~(x) & (z)))
#define MAJ(x, y, z) (((x) & (y)) ^ ((x) & (z)) ^ ((y) & (z)))
#define EP0(x) (ROTRIGHT(x, 2) ^ ROTRIGHT(x, 13) ^ ROTRIGHT(x, 22))
#define EP1(x) (ROTRIGHT(x, 6) ^ ROTRIGHT(x, 11) ^ ROTRIGHT(x, 25))
#define SIG0(x) (ROTRIGHT(x, 7) ^ ROTRIGHT(x, 18) ^ ((x) >> 3))
#define SIG1(x) (ROTRIGHT(x, 17) ^ ROTRIGHT(x, 19) ^ ((x) >> 10))

typedef struct {
    uint8_t data[64];
    uint32_t datalen;
    uint64_t bitlen;
    uint32_t state[8];
} sha256_ctx;

static const uint32_t k[64] = {
    0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5,
    0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
    0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3,
    0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
    0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc,
    0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
    0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7,
    0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
    0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13,
    0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
    0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3,
    0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
    0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5,
    0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
    0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208,
    0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2
};

static void sha256_transform(sha256_ctx *ctx, const uint8_t data[64]) {
    uint32_t a, b, c, d, e, f, g, h;
    uint32_t m[64];

    for (uint32_t i = 0, j = 0; i < 16; i++, j += 4) {
        m[i] = ((uint32_t)data[j] << 24) |
               ((uint32_t)data[j + 1] << 16) |
               ((uint32_t)data[j + 2] << 8) |
               ((uint32_t)data[j + 3]);
    }
    for (uint32_t i = 16; i < 64; i++) {
        m[i] = SIG1(m[i - 2]) + m[i - 7] + SIG0(m[i - 15]) + m[i - 16];
    }

    a = ctx->state[0];
    b = ctx->state[1];
    c = ctx->state[2];
    d = ctx->state[3];
    e = ctx->state[4];
    f = ctx->state[5];
    g = ctx->state[6];
    h = ctx->state[7];

    for (uint32_t i = 0; i < 64; i++) {
        uint32_t t1 = h + EP1(e) + CH(e, f, g) + k[i] + m[i];
        uint32_t t2 = EP0(a) + MAJ(a, b, c);
        h = g;
        g = f;
        f = e;
        e = d + t1;
        d = c;
        c = b;
        b = a;
        a = t1 + t2;
    }

    ctx->state[0] += a;
    ctx->state[1] += b;
    ctx->state[2] += c;
    ctx->state[3] += d;
    ctx->state[4] += e;
    ctx->state[5] += f;
    ctx->state[6] += g;
    ctx->state[7] += h;
}

static void sha256_init(sha256_ctx *ctx) {
    ctx->datalen = 0;
    ctx->bitlen = 0;
    ctx->state[0] = 0x6a09e667;
    ctx->state[1] = 0xbb67ae85;
    ctx->state[2] = 0x3c6ef372;
    ctx->state[3] = 0xa54ff53a;
    ctx->state[4] = 0x510e527f;
    ctx->state[5] = 0x9b05688c;
    ctx->state[6] = 0x1f83d9ab;
    ctx->state[7] = 0x5be0cd19;
}

static void sha256_update(sha256_ctx *ctx, const uint8_t *data, size_t len) {
    for (size_t i = 0; i < len; i++) {
        ctx->data[ctx->datalen] = data[i];
        ctx->datalen++;
        if (ctx->datalen == 64) {
            sha256_transform(ctx, ctx->data);
            ctx->bitlen += 512;
            ctx->datalen = 0;
        }
    }
}

static void sha256_final(sha256_ctx *ctx, uint8_t hash[32]) {
    uint32_t i = ctx->datalen;

    if (ctx->datalen < 56) {
        ctx->data[i++] = 0x80;
        while (i < 56) ctx->data[i++] = 0x00;
    } else {
        ctx->data[i++] = 0x80;
        while (i < 64) ctx->data[i++] = 0x00;
        sha256_transform(ctx, ctx->data);
        memset(ctx->data, 0, 56);
    }

    ctx->bitlen += ctx->datalen * 8;
    ctx->data[63] = (uint8_t)(ctx->bitlen);
    ctx->data[62] = (uint8_t)(ctx->bitlen >> 8);
    ctx->data[61] = (uint8_t)(ctx->bitlen >> 16);
    ctx->data[60] = (uint8_t)(ctx->bitlen >> 24);
    ctx->data[59] = (uint8_t)(ctx->bitlen >> 32);
    ctx->data[58] = (uint8_t)(ctx->bitlen >> 40);
    ctx->data[57] = (uint8_t)(ctx->bitlen >> 48);
    ctx->data[56] = (uint8_t)(ctx->bitlen >> 56);
    sha256_transform(ctx, ctx->data);

    for (i = 0; i < 4; i++) {
        hash[i]      = (uint8_t)((ctx->state[0] >> (24 - i * 8)) & 0xff);
        hash[i + 4]  = (uint8_t)((ctx->state[1] >> (24 - i * 8)) & 0xff);
        hash[i + 8]  = (uint8_t)((ctx->state[2] >> (24 - i * 8)) & 0xff);
        hash[i + 12] = (uint8_t)((ctx->state[3] >> (24 - i * 8)) & 0xff);
        hash[i + 16] = (uint8_t)((ctx->state[4] >> (24 - i * 8)) & 0xff);
        hash[i + 20] = (uint8_t)((ctx->state[5] >> (24 - i * 8)) & 0xff);
        hash[i + 24] = (uint8_t)((ctx->state[6] >> (24 - i * 8)) & 0xff);
        hash[i + 28] = (uint8_t)((ctx->state[7] >> (24 - i * 8)) & 0xff);
    }
}

static void sha256(const uint8_t *data, size_t len, uint8_t out[32]) {
    sha256_ctx ctx;
    sha256_init(&ctx);
    sha256_update(&ctx, data, len);
    sha256_final(&ctx, out);
}

static void hmac_sha256(const uint8_t *key, size_t key_len,
                        const uint8_t *data, size_t data_len,
                        uint8_t out[32]) {
    uint8_t k_ipad[64];
    uint8_t k_opad[64];
    uint8_t tk[32];

    if (key_len > 64) {
        sha256(key, key_len, tk);
        key = tk;
        key_len = 32;
    }

    memset(k_ipad, 0x36, sizeof(k_ipad));
    memset(k_opad, 0x5c, sizeof(k_opad));

    for (size_t i = 0; i < key_len; i++) {
        k_ipad[i] ^= key[i];
        k_opad[i] ^= key[i];
    }

    sha256_ctx ctx;
    sha256_init(&ctx);
    sha256_update(&ctx, k_ipad, sizeof(k_ipad));
    sha256_update(&ctx, data, data_len);
    sha256_final(&ctx, tk);

    sha256_init(&ctx);
    sha256_update(&ctx, k_opad, sizeof(k_opad));
    sha256_update(&ctx, tk, sizeof(tk));
    sha256_final(&ctx, out);
}

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

static void stream_xor(const uint8_t key[SMB_KEY_LEN],
                       const uint8_t nonce[SMB_NONCE_LEN],
                       const uint8_t *in,
                       uint8_t *out,
                       size_t len) {
    uint32_t counter = 0;
    uint8_t block[32];
    size_t off = 0;

    while (off < len) {
        sha256_ctx ctx;
        sha256_init(&ctx);
        sha256_update(&ctx, key, SMB_KEY_LEN);
        sha256_update(&ctx, nonce, SMB_NONCE_LEN);
        uint32_t ctr_be = htonl(counter);
        sha256_update(&ctx, (uint8_t *)&ctr_be, sizeof(ctr_be));
        sha256_final(&ctx, block);

        size_t chunk = len - off;
        if (chunk > sizeof(block)) chunk = sizeof(block);
        for (size_t i = 0; i < chunk; i++) {
            out[off + i] = in[off + i] ^ block[i];
        }
        off += chunk;
        counter++;
    }
}

static int ct_equal(const uint8_t *a, const uint8_t *b, size_t len) {
    uint8_t diff = 0;
    for (size_t i = 0; i < len; i++) diff |= (uint8_t)(a[i] ^ b[i]);
    return diff == 0;
}

static int hex_value(int c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return 10 + (c - 'a');
    if (c >= 'A' && c <= 'F') return 10 + (c - 'A');
    return -1;
}

static int load_key_file(const char *path, uint8_t key[SMB_KEY_LEN]) {
    int fd = open(path, O_RDONLY);
    if (fd < 0) return -1;

    char buf[128];
    ssize_t n = read(fd, buf, sizeof(buf) - 1);
    close(fd);
    if (n <= 0) return -1;

    size_t len = (size_t)n;
    while (len > 0 && (buf[len - 1] == '\n' || buf[len - 1] == '\r' || buf[len - 1] == ' ' || buf[len - 1] == '\t')) {
        len--;
    }
    buf[len] = '\0';

    if (len != (SMB_KEY_LEN * 2)) return -1;

    for (size_t i = 0; i < SMB_KEY_LEN; i++) {
        int hi = hex_value(buf[i * 2]);
        int lo = hex_value(buf[i * 2 + 1]);
        if (hi < 0 || lo < 0) return -1;
        key[i] = (uint8_t)((hi << 4) | lo);
    }
    return 0;
}

int smb_load_key(uint8_t key[SMB_KEY_LEN], const char *key_path) {
    if (key_path) {
        const char *path = key_path;
        if (path[0] == '\0') path = SMB_DEFAULT_KEY_PATH;
        return load_key_file(path, key);
    }

    const char *pass = getenv("SMB_KEY");
    if (pass && pass[0] != '\0') {
        sha256((const uint8_t *)pass, strlen(pass), key);
        return 0;
    }

    if (load_key_file(SMB_DEFAULT_KEY_PATH, key) == 0) {
        return 0;
    }

    return -1;
}

size_t smb_secure_pack(const uint8_t key[SMB_KEY_LEN],
                        const uint8_t *plain,
                        size_t plain_len,
                        uint8_t *out,
                        size_t out_cap) {
    if (plain_len > 0xFFFFu) return 0;
    size_t needed = SMB_SECURE_HEADER_LEN + plain_len + SMB_MAC_LEN;
    if (out_cap < needed) return 0;

    memcpy(out, SMB_MAGIC, SMB_MAGIC_LEN);
    uint8_t *nonce = out + SMB_MAGIC_LEN;
    if (random_bytes(nonce, SMB_NONCE_LEN) != 0) return 0;

    uint16_t len_be = htons((uint16_t)plain_len);
    memcpy(out + SMB_MAGIC_LEN + SMB_NONCE_LEN, &len_be, sizeof(len_be));

    uint8_t *cipher = out + SMB_SECURE_HEADER_LEN;
    stream_xor(key, nonce, plain, cipher, plain_len);

    uint8_t mac[SMB_MAC_LEN];
    hmac_sha256(key, SMB_KEY_LEN, out, SMB_SECURE_HEADER_LEN + plain_len, mac);
    memcpy(out + SMB_SECURE_HEADER_LEN + plain_len, mac, SMB_MAC_LEN);

    return needed;
}

int smb_secure_unpack(const uint8_t key[SMB_KEY_LEN],
                      const uint8_t *pkt,
                      size_t pkt_len,
                      uint8_t *plain,
                      size_t plain_cap,
                      size_t *plain_len) {
    if (pkt_len < SMB_SECURE_OVERHEAD) return -1;
    if (memcmp(pkt, SMB_MAGIC, SMB_MAGIC_LEN) != 0) return -1;

    const uint8_t *nonce = pkt + SMB_MAGIC_LEN;
    uint16_t len_be;
    memcpy(&len_be, pkt + SMB_MAGIC_LEN + SMB_NONCE_LEN, sizeof(len_be));
    size_t clen = ntohs(len_be);

    size_t expected = SMB_SECURE_HEADER_LEN + clen + SMB_MAC_LEN;
    if (expected != pkt_len) return -1;
    if (clen > plain_cap) return -1;

    const uint8_t *cipher = pkt + SMB_SECURE_HEADER_LEN;
    const uint8_t *mac = pkt + SMB_SECURE_HEADER_LEN + clen;

    uint8_t calc[SMB_MAC_LEN];
    hmac_sha256(key, SMB_KEY_LEN, pkt, SMB_SECURE_HEADER_LEN + clen, calc);
    if (!ct_equal(calc, mac, SMB_MAC_LEN)) return -1;

    stream_xor(key, nonce, cipher, plain, clen);
    *plain_len = clen;
    return 0;
}
