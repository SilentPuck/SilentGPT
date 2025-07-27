#ifndef CONFIG_H
#define CONFIG_H

#define CONFIG_DIR "config"
#define CONFIG_FILE "config.enc"

typedef struct
{
    char api_key[512];
    int loaded;
    char token_name[64];
    char config_path[128];
    int secure_mode;                    // 1 if enabled --secure
    unsigned char encryption_key[32];   // 256 bit = 32 byte
    char chat_path[128];                // path to current chat
    char model[64];
} config_t;

int load_config(config_t *cfg);

int save_config(const config_t *cfg);

#endif