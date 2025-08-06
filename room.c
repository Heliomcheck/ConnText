#include "room.h"
#include <stdio.h>
#include <sqlite3.h>
#include <stdbool.h>
#include <cJSON.h>
#include <string.h>
#include "connect.h"
#include "json.h"

room_table_t room_table = {.count = 0};

int create_table_room(sqlite3 *db) {
    char *err_msg;
    const char *sql = "CREATE TABLE IF NOT EXISTS room("
                        "room_id INTEGER PRIMARY KEY AUTOINCREMENT, "
                        "room_name TEXT NOT NULL UNIQUE, "
                        "owner_id INTEGER, "
                        "created_at DATETIME DEFAULT CURRENT_TIMESTAMP, "
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

int create_message_table(sqlite3 *db) {
    char *err_msg;

    const char *sql = "CREATE TABLE IF NOT EXISTS message("
                        "room_id INTEGER NOT NULL, "
                        "user_id TEXT NOT NULL, "
                        "text TEXT NOT NULL, "
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

int create_room(int client_fd, sqlite3 *db, char *room_name, bool open) {
    (void)open;
    int idx = find_user_by_socket(client_fd);
    int id = user_table.users[idx].id;
    printf("user id: %d\n, room_name: %s\n", id, room_name);
    
    const char *sql = "INSERT INTO room(room_name, owner_id) VALUES (?, ?);";
    sqlite3_stmt *stmt; 
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        fprintf(stderr, "sql error: %s\n", sqlite3_errmsg(db));
        return 1;
    }

    sqlite3_bind_text(stmt, 1, room_name, -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 2, id);


    if (sqlite3_step(stmt) != SQLITE_DONE) {
        fprintf(stderr, "sql error: %s\n", sqlite3_errmsg(db));
        sqlite3_finalize(stmt);
        return 2;
    }
    sqlite3_finalize(stmt);
    stmt = NULL;

    
    const char *nickname = user_table.users[idx].nickname;
    printf("nickname: %s\n", nickname);
    const char *sql1 = "SELECT last_insert_rowid();";
    sqlite3_stmt *stmt2;

    if (sqlite3_prepare_v2(db, sql1, -1, &stmt2, NULL) != SQLITE_OK) {
        fprintf(stderr, "sql error: %s\n", sqlite3_errmsg(db));
        return 3;
    }

    int rc = sqlite3_step(stmt2);

    printf("rc sq = %d\n", rc);

    if (rc == SQLITE_ROW) {
        int room_id = sqlite3_column_int(stmt2, 0);
        printf("room_id: %d\n", room_id);
        int rcc = add_user_into_room(client_fd, db, nickname, room_id, room_name);
        if (rcc != 0) {
            printf("error with adding user in chat\n");
            return 4;
        }
    }

    /*if (rc != SQLITE_DONE) {
        fprintf(stderr, "sql error: %s\n", sqlite3_errmsg(db));
        return 5;
    }*/

    sqlite3_finalize(stmt2);

    return 0;
}

int create_table_room_user(sqlite3 *db) {
    char *err_msg;

    const char *sql = "CREATE TABLE IF NOT EXISTS room_user("
                        "room_id INTEGER NOT NULL, "
                        "room_name TEXT NOT NULL, "
                        "user_id INTEGER NOT NULL, "
                        "time DATETIME DEFAULT CURRENT_TIMESTAP, "
                        "FOREIGN KEY (user_id) REFERENCES users(id),"
                        "FOREIGN KEY (room_id) REFERENCES room(room_id), "
                        "FOREIGN KEY (room_name) REFERENCES room(room_name));";
                
    int rc = sqlite3_exec(db, sql, 0, 0, &err_msg);
    if (rc != SQLITE_OK){
        fprintf(stderr, "SQLITE error: %s\n", err_msg);
        sqlite3_close(db);
        sqlite3_free(err_msg);
        return 1;
    }
    return 0;
}

int message_to_room(int client_fd, sqlite3 *db, const char *text) {
    int idx = find_user_by_socket(client_fd);
    int id = user_table.users[idx].id;
    int room_id = user_table.users[idx].room_id;

    const char *sql = "INSERT INTO message(room_id, user_id, text) VALUES (?, ?, ?);";
    sqlite3_stmt *stmt;

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        fprintf(stderr, "sqlite error: %s\n", sqlite3_errmsg(db));
        return 1;
    }

    sqlite3_bind_int(stmt, 1, room_id);
    sqlite3_bind_int(stmt, 2, id);
    sqlite3_bind_text(stmt, 3, text, -1, SQLITE_STATIC);

    if (sqlite3_step(stmt) != SQLITE_DONE) {
        fprintf(stderr, "sqlite error: %s\n", sqlite3_errmsg(db));
        sqlite3_finalize(stmt);
        return 2;
    }
    sqlite3_finalize(stmt);
    return 0;
}

/*sqlite3 *load_user_messages(void *NotUsed, int argc, char **argv, char **azColName) {
    (void)NotUsed;
    (void)argc;
    (void)azColName;

    return argv[0];
}*/

