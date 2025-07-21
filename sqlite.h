#ifndef SQLITE_H
#define SQLITE_H

#include <sqlite3.h>

sqlite3 *start_db();

int add_user_in_db_table(sqlite3 *db, const char *nickname, const char *password);

int delete_user_from_db_table(sqlite3 *db, const char* nickname);

int load_user_callback(void *NotUsed, int argc, char **argv, char **azColName);

void load_users_from_db(sqlite3 *db);

#endif
