#include <arpa/inet.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <cJSON.h>
#include "erproc.h"
#include "connect.h"
#include "json.h"
#include "sqlite.h"
#include "room.h"


int start_server(char *val, int backlog)
{
    char *endptr;
    long port = strtol(val, &endptr, 10);
    if (*endptr != '\0'|| port <= 0 || port > 65535){
        fprintf(stderr, "Invalid port number\n");
        exit(EXIT_FAILURE);
    }

    int fd = Socket(AF_INET, SOCK_STREAM, 0);
		
	int yes = 1;
	setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof yes);
		
	struct sockaddr_in adr = {0};
	adr.sin_family = AF_INET;
	adr.sin_port = htons(port);
	adr.sin_addr.s_addr = htonl(INADDR_ANY);

	Bind(fd, (struct sockaddr*)&adr, sizeof adr);
	Listen(fd, backlog);
    
    return fd;
}

user_table_t user_table = {.count = 0};


int find_user_by_nick(const char *nick)
{
    for (int i = 1; i <= user_table.count; i++)
    {
        if (strcmp(user_table.users[i].nickname, nick) == 0) {
            return i;
        }
    }
    return -1;
}

int find_user_by_socket(int sockfd) {
    for (int i = 0; i < user_table.count; i++) {
        if (sockfd == user_table.users[i].socket_fd) {
            return i;
        }
    }
    return -1;
}

int register_user(const char *nickname, const char *password, int socketfd, sqlite3 *db)
{
    int err = add_user_in_db_table(db, nickname, password);
    if (err != 0) {
        printf("Something wend wrong: user wasn't add to db");
        return -1;
    }

    if (user_table.count >= MAX_USERS)
        return -1;
    if (find_user_by_nick(nickname) != -1)
        return -2;
    
    user_t *usr = &user_table.users[user_table.count];
    usr->id = user_table.count + 1;
    strncpy(usr->nickname, nickname, MAX_LEN_USER);
    strncpy(usr->password, password, MAX_LEN_PASS);
    usr->socket_fd = socketfd;
    usr->online = true;
    
    user_table.count++;

    return 0;
}

int check_password(const char *nickname, const char *password, int socketfd)
{
    int index = find_user_by_nick(nickname);
    if (index == -1) { return -1; }

    user_t *u = &user_table.users[index];
    if (strcmp(u->password, password) == 0)
    {
        u->online = true;
        u->socket_fd = socketfd;
        return 0;
    }
    return -2;
}

int login_user(const char *nick, const char *password, int sockfd) {
    int idx = find_user_by_nick(nick);
    if (idx == -1) return -1;

    user_t *u = &user_table.users[idx];
    if (strcmp(u->password, password) != 0) return -2;

    u->socket_fd = sockfd;
    u->online = true;
    return 0;
}

int logout_user(int sockfd) {
    int idx = find_user_by_socket(sockfd);
    if (idx == -1) { return -1; }
    user_table.users[idx].online = false;
    int id = user_table.users[idx].id;
    printf("Client logged out: ID: %d, FD: %d\n", id, sockfd);
    return 0;
}

int exit_app(int sockfd) {
    logout_user(sockfd);
    close(sockfd);
    printf("Client exit\n");
    return 0;
}

void reset_fd(int client_fd) {
    int idx = find_user_by_socket(client_fd);
    if (idx == -1) { return; }
    user_table.users[idx].socket_fd = -1;
    user_table.users[idx].online = false;
}

