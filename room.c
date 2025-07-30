#include "room.h"
#include <stdio.h>
#include <sqlite3.h>
#include <stdbool.h>
#include <cJSON.h>

int create_table_room(sqlite *db) {
    char *err_msg;
    const char *sql = "CREATE TABLE IF NOT EXISTS room("
                        "room_id INTEGER PRIMARY KEY AUTOINCREMENT, "
                        "name TEXT NOT NULL UNIQUE, "
                        "owner_id INTEGER, "
                        "created_at DATATIME DEFAULT CURRENT_TIMESTAMP, "
                        "FOREIGN KEY (owner_id) REFERENCES users(id)"
                        ");" ;
    int rc = sqlite3_exec(db, sql, 0, 0, &err_msg);
    if (rc != SQLITE_OK) {
        printf("sql error: %s\n", err_msg);
        sqlite3_close(db);
        sqlite3_free(err_msg);
        return 1;
    }
    return 0;
}

int create_message_table(sqlite *db, int idx) {
    char *err_msg;

    const char *sql = "CREATE TABLE IF NOT EXISTS messages("
                        "room_id INTEGER NOT NULL, "
                        "user_id TEXT NOT NULL, "
                        "FOREIGN KEY (user_id) REFERENCES users(id), "
                        "FOREIGN KEY (room_id) REFERENCES room(room_id));";
    int rc = sqlite3_exec(db, sql, 0, 0, &err_msg);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "SQLITE error: %s\n", err_msg);
        sqlite3_close(db);
        sqlite3_free(err_msg);
        return 1;
    }

    return 0;
}

int create_room(int client_fd, sqlite *db, const char *room_name, bool open) {
    int idx = find_user_by_socket(client_fd);
    
    const char *sql = "INSERT INTO room(name, owner_id) VALUES (?, ?);";
    sqlite3_stmt *stmt; 
    if (sqlite3_prepare_v2(db, sql, -1, &smtm, NULL) != SQLITE_OK) {
        fprintf(stderr, "sql error: %s\n", sqlite3_errmsg(db));
        return 1;
    }

    sqlite3_bind_text(stmt, 1, room_name, -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 2, idx);


    if (sqlite3_step(stmt) != SQLITE_DONE) {
        fprintf(stderr, "sql error: %s\n", sqlite3_errmsg(db));
        sqlite3_finalize(stmt);
        return 1;
    }
    sqlite3_finalize(db);
    return 0;
}

int add_user_into_room(int client_fd, sqlite *db, char room_name) {
    int idx = find_user_by_socket(client_fd);
    char *err_msg;

    const char *sql = "CREATE TABLE IF NOT EXISTS room_user("
                        "room_id INTEGER NOT NULL, "
                        "user_id INTEGER NOT NULL, "
                        "text TEXT NOT NULL, "
                        "time DATATIME DEFAULT CURRENT_TIMESTAP, "
                        "FOREIGN KEY (user_id) REFERENCES users(id));";
    int rc = sqlite3_exec(db, sql, 0, 0, &err_msg);
    if (rc != SQLITE_OK){
        fprintf(stderr, "SQLITE error: %s\n", err_msg);
        sqlite3_close(db);
        sqlite3_free(err_msg);
        return 1;
    }
    return 0;
}

int message_to_room(int client_fd, sqlite *db, const char *text) {
    int idx = find_user_by_socket(client_fd);
    int id = user_table.users[idx].id;
    int room_id = user_table.users[idx].room_id;
    char *err_msg;

    const char *sql = "INSERT INTO message(room_id, user_id, text) VALUES (?, ?, ?);";
    sqlite3_stmt *stmt;

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        fprintp(stderr, "sqlite error: %s\n", sqlite_errmsg(db));
        return 1;
    }

    sqlite3_bind_int(stmt, 1, room_id);
    sqlite3_bind_int(stmt, 2, id);

    if (sqlite3_step(stmt) != SQLITE_DONE) {
        fprintf(stderr, "sqlite error: %s\n", sqlite3_errmsg(db));
        sqlite_finalize(db);
        return 1;
    }
    sqlite_finalize(db);
    return 0;
}
