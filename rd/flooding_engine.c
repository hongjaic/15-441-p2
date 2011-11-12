/**
 * CS 15-441 Computer Networks
 *
 * @file    flooding_engine.c
 * @author  Hong Jai Cho <hongjaic>, Raul Gonzalez <rggonzal>
 */

#include "flooding_engine.h"

int flooding_engine_create()
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

routing_entry *get_routing_entry(routing_table *rt, int node_id)
{
    int i, num_entry;
    routing_entry *table;

    if (rt == NULL)
    {
        return NULL;
    }

    table = rt->table;

    num_entry = rt->num_entry;
    for (i = 0; i < num_entry; i++)
    {
        if (table[i].id == node_id)
        {
            return &((rt->table)[i]);
        }
    }

    return NULL;
}

link_entry *lookup_link_entry(direct_links *dl, struct in_addr in)
{
    int i, num_links;
    char *address = inet_ntoa(in);
    link_entry entry;

    num_links = dl->num_links;
    for (i = 0; i < num_links; i++)
    {
        entry = (dl->links)[i];
        
        if (strcmp(entry.host, address) == 0)
        {
			return &((dl->links)[i]);
        }
	}

	return NULL;
}

link_entry *lookup_link_entry_node_id(direct_links *dl, int node_id)
{
    int i, num_links;
    link_entry entry;

    num_links = dl->num_links;
    for (i = 0; i < num_links; i++)
    {
        entry = (dl->links)[i];
        
        if (entry.id == node_id)
        {
			return &((dl->links)[i]);
        }
	}

	return NULL;
}

