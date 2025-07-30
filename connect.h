#ifndef CONNECT_H
#define CONNECT_H

#include <sqlite3.h>

#define MAX_LEN_USER 32 
#define MAX_LEN_PASS 65
#define MAX_USERS 1024

typedef struct 
{
    int id;
    char nickname[MAX_LEN_USER];
    char password[MAX_LEN_PASS];
    int socket_fd;
    int room_id;
    bool online;
} user_t;

typedef struct
{
    user_t users[MAX_USERS];
    int count;
} user_table_t;

extern user_table_t user_table;

int start_server(char *port, int backlog);

int find_user_by_nick(const char *nick);

int find_user_by_socket(int sockfd);

int register_user(const char *nickname, const char *password, int socketfd, sqlite3 *db);

int check_password(const char *nickname, const char *password, int socketfd);

int login_user(const char *nick, const char *password, int sockfd);

int logout_user(int sockfd);

int exit_app(int sockfd);

void reset_fd(int client_fd);

int handle_client_json(int client_fd, const char *buf, sqlite3 *db);

#endif 
