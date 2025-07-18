#ifndef ERPROC_H
#define ERPROC_H

#include <sys/socket.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>

int Socket(int domain, int type, int protocol);

int Bind(int sockfd, const struct sockaddr *addr, socklen_t addrlen);

int Listen(int sockfg, int backlog);

int Accept(int serverfg, struct sockaddr *addr, socklen_t *addrlen);

int Connect(int sockfd, const struct sockaddr *addr, socklen_t addrlen);

int Inet_pton(int af, const char *restrict src, void *restrict dst);

int Select(int nfds, fd_set *r, fd_set *w, fd_set *e, struct timeval *t);

#endif
