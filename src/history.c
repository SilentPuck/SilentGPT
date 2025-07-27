#include <stdio.h>
#include "config.h"
#include "crypto.h"
#include <time.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <string.h>
#include "cJSON.h"
#include "history.h"
#include <stdlib.h>

chat_meta_t chat_index[MAX_CHATS];
int chat_index_count = 0;


int create_new_chat(config_t *cfg)
{
    mkdir("history", 0700);
    time_t now = time(NULL);
    snprintf(cfg->chat_path, sizeof(cfg->chat_path), "history/chat_%ld.enc", now);

    extern chat_meta_t chat_index[];
    extern int chat_index_count;

    if (chat_index_count < MAX_CHATS)
    {
        snprintf(chat_index[chat_index_count].file, sizeof(chat_index[0].file), "chat_%ld.enc", now);
        strncpy(chat_index[chat_index_count].title, "Untitled", sizeof(chat_index[0].title));
        chat_index[chat_index_count].created = now;
        chat_index_count++;
    }


    const unsigned char *plaintext = (const unsigned char *)"[]";
    size_t plaintext_len = 2;

    unsigned char nonce[CRYPTO_NONCE_LEN];
    unsigned char tag[CRYPTO_TAG_LEN];
    unsigned char ciphertext[128];

    const unsigned char *key = cfg->secure_mode ? cfg->encryption_key : (const unsigned char *)"testkey123456789testkey123456789";

    encrypt_data(plaintext, plaintext_len, key, ciphertext, nonce, tag);

    unsigned char outbuf[CRYPTO_NONCE_LEN + CRYPTO_TAG_LEN + plaintext_len];
    memcpy(outbuf, nonce, CRYPTO_NONCE_LEN);
    memcpy(outbuf + 12, tag, CRYPTO_TAG_LEN);
    memcpy(outbuf + 28, ciphertext, plaintext_len);

    int fd = open(cfg->chat_path, O_WRONLY | O_CREAT | O_TRUNC, 0600);
    if (fd == -1)
    {
        perror("open chat file");
        return 0;
    }
    
    ssize_t written = write(fd, outbuf, sizeof(outbuf));
    close(fd);
    save_index();
    return (written == (ssize_t)sizeof(outbuf));
}

int write_message(config_t *cfg, const message_t *msg)
{
    int fd = open(cfg->chat_path, O_RDONLY);
    if (fd == -1)
    {
        perror("open chat file");
        return 0;
    }

    unsigned char filebuf[8192];
    ssize_t numRead = read(fd, filebuf, sizeof(filebuf));
    close(fd);

    if (numRead <= 28)
    {
        fprintf(stderr, "Chat file too small or corrupted\n");
        return 0;
    }

    unsigned char nonce[CRYPTO_NONCE_LEN];
    unsigned char tag[CRYPTO_TAG_LEN];
    unsigned char ciphertext[8096];

    memcpy(nonce, filebuf, 12);
    memcpy(tag, filebuf + 12, 16);
    memcpy(ciphertext, filebuf + 28, numRead - 28);

    const unsigned char *key = cfg->secure_mode ? cfg->encryption_key : (const unsigned char *)"testkey123456789testkey123456789";

    unsigned char plaintext[8096] = {0};
    if (!decrypt_data(ciphertext, numRead - 28, key, nonce, tag, plaintext))
    {
        fprintf(stderr, "Failed to decrypt chat file\n");
        return 0;
    }

    cJSON *root = cJSON_Parse((const char *)plaintext);
    if (!root || !cJSON_IsArray(root))
    {
        fprintf(stderr, "Invalid chat JSON format\n");
        cJSON_Delete(root);
        return 0;
    }

    cJSON *entry = cJSON_CreateObject();

    cJSON_AddStringToObject(entry, "role", msg->role);
    cJSON_AddStringToObject(entry, "content", msg->content);
    cJSON_AddNumberToObject(entry, "timestamp", msg->timestamp);
    cJSON_AddNumberToObject(entry, "tokens", msg->tokens);

    cJSON_AddItemToArray(root, entry);

    char *updated_json = cJSON_PrintUnformatted(root);
    cJSON_Delete(root); 

    if (!updated_json)
    {
        fprintf(stderr, "Failed to serialize updated chat\n");
        return 0;
    }

    size_t json_len = strlen(updated_json);

    unsigned char new_nonce[CRYPTO_NONCE_LEN];
    unsigned char new_tag[CRYPTO_TAG_LEN];
    unsigned char new_ciphertext[16384];

    encrypt_data((unsigned char *)updated_json, json_len, key,
             new_ciphertext, new_nonce, new_tag);

    extern int GLOBAL_CIPHER_LEN;
    free(updated_json);

    int out_fd = open(cfg->chat_path, O_WRONLY | O_CREAT | O_TRUNC, 0600);
    if (out_fd == -1)
    {
        perror("open for write");
        return 0;
    }

    unsigned char outbuf[CRYPTO_NONCE_LEN + CRYPTO_TAG_LEN + 16384];
    memcpy(outbuf, new_nonce, CRYPTO_NONCE_LEN);
    memcpy(outbuf + 12, new_tag, CRYPTO_TAG_LEN);
    memcpy(outbuf + 28, new_ciphertext, GLOBAL_CIPHER_LEN);

    ssize_t written = write(out_fd, outbuf, 28 + GLOBAL_CIPHER_LEN);
    close(out_fd);

    //printf("[DEBUG] write_message() result: written = %zd, expected = %d\n", written, 28 + GLOBAL_CIPHER_LEN);

    return (written == (ssize_t)(28 + GLOBAL_CIPHER_LEN));
}

