#ifndef ROUTING_TABLE_H
#define ROUTING_TABLE_H

#include "constants.h"
#include "lsa.h"

typedef struct ack_checker
{
    int id;
    int ack_received;
    int retransmit;
} ack_checker;

typedef struct ack_checkers
{
    int num_links;
    ack_checker checkers[];
} ack_checkers;

typedef struct routing_entry
{
    int id;
    int nexthop;
    int cost;
    int node_status;
    LSA *lsa;
    ack_checkers *checkers_list;
} routing_entry;

typedef struct routing_table
{
    routing_entry table[MAX_NODES];
    int num_entry;
} routing_table;

#endif
