#ifndef SMB_SECURE_H
#define SMB_SECURE_H

#include <stddef.h>
#include <stdint.h>

#define SMB_MAGIC "SMB1"
#define SMB_MAGIC_LEN 4
#define SMB_KEY_LEN 32
#define SMB_NONCE_LEN 12
#define SMB_MAC_LEN 32
#define SMB_SECURE_HEADER_LEN (SMB_MAGIC_LEN + SMB_NONCE_LEN + 2)
#define SMB_SECURE_OVERHEAD (SMB_SECURE_HEADER_LEN + SMB_MAC_LEN)
#define SMB_DEFAULT_KEY_PATH "build/.key"

int smb_load_key(uint8_t key[SMB_KEY_LEN], const char *key_path);
size_t smb_secure_pack(const uint8_t key[SMB_KEY_LEN],
                        const uint8_t *plain,
                        size_t plain_len,
                        uint8_t *out,
                        size_t out_cap);
int smb_secure_unpack(const uint8_t key[SMB_KEY_LEN],
                      const uint8_t *pkt,
                      size_t pkt_len,
                      uint8_t *plain,
                      size_t plain_cap,
                      size_t *plain_len);

#endif