int load_index()
{
    int fd = open("history/index.json", O_RDONLY);
    if (fd == -1)
    {
        perror("open index.json");
        return 0;
    }

    char buffer[16384];
    ssize_t size = read(fd, buffer, sizeof(buffer) - 1);
    close(fd);

    if (size <= 0)
    {
        fprintf(stderr, "index.json read failed\n");
        return 0;
    }
    buffer[size] = '\0';

    cJSON *arr = cJSON_Parse(buffer);
    if (!arr || !cJSON_IsArray(arr))
    {
        fprintf(stderr, "index.json not valid array\n");
        return 0;
    }
    
    chat_index_count = 0;
    for (int i = 0; i < cJSON_GetArraySize(arr) && i < MAX_CHATS; i++)
    {
        cJSON *item = cJSON_GetArrayItem(arr, i);
        cJSON *f = cJSON_GetObjectItem(item, "file");
        cJSON *t = cJSON_GetObjectItem(item, "title");
        cJSON *c = cJSON_GetObjectItem(item, "created");
        if (!cJSON_IsString(f) || !cJSON_IsString(t) || !cJSON_IsNumber(c))
        {
            continue;
        }
        strncpy(chat_index[chat_index_count].file, f->valuestring, sizeof(chat_index[0].file));
        strncpy(chat_index[chat_index_count].title, t->valuestring, sizeof(chat_index[0].title));
        chat_index[chat_index_count].created = (time_t)c->valuedouble;

        chat_index_count++;
    }
    cJSON_Delete(arr);
    return 1;
}

int save_index() {
    cJSON *arr = cJSON_CreateArray();

    for (int i = 0; i < chat_index_count; i++)
    {
        cJSON *item = cJSON_CreateObject();
        cJSON_AddStringToObject(item, "file", chat_index[i].file);
        cJSON_AddStringToObject(item, "title", chat_index[i].title);
        cJSON_AddNumberToObject(item, "created", (double)chat_index[i].created);
        cJSON_AddItemToArray(arr, item);
    }

    char *json_data = cJSON_PrintUnformatted(arr);
    cJSON_Delete(arr);

    if (!json_data)
    {
        fprintf(stderr, "Failed to serialize index\n");
        return 0;
    }
    int fd = open("history/index.json", O_WRONLY | O_CREAT | O_TRUNC, 0600);
    if (fd == -1)
    {
        perror("open index.json for write");
        free(json_data);
        return 0;
    }

    size_t len = strlen(json_data);
    ssize_t written = write(fd, json_data, len);
    close(fd);
    free(json_data);

    if (written != (ssize_t)len)
    {
        fprintf(stderr, "Failed to write full index.json\n");
        return 0;
    }

    return 1;
}


int rename_chat(int index, const char *new_title)
{
    if (index <  0 || index >= chat_index_count)
    {
        fprintf(stderr, "[ERROR] Invalid chat index.\n");
        return 0;
    }
    strncpy(chat_index[index].title, new_title, sizeof(chat_index[0].file) - 1);
    save_index();
    return 1;
}
void list_chats(config_t *cfg)
{
    if (chat_index_count == 0)
    {
        printf("[!] No chats found.\n");
        return;
    }

    for (int i = 0; i < chat_index_count; i++)
    {
        char full_path[256];
        snprintf(full_path, sizeof(full_path), "history/%.127s", chat_index[i].file);
        full_path[sizeof(full_path) - 1] = '\0';

        int is_active = (strcmp(cfg->chat_path, full_path) == 0);

        printf("[%d]%s %s - %s\n",
               i,
               is_active ? " *" : "  ",
               chat_index[i].title,
               chat_index[i].file);
    }
}

