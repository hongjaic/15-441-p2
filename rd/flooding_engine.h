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

#define SET_VERSION(line1, version)     (line1|(version<<24))
#define SET_TTL(line1, ttl)             (line1|(ttl<16))
#define SET_TYPE(line1, type)           (line1|type)

#define TYPE_LSA                0x00000000
#define TYPE_ACK                0x00000001
#define TYPE_DOWN               0x00000010

#define STATUS_UP               1
#define STATUS_DOWN             0

#define DEFAULT_TTL             0x20

#define MAX_NODES               150
#define MAX_BUF                 1024

#define LSA_SENT                1
#define LSA_NOT_SENT            0

#define LSA_TIMEOUT             3


extern engine_wrapper engine;

#pragma pack(1)
typedef struct LSA
{
    int version_ttl_type;
    int sender_node_id;
    int sequence_num;
    int num_links;
    int num_objects;
    char links_objects[];
} LSA;
#pragma pack()

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

typedef struct neighbor
{
    char host[MAX_BUF];
    int routing_port;
    int local_port;
    int server_port;
    int lsa_sent;
    int lsa_timeout;
} neighbor;

typedef struct direct_neighbors
{
    neighbor neighbors[MAX_NODES]; 
} direct_neighbors;

typedef struct _link_entry{
	int id;
	char host[MAX_HOST_LEN];
	int local_p;
	int route_p;
	int server_p;
	int retransmits;
	int ack_received;
	
}link_entry;

typedef struct _direct_links{
	int num_links;
	link_entry links[MAX_NODES];
}direct_links;

int flooding_engine_create(int port);
void update_table(routing_table *rt, LSA *lsa, int nexthop);
void update_entry(routing_table *rt, routing_entry *entry);
void lookup_entry(routing_table *rt, struct in_addr in);
int bytes_to_packet(LSA *lsa, char *buf, int size);
char *get_neighbor_address(int node_id);
routing_entry *get_routing_entry(routing_table *rt, int node_id);
void flood_lsa(direct_links neighbors);


#endif