void show_history(int client_fd, sqlite3 *db) {
    int idx = find_user_by_socket(client_fd);
    sqlite3_stmt *stmt;

    const char *sql = "SELECT text, user_id FROM message WHERE room_id = ?;";

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK){
        fprintf(stderr, "sqlite error: %s\n", sqlite3_errmsg(db));
        return;
    }

    sqlite3_bind_int(stmt, 1, user_table.users[idx].room_id);

    int rc = sqlite3_step(stmt);

    while (rc == SQLITE_ROW) {
        const char *text = (const char *)sqlite3_column_text(stmt, 0);
        int id = sqlite3_column_int(stmt, 1);

        int iddx = find_user_by_id(id);
        //printf("[%s]: %s\n", user_table.users[iddx].nickname, text);
        cJSON *payload = cJSON_CreateObject();
        
        cJSON_AddStringToObject(payload, "from", user_table.users[iddx].nickname);
        cJSON_AddStringToObject(payload, "text", text);
        send_json(client_fd, "chat", payload);
        cJSON_Delete(payload);

        rc = sqlite3_step(stmt);
    }

    if (rc != SQLITE_DONE) {
        fprintf(stderr, "sqlite error: %s\n", sqlite3_errmsg(db));
    }
    
    sqlite3_finalize(stmt);
}

int list_user_rooms(int client_fd, sqlite3 *db) {
    int idx = find_user_by_socket(client_fd);
    int id = user_table.users[idx].id;
    sqlite3_stmt *stmt;

    const char *sql = "SELECT room_id, room_name FROM room_user WHERE user_id = ?;";

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        fprintf(stderr, "sqlite error: %s\n", sqlite3_errmsg(db));
        return 1;
    }

    sqlite3_bind_int(stmt, 1, id);

    int rc = sqlite3_step(stmt);
    if (rc == SQLITE_ROW) {
        while (rc == SQLITE_ROW) {
            int room_id = sqlite3_column_int(stmt, 0);
            const char *room_name = (const char *)sqlite3_column_text(stmt, 1);

            cJSON *payload = cJSON_CreateObject();

            cJSON_AddNumberToObject(payload, "room_id", room_id);
            cJSON_AddStringToObject(payload, "room_name", room_name);
            send_json(client_fd, "rooms", payload);
            cJSON_Delete(payload);

            rc = sqlite3_step(stmt);
        }
    }

    else {
        fprintf(stderr, "sqlite error: %s\n", sqlite3_errmsg(db));
        send_json(client_fd, "info", cJSON_CreateString("U don't have rooms\n"));
    }

    sqlite3_finalize(stmt);

    return 0;
}

int enter_into_room(int client_fd, sqlite3 *db, int room_id) {
    int idx = find_user_by_socket(client_fd);
    int id = user_table.users[idx].id;
    sqlite3_stmt *stmt;

    const char *sql = "SELECT room_id, user_id FROM room_user WHERE room_id = ? AND user_id = ?;";
    

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        fprintf(stderr, "sqlite error: %s\n", sqlite3_errmsg(db));
        return 1;
    }

    sqlite3_bind_int(stmt, 1, room_id);
    sqlite3_bind_int(stmt, 2, id);

    int rc = sqlite3_step(stmt);

    if (rc == SQLITE_ROW) {
        sqlite3_finalize(stmt);
        return 0;
    }

    if (rc != SQLITE_DONE) {
        fprintf(stderr, "sqlite error: %s\n", sqlite3_errmsg(db));
    }

    sqlite3_finalize(stmt);

    return 2;

}

int add_user_into_room(int client_fd,sqlite3 *db, const char *nickname, int room_id, char *room_name) {
    (void)client_fd;
    //int idn = user_table.users[find_user_by_socket(client_fd)].id;
    int idf = user_table.users[find_user_by_nick(nickname)].id;

    sqlite3_stmt *stmt;

    printf("room_name in add_user: %s\n", room_name);
    
    char *sql = "INSERT INTO room_user (room_id, room_name, user_id) VALUES (?, ?, ?);";

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        fprintf(stderr, "1 sqlite error: %s\n", sqlite3_errmsg(db));
        return 1;
    }

    sqlite3_bind_int(stmt, 1, room_id);
    sqlite3_bind_text(stmt, 2, room_name, -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 3, idf);
    
    //free(room_name);
    
    int rc = sqlite3_step(stmt);

    if (rc != SQLITE_DONE) {
        fprintf(stderr, "2 sqlite error: %s\n", sqlite3_errmsg(db));
        return 1;
    }
    sqlite3_finalize(stmt);
    free(room_name);
    return 0;
}

char *find_room_name_by_id(sqlite3 *db, int id) {
    const char *sql = "SELECT room_name FROM room WHERE room_id = ? LIMIT 1;";
    char *room_name_dup;
    sqlite3_stmt *stmt;

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        fprintf(stderr, "sqlite error: %s\n", sqlite3_errmsg(db));
        return NULL;
    }

    sqlite3_bind_int(stmt, 1, id);
    int rc = sqlite3_step(stmt);

    if (rc == SQLITE_ROW) {
        const char *room_name = (const char *)sqlite3_column_text(stmt, 0);
        if (room_name) {
            room_name_dup = strdup(room_name);
                printf("Memory allocated failed\n");
        }
    }
    
    
    else {
        fprintf(stderr, "sqlite error: %s\n", sqlite3_errmsg(db));
        sqlite3_finalize(stmt);
        return NULL;
    }

    sqlite3_finalize(stmt);

    return room_name_dup;
}
