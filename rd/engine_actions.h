/**
 * CS 15-441 Computer Networks
 *
 * @file    generic_engine.h
 * @author  Hong Jai Cho <hongjaic>, Raul Gonzalez <rrgonzal>
 */

#ifndef ENGINE_ACTIONS_H
#define ENGINE_ACTIONS_H

#include "engine_wrapper.h"
#include "tcp_connection.h"
#include "flask_engine.h"
#include "flooding_engine.h"
#include "direct_links.h"
#include "local_objects.h"
#include "routing_table.h"
#include "constants.h"
#include "lsa.h"

extern engine_wrapper engine;
extern int my_node_id;
extern int sequence_number;
extern routing_table rt;
extern direct_links dl;
extern local_objects ol;
extern liso_hash gol;

int create_engine(int udp_port, int tcp_port);
int engine_event();

#endif
