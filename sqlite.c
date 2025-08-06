#include <stdio.h>
#include <stdlib.h>
#include <sqlite3.h>
#include "connect.h"
#include <string.h>
#include "sqlite.h"
#include "room.h"

#define MAX_USERS 1024

sqlite3 *start_db() {
    sqlite3 *db;
    char *err_msg;
    int rc = sqlite3_open("chat.db", &db);
    if (rc != SQLITE_OK) {
        printf("error opening db\n");
        sqlite3_close(db);
        return NULL;
    }
    const char *sql = "CREATE TABLE IF NOT EXISTS users("
                    "id INTEGER PRIMARY KEY AUTOINCREMENT, "
                    "nickname TEXT NOT NULL UNIQUE, "
                    "password INTEGER NOT NULL);";

    rc = sqlite3_exec(db, sql, 0, 0, &err_msg);
    if (rc != SQLITE_OK) {
        printf("sql error: %s\n", err_msg);
        sqlite3_close(db);
        return NULL;
    }
    sqlite3_free(err_msg);
    create_table_room(db);
    create_message_table(db);
    create_table_room_user(db);
    return db;
}

int add_user_in_db_table(sqlite3 *db, const char *nickname, const char *password) {
    const char *sql = "INSERT INTO users(nickname, password) VALUES (?, ?);";
    sqlite3_stmt *stmt;
    
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        fprintf(stderr, "Prepare failed: %s\n", sqlite3_errmsg(db));
        return 1;
    }

    sqlite3_bind_text(stmt, 1, nickname, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, password, -1, SQLITE_STATIC);

    if (sqlite3_step(stmt) != SQLITE_DONE) {
        fprintf(stderr, "Execution failed: %s\n", sqlite3_errmsg(db));
        sqlite3_finalize(stmt);
        return 2;
    }
    sqlite3_finalize(stmt);
    return 0;
}

int delete_user_from_db_table(sqlite3 *db, const char* nickname) {
    const char *sql = "DELETE FROM users WHERE username = ?;";
    sqlite3_stmt *stmt;

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) == SQLITE_OK) {
        sqlite3_bind_text(stmt, 1, nickname, -1, SQLITE_STATIC);
        if (sqlite3_step(stmt) == SQLITE_DONE) {
            if (sqlite3_changes(db) > 0) { 
                printf("user %s was deleted\n", nickname); 
            } else { 
                printf("user %s not found\n", nickname); 
            } 
        } else {
            printf("error of deleting: %s\n", sqlite3_errmsg(db));
        }
    }
    return 0;
}

int load_user_callback(void *NotUsed, int argc, char **argv, char **azColName) {
    (void)NotUsed;
    (void)argc;
    (void)azColName;

    if (user_table.count >= MAX_USERS) {
        printf("[error]: limit of max users\n");
        return 1;
    }

    user_table.users[user_table.count].id = atoi(argv[0]);
    strncpy(user_table.users[user_table.count].nickname, argv[1],
            sizeof(user_table.users[user_table.count].nickname) - 1);
    strncpy(user_table.users[user_table.count].password, argv[2],
            sizeof(user_table.users[user_table.count].password));
    user_table.count++;

    return 0;
}

void load_users_from_db(sqlite3 *db) {
    char *err_msg = 0;
    const char *sql = "SELECT id, nickname, password FROM users;";

    user_table.count = 0;

    if (sqlite3_exec(db, sql, load_user_callback, 0, &err_msg) != SQLITE_OK) {
        fprintf(stderr, "error of SQL: %s\n", err_msg);
        sqlite3_free(err_msg);
        return;
    }
    printf("uploaded %d userd from db\n", user_table.count);
}
