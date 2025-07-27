#ifndef CRYPTO_H
#define CRYPTO_H

#include <stddef.h>

#define CRYPTO_KEY_LEN 32    // 256 bit
#define CRYPTO_NONCE_LEN 12  // GCM standard
#define CRYPTO_TAG_LEN 16

// Encryption
int encrypt_data(const unsigned char *plaintext, size_t plaintext_len,
                 const unsigned char *key, unsigned char *ciphertext, unsigned char *nonce, unsigned char *tag);

// Decryption
int decrypt_data(const unsigned char *ciphertext, size_t ciphertext_len,
                const unsigned char *key, const unsigned char *nonce, unsigned char *tag, unsigned char *plaintext);

#endif