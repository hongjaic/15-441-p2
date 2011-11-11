/**
 * CS 15-441 Computer Networks
 *
 * @file    flooding_engine.c
 * @author  Hong Jai Cho <hongjaic>, Raul Gonzalez <rggonzal>
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <stdio.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <arpa/inet.h>
#include <netdb.h>
#include "flooding_engine.h"

/*
int flooding_engine_create(int port)
{
    struct sockaddr_in serv_addr;

    engine.udp_sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

    if (engine.udp_sock < 0)
    {
        fprintf(stderr, "Failed to create UDP socket.\n");
        exit(EXIT_FAILURE);
    }

    memset(&serv_addr, '\0', sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(engine.udp_port);

    if (bind(engine.udp_sock, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0)
    {
        close(engine.udp_sock);
        fprintf(stderr, "Failed to bind UDP socket.\n");
        exit(EXIT_FAILURE);
    }

    return 1;
}
*/
/*
void update_entry(routing_table *rt, LSA *lsa, int nexthop)
{
    int node_id;
    int old_cost;
    int new_cost;
    int type;
    routing_entry *entry;

    if (rt == NULL)
    {
        return;
    }
    
    if (lsa == NULL)
    {
        return;
    }

    type = GET_TYPE(lsa->version_ttl_type);
    node_id = lsa->sender_node_id;
    entry = get_routing_entry(rt, node_id);

    if (type == TYPE_LSA)
    {
        new_cost = DEFAULT_TTL - GET_TTL(lsa->version_ttl_type);

        if (entry == NULL)
        {
            entry = &((rt->table)[rt->num_entry]);
            entry->id = node_id;
            entry->nexthop = nexthop;
            entry->cost = new_cost;
            entry->node_status = STATUS_UP;
        }
        else
        {
            old_cost = entry->cost;

            if (new_cost < old_cost)
            {
                entry->nexthop = nexthop;
                entry->cost = new_cost;
            }
        }
    }
    else if (type == TYPE_DOWN)
    {
        entry->node_status = STATUS_DOWN;
    }
}
*/
routing_entry *get_routing_entry(routing_table *rt, int node_id)
{
    int i;
    routing_entry *entry;

    if(rt == NULL)
    {
        return NULL;
    }

    entry = rt->entry;

    for (i = 0; i < rt->num_entry; i++)
    {
        if (entry[i].id == node_id)
        {
            return &(entry[i]);
        }
    }

    return NULL;
}


char * get_link_address(int id,direct_links *neighbors)
{

	char *address = (char *)malloc(MAX_OBJ_LEN);
	int i;
	for(i = 0; i<neighbors->num_link;i++)
	{
		if(neighbors->link[i].id == id)
		{
			sprintf(address,"%s:%d/r/",neighbors->link[i].host,neighbors->link[i].server_p);
		} 
		
	}
	address[0]='\0';
	return address;
}


/*
routing_entry *lookup_entry(routing_table *rt, struct in_addr in)
{
    int i;
    char *address = inet_ntoa(in);
    routing_entry entry;
    

    for (i = 1; i < MAX_NODES; i++)
    {
        entry = rt->nodes[i];
        
        if (strcmp(entry.host, address) == 0)
        {
			return &(entry);
        }
    }

	return NULL;
}
*/
/*
routing_entry *lookup_id(routing_table *rt, struct in_addr in)
{
    int i;
    char *address = inet_ntoa(in);
    routing_entry entry;

    for (i = 1; i < MAX_NODES; i++)
    {
        entry = (rt->table)[i];
        
        if (strcmp(entry.host, address) == 0)
        {
			return entry.id;
        }
	}

	return NULL;
}
*/


void create_packet(LSA *lsa, int type, int sequence_num, direct_links *dl, local_objects *ol)
{
    int i, name_len;
    int num_links = 0;
    int num_objects = 0;
    int link_size_total = 0;
    int object_size_total = 0;
    object_entry *objects;
    link_entry *links;
    int *i_ptr;
    char *c_ptr;
    
    /*    
    SET_VERSION(lsa->version_ttl_type, 0x1);
    SET_TTL(lsa->version_ttl_type, DEFAULT_TTL);
    SET_TYPE(lsa->version_ttl_type, type);
	*/
    lsa->sequence_num = sequence_num;

    if (type == TYPE_ACK || type == TYPE_DOWN)
    {
        lsa = (LSA *)malloc(sizeof(LSA));
    }
    else if (type == TYPE_LSA)
    {
        num_objects = ol->num_objects;
        num_links = dl->num_link;
        link_size_total += num_links*sizeof(int);
        object_size_total += num_objects*sizeof(int);
        
        links = dl->link;
        objects = ol->objects;

        for (i = 0; i < num_objects; i++)
        {
            object_size_total += strlen(objects[i].name) + 1;
        }

        lsa = (LSA *)malloc(sizeof(LSA) + link_size_total + object_size_total);

        c_ptr = lsa->links_objects;
        i_ptr = (int *)c_ptr;

        for (i = 0; i < num_links; i++)
        {
            *i_ptr = links[i].id;
            i_ptr++;
        }

        for (i = 0; i < num_objects; i++)
        {
            name_len = strlen(objects[i].name) + 1;
            *i_ptr = name_len;
            i_ptr++;

            c_ptr = (char *)i_ptr;
            strcpy(c_ptr, objects[i].name);
            c_ptr += name_len;
            i_ptr = (int *)c_ptr;
        }
    }
}

