#include "erproc.h"
#include <sys/socket.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>

int Socket(int domain, int type, int protocol){
        int res = socket(domain, type, protocol);
        if (res == -1){
                perror("socket failure");
                exit(EXIT_FAILURE);
        }
        return res;
}

int Bind(int sockfd, const struct sockaddr *addr, socklen_t addrlen){
        int res = bind(sockfd, addr, addrlen);
        if (res == -1){
                perror("bind failure");
                exit(EXIT_FAILURE);
        }
        return res;
}

int Listen(int sockfg, int backlog){
        int res = listen(sockfg, backlog);
        if (res == -1){
                perror("listen failure");
                exit(EXIT_FAILURE);
        }
        return res;
}

int Accept(int serverfg, struct sockaddr *addr, socklen_t *addrlen){
        int res = accept(serverfg, addr, addrlen);
        if (res == -1){
                perror("accept failure");
                exit(EXIT_FAILURE);
        }
        return res;
}

int Inet_pton(int af, const char *restrict src, void *restrict dst){
		int res = inet_pton(af, src, dst);
		if (res == 0){
				printf("inet_pton failed: ip addres is not corrected");
				exit(EXIT_FAILURE);
		}
		if (res == -1){
				perror("inet_pton failure");
				exit(EXIT_FAILURE);
		}
		return res;
}

int Connect(int sockfd, const struct sockaddr *addr, socklen_t addrlen){
		int res = connect(sockfd, addr, addrlen);
		if (res == -1){
				perror("connect failed");
				exit(EXIT_FAILURE);
		}
		return res;
}

int Select(int nfds, fd_set *f, fd_set *w, fd_set *e, struct timeval *t){
		int res = select(nfds, f, w, e, t);
		if (res == -1){
				perror("select failed");
				exit(EXIT_FAILURE);
		}
		return res;
}
