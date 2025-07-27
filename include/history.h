#ifndef HISTORY_H
#define HISTORY_H

#include "config.h"
#include <time.h>

#define MAX_CHATS 256

typedef struct 
{
    char role[32];
    char content[4096];
    long timestamp;
    int tokens;
}message_t;

typedef struct
{
    char file[128];
    char title[128];
    time_t created;
} chat_meta_t;

extern chat_meta_t chat_index[MAX_CHATS];
extern int chat_index_count;

int create_new_chat(config_t *cfg);
int write_message(config_t *cfg, const message_t *msg);
int load_index();
int save_index();
void list_chats(config_t *cfg);
int rename_chat(int index, const char *new_title);
int load_chat(config_t *cfg, int index);
int validate_chat_file(config_t *cfg);
int export_chat(config_t *cfg, int to_file);
int delete_chat(int index);

#endif