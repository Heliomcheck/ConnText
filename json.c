#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <stddef.h>
#include "encryption.h"
#include "json.h"

void send_json(int sockfd, const char *type, cJSON *payload)
{
    cJSON *root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "type", type);
    cJSON_AddItemToObject(root, "payload", payload);

    char *out = cJSON_PrintUnformatted(root);

    size_t json_len = strlen(out);
    char *json_with_newline = malloc(json_len + 2);

    memcpy(json_with_newline, out, json_len);
    json_with_newline[json_len] = '\n';
    json_with_newline[json_len + 1] = '\0';

    send(sockfd, json_with_newline, json_len + 1, 0);
    
    printf("json text %s\n", out);

    free(out);
    free(json_with_newline);
}

void handle_server_response(const char *buf)
{
    cJSON *root = cJSON_Parse(buf);
    if (!root){
        printf("Unknown format JSON from server\n");
        return;
    }
    
    cJSON *type = cJSON_GetObjectItem(root, "type");
    if (!cJSON_IsString(type)) {
        printf("Unknown format of type\n");
        return;
    }
    
    cJSON *payload = cJSON_GetObjectItem(root, "payload");
    /*if (!cJSON_IsObject(payload)) {
        printf("Unknown format of payload\n");
        return;
    }*/

    if (strcmp(type->valuestring, "chat") == 0) {
        const char *from = cJSON_GetObjectItem(payload, "from")->valuestring;
        const char *text = cJSON_GetObjectItem(payload, "text")->valuestring;
        
        printf("[%s]: %s\n", from, text);
    }

    else if (strcmp(type->valuestring, "auth") == 0) {
        cJSON *auth_text = cJSON_GetObjectItem(root, "payload");
        printf("[auth]: %s\n", auth_text->valuestring);
    }

    else if (strcmp(type->valuestring, "error") == 0) {
        cJSON *type_error = cJSON_GetObjectItem(root, "payload");
        printf("[error]: %s\n", type_error->valuestring);
    }

    else if (strcmp(type->valuestring, "status") == 0) {
        const char *nickname = cJSON_GetObjectItem(payload, "nickname")->valuestring;
         bool online = cJSON_IsTrue(cJSON_GetObjectItem(payload, "online"));

        printf("[status]: Nickname: %s, Online: %s\n", nickname, online ? "Yes" : "No");
    }

    else if (strcmp(type->valuestring, "self status") == 0) {
        int id = cJSON_GetObjectItem(payload, "id")->valueint;
        const char *nickname = cJSON_GetObjectItem(payload, "nickname")->valuestring;
        bool online = cJSON_IsTrue(cJSON_GetObjectItem(payload, "online"));

        printf("[status]: ID: %d, Nickname: %s, Online: %s\n", id, nickname, online ? "No": "Yes");
    }

    /*cJSON *root = cJSON_Parse(buf);
    if (!root) {
        printf("[SERVER RAW]: %s\n", buf);
        return;
    }
    cJSON *type = cJSON_GetObjectItem(root, "type");
    if (!cJSON_IsString(type)) {
        printf("[SERVER]: Неизвестный формат\n");
        cJSON_Delete(root);
        return;
    }

    if (strcmp(type->valuestring, "chat") != 0) {
        printf("non chat\n");
    }

    if (strcmp(type->valuestring, "error") == 0) {
        const cJSON *eerror = cJSON_GetObjectItem(root, "payload");
        printf("[%s]: %s\n", type->valuestring, eerror->valuestring);
    } 

    if (strcmp(type->valuestring, "status") == 0) {
        const cJSON *status = cJSON_GetObjectItem(root, "payload");
        printf("[%s]: %s\n", type->valuestring, status->valuestring);
    }
 
    if (strcmp(type->valuestring, "chat") == 0) {
        const cJSON *from = cJSON_GetObjectItem(root, "from");
        const cJSON *text = cJSON_GetObjectItem(root, "text");
        if (cJSON_IsString(from) && cJSON_IsString(text)) {
            printf("[%s]: %s\n", from->valuestring, text->valuestring);
        }
    } else if (strcmp(type->valuestring, "status") == 0 || strcmp(type->valuestring, "error") == 0) {
        const cJSON *msg = cJSON_GetObjectItem(root, "message");
        if (cJSON_IsString(msg)) {
            printf("[%s]: %s\n", type->valuestring, msg->valuestring);
        }
    } else {
        printf("[SERVER]: Неизвестный тип '%s'\n", type->valuestring);
    }

    cJSON_Delete(root);*/
}

