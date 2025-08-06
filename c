sudo gcc -I/usr/include/cjson -Wall -Wextra -Wpedantic client.c connect.c erproc.c json.c sqlite.c encryption.c room.c -o client -lcjson -lsqlite3 -lssl -lcrypto
