#include "api.h"
#include "cJSON.h"
#include <string.h>
#include <time.h>
#include <stdio.h>
#include <curl/curl.h>
#include <stdlib.h>

struct response_data{
    char *memory;
    size_t size;
};

static size_t write_callback(void *contents, size_t size, size_t nmemb, void *userp)
{
    size_t realsize = size * nmemb;
    struct response_data *mem = (struct response_data *)userp;

    char *ptr = realloc(mem->memory, mem->size + realsize + 1);
    if (!ptr) return 0;

    mem->memory = ptr;
    memcpy(&(mem->memory[mem->size]), contents, realsize);
    mem->size += realsize;
    mem->memory[mem->size] = 0;

    return realsize;
}


int send_prompt(config_t *cfg, const char *input)
{

    message_t user_msg = {0};
    strncpy(user_msg.role, "user", sizeof(user_msg.role) - 1);
    strncpy(user_msg.content, input, sizeof(user_msg.content) - 1);
    user_msg.timestamp = time(NULL);
    user_msg.tokens = strlen(user_msg.content) / 4;

    if (!write_message(cfg, &user_msg))
    {
        fprintf(stderr, "[ERROR] Failed to write user message\n");
        return 0;
    }


    cJSON *root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "model", cfg->model);
    cJSON *messages = cJSON_CreateArray();

    cJSON *msg = cJSON_CreateObject();
    cJSON_AddStringToObject(msg, "role", "user");
    cJSON_AddStringToObject(msg, "content", input);

    cJSON_AddItemToArray(messages, msg);

    cJSON_AddItemToObject(root, "messages", messages);

    char *json_data = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);

    if (!json_data)
    {
        fprintf(stderr, "Failed tp generate JSON\n");
        return 0;
    }

    //<- stop here now

    //printf("[DEBUG] Sending request JSON:\n%s\n", json_data);

    CURL *curl = curl_easy_init();
    if (!curl)
    {
        fprintf(stderr, "curl init failed\n");
        free(json_data);
        return 0;
    }
    
    struct response_data chunk = {0};
    chunk.memory = malloc(1);
    chunk.size = 0;

    struct curl_slist *headers = NULL;
    char auth_header[600];
    snprintf(auth_header, sizeof(auth_header), "Authorization: Bearer %s", cfg->api_key);

    headers = curl_slist_append(headers, "Content-Type: application/json");
    headers = curl_slist_append(headers, auth_header);

    
    curl_easy_setopt(curl, CURLOPT_URL, "https://api.openai.com/v1/chat/completions");
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, json_data);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&chunk);

    CURLcode res = curl_easy_perform(curl);
    if (res != CURLE_OK)
    {
        fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
        curl_easy_cleanup(curl);
        curl_slist_free_all(headers);
        free(json_data);
        return 0;
    }

    //printf("[DEBUG] API response:\n%s\n", chunk.memory);

    cJSON *resp = cJSON_Parse(chunk.memory);
    if (!resp)
    {
        fprintf(stderr, "[ERROR] Failed to parse API response\n");
        free(chunk.memory);
        return 0;
    }
    cJSON *choices = cJSON_GetObjectItem(resp, "choices");
    if (!choices || !cJSON_IsArray(choices) || cJSON_GetArraySize(choices) == 0)
    {
        fprintf(stderr, "[ERROR] No choices in response\n");
        cJSON_Delete(resp);
        free(chunk.memory);
        return 0;
    }

    cJSON *first = cJSON_GetArrayItem(choices, 0);
    cJSON *message = cJSON_GetObjectItem(first, "message");
    cJSON *content = cJSON_GetObjectItem(message, "content");

    if (!content || !cJSON_IsString(content))
    {
        fprintf(stderr, "[ERROR] No assistant content in response\n");
        cJSON_Delete(resp);
        free(chunk.memory);
        return 0;
    }

    printf("\n========= ASSISTANT =========\n");
    printf("%s\n", content->valuestring);
    printf("=============================\n\n");

    message_t reply = {0};
    
    strncpy(reply.role, "assistant", sizeof(reply.role) - 1);
    strncpy(reply.content, content->valuestring, sizeof(reply.content) - 1);
    reply.timestamp = time(NULL);
    reply.tokens = strlen(reply.content) / 4;
    

    if (!write_message(cfg, &reply))
    {
        fprintf(stderr, "[ERROR] Failed to write assistant reply\n");
        cJSON_Delete(resp);
        curl_easy_cleanup(curl);
        curl_slist_free_all(headers);
        free(json_data);
        free(chunk.memory);
        return 0;
    }


    cJSON_Delete(resp);
    curl_easy_cleanup(curl);
    curl_slist_free_all(headers);
    free(json_data);
    free(chunk.memory);
    return 1;
}