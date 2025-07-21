#include <stdio.h>
#include <sqlite3.h>

int main() {
    sqlite3 *db;
    char *err_msg = 0;
    int rc = sqlite3_open("test.db", &db);
    if (rc != SQLITE_OK) {
        printf("Db not created");
        sqlite3_close(db);
        return 1;
    }
    char *sql = "DROP TABLE IF EXISTS people; "
                "CREATE TABLE people(id INTEGER PRIMARY KEY AUTOINCREMENT, name TEXT, age INTEGER); ";
    rc = sqlite3_exec(db, sql, 0, 0, &err_msg);
    if (rc != SQLITE_OK) {
        printf("sqlite error: ", rc);
        sqlite3_free(err_msg);
        sqlite3_close(db);
        return 1;
    }
    sqlite3_close(db);
    printf("db was normal created");
    return 0;
}
