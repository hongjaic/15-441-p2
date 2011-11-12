#ifndef ROUTING_TABLE_H
#define ROUTING_TABLE_H

#include "constants.h"

typedef struct routing_entry
{
    int id;
    int nexthop;
    int cost;
    int node_status;
} routing_entry;

typedef struct routing_table
{
    routing_entry table[MAX_NODES];
    int num_entry;
} routing_table;

#endif
