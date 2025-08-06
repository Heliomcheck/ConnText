#ifndef ROOM_H
#define ROOM_H

#include "json.h"
#include <stdbool.h>
#include <stdio.h>
#include <sqlite3.h>
#include <stdlib.h>
#include "sqlite.h"

#define MAX_ROOMS 1000000

typedef struct {
    int room_id;
    char name;
} room_t;

typedef struct {
    room_t rooms[MAX_ROOMS];
    int count;
} room_table_t;

extern room_table_t room_table;

int create_table_room(sqlite3 *db);

int create_message_table(sqlite3 *db);

int create_room(int client_fd, sqlite3 *db, const char *room_name, bool open);

int create_table_room_user(sqlite3 *db);

int message_to_room(int client_fd, sqlite3 *db, const char *text);

//void load_user_message(void *NotUsed, int argc, char **argv, char **azColName);

void show_history(int client_fd, sqlite3 *db);

int list_user_rooms(int client_fd, sqlite3 *db);

int enter_into_room(int client_fd, sqlite3 *db, int room_id);

int add_user_into_room(int client_fd, sqlite3 *db, const char *nickname, int room_id, const char *room_name);

const char *find_room_name_by_id(sqlite3 *db, int id);

#endif