int validate_chat_file(config_t *cfg)
{
    int fd = open(cfg->chat_path, O_RDONLY);
    if (fd == -1)
    {
        perror("open chat file");
        return 0;
    }

    unsigned char filebuf[8192];
    ssize_t numRead = read(fd, filebuf, sizeof(filebuf));
    close(fd);

    if (numRead <= 28)
    {
        fprintf(stderr, "[ERROR] Chat file too small to validate\n");
        return 0;
    }

    unsigned char nonce[12];
    unsigned char tag[16];
    unsigned char ciphertext[8192];

    memcpy(nonce, filebuf, 12);
    memcpy(tag, filebuf + 12, 16);
    memcpy(ciphertext, filebuf + 28, numRead - 28);

    const unsigned char *key = cfg->secure_mode
        ? cfg->encryption_key
        : (const unsigned char *)"testkey123456789testkey123456789";

    unsigned char plaintext[8192];
    if (!decrypt_data(ciphertext, numRead - 28, key, nonce, tag, plaintext))
    {
        fprintf(stderr, "[ERROR] Cannot decrypt chat: wrong password or corrupted file.\n");
        return 0;
    }

    return 1;
}

int load_chat(config_t *cfg, int index)
{
    if (index < 0 || index >= chat_index_count)
    {
        fprintf(stderr, "[ERROR] Invalid chat index.\n");
        return 0;
    }
    snprintf(cfg->chat_path, sizeof(cfg->chat_path), "history/%s", chat_index[index].file);
    return 1;
}

int export_chat(config_t *cfg, int to_file)
{
    int fd = open(cfg->chat_path, O_RDONLY);
    if (fd == -1)
    {
        perror("open chat file");
        return 0;
    }

    unsigned char filebuf[16384];
    ssize_t numRead = read(fd, filebuf, sizeof(filebuf));
    close(fd);

    if (numRead <= 28)
    {
        fprintf(stderr, "Chat file too small or corrupted\n");
        return 0;
    }
    
    unsigned char nonce[CRYPTO_NONCE_LEN];
    unsigned char tag[CRYPTO_TAG_LEN];
    unsigned char ciphertext[16384];

    memcpy(nonce, filebuf, CRYPTO_NONCE_LEN);
    memcpy(tag, filebuf + CRYPTO_NONCE_LEN, CRYPTO_TAG_LEN);
    memcpy(ciphertext, filebuf + 28, numRead - 28);

    const unsigned char *key = cfg->secure_mode
        ? cfg->encryption_key
        : (const unsigned char *)"testkey123456789testkey123456789";

    unsigned char plaintext[16384] = {0};
    if (!decrypt_data(ciphertext, numRead - 28, key, nonce, tag, plaintext))
    {
        fprintf(stderr, "Failed to decrypt chat\n");
        return 0;
    }

    cJSON *root = cJSON_Parse((const char *)plaintext);
    if (!root || !cJSON_IsArray(root))
    {
        fprintf(stderr, "Invalid chat format\n");
        cJSON_Delete(root);
        return 0;
    }

    if (to_file)
    {
        char *json_data = cJSON_PrintUnformatted(root);
        if (!json_data)
        {
            fprintf(stderr, "Failed to serialize JSON\n");
            cJSON_Delete(root);
            return 0;
        }

        int out_fd = open("history/export.json", O_WRONLY | O_CREAT | O_TRUNC, 0600);
        if (out_fd == -1)
        {
            perror("open export.json");
            free(json_data);
            cJSON_Delete(root);
            return 0;
        }

        ssize_t written = write(out_fd, json_data, strlen(json_data));
        close(out_fd);
        free(json_data);

        if (written <= 0)
        {
            fprintf(stderr, "Failed to write export file\n");
            cJSON_Delete(root);
            return 0;
        }
    }
    else
    {
        for (int i = 0; i < cJSON_GetArraySize(root); i++)
        {
            cJSON *item = cJSON_GetArrayItem(root, i);
            cJSON *role = cJSON_GetObjectItem(item, "role");
            cJSON *content = cJSON_GetObjectItem(item, "content");

            if (cJSON_IsString(role) && cJSON_IsString(content))
            {
                printf("\n========= %s =========\n%s\n", role->valuestring, content->valuestring);
                printf("=============================\n");
            }
        }
    }


    cJSON_Delete(root);
    return 1;
}

int delete_chat(int index)
{
    if (index < 0 || index >= chat_index_count)
    {
        fprintf(stderr, "[ERROR] Invalid chat index.\n");
        return 0;
    }

    char full_path[256];
    snprintf(full_path, sizeof(full_path), "history/%s", chat_index[index].file);

    if (unlink(full_path) == -1)
    {
        perror("unlink");
        return 0;
    }
    
    for (int i = index; i < chat_index_count - 1; i++)
    {
        chat_index[i] = chat_index[i + 1];
    }
    chat_index_count--;

    save_index();
    return 1;
    
}
