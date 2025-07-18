sudo gcc -I/usr/include/cjson -I/usr/include -Wall -Wextra -Wpedantic server.c connect.c erproc.c json.c sqlite.c -o server -lcjson -lsqlite3

