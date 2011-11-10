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
#include <unistd.h>

#include <netinet/in.h>
#include <netinet/ip.h>
#include <arpa/inet.h>
#include <netdb.h>


#include "flooding_engine.h"


void update_entry(routing_table *rt, LSA *lsa, int nexthop)
{
    int node_id;
    int new_cost;
	int old_cost;
    if (rt == NULL)
    {
        return;
    }
    
    if (lsa == NULL)
    {
        return;
    }

    node_id = lsa->sender_node_id;
    new_cost = DEFAULT_TTL - GET_TTL(lsa->version_ttl_type);
    old_cost = (rt->table[node_id]).cost;

    if (new_cost < old_cost)
    {
        (rt->table[node_id]).nexthop = nexthop;
        (rt->table[node_id]).cost = new_cost;
    }
}

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

LSA *rt_recvfrom(routing_table *rt,int sock, int *forwarder_id)
{
    struct sockaddr_in cli_addr;
	socklen_t clilen;
    clilen = sizeof(cli_addr);
	
	LSA *lsa = NULL;
    char buf[MAX_BUF];

    //routing_entry *entry;

    int ret;

    int source_id;


	ret = recvfrom(sock, buf, MAX_BUF, 0, (struct sockaddr *)&cli_addr, &clilen);
	
	if (ret < 5*sizeof(int))
    {

		//could not retrienve src_id from lsa. no way of knowing who is original sender. 
		//no way of buffering lsa until it has been completely received. 
		// ignore this LSA. hope retransmission works.
		return NULL;
	}

    bytes_to_packet(lsa, buf, ret);

    //entry = lookup_entry(rt, cli_addr.sin_addr);
    //*forwarder_id = lookup_id(rt, cli_addr.sin_addr);

    source_id = *((int *)&(buf[4]));

    
	
	
	//create_packet(src_node,lsa);
		
	return lsa;

	

}



/*
creates a UDP socket and sends the given string to the given node.

*/
int rt_send(link_entry link,LSA *lsa)
{
	int ret;
	int sockfd;

	struct addrinfo hints, *servinfo;
	char rp[4];
	sprintf(rp,"%d",link.route_p);	
	if ((ret = getaddrinfo(link.host, rp, &hints, &servinfo)) != 0) {
        	//fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(ret));
        	return 1;
    	}	


	sockfd = socket(servinfo->ai_family, servinfo->ai_socktype, servinfo->ai_protocol);
	if(sockfd < 0) {
		printf("error on socket() call");
		return FAIL;
	}

	sendto(link.route_p,lsa, sizeof(lsa), 0, servinfo->ai_addr,servinfo->ai_addrlen);	
	freeaddrinfo(servinfo);
	close(sockfd);

	return SUCCESS;
}




void retransmit_lsa(link_entry link){


}

void flood_lsa(direct_links neighbors){


}