int create_packet(LSA *lsa, int type, int node_id, int sequence_num, direct_links *dl, local_objects *ol)
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

    if (type == TYPE_ACK || type == TYPE_DOWN)
    {
        lsa = (LSA *)malloc(sizeof(LSA));
    }
    else if (type == TYPE_LSA)
    {
        num_objects = ol->num_objects;
        num_links = dl->num_links;
        link_size_total += num_links*sizeof(int);
        object_size_total += num_objects*sizeof(int);
        
        links = dl->links;
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

    lsa->version_ttl_type = SET_VERSION(lsa->version_ttl_type, 0x1);
    lsa->version_ttl_type = SET_TTL(lsa->version_ttl_type, DEFAULT_TTL);
    lsa->version_ttl_type = SET_TYPE(lsa->version_ttl_type, type);
    lsa->sender_node_id = node_id; 
    lsa->sequence_num = sequence_num;
    lsa->num_links = num_links;
    lsa->num_objects = num_objects;

    return sizeof(LSA) + link_size_total + object_size_total;
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

LSA *rt_recvfrom(int sockfd, int *forwarder_id, direct_links *dl, int *lsa_size)
{
    struct sockaddr_in cli_addr;
	socklen_t clilen;
    clilen = sizeof(cli_addr);
	
    char buf[MAX_BUF];

    int ret;

    LSA *lsa = NULL;

    link_entry *entry;

	ret = recvfrom(sockfd, buf, MAX_LSA_LEN, 0, (struct sockaddr *)&cli_addr, &clilen);
	
	if (ret < 5*sizeof(int))
    {

		//could not retrienve src_id from lsa. no way of knowing who is original sender. 
		//no way of buffering lsa until it has been completely received. 
		// ignore this LSA. hope retransmission works.
		return NULL;
	}

    bytes_to_packet(lsa, buf, ret);

    entry = lookup_link_entry(dl, cli_addr.sin_addr);

    *forwarder_id = entry->id;
    *lsa_size = ret;

    return lsa;
}

/*
creates a UDP socket and sends the given string to the given node.

*/
int rt_sendto(int sockfd, LSA *lsa, char *host, int port, int size)
{
    struct sockaddr_in cli_addr;
    int clilen;
    int ret;

    memset(&cli_addr, '\0', sizeof(cli_addr));
    cli_addr.sin_family = AF_INET;
    cli_addr.sin_addr.s_addr = inet_addr(host);
    cli_addr.sin_port = htons(port);

    clilen = sizeof(cli_addr);

    ret = sendto(sockfd, lsa, size, 0, (struct sockaddr *)&cli_addr, sizeof(cli_addr));

    return ret;
}



/*
handler for any incoming lsa advertisements from other routing daemons.
receives the incoming lsa from a neighbor (forwarder_id) 
decrements TTL.
forwards lsa to all its other neighbors.
cd 
*/
int lsa_handler(int sockfd, direct_links *dl, routing_table *rt)
{
    int forwarder_id;
    int type;
    int new_ttl;
    int lsa_size;
    LSA *lsa;
    int i;

    lsa = rt_recvfrom(sockfd, &forwarder_id, dl, &lsa_size);

    type = GET_TYPE(lsa->version_ttl_type);

    if (type == TYPE_LSA || type == TYPE_DOWN)
    {
        update_entry(rt, lsa, forwarder_id);
        
        new_ttl = GET_TTL(lsa->version_ttl_type) - 1;
        lsa->version_ttl_type = SET_TTL(lsa->version_ttl_type, new_ttl);
        
        flood_received_lsa(sockfd, lsa, dl, rt, lsa_size, forwarder_id);
    }
    else if (type == TYPE_ACK)
    {
        for (i = 0; i < dl->num_links; i++)
        {
            if (dl->num_links == forwarder_id)
            {
                //
                // do something with routing table
                //
            }
        }
    }

    return 1;
}

/*
Initializes all global variables, and structures.
*/

int flood_lsa(int sockfd, direct_links *dl, local_objects *ol, routing_table *rt, int node_id, int sequence_num)
{
    int i, num_links;
    int port;
    char *host;
    int size;

    LSA *lsa = NULL;
    size = create_packet(lsa, TYPE_LSA, node_id, sequence_num, dl, ol);

    num_links = dl->num_links;
    for (i = 1; i < num_links; i++)
    {
        if (get_routing_entry(rt, ((dl->links)[i]).id)->node_status != STATUS_DOWN)
        {
            port = ((dl->links)[i]).route_p;
            host = ((dl->links)[i]).host;
            rt_sendto(sockfd, lsa, host, port, size);
            //
            //do something with routing table
            //
        }
    }

    //free(lsa);
    
    return 1;
}

void retransmit(int sockfd, LSA *lsa, direct_links *dl, routing_table *rt, int lsa_size, int forwarder_id)
{
    int i, num_links;
    //int port;
    //char *host;
    int node_id;

    num_links = dl->num_links;
    for (i = 1; i < num_links; i++)
    {
        node_id = ((dl->links)[i]).id;

        if (node_id != forwarder_id)
        {
            // Do something with routing table
            //if ((dl->links)[i].ack_received == ACK_NOT_RECEIVED)
            //{
            //    if (get_routing_entry(rt, ((dl->links)[i]).id)->node_status != STATUS_DOWN)
            //    {
            //        port = ((dl->links)[i]).route_p;
            //        host = ((dl->links)[i]).host;
            ///        rt_sendto(sockfd, lsa, host, port, lsa_size);
            //    }
            //}
        }
    }

    //free(lsa);
}

int flood_received_lsa(int sockfd, LSA *lsa, direct_links *dl, routing_table *rt, int lsa_size, int forwarder_id)
{
    int i, num_links;
    int port;
    char *host;
    int node_id;

    num_links = dl->num_links;
    for (i = 1; i < num_links; i++)
    {
        node_id = ((dl->links)[i]).id;

        if (node_id != forwarder_id)
        {
            if (get_routing_entry(rt, ((dl->links)[i]).id)->node_status != STATUS_DOWN)
            {
                port = ((dl->links)[i]).route_p;
                host = ((dl->links)[i]).host;
                rt_sendto(sockfd, lsa, host, port, lsa_size);
            
                //
                //// 
                //Do something with routing table
                ///
            }
        }
    }

    //free(lsa);

    return 1;
}
