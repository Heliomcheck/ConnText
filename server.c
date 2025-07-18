#include <arpa/inet.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <cJSON.h>
#include <sqlite3.h>
#include "erproc.h"
#include "connect.h"
#include "json.h"
#include "sqlite.h"

#define MAX_CLIENTS 1024
#define BACKLOG 10
#define BUF_SIZE 1024
#define MAXCL FD_SETSIZE

//static int add_client(int fd, int *arr);
static void del_client(int idx, int *arr);

int main(int argc, char *argv[])
{
	if (argc != 2)
	{
    	fprintf(stderr, "Usage: %s <Port>", argv[0]);
		return 1;
	}

    sqlite3 *db = start_db();
    load_users_from_db(db);

    int fd = start_server((char*)argv[1], BACKLOG);

	int clients[MAXCL];
	for (int i = 0; i < MAXCL; i++) clients[i] = -1;

	fd_set allset, rset;
	FD_ZERO(&allset);
	FD_SET(fd, &allset);
	int maxfd = fd;
	int maxi = -1;

	printf("Server listening on port %s\n", (char*)argv[1]);

	char buf[BUF_SIZE];
	for (;;) {
        rset = allset;
        select(maxfd + 1, &rset, NULL, NULL, NULL);

        if (FD_ISSET(fd, &rset)) {
            struct sockaddr_in cli;
            socklen_t len = sizeof(cli);
            int client_fd = accept(fd, (struct sockaddr*)&cli, &len);

            int i;
            for (i = 0; i < MAX_CLIENTS; i++) {
                if (clients[i] < 0) {
                    clients[i] = client_fd;
                    break;
                }
            }

            if (i == MAX_CLIENTS) {
                fprintf(stderr, "Too many clients\n");
                close(client_fd);
            } else {
                FD_SET(client_fd, &allset);
                if (client_fd > maxfd) maxfd = client_fd;
                if (i > maxi) maxi = i;
                printf("Client connected (fd=%d)\n", client_fd);
            }
        }
        for (int i = 0; i <= maxi; i++) {
            int client = clients[i];
            if (client < 0 || !FD_ISSET(client, &rset)) continue;
            
            int n = recv(client, buf, sizeof(buf) - 1, 0);
            if (n == 0) {
                printf("Client disconnected (fd=%d)\n", client);
                reset_fd(client);
                close(client);
                FD_CLR(client, &allset);
                del_client(i, clients);
            } else {
                buf[n] = '\0';
                handle_client_json(client, buf, db);
            }
        }
    }
}

/*static int add_client(int fd, int *a)
{
		for (int i = 0; i < MAXCL; i++)
		{
				if (a[i] < 0) {a[i] = fd; return 1; }
		}
		return 0;
}*/
static void del_client(int i, int *a) { a[i] = -1; }
