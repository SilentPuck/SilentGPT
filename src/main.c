#include <stdio.h>
#include "config.h"
#include <string.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <openssl/sha.h>
#include "cli.h"
#include "history.h"
#include <termios.h>

void print_help()
{
    printf("\n=== SilentGPT Startup Flags ===\n\n");
    printf("--token <name>   Set named config (stored in config_<name>.enc)\n");
    printf("--secure         Enable password protection (AES key from password)\n");
    printf("--help           Show this help and exit\n");
    printf("--test           Test encryption config (dev use only)\n\n");
    printf("Usage example:\n");
    printf("  ./silentgpt --token test --secure\n");

    printf("\nThen use /help inside to see available commands.\n");
    printf("With silence and precision. — SilentPuck 🕶️\n\n");

}
void read_password(char *buffer, size_t size)
{
    struct termios oldt, newt;
    printf("Enter password: ");
    fflush(stdout);

    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;
    newt.c_lflag &= ~ECHO;

    tcsetattr(STDIN_FILENO, TCSANOW, &newt);

    if (fgets(buffer, size, stdin))
    {
        size_t len = strlen(buffer);
        if (len > 0 && buffer[len - 1] == '\n')
        {
            buffer[len - 1] = '\0';
        }
    }

    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
    printf("\n");
}

int main(int argc, char *argv[])
{
    config_t cfg = {0};
    strncpy(cfg.model, "gpt-3.5-turbo", sizeof(cfg.model) - 1);

    for (int i = 1; i < argc; i++)
    {
        if (strcmp(argv[i], "--test") == 0)
        {
            strncpy(cfg.api_key, "testkey123456789", sizeof(cfg.api_key) - 1);
            save_config(&cfg);
            printf("Save API-token!\n");

            config_t cfg2 = {0};
            load_config(&cfg2);
            printf("Load API-token: %s\n", cfg2.api_key);

            return 0;

        }
        else if (strcmp(argv[i], "--token") == 0)
        {
            if (i + 1 >= argc)
            {
                fprintf(stderr, "Error: --token requires a name.\n");
                exit(EXIT_FAILURE);
            }
            strncpy(cfg.token_name, argv[i + 1], sizeof(cfg.token_name) - 1);
            cfg.token_name[sizeof(cfg.token_name) - 1] = '\0';

            snprintf(cfg.config_path, sizeof(cfg.config_path), "%s/config_%s.enc", CONFIG_DIR, cfg.token_name);
            i++;
            
        }
        else if (strcmp(argv[i], "--secure") == 0)
        {
            cfg.secure_mode = 1;
            char password[128];
            read_password(password, sizeof(password));
            SHA256((unsigned char*)password, strlen(password), cfg.encryption_key);
            printf("[SECURE] Secure mode active: key derived from password (SHA-256)\n");
        }
        
        else if (strcmp(argv[i], "--help") == 0)
        {
            print_help();
            return 0;
        }
        
        
    }
    if (cfg.config_path[0] == '\0')
    {
        snprintf(cfg.config_path, sizeof(cfg.config_path), "%s/%s", CONFIG_DIR, CONFIG_FILE);
    }

    
    mkdir(CONFIG_DIR, 0700);
    mkdir("history", 0700);
    if (access(cfg.config_path, F_OK) == -1)
    {
        char input[512];
        printf("Enter API-token: ");

        if (fgets(input, sizeof(input), stdin))
        {
            size_t len = strlen(input);
            if (len >= sizeof(cfg.api_key))
            {
                fprintf(stderr, "API-token is too long! Max 127 symbols.\n");
                exit(EXIT_FAILURE);
            }
            
            if (len > 0 && input[len - 1] == '\n')
            {
                input[len - 1] = '\0';
            }
            
        }
        strncpy(cfg.api_key, input, sizeof(cfg.api_key) - 1);
        cfg.api_key[sizeof(cfg.api_key) - 1] = '\0';
        save_config(&cfg);
    }
    if (!load_index())
    {
        fprintf(stderr, "[WARN] Could not load chat index. Starting fresh.\n");
    }

    
    if (!load_config(&cfg))
    {
        fprintf(stderr, "Fatal: could not load %s\n", cfg.config_path);
        exit(EXIT_FAILURE);
    }

    
    
    printf("🕶 SilentGPT 🕶 v1.0 -- ready\n");
    char input[4096];
    while (1)
    {
        char title[128] = "None";

        for (int i = 0; i < chat_index_count; i++)
        {
            char full_path[256];
            snprintf(full_path, sizeof(full_path), "history/%s", chat_index[i].file);
            if (strcmp(cfg.chat_path, full_path) == 0)
            {
                strncpy(title, chat_index[i].title, sizeof(title) - 1);
                break;
            }
        }

        printf("[chat: %s] > ", title);
        fflush(stdout);

        if (!fgets(input, sizeof(input), stdin))
        {    
            break;
        }
        handle_input(&cfg, input);

    }

    return 0;
}
