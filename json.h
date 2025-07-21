#ifndef JSON_H
#define JSON_H

#include "cJSON.h"

void send_json(int sockfd, const char *type, cJSON *payload);

void handle_server_response(const char *buf);

void print_menu();

void parse_user_input(const char *input, int sockfd);

#endif
