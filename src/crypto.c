#include "crypto.h"
#include <stdio.h>
#include <openssl/evp.h>
#include <openssl/rand.h>

int GLOBAL_CIPHER_LEN = 0;

int encrypt_data(const unsigned char *plaintext, size_t plaintext_len,
                 const unsigned char *key, unsigned char *ciphertext, unsigned char *nonce, unsigned char *tag)
{
    if (RAND_bytes(nonce, CRYPTO_NONCE_LEN) != 1)
    {
        fprintf(stderr, "ERROR: generation nonce\n");
        return 0;
    }
    EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
    if (!ctx)
    {
        fprintf(stderr, "Creation error EVP_CIPHER_CTX\n");
        return 0;
    }
    if (EVP_EncryptInit_ex(ctx, EVP_aes_256_gcm(), NULL, NULL, NULL) != 1)
    {
        fprintf(stderr, "Initialization error AES-256-GCM\n");
        EVP_CIPHER_CTX_free(ctx);
        return 0;
    }
    if (EVP_EncryptInit_ex(ctx, NULL, NULL, key, nonce) != 1)
    {
        fprintf(stderr, "Error setting key and nonce\n");
        EVP_CIPHER_CTX_free(ctx);
        return 0;
    }
    int len = 0;
    int ciphertext_len = 0;
    if (EVP_EncryptUpdate(ctx, ciphertext, &len, plaintext, plaintext_len) != 1)
    {
        fprintf(stderr, "Data encryption error\n");
        EVP_CIPHER_CTX_free(ctx);
        return 0;
    }
    ciphertext_len = len;
    if (EVP_EncryptFinal_ex(ctx, ciphertext + ciphertext_len, &len) != 1)
    {
        fprintf(stderr, "Encryption finalization error\n");
        EVP_CIPHER_CTX_free(ctx);
        return 0;
    }
    ciphertext_len += len;
    

    if (EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_GET_TAG, CRYPTO_TAG_LEN, tag) != 1)
    {
        fprintf(stderr, "Error getting GCM-tag\n");
        EVP_CIPHER_CTX_free(ctx);
        return 0;
    }
    GLOBAL_CIPHER_LEN = ciphertext_len;

    EVP_CIPHER_CTX_free(ctx);
    return 1;
}

int decrypt_data(const unsigned char *ciphertext, size_t ciphertext_len,
                const unsigned char *key, const unsigned char *nonce, unsigned char *tag, unsigned char *plaintext)
{
    EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
    if (!ctx)
    {
        fprintf(stderr, "Error creating context\n");
        return 0;
    }

 
    if (EVP_DecryptInit_ex(ctx, EVP_aes_256_gcm(), NULL, NULL, NULL) != 1)
    {
        fprintf(stderr, "Initialization error AES-256-GCM\n");
        EVP_CIPHER_CTX_free(ctx);
        return 0;
    }

    if (EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_IVLEN, CRYPTO_NONCE_LEN, NULL) != 1)
    {
        fprintf(stderr, "Failed to set IV length\n");
        EVP_CIPHER_CTX_free(ctx);
        return 0;
    }


    if (EVP_DecryptInit_ex(ctx, NULL, NULL, key, nonce) != 1)
    {
        fprintf(stderr, "Error setting key and nonce\n");
        EVP_CIPHER_CTX_free(ctx);
        return 0;
    }

    if (EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_TAG, CRYPTO_TAG_LEN, (void *)tag) != 1)
    {
        fprintf(stderr, "Error setting GCM-tag\n");
        EVP_CIPHER_CTX_free(ctx);
        return 0;
    }

    int len = 0;
    int plaintext_len = 0;

    if (EVP_DecryptUpdate(ctx, plaintext, &len, ciphertext, ciphertext_len) != 1)
    {
        fprintf(stderr, "Error decrypting\n");
        EVP_CIPHER_CTX_free(ctx);
        return 0;
    }
    plaintext_len = len;

    if (EVP_DecryptFinal_ex(ctx, plaintext + plaintext_len, &len) != 1)
    {
        fprintf(stderr, "Error: tag did not match, data is corrupted, or key is invalid\n");
        EVP_CIPHER_CTX_free(ctx);
        return 0;
    }

    plaintext_len += len;
    plaintext[plaintext_len] = '\0';

    EVP_CIPHER_CTX_free(ctx);
    return 1;
}
