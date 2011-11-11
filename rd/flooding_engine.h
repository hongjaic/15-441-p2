/**
 * CS 15-441 Computer Netwoks 
 *
 * @file    flooding_engine.h
 * @author  Hong Jai Cho <hongjaic>, Raul Gonzales <rggonzal>
 */

#ifndef FLOODING_ENGINE_H
#define FLOODING_ENGINE_H
#include "rd.h"

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

#define FAIL -1
#define SUCCESS 1



//extern engine_wrapper engine;

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
    int sequence_num;
} routing_entry;

typedef struct routing_table
{
    routing_entry entry[MAX_NODES];
    int num_entry;
} routing_table;


typedef struct _link_entry
{
	int id;
	char host[MAX_BUF];
	int local_p;
	int route_p;
	int server_p;
	int retransmits;
	int ack_received;
	
}link_entry;

typedef struct _direct_links
{
	int num_link;
	link_entry link[MAX_NODES];
}direct_links;




int flooding_engine_create(int port);
void update_table(routing_table *rt, LSA *lsa, int nexthop);
void update_entry(routing_table *rt, LSA *lsa, int nexthop);
routing_entry *lookup_entry(routing_table *rt, struct in_addr in);
void bytes_to_packet(LSA *lsa, char *buf, int size);
char *get_neighbor_address(int node_id);
routing_entry *get_routing_entry(routing_table *rt, int node_id);
int lsa_handler(direct_links *neighbors, routing_table *rt ,int rd_sock);
LSA *rt_recvfrom(int sockfd, int *forwarder_id, routing_table *rt);
int rt_send(int sockfd, LSA *lsa, int node_id, int port, int size,direct_links *neighbors);
void flood_lsa(direct_links *neighbors, LSA *lsa, int sock);
void create_packet(LSA *lsa, int type, int sequence_num, direct_links *dl, local_objects *ol);
char * get_link_address(int id,direct_links *neighbors);
#endif
