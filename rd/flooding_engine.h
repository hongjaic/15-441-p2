/**
 * CS 15-441 Computer Netwoks 
 *
 * @file    flooding_engine.h
 * @author  Hong Jai Cho <hongjaic>, Raul Gonzales <rggonzal>
 */

#ifndef FLOODING_ENGINE_H
#define FLOODING_ENGINE_H

#define GET_VERSION(version_ttl_type)   ((version_ttl_type>>24)&0xFF)
#define GET_TTL(version_ttl_type)       ((version_ttl_type>>16)&0xFF)
#define GET_TYPE(version_ttl_type)      (version_ttl_type&0xFFFF)

#define SET_VERSION(line1, version) (line1|(version<<24))
#define SET_TTL(line1, ttl)     (line1|(ttl<16))
#define SET_TYPE(line1, type)    (line1|type)

#define DEFAULT_LSA_LINE1       0x01200000
#define DEFAULT_ACK_LINE1       0x01000001    

#define TYPE_LSA                0x00000000
#define TYPE_ACK                0x00000001
#define TYPE_DOWN               0x00000010

#define DEFAULT_TTL             0x20

#define MAX_NODES               150

typedef struct LSA
{
    int version_ttl_type;
    int sender_node_id;
    int sequence_num;
    int num_links;
    int num_objects;
    char links_objects[];
} LSA;

typedef struct routing_entry
{
    int id;
    int host;
    int nexthop;
    int cost;
} routing_entry;

typedef struct routing_table
{
    routing_entry table[MAX_NODE_NUM];
} routing_table;


void update_table(routing_table *rt, LSA *lsa, int nexthop);
void update_entry(routing_table *rt, routing_entry *entry);
void lookup_entry(routing_table *rt, struct in_addr in);
int bytes_to_packet(LSA *lsa, char *buf, int size);


#endif
