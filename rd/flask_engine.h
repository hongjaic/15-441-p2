/**
 * CS 15-441 Computer Networks
 *
 * @file    flask_engine.h
 * @author  Hong Jai Cho <hongjaic>, Raul Gonzales <rggonzal>
 */

#ifndef FLASK_ENGINE_H
#define FLASK_ENGINE_H

#include "engine_wrapper.h"
#include "routing_table.h"
#include "local_objects.h"
#include "constants.h"
#include "hashset.h"
#include "direct_links.h"
#include "lsa.h"

extern engine_wrapper engine;
extern direct_links dl;
extern local_objects ol;
extern liso_hash gol;
extern routing_table rt;
extern int my_node_id;
extern char my_uri[MAX_URI_LEN];
extern char *conffile;
extern char *filefile;

int flask_engine_create();
int new_client_handler(int sock);
int flask_request_handler(int i);
int flask_response_handler(int i);

#endif
