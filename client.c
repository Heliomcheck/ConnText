#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <arpa/inet.h>
#include "erproc.h"
#include <sys/select.h>
#include <sys/time.h>
#include <cJSON.h>
#include <string.h>
#include "json.h"

#define PORT 6868
#define BUF_SIZE 1024

int main(){
		int fd = Socket(AF_INET, SOCK_STREAM, 0);
        struct sockaddr_in adr = {0}; 
        adr.sin_family = AF_INET; 	
        adr.sin_port = htons(6868); 
        Inet_pton(AF_INET, "127.0.1.1", &adr.sin_addr); 
		Connect(fd, (struct sockaddr*)&adr, sizeof adr); 
		char buf[BUF_SIZE] = {0};

		for (;;)
		{
				fd_set set;
				FD_ZERO(&set);
				FD_SET(STDIN_FILENO, &set);
				FD_SET(fd, &set);

				int maxfd = (fd > STDIN_FILENO ? fd : STDIN_FILENO);
				Select(maxfd + 1, &set, NULL, NULL, NULL);
                
                if (FD_ISSET(STDIN_FILENO, &set))
                {
                    char input[BUF_SIZE];
                    if(fgets(input, sizeof(input), stdin))
                    {
                        input[strcspn(input, "\n")] = 0;
                        parse_user_input(input, fd);
                    }
                }

                if (FD_ISSET(fd, &set))
                {
                    int n = recv(fd, buf, sizeof(buf) - 1, 0);
                    if (n <= 0)
                    {
                        printf("Server disconnected\n");
                        break;
                    }
                    buf[n] = '\0';
                    handle_server_response(buf);
                }
        }
		close(fd);
		return 0;
}