void print_menu() {
    printf("/help - all commands\n");
    printf("/reg <nick> <pass> - register new account\n");
    printf("/login <nick> <pass> - login to account\n");
    printf("/msg <text> - send message to all users\n");
    printf("/logout - exit from chat\n");
    printf("/exit - close application and logout\n");
    printf("/status <nick> - print status of user,\n"
            "if you does't print nick, this command\n"
            "print YOUR status\n");
}

void parse_user_input(const char *input, int sockfd) {
    char *command = strtok((char *)input, " ");
    if (command == NULL) return;

    cJSON *root = cJSON_CreateObject();
    cJSON *payload = cJSON_CreateObject();

    if (strcmp(command, "/help") == 0) {
        print_menu();
    }

    else if (strcmp(command, "/reg") == 0) {
        char *nick = strtok(NULL, " ");
        char *pass = strtok(NULL, " ");
        if (!nick || !pass) {
            printf("Usage: /reg <nickname> <password>\n");
            cJSON_Delete(root);
            cJSON_Delete(payload);
            return;
        }
        
        cJSON_AddStringToObject(payload, "nick", nick);
        
        char password_h[65];
        hash_pass(pass, password_h);
        printf("Hashed password: %s\n", password_h);
        cJSON_AddStringToObject(payload, "password", password_h);

        send_json(sockfd, "reg", payload);
    }

    else if (strcmp(command, "/login") == 0) {
        char *nick = strtok(NULL, " ");
        char *pass = strtok(NULL, " ");
        if (!nick || !pass) {
            printf("Usage: /login <nickname> <password>\n");
            cJSON_Delete(root);
            cJSON_Delete(payload);
            return;
        }
        
        cJSON_AddStringToObject(payload, "nick", nick);
        
        char password_h[65];
        hash_pass(pass, password_h);
        printf("Hashed password: %s\n", password_h);
        cJSON_AddStringToObject(payload, "password", password_h);
        
        send_json(sockfd, "login", payload);
    }

    else if (strcmp(command, "/msg") == 0) {
        char *text = strtok(NULL, "");
        if (!text) {
            printf("Usage: /msg <message text>\n");
            cJSON_Delete(root);
            cJSON_Delete(payload);
            return;
        }
        
        cJSON_AddStringToObject(payload, "text", text);
        send_json(sockfd, "message", payload);
    }

    else if (strcmp(command, "/logout") == 0) {
        cJSON_AddStringToObject(payload, "logout", "1");
        send_json(sockfd, "logout", payload);
    }

    else if (strcmp(command, "/exit") == 0) {
        cJSON_AddStringToObject(payload, "exit", "1");
        send_json(sockfd, "exit", payload);
    }

    else if (strcmp(command, "/status") == 0) {
        char *nick = strtok(NULL, "");
        if (nick) {
            cJSON_AddStringToObject(payload, "nickname", nick);
            send_json(sockfd,"status", payload);
        } else {
            cJSON_AddStringToObject(payload, "nickname", "self");
            send_json(sockfd, "status", payload);
        }
        
    }

    else {
        printf("Unknown command. Available commands:\n");
        print_menu();
        cJSON_Delete(root);
        cJSON_Delete(payload);
        return;
    }
}
