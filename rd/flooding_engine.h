/**
 * CS 15-441 Computer Netwoks 
 *
 * @file    flooding_engine.h
 * @author  Hong Jai Cho <hongjaic>, Raul Gonzales <rggonzal>
 */

#ifndef FLOODING_ENGINE_H
#define FLOODING_ENGINE_H

#include "engine_wrapper.h"
#include "direct_links.h"
#include "local_objects.h"
#include "routing_table.h"
#include <netinet/in.h>
#include <netinet/ip.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <stdio.h>
#include <unistd.h>
#include <netinet/ip.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/select.h>
#include "constants.h"
#include "lsa.h"

#define GET_VERSION(version_ttl_type)   ((version_ttl_type>>24)&0xFF)
#define GET_TTL(version_ttl_type)       ((version_ttl_type>>16)&0xFF)
#define GET_TYPE(version_ttl_type)      (version_ttl_type&0xFFFF)

#define SET_VERSION(line1, version)     (line1|(version<<24))
#define SET_TTL(line1, ttl)             (line1|(ttl<16))
#define SET_TYPE(line1, type)           (line1|type)



extern engine_wrapper engine;

int flooding_engine_create();
void update_entry(routing_table *rt, LSA *lsa, int nexthop);
routing_entry *get_routing_entry(routing_table *rt, int node_id);
link_entry *lookup_link_entry(direct_links *dl, struct in_addr in);
link_entry *lookup_link_entry_node_id(direct_links *dl, int node_id);
void bytes_to_packet(LSA *lsa, char *buf, int size);
int create_packet(LSA *lsa, int type, int node_id, int sequence_num, direct_links *dl, local_objects *ol);
char *get_neighbor_address(int node_id);
int flood_lsa(int sockfd, direct_links *dl, local_objects *ol, routing_table *rt, int node_id, int sequence_num);
int flood_received_lsa(int sockfd, LSA *lsa, direct_links *dl, routing_table *rt, int lsa_size, int forwarder_id);
void retransmit(int sockfd, LSA *lsa, direct_links *dl, routing_table *rt, int lsa_size, int forwarder_id);
LSA *rt_recvfrom(int sockfd, int *forwarder_id, direct_links *dl, int *lsa_size);
int rt_sendto(int sockfd, LSA *lsa, char *host, int port, int size);
int lsa_handler(int sockfd, direct_links *dl, routing_table *rt);

#endif
