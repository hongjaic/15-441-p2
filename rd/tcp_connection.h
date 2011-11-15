/**
 * CS 15-441 Computer Networks
 *
 * Struct definition for flask tcp connections, used to maintain states.
 *
 * @file    tcp_connection.h
 * @author  Hong Jai Cho <hongjaic>, Raul Gonzalez <rggonzal>
 */

#ifndef TCP_CONNECTION_H
#define TCP_CONNECTION_H

#include "constants.h"

typedef struct tcp_connection
{
    char request[MAX_BUF];
    int request_index;
    char response[MAX_BUF];
    int response_index;
    char method[MAX_BUF];
    char name_length[MAX_BUF];
    char name[MAX_BUF];
    char path_length[MAX_BUF];
    char path[MAX_BUF];
    int status;
} tcp_connection;

void init_connection(tcp_connection *connection);

#endif
