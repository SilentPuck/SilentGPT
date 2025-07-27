#include <stdio.h>
#include "cli.h"
#include <string.h>
#include "history.h"
#include <stdlib.h>
#include <time.h>
#include "api.h"

void show_help()
{
    printf("\n=== SilentGPT Commands ===\n\n");
    printf("/new                - Start a new chat\n");
    printf("/list               - List all chats\n");
    printf("/load <index|name>  - Load chat by index or name\n");
    printf("/rename <i> <title> - Rename chat\n");
    printf("/delete <i>         - Delete chat\n");
    printf("/model <name>       - Set model (gpt-3.5-turbo, gpt-4, gpt-4o...)\n");
    printf("/prompt <text>      - Send prompt to assistant\n");
    printf("/export             - Export chat to terminal\n");
    printf("/export --to-file   - Export chat to export.json\n");
    printf("/help               - Show this help\n");
    printf("/exit               - Exit program\n");
    printf("\nWith silence and precision. — SilentPuck 🕶️\n\n");
}



void handle_input(config_t *cfg, const char *input)
{
    if (strncmp(input, "/exit", 5) == 0)
    {
        printf("With silence and precision. — SilentPuck 🕶️\n");
        exit(0);
    }
    else if (strncmp(input, "/new", 4) == 0)
    {
        if (create_new_chat(cfg))
        {
            printf("[OK] New chat created: %s\n", cfg->chat_path);
        }
        else
        {
            fprintf(stderr, "[ERROR] Failed to create chat.\n");
        }
    }

    else if (strncmp(input, "/model ", 7) == 0)
    {
        const char *requested = input + 7;

        char *newline = strchr((char *)requested, '\n');
        if (newline) *newline = '\0';

        static const char *known_models[] = {
            "gpt-3.5-turbo",
            "gpt-4",
            "gpt-4.1",
            "gpt-4-turbo",
            "gpt-4o",
            NULL
        };

        int found = 0;
        for (int i = 0; known_models[i]; i++)
        {
            if (strcmp(requested, known_models[i]) == 0)
            {
                strncpy(cfg->model, requested, sizeof(cfg->model) - 1);
                cfg->model[sizeof(cfg->model) - 1] = '\0';
                printf("[OK] Model set to: %s\n", cfg->model);
                found = 1;
                break;
            }
        }
        if (!found)
        {
            fprintf(stderr, "[ERROR] Unknown model: %s\n", requested);
        }
    }

    else if (strncmp(input, "/prompt ", 8) == 0)
    {
        const char *text = input + 8;
        if (send_prompt(cfg, text))
        {
            printf("[OK] Promt sent and response saved.\n");
        }
        else
        {
            fprintf(stderr, "[ERROR] Failed to send prompt.\n");
        }
        
    }
    else if (strncmp(input, "/list", 5) == 0)
    {
        list_chats(cfg);
        return;
    }
    else if (strncmp(input, "/rename ", 8) == 0)
    {
        const char *args = input + 8;
        int index;
        char title[128] = {0};
        if (sscanf(args, "%d %[^\n]", &index, title) != 2)
        {
            fprintf(stderr, "[ERROR] Usage: /rename <index> <new title>\n");
            return;
        }
        if (rename_chat(index,title))
        {
            printf("[OK] Chat [%d] rename to: %s\n", index, title);
        }
        else
        {
            fprintf(stderr, "[ERROR] Failed to rename chat.\n");
        }
        return;
    }

    else if (strncmp(input, "/delete", 7) == 0)
    {
        int index = -1;
        sscanf(input + 8, "%d", &index);
        if (delete_chat(index))
        {
            printf("[OK] Chat [%d] deleted.\n", index);
        }
        else
        {
            fprintf(stderr, "[ERROR] Failed to delete chat [%d].\n", index);
        }
        
    }

    else if (strncmp(input, "/load ", 6) == 0)
    {
        int index = atoi(input + 6);
        if (load_chat(cfg, index))
        {
            printf("[OK] Chat [%d] loaded: %s\n", index, cfg->chat_path);
            if (!validate_chat_file(cfg))
            {
                fprintf(stderr, "[ERROR] Failed to load chat due to decryption failure.\n");
                cfg->chat_path[0] = '\0';
                return;
            }

        }
        else
        {
            fprintf(stderr, "[ERROR] Failed to load chat.\n");
        }
    }
    else if (strncmp(input, "/export", 7) == 0)
    {
        int to_file = 0;
        if (strstr(input, "--to-file"))
        {
            to_file = 1;
        }
        if (export_chat(cfg, to_file))
        {
            printf("[OK] Chat exported %s\n", to_file ? "to file" : "to console");
        }
        else
        {
            fprintf(stderr, "[ERROR] Failed to export chat\n");
        }
        
    }
    else if (strncmp(input, "/help", 5) == 0)
    {
        show_help();
    }
    
    else
    {
        printf("[CLI] Unknown command: %s", input);
    }
}

