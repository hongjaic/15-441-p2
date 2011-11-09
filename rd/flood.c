
/*
returns the node ID of in_addr. 
gets in_addrs IP address, and finds the matching IP address in the nodes array. 
returns that nodes entry's ID.
*/
int getID(struct sockaddr_storage *in_addr){
	int i;
    	char s[INET6_ADDRSTRLEN];
	for(i=1;i<MAX_NODES;i++){
                inet_ntop(in_addr->ss_family, &(((struct sockaddr_in *)in_addr)->sin_addr),s, sizeof s);
		if(strcmp(nodes[i].host,s) == 0){
			return nodes[i].id;
                }
	
	}
	return FAIL;
}



/*
returns a point to the node in the nodes array with an id equal to that of the input id
*/
node *getNode(int id){
	int i;
	for(i=1;i<MAX_NODES;i++){
		if(nodes[i].id == id){
			return &(nodes[i]);
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
int create_packet(node *node,char *lsa){

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
	node->lsa.link_count = (int)lsa;
	lsa+=4;
	node->lsa.obj_count =(int)lsa;
	lsa+=4;
	

	for(i=0;i<node->lsa.link_count;i++){
		node->lsa.links[i]=(int)lsa;	
		lsa+=4;
	}

	for(i=0;i<node->lsa.obj_count;i++){
		obj_len =(int)lsa;
		lsa+=4;
		strncpy(node->lsa.objs[i],lsa,obj_len);
		lsa+=obj_len;
	}

	return SUCCESS;

}


/*
wrapper for recv_from. 
binds received lsa info to the node that sent it.
returns pointer to that node.
*/

node *rd_recvfrom(int sock, int *forwarder_id){


    	struct sockaddr_storage in_addr;
	socklen_t in_len;

	char buf[MAX_LSA_LEN];
	node *src_node;


	char *lsa;
	int newlen;
	in_len = sizeof in_addr;
	recvfrom(sock,buf,MAX_LSA_LEN,0,(struct sockaddr *)&in_addr, &in_len);
	
	*forwarder_id= getID(&in_addr);	
	if(strlen(buf) < 8){

		//could not retrienve src_id from lsa. no way of knowing who is original sender. 
		//no way of buffering lsa until it has been completely received. 
		// ignore this LSA. hope retransmission works.
		return NULL;
	}


	src_node = getNode((int)&buf[4]);
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
int rd_send(node *to_node,char *str){
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
receives the incoming lsa from a links (forwarder_id) 
decrements TTL.
forwards lsa to all its other links.
cd 
*/
int lsa_handler(int rd_sock){



	int i;
	int forwarder_id;
	node *src_node;

	src_node = rd_recvfrom(rd_sock,&forwarder_id);

	if (src_node == NULL){
		return FAIL;
	}

	//forward lsa to all other links
	for (i=1; i<nodes[0].lsa.link_count;i++){ //skip nodes[0] = self.
		if(nodes[i].id != forwarder_id){	
			src_node->lsa_buf[0]--; //decremet TTL
			rd_send(&(nodes[i]),src_node->lsa_buf);
		}
	}
	printf("received lsa from %d\n",src_node->id);
	return SUCCESS;
}


char *create_lsa(char *lsa){
		
	char objs_entry[MAX_OBJS*MAX_OBJ_LEN];
	char *obj;
	char obj_len[4];
	int i;
	char link_id[4];
	char links_entry[MAX_OBJS];
	objs_entry[0]='\0';
	for(i = 0; i<nodes[0].lsa.obj_count;i++){
		obj = nodes[0].lsa.objs[i];
		sprintf(obj_len,"%d",strlen(obj));
		strcat(objs_entry,obj_len);
		strcat(objs_entry,obj);
	}		

	links_entry[0]='\0';
	for(i=0;i<nodes[0].lsa.link_count;i++){
		sprintf(link_id,"%d",nodes[0].lsa.links[i]);
		strcat(links_entry,link_id);
	}
 
	sprintf(lsa,"%c%c%s%d%d%d%d%s%s",
			nodes[0].lsa.version,
	 		nodes[0].lsa.ttl, 
	 		nodes[0].lsa.type,
	 		nodes[0].id,
	 		nodes[0].lsa.seq_num,
	 		nodes[0].lsa.link_count,
	 		nodes[0].lsa.obj_count,
	 		links_entry,
	 		objs_entry);

	nodes[0].lsa.seq_num++;

return lsa;
}

