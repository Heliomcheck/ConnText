sudo gcc -I/usr/include/cjson -I/usr/include -Wall -Wextra -Wpedantic server.c connect.c erproc.c json.c sqlite.c encryption.c room.c -o server -lcjson -lsqlite3 -lssl -lcrypto