/*
 * Copies received bytes into packet.
 *
 */
void bytes_to_packet(LSA *lsa, char *buf, int size)
{
    lsa = (LSA *)malloc(size);
    memcpy(lsa, buf, size); 
}

/*
wrapper for recv_from. 
binds received lsa info to the node that sent it.
returns pointer to that node.
*/

LSA *rt_recvfrom(int sockfd, int *forwarder_id, routing_table *rt)
{
    struct sockaddr_in cli_addr;
	socklen_t clilen;
    clilen = sizeof(cli_addr);
    char buf[MAX_BUF];
   // routing_entry *entry;
    int ret;

    LSA *lsa = NULL;

	ret = recvfrom(sockfd, buf, MAX_BUF, 0, (struct sockaddr *)&cli_addr, &clilen);
	
	if (ret < 5*sizeof(int))
    {

		//could not retrienve src_id from lsa. no way of knowing who is original sender. 
		//no way of buffering lsa until it has been completely received. 
		// ignore this LSA. hope retransmission works.
		return NULL;
	}

    bytes_to_packet(lsa, buf, ret);

   // entry = lookup_entry(rt, cli_addr.sin_addr);
   // *forwarder_id = lookup_id(rt, cli_addr.sin_addr);

    return lsa;
}

/*
creates a UDP socket and sends the given string to the given node.

*/
int rt_send(int sockfd, LSA *lsa, int node_id, int port, int size, direct_links *neighbors)
{
    struct sockaddr_in cli_addr;
    struct hostent *h;
    int clilen;
    
    char *cli_address =get_link_address(node_id, neighbors);

    if ((h = gethostbyname(cli_address)) == NULL)
    {
        return -1;
    }

    memset(&cli_addr, '\0', sizeof(cli_addr));
    cli_addr.sin_family = AF_INET;
    cli_addr.sin_addr.s_addr = *(in_addr_t *)h->h_addr;
    cli_addr.sin_port = htons(port);

    clilen = sizeof(cli_addr);

  
   return sendto(sockfd, lsa, size, 0, (struct sockaddr *)&cli_addr, sizeof(cli_addr));
}


/*
handler for any incoming lsa advertisements from other routing daemons.
receives the incoming lsa from a neighbor (forwarder_id) 
decrements TTL.
forwards lsa to all its other neighbors.
cd 
*/
int lsa_handler(direct_links *neighbors, routing_table *rt ,int sock)
{
	int i = 1;
	int forwarder_id;
	short type;
	LSA *lsa;

	lsa = rt_recvfrom(sock,&forwarder_id,rt);
	type = GET_TYPE(lsa->version_ttl_type);
	
	if(type == 0x00)
	{
		for(i=1; i<neighbors->num_link-1;i++)
		{ 
			if(neighbors->link[i].id != forwarder_id)
			{	
				//decremet TTL !!!!!!!
				rt_send(sock,lsa,neighbors->link[i].id,neighbors->link[i].route_p,sizeof(lsa),neighbors);
			}
		}

	}else if (type == 0x01)
	{
		while(neighbors->link[i].id!=forwarder_id)
		{
			i++;
		}
				
	}else
	{
		printf("hey");
	}
		
	printf("received lsa from %d\n",lsa->sender_node_id);
	return SUCCESS;
}


/*
Initializes all global variables, and structures.
*/

void flood_lsa(direct_links *neighbors, LSA *lsa, int sock)
{
	int i;
//forward lsa to all other neighbors
	for(i=1; i<neighbors->num_link-1;i++)
	{ 
		rt_send(sock,lsa,neighbors->link[i].id,neighbors->link[i].route_p,sizeof(lsa),neighbors);
	}

}
