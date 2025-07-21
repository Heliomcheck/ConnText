#ifndef ROOM_H
#define ROOM_H

#define MAX_ROOM 1000000

typedef struct {
    int room_id
    char name;
} room_t;

typedef struct {
    room_t rooms[MAX_ROOMS];
    int count;
} room_table_t;

room_table_t room_table = {.count = 0;};

int create_table_room(sqlite *db);

int create_room(int client_fd, sqlite *db, char room_name, bool open);



#endif
