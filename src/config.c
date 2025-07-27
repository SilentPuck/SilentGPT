#include <stdio.h>
#include "config.h"
#include "crypto.h"
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

int load_config(config_t *cfg)
{
    unsigned char buffer[1024];

    int fd = open(cfg->config_path, O_RDONLY);
    if (fd == -1)
    {
        perror("open");
        return 0;
    }

    ssize_t numRead = read(fd, buffer, sizeof(buffer));
    close(fd);

    if (numRead <= 33)
    {
        fprintf(stderr, "load_config: file too small or corrupt\n");
        return 0;
    }

  
    if (buffer[0] != 'S' || buffer[1] != 'G' || buffer[2] != 'P' || buffer[3] != 'T')
    {
        fprintf(stderr, "load_config: invalid file header\n");
        return 0;
    }

    
    cfg->secure_mode = buffer[4];

    const unsigned char *key = cfg->secure_mode ?
        cfg->encryption_key :
        (const unsigned char *)"testkey123456789testkey123456789";

    
    unsigned char nonce[CRYPTO_NONCE_LEN];
    unsigned char tag[CRYPTO_TAG_LEN];
    unsigned char ciphertext[512];

    memcpy(nonce, buffer + 5, CRYPTO_NONCE_LEN);
    memcpy(tag, buffer + 17, CRYPTO_TAG_LEN);
    size_t cipher_len = numRead - 33;
    memcpy(ciphertext, buffer + 33, cipher_len);

    if (!decrypt_data(ciphertext, cipher_len, key, nonce, tag, (unsigned char *)cfg->api_key))
    {
        fprintf(stderr, "Load_config: error decryption\n");
        return 0;
    }

    cfg->loaded = 1;
    return 1;
}


int save_config(const config_t *cfg)
{
    unsigned char nonce[CRYPTO_NONCE_LEN]; 
    unsigned char tag[CRYPTO_TAG_LEN];     
    unsigned char ciphertext[512];         
    unsigned char outbuf[1024];            

    const unsigned char *key = cfg->secure_mode ?
        cfg->encryption_key :
        (const unsigned char *)"testkey123456789testkey123456789";


    encrypt_data((unsigned char *)cfg->api_key, strlen(cfg->api_key), key, ciphertext, nonce, tag);
    extern int GLOBAL_CIPHER_LEN;

    outbuf[0] = 'S';
    outbuf[1] = 'G';
    outbuf[2] = 'P';
    outbuf[3] = 'T';
    outbuf[4] = cfg->secure_mode ? 1 : 0;

    memcpy(outbuf + 5, nonce, CRYPTO_NONCE_LEN);
    memcpy(outbuf + 17, tag, CRYPTO_TAG_LEN);
    memcpy(outbuf + 33, ciphertext, GLOBAL_CIPHER_LEN);

    size_t total_len = 33 + GLOBAL_CIPHER_LEN;

    int fd = open(cfg->config_path, O_WRONLY | O_CREAT | O_TRUNC, 0600);
    if (fd == -1)
    {
        perror("open");
        return 0;
    }

    ssize_t written = write(fd, outbuf, total_len);
    close(fd);

    if (written != (ssize_t)total_len)
    {
        fprintf(stderr, "save_config: write failed\n");
        return 0;
    }

    return 1;
}
