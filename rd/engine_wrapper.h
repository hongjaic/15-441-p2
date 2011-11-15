/**
 * CS 15-441 Computer Networks
 *
 * Engine struct to maintain states of routing daemon.
 *
 * @file    engine_wrapper.h
 * @Author  Hong Jai Cho <hongjaic>, Raul Gonzales <rggonzal>
 */

#ifndef ENGINE_WRAPPER_H
#define ENGINE_WRAPPER_H

#include <sys/select.h>
#include "constants.h"
#include "tcp_connection.h"

typedef struct engine_wrapper
{
    int udp_sock;
    int udp_port;
    int tcp_sock;
    int tcp_port;
    fd_set rfds;
    fd_set temp_rfds;
    fd_set wfds;
    fd_set temp_wfds;
    int nfds;
    int fdmax;
    char buf[BUF_SIZE];
    int my_node_id;
    tcp_connection connections[MAX_CONNECTIONS];
} engine_wrapper;

#endif
