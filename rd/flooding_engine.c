/**
 * CS 15-441 Computer Networks
 *
 * @file    flooding_engine.c
 * @author  Hong Jai Cho <hongjaic>, Raul Gonzalez <rggonzal>
 */

#include "flooding_engine.h"

void update_entry(routing_table *rt, LSA *lsa, int nexthop)
{
    int node_id;
    int new_cost;

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
    old_cost = (rt->table[node_id])->cost;

    if (new_cost < old_cost)
    {
        (rt->table[node_id])->nexthop = nexthop;
        (rt->table[node_id])->cost = newcost;
    }
}

routing_entry *lookup_entry(routing_table *rt, struct in_addr in)
{
    int i;
    char *address = inet_ntoa(in);
    routing_entry entry;

    for (i = 1; i < MAX_NODES; i++)
    {
        entry = (rt->table)[i];
        
        if (strcmp(entry.host, address) == 0)
        {
			return &(entry);
        }
	}

	return NULL;
}


routin_entry *lookup_id(routing_table *rt, struct in_addr in)
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
/*
given an lsa string, decompose it onto its corresponding
"packet" structure parts. e.g. first byte in lsa  == packet version. etc.. 
see packet structure for more deets.
essentially copies string lsa to packet lsa. 
binds lsa packet info to the node that originally sent the lsa.
*/
int create_packet(node *node,char *lsa)
{

	int i;
	int obj_len;
	
	node->lsa.version = lsa[0];
	lsa+=1;
	node->lsa.ttl = lsa[0];
	lsa+=1;
	node->lsa.type[0] = lsa[0];
	node->lsa.type[1] = lsa[1];
	lsa+=2;
	node->lsa.src_id = (int)lsa;
	lsa+=4;
	node->lsa.seq_num = (int)lsa;
	lsa+=4;
	node->lsa.num_neighbors = (int)lsa;
	lsa+=4;
	node->lsa.num_objs =(int)lsa;
	lsa+=4;
	

	for(i=0;i<node->lsa.num_neighbors;i++){
		node->lsa.neighbors[i]=(int)lsa;	
		lsa+=4;
	}

	for(i=0;i<node->lsa.num_objs;i++){
		obj_len =(int)lsa;
		lsa+=4;
		strncpy(node->lsa.objs[i],lsa,obj_len);
		lsa+=obj_len;
	}

	return SUCCESS;

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

LSA *rt_recvfrom(int sock, int *forwarder_id, routing_table *rt)
{
    struct sockaddr_in cli_addr;
	socklen_t clilen;
    clilen = sizeof(cli_addr);
	
    char buf[MAX_LSA_LEN];

    routing_entry *entry;

    int ret;

    int source_id;

    LSA *lsa;

	ret = recvfrom(sock, buf, MAX_LSA_LEN, 0, (struct sockaddr *)&cli_addr, &clilen);
	
	if (ret < 5*sizeof(int))
    {

		//could not retrienve src_id from lsa. no way of knowing who is original sender. 
		//no way of buffering lsa until it has been completely received. 
		// ignore this LSA. hope retransmission works.
		return NULL;
	}

    bytes_to_packet(lsa, buf, ret);

    entry = lookup_entry(rt, cli_addr.sin_addr);
    *forwarder_id = lookup_id(rt, cli_addr.sin_addr);

    source_id = *((int *)&(buf[4]));

    


	newlen = strlen(src_node->lsa_buf)+strlen(buf);
	if( newlen > MAX_LSA_LEN){
		//error , lsa packet is too large!
		return NULL;
	}else{
		strcat(src_node->lsa_buf,buf);
	
		lsa = src_node->lsa_buf;
		if(strstr(lsa,"\r\n") == NULL){
			return NULL;
		}
	
		create_packet(src_node,lsa);
		
		return src_node;

	}

}

/*
creates a UDP socket and sends the given string to the given node.

*/
int rd_send(node *to_node,char *str)
{
	int ret;
	int sockfd;

	struct addrinfo hints, *servinfo;
	char rp[4];
	sprintf(rp,"%d",to_node->route_p);	
	if ((ret = getaddrinfo(to_node->host, rp, &hints, &servinfo)) != 0) {
        	//fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(ret));
        	return 1;
    	}	


	sockfd = socket(servinfo->ai_family, servinfo->ai_socktype, servinfo->ai_protocol);
	if(sockfd < 0) {
		printf("error on socket() call");
		return -1;
	}

	sendto(to_node->route_p,str , strlen(str), 0, servinfo->ai_addr,servinfo->ai_addrlen);	
	freeaddrinfo(servinfo);
	close(sockfd);

	return SUCCESS;
}



/*
handler for any incoming lsa advertisements from other routing daemons.
receives the incoming lsa from a neighbor (forwarder_id) 
decrements TTL.
forwards lsa to all its other neighbors.
cd 
*/
int lsa_handler(int rd_sock,int neighbors)
{
	int i;
	int forwarder_id;
	node *src_node;

	src_node = rd_recvfrom(rd_sock,&forwarder_id);

	if (src_node == NULL){
		return FAIL;
	}

	//forward lsa to all other neighbors
	for (i=1; i<neighbors;i++)
    { //skip nodes[0] = self.
		if(nodes[i].id != forwarder_id){	
			src_node->lsa_buf[0]--; //decremet TTL
			rd_send(&(nodes[i]),src_node->lsa_buf);
		}
	}
	printf("received lsa from %d\n",src_node->id);
	return SUCCESS;
}


/*
Initializes all global variables, and structures.
*/
