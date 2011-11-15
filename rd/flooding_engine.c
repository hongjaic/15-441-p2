/**
 * CS 15-441 Computer Networks
 *
 * @file    flooding_engine.c
 * @author  Hong Jai Cho <hongjaic>, Raul Gonzalez <rggonzal>
 */

#include "flooding_engine.h"
void store_node_objects(liso_hash *ht,LSA *lsa)
{
   int i; 
   char object[MAX_OBJ_LEN];
   int num_links = lsa->num_links;
   int lsa_index = num_links << 2;
   int *object_len;
    for (i =0; i< lsa->num_objects;i++)
    {

        object_len =(int *)(lsa->links_objects+lsa_index);
        lsa_index+=4;
        strncpy(object,lsa->links_objects+lsa_index,*object_len);
        object[*object_len]='\0';
        lsa_index+= *object_len;
        hash_add(ht,object, lsa->sender_node_id,DEFAULT_TTL - GET_TTL(lsa->version_ttl_type)+1);
  
    }


}


int flooding_engine_create()
{
    struct sockaddr_in serv_addr;
    int yes = 1;
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
    setsockopt(engine.udp_sock,SOL_SOCKET, SO_REUSEADDR, &yes,sizeof(int));
    if (bind(engine.udp_sock, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0)
    {
        close(engine.udp_sock);
        fprintf(stderr, "Failed to bind UDP socket: %s\n",strerror(errno));
        exit(EXIT_FAILURE);
    }

    return 1;
}

void update_entry(routing_entry *entry, routing_table *rt, direct_links *dl, LSA *lsa, int lsa_size, int nexthop)
{
    int old_cost;
    int new_cost;
    int type;
    ack_checkers *curr_checkers;
    int i;

    if (rt == NULL)
    {
        return;
    }

    if (lsa == NULL)
    {
        return;
    }

    type = GET_TYPE(lsa->version_ttl_type);

    /* 
   if (entry->lsa == NULL)
    {
        entry->lsa = (LSA *)malloc(lsa_size);
    }

    entry->lsa->version_ttl_type = lsa->version_ttl_type;
    entry->lsa->sender_node_id = lsa->sender_node_id;
    entry->lsa->sequence_num = lsa->sequence_num;
    entry->lsa->num_links = lsa->num_links;
    entry->lsa->num_objects = lsa->num_objects;
    memcpy(entry->lsa->links_objects, lsa->links_objects, lsa_size - 20);
     */
    

    if (type == TYPE_LSA)
    {

       new_cost = DEFAULT_TTL - GET_TTL(lsa->version_ttl_type)+1;

        if (entry == NULL)
        {
            entry = &((rt->table)[rt->num_entry]);
            entry->id = lsa->sender_node_id;
            entry->nexthop = nexthop;
            entry->cost = new_cost;		
	   
	    rt->num_entry++;
 
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
 
  	if(entry->node_status == STATUS_DOWN){
   	   printf("node %d is up\n",entry->id);       
   	}
   	 entry->node_status = STATUS_UP;
          
    	
    }
   /* 
    if (type == TYPE_DOWN)
    {
        entry->node_status = STATUS_DOWN;
    }else{
        entry->node_status = STATUS_UP;
    }
   */
    
    entry->lsa = lsa;
    entry->lsa_is_new = 1;
    entry->lsa_size = lsa_size;
    entry->forwarder_id = nexthop;


    if (entry->checkers_list == NULL)
    {
        entry->checkers_list = (ack_checkers *)malloc(sizeof(ack_checkers) + dl->num_links*sizeof(ack_checker));
    }

    curr_checkers = entry->checkers_list;

    for (i = 0; i < dl->num_links; i++)
    {
        curr_checkers->checkers[i].id = dl->links[i].id;
        curr_checkers->checkers[i].ack_received = ACK_RECEIVED;
        curr_checkers->checkers[i].retransmit = 0;
	
    }
}


routing_entry *get_routing_entry(routing_table *rt, int node_id)
{
    int i, num_entry;

    if (rt == NULL)
    {
        return NULL;
    }

    num_entry = rt->num_entry;
    for (i = 0; i < num_entry; i++)
    {
        if (rt->table[i].id == node_id)
        {
            return &(rt->table[i]);
        }
    }
   return NULL;
}

link_entry *lookup_link_entry(direct_links *dl, struct sockaddr_in *cli_addr)
{
    int i, num_links;
    char *address = inet_ntoa(cli_addr->sin_addr);
    //int port = cli_addr->sin_port;
    link_entry entry;

    num_links = dl->num_links;
    for (i = 1; i < num_links; i++)
    {
        entry = (dl->links)[i];

        if (strcmp(entry.host, address) == 0)//&& entry.route_p == port)
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

// !!! this function used to return int size, and we passed in LSA *lsa as a param.
// not sure why it wasn't working, but the lsa was returning NULL. I have a hunch it was because of the whole lsa flexible structure
// but idk why that should matter.... anyway, passing in lsa, and mallocing here  wasn't working. so instead, I pass in size, and simply return
// the lsa. this works.
LSA * create_packet(int *size, int type, int node_id, int sequence_num, direct_links *dl, local_objects *ol)
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
    LSA *lsa;
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
            object_size_total += strlen(objects[i].name);
        }

        lsa = (LSA *)malloc(sizeof(LSA) + link_size_total + object_size_total);

        c_ptr = lsa->links_objects;
        i_ptr = (int *)c_ptr;

        for (i = 1; i < num_links; i++)
        {
            *i_ptr = links[i].id;
            i_ptr++;
        }

        for (i = 0; i < num_objects; i++)
        {
            name_len = strlen(objects[i].name);
            *i_ptr = name_len;
            i_ptr++;

            c_ptr = (char *)i_ptr;
            memcpy(c_ptr, objects[i].name, name_len);
            c_ptr += name_len;
            i_ptr = (int *)c_ptr;
        }
    }

    lsa->version_ttl_type = 0;
    lsa->version_ttl_type = SET_VERSION(lsa->version_ttl_type, 0x1);
    lsa->version_ttl_type = SET_TTL(lsa->version_ttl_type, DEFAULT_TTL);
    lsa->version_ttl_type = SET_TYPE(lsa->version_ttl_type, type);
    lsa->sender_node_id = node_id;
    lsa->sequence_num = sequence_num;
    lsa->num_links = num_links-1;
    lsa->num_objects = num_objects;

    *size = sizeof(LSA) + link_size_total + object_size_total;
    return lsa;
}

/*
 * Copies received bytes into packet.
 *
 */
LSA* bytes_to_packet(char *buf, int size)
{

    LSA *lsa = (LSA *)malloc(size);
    memcpy(lsa, buf, size);
    return lsa;
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

    link_entry *forwarder;

    ret = recvfrom(sockfd, buf, MAX_LSA_LEN, 0, (struct sockaddr *)&cli_addr, &clilen);

    if (ret < 5*sizeof(int))
    {

        //could not retrienve src_id from lsa. no way of knowing who is original sender.
        //no way of buffering lsa until it has been completely received.
        // ignore this LSA. hope retransmission works.
        return NULL;
    }

    lsa = bytes_to_packet( buf, ret);

    forwarder = lookup_link_entry(dl, &cli_addr);

    *forwarder_id = forwarder->id;
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


void send_ack(LSA *lsa){


}

/*
   handler for any incoming lsa advertisements from other routing daemons.
   receives the incoming lsa from a neighbor (forwarder_id)
   decrements TTL.
   forwards lsa to all its other neighbors.
   cd
   */
int lsa_handler(int sockfd, direct_links *dl, routing_table *rt, liso_hash *ht)
{
    int forwarder_id;
    int type;
    int lsa_size;
    LSA *lsa;
    int i;
    int tmp_type;
    routing_entry * entry;
	link_entry *src_link;
    lsa = rt_recvfrom(sockfd, &forwarder_id, dl, &lsa_size);

    type = GET_TYPE(lsa->version_ttl_type);
    entry = get_routing_entry(rt, lsa->sender_node_id);
  

    if(entry != NULL && entry->lsa!= NULL && ((type == TYPE_LSA && lsa->sequence_num <= entry->lsa->sequence_num) || (type == TYPE_ACK && lsa->sequence_num != entry->lsa->sequence_num))){
       // !!! not a new lsa . ignore!
       printf("ignoring lsa type %d seq num %d from node %d\n",GET_TYPE(lsa->version_ttl_type),lsa->sequence_num,lsa->sender_node_id);
      return -1;
    }

    if (type == TYPE_LSA) // || type == TYPE_DOWN)
    {
	

        tmp_type = lsa->version_ttl_type;
	lsa->version_ttl_type = SET_TYPE(lsa->version_ttl_type,TYPE_ACK);
        src_link = lookup_link_entry_node_id(dl,forwarder_id);
        
        rt_sendto(sockfd, lsa, src_link->host, src_link->route_p, lsa_size);
 	lsa->version_ttl_type = SET_TYPE(lsa->version_ttl_type, tmp_type);	 
        printf("received LSA %d from node %d\nsending ACK %d to node %d\n",lsa->sequence_num,lsa->sender_node_id,lsa->sequence_num,src_link->id);
        update_entry(entry, rt, dl, lsa, lsa_size, forwarder_id);
	//store_node_objects(ht,lsa);        
        //flood_received_lsa(sockfd, lsa, dl, rt, lsa_size, forwarder_id);
	// !!! commented out above line, we shouldn't flood anything here.... flooding is done in select-loop
    }
    else if (type == TYPE_ACK)
    {

       /* !!! took care of this above....
       if (entry->lsa->sequence_num != lsa->sequence_num)
        {        }

        */

        for (i = 0; i <  entry->checkers_list->num_links; i++)
        {
            if (entry->checkers_list->checkers[i].id == forwarder_id)
            {
        		printf("received ACK %d for node %d from node %d\n",lsa->sequence_num,lsa->sender_node_id,forwarder_id);	
                rt->table[0].checkers_list->checkers[i].ack_received = ACK_RECEIVED;
                rt->table[0].checkers_list->checkers[i].retransmit = 0;
		
            }
        }
    }

    return 1;
}

// !!! changed flood_lsa, to a more general flood() function. this way we can use this function to flood our own LSAs, as well as flooding
// when a neighbor is down tryied to merge this with flood_recevied_lsas, but it started getting ugly... too many params, stuff...
// so just left it for now.
int flood(int lsa_type,int sockfd, direct_links *dl, local_objects *ol, routing_table *rt, int node_id, int sequence_num)
{
    int i, num_links;
    int port;
    char *host;
    int size;
    ack_checkers *curr_checkers;
    routing_entry *entry;
    LSA *lsa;
    lsa = create_packet(&size, lsa_type, node_id, sequence_num, dl, ol);

    // might need to change this part

    entry = get_routing_entry(rt,node_id);
    update_entry(entry,rt, dl,lsa, size, node_id);

    curr_checkers = entry->checkers_list;

    num_links = dl->num_links;
    for (i = 1; i < num_links; i++)
    {
        if (get_routing_entry(rt, ((dl->links)[i]).id)->node_status != STATUS_DOWN)
        {

            if (lsa_type == TYPE_DOWN){
               printf("sending NODE %d DOWN to node %d\n",dl->links[i].id,node_id);
            }else{
               printf("sending my LSA %d to node %d\n",lsa->sequence_num,dl->links[i].id);
            }
            port = ((dl->links)[i]).route_p;
            host = ((dl->links)[i]).host;
            rt_sendto(sockfd, lsa, host, port, size);

            curr_checkers->checkers[i].ack_received = ACK_NOT_RECEIVED;
            //curr_checkers->checkers[i].ack_received = 0; //!!!! is this just a typo?
        }
    }

    //free(lsa);

    return 1;
}

void retransmit_missing(int sockfd, LSA *lsa, direct_links *dl, routing_table *rt, int lsa_size, int forwarder_id,liso_hash *ht)
{
    int i, num_links;
    int port;
    char *host;
    int node_id;
    ack_checkers *curr_checkers;
    routing_entry *entry;
    if (lsa == NULL)
    {
       return;
    }

    curr_checkers = get_routing_entry(rt, lsa->sender_node_id)->checkers_list;

    num_links = dl->num_links;
    for (i = 1; i < num_links; i++)
    {
        node_id = ((dl->links)[i]).id;

        if (node_id != forwarder_id)
        {
            if (curr_checkers->checkers[i].ack_received == ACK_NOT_RECEIVED)
            {
                entry = get_routing_entry(rt, node_id);
                if(curr_checkers->checkers[i].retransmit >= RETRANSMIT_TIME -1)
                {
		   if(entry->lsa != NULL){
			free(entry->lsa);
		   }
		   entry->lsa = NULL;
                   entry->node_status = STATUS_DOWN;
                   hash_remove_node(ht,entry->id);
                   printf("Node %d is down!!!\n",node_id);
                   curr_checkers->checkers[i].ack_received = ACK_RECEIVED;
		  

                }else if (entry->node_status != STATUS_DOWN)
                {
                    port = ((dl->links)[i]).route_p;
                    
	            host = ((dl->links)[i]).host;
                    rt_sendto(sockfd, lsa, host, port, lsa_size);
                    curr_checkers->checkers[i].retransmit++;
                    printf("retransmition #%d: %d's LSA %d to link %d\n",curr_checkers->checkers[i].retransmit,lsa->sender_node_id,lsa->sequence_num,node_id);
                }
            }
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
    int new_ttl;
    ack_checkers *curr_checkers;

    new_ttl = GET_TTL(lsa->version_ttl_type) - 1;
    lsa->version_ttl_type = SET_TTL(lsa->version_ttl_type, new_ttl);

    curr_checkers = get_routing_entry(rt, lsa->sender_node_id)->checkers_list;

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
		printf("forward lsa %d frm node %d to node %d\n",lsa->sequence_num,lsa->sender_node_id,dl->links[i].id);
                rt_sendto(sockfd, lsa, host, port, lsa_size);

                curr_checkers->checkers[i].ack_received = ACK_NOT_RECEIVED;
                curr_checkers->checkers[i].ack_received = 0;
            }
        }
    }

    //free(lsa);

    return 1;
}