int handle_client_json(int client_fd, const char *buf, sqlite3 *db) {
    cJSON *root = cJSON_Parse(buf);
    if (!root) {
        send_json(client_fd, "error", cJSON_CreateString("Invalid JSON"));
        printf("Error parsing json\n");
        return 0;
    }

    cJSON *type = cJSON_GetObjectItem(root, "type");
    cJSON *payload = cJSON_GetObjectItem(root, "payload");

    if (!cJSON_IsString(type) || !cJSON_IsObject(payload)) {
        send_json(client_fd, "error", cJSON_CreateString("Missing type or payload"));
        cJSON_Delete(root);
        return 0;
    }

    if (strcmp(type->valuestring, "reg") == 0) {
        const char *nick = cJSON_GetObjectItem(payload, "nick")->valuestring;
        const char *pass = cJSON_GetObjectItem(payload, "password")->valuestring;

        int res = register_user(nick, pass, client_fd, db);
        if (res == 0) {
            send_json(client_fd, "auth", cJSON_CreateString("Registration successful"));
        } else if (res == -2) {
            send_json(client_fd, "error", cJSON_CreateString("Nickname already exists"));
        } else {
            send_json(client_fd, "error", cJSON_CreateString("User limit reached"));
        }
    }

    else if (strcmp(type->valuestring, "login") == 0) {
        const char *nick = cJSON_GetObjectItem(payload, "nick")->valuestring;
        const char *pass = cJSON_GetObjectItem(payload, "password")->valuestring;

        int res = login_user(nick, pass, client_fd);
        if (res == 0) {
            send_json(client_fd, "auth", cJSON_CreateString("Login successful"));
        } else if (res == -1) {
            send_json(client_fd, "error", cJSON_CreateString("User not found"));
        } else {
            send_json(client_fd, "error", cJSON_CreateString("Incorrect password"));
        }
    }

    else if (strcmp(type->valuestring, "message") == 0) {
        const char *text = cJSON_GetObjectItem(payload, "text")->valuestring;

        char sender[32] = "Unknown";
        for (int i = 0; i < user_table.count; i++) {
            if (user_table.users[i].socket_fd == client_fd) {
                strcpy(sender, user_table.users[i].nickname);
                break;
            }
        }

        if (strcmp(sender, "Unknown") == 0) {
            send_json(client_fd, "error", cJSON_CreateString("You must be loginded"));
            return 0;
        }

        if (strcmp(sender, "Unknown") != 0) {
            message_to_room(client_fd, db, buf);
        }

        for (int i = 0; i < user_table.count; i++) {
            if (user_table.users[i].online && user_table.users[i].socket_fd != client_fd) {
                cJSON *msg = cJSON_CreateObject();
                cJSON_AddStringToObject(msg, "from", sender);
                cJSON_AddStringToObject(msg, "text", text);
                send_json(user_table.users[i].socket_fd, "chat", msg);
                cJSON_Delete(msg);
            }
        }
    }

    else if (strcmp(type->valuestring, "logout") == 0) {
        int ans = logout_user(client_fd);
        if (ans == -1) {
            send_json(client_fd, "error", cJSON_CreateString("Something went wrong"));
        } else {
            return 1;
        }
    }

    else if (strcmp(type->valuestring, "exit") == 0) {
        exit_app(client_fd);
        return 1;
    }

    else if (strcmp(type->valuestring, "status") == 0) {
        char *nickname = cJSON_GetObjectItem(payload, "nickname")->valuestring;
        if (strcmp(nickname, "self") == 0) {
            int idx = find_user_by_socket(client_fd);
            cJSON *status = cJSON_CreateObject();
            cJSON_AddNumberToObject(status, "id", user_table.users[idx].id);
            cJSON_AddStringToObject(status, "nickname", user_table.users[idx].nickname);
            cJSON_AddBoolToObject(status, "online", user_table.users[idx].online);
            send_json(client_fd, "self status", status);
            return 0;
        }

        int idx = find_user_by_nick(nickname);
        if (idx == -1) {
            send_json(client_fd, "error", cJSON_CreateString("Nickname is incorrect"));
            return 0;
        } else {
            cJSON *status = cJSON_CreateObject();
            cJSON_AddStringToObject(status, "nickname", user_table.users[idx].nickname);
            cJSON_AddBoolToObject(status, "online", user_table.users[idx].online);
            send_json(client_fd, "status", status);
            return 0;
        } // добавить проверку, если клиент пишет свой ник
        
    }

    else {
        send_json(client_fd, "error", cJSON_CreateString("Unknown command"));
    }

    cJSON_Delete(root);
    return 0;
}

