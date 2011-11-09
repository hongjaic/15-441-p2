#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>


#include <errno.h>
#include <stdio.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <arpa/inet.h>
#include <netdb.h>

#include "rd.h"
#include "flood.c"

char g_my_uri[MAX_URI_LEN];
fd_set g_master_wfds;
fd_set g_master_rfds;
int g_flask_sock;
int g_rd_sock;
int g_local_obj_count;
int g_node_count;
node *g_my_node;

connection  connections[MAX_CONNECTIONS];
liso_hash ht;
int rt[MAX_NODES][2];


int check_input(int argc, char *argv[]){

	if(argc != 3){
		return FAIL;
	}else{
		return SUCCESS;
	}

}

void usage(){
	printf("Usage: ./rd configuration-file object-file\n");
}





/* adds all of the object names in the files_fd file
to the object  hashtable. key = object name. value = localhost+object path
*/
int hash_add_my_objs(FILE *files_fd, int port){

	int count = 0;
	int i = 0;
	int j= 0;
	char car = '0';
  	char name[MAX_OBJ_LEN];
	int g_my_uri_len = strlen(g_my_uri);
	j = g_my_uri_len;
	int quit = 0;

	printf("default URI: %s\n", g_my_uri);	
	while(1){
		while (car != ' '){
			if((car = fgetc(files_fd)) == EOF){
				name[i] = '\0';
				return count;
			}else{
				if(car !='\r' && car != '\n' && car!=' '){
					name[i++] = car;		
				}
			}
		}

		

		while (car != '\r' && car!='\n'){
			if((car = fgetc(files_fd)) != EOF){
				g_my_uri[j++] = car;		
			}else{
				quit = 1;
				break;
			}
			//printf("!%c!\n",uri[j-1]);		
		}
		
		g_my_uri[j-1] = '\0';
		printf("local object: %s at path:%s\n\n",name,g_my_uri);
		hash_add(&ht,name,g_my_uri,0);		
		i = 0;
		j = g_my_uri_len;
		count++;
		
		if (quit){
			return count;
		
		}
	}
}


/* 
initializes the nodes array, which keeps track of all the nodes in the system
and their information. this initializes the nodes array to include the local node
as nodes[0], as well as all of its direct link nodes as 
nodes[1] ... nodes[j] (assuming local node has j links) 
nodes[j+1] ... nodes[MAX_NODES] are initialized to NULL
*/

void init_nodes( FILE *conf_fd){

	char buf[MAX_READ_LEN];
	char *ptr;
	int tmp;
	while(g_node_count < MAX_NODES && fgets(buf,MAX_READ_LEN,conf_fd) != NULL){

		ptr = strtok(buf," ");
		nodes[g_node_count].id=atoi(ptr);
		
		ptr = strtok(NULL," ");
		strcpy(nodes[g_node_count].host,ptr);

		ptr = strtok(NULL," ");
		nodes[g_node_count].route_p= atoi(ptr);

		ptr = strtok(NULL," ");
		nodes[g_node_count].local_p= atoi(ptr);

		ptr = strtok(NULL," ");
		nodes[g_node_count].server_p = atoi(ptr);

		nodes[g_node_count].state = UP;
		
		g_node_count++;
		
	}
	tmp = g_node_count;
	while(tmp < MAX_NODES){
		nodes[tmp].id = -1;
		tmp++;
	}

}


/*
Creates  a socket.
Binds it to the given port.
Listens fo action.
*/

void socket_setup(int *sock, int port,int type){

	struct sockaddr_in addr; 
	int yes = 1;

	if (type == TCP){
		*sock = socket(PF_INET, SOCK_STREAM,0);
	}else{
		*sock = socket(PF_INET, SOCK_DGRAM,0);
	}
	if (*sock == -1){ 
		exit(EXIT_FAILURE);
	}
	 
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);
	addr.sin_addr.s_addr = INADDR_ANY; //NOT ANY	
	// lose the "address already in use" error message
	setsockopt(*sock, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));

	if (bind(*sock, (struct sockaddr *) &addr, sizeof(addr))== -1 ){
		exit(EXIT_FAILURE);
	}

	if(type == TCP){
		if (listen(*sock,5)== -1){
			exit(EXIT_FAILURE);
		}
	}
	
}


/*
new connection handler is called whenever there is a flask request on the server TCP socket.
accepts the connection.
initializes the buf and resp variables for this newly created connections structure entry.
adds the port to the g_master_rfds so that our select loop can poll it.
*/

int new_client_handler(int sock,int *fd_high ){

	int redir;
	struct sockaddr_storage client_addr; // client address
	socklen_t cli_size = sizeof(client_addr);
	
	redir = accept(sock,(struct sockaddr *) &client_addr, &cli_size);
	if (redir == FAIL){
		return FAIL;	
	}

	connections[redir].buf[0]='\0';
	connections[redir].resp[0]='\0';
	
	if (redir > *fd_high){    // keep track of the max
		*fd_high = redir;
	}
	FD_SET(redir, &g_master_rfds);
	
	return SUCCESS;
}




/*
called whenever there is a new flask request.
if request length = 0, we close the connection with flask client.
if request is an RDGET, 
	find the given object in the object hashtable,
	create response with 
		objects path if found,
		404 if not found.
if request is an ADDFILE, 
	we append the object name and path to the .files file. 
	add it to the object hashtable with key = object name, value = localhost + path
	create response with OK

add the response to the connection's response buffer.
add the connections socket to the g_master_wfds so that the select loop can do its thing on it

*/

int flask_request_handler(connection *curr_connection,int sock,char *objs_file){

	char * cmd;
	char * name;
	char *tmp_uri;
	char tmp_str[MAX_BUF_LEN];
	FILE * files_fd;

	int g_my_uri_len;
	
	path * host_path_s;
	int ret;
	char buf[MAX_BUF_LEN];
	memset(buf,0,MAX_BUF_LEN);
	g_my_uri_len = strlen(g_my_uri);


	if((ret = recv(sock, buf, MAX_BUF_LEN, 0)) < 0){
		return FAIL;
	}else if(ret == 0){
		close(sock);
		FD_CLR(sock,&g_master_rfds);
		FD_CLR(sock,&g_master_wfds);
		return CLOSED  ;   
	}


	strcat(curr_connection->buf,buf);
 	if(strstr(curr_connection->buf,"\r\n") == NULL){
		return SUCCESS;
	}

	cmd = strtok(buf," ");
	name = strtok(NULL," ");

	if(strcmp(cmd,"RDGET") == 0){

		name[strlen(name)-2]='\0';
		host_path_s = get_paths(&ht,name);
			
		if(host_path_s == NULL){
			strcpy(curr_connection->resp,"404 NF\r\n");
		}else{
			sprintf(tmp_str,"OK %s\r\n",host_path_s->uri);
			strcpy(curr_connection->resp,tmp_str);
		}	
	}else if(strcmp(cmd,"ADDFILE")==0){ // add file to local node
		tmp_uri = strtok(NULL," ");
		tmp_uri[strlen(tmp_uri)-2]='\0';

		files_fd = fopen(objs_file,"a");
		fprintf(files_fd,"%s %s\n",name,tmp_uri);
		fclose(files_fd);
		
		strcat(g_my_uri,tmp_uri);							
		hash_add(&ht,name,g_my_uri,0);
		g_my_uri[g_my_uri_len] = '\0';
		g_local_obj_count++;
		strcpy(curr_connection->resp,"OK\r\n");
	} 

	FD_SET(sock,&g_master_wfds); 

			
	return SUCCESS;
	
}


int init_all(fd_set* curr_rfds,fd_set* curr_wfds,int* fd_high,char* conf_file,char* objs_file){

	//int i;
	FILE *conf_fd, *objs_fd;
    	char hostname[MAX_URI_LEN];

	gethostname(hostname, MAX_URI_LEN);
	
	conf_fd = fopen(conf_file,"r");
	
	//init localnode as well as all link nodes
	init_nodes(conf_fd); 
	g_my_node = &(nodes[0]);

	nodes[0].lsa.link_count = g_node_count - 1;
	fclose(conf_fd);

	objs_fd = fopen(objs_file,"r");
	
	sprintf(g_my_uri,"http://%s:%d", hostname, g_my_node->server_p);

	 //add all local {object,path} pairs to system hashtable
	g_local_obj_count = hash_add_my_objs(objs_fd,g_my_node->server_p);
	fclose(objs_fd);

	/*
	for(i=0;i<MAX_NODES;i++){
		rt[i][1]=-1; //next hop to i 
		rt[i][2]=-1; //cost to i
	}	
	rt[nodes[0]->id][0]=nodes[0]->id;
	rt[nodes[0]->id][1] = 0;
	*/

	 //set up flask request TCP listener socket
	socket_setup(&g_flask_sock,g_my_node->local_p,TCP);

	//set up routing daemon UDP listener socket
	socket_setup(&g_rd_sock,g_my_node->route_p,UDP);
	if(g_rd_sock > g_flask_sock){
		*fd_high = g_rd_sock;
	}else{
		*fd_high = g_flask_sock;
	}

        // zero out all fd_sets
	FD_ZERO(curr_wfds);
	FD_ZERO(&g_master_wfds);
	FD_ZERO(curr_rfds);
	FD_ZERO(&g_master_rfds);
       
	// add both TCP and UDP listener sockets to the master read fd sets.
	FD_SET(g_flask_sock,&g_master_rfds);
	FD_SET(g_rd_sock,&g_master_rfds); 	

	return SUCCESS;
}



/*
called whenever there is any data to read.
Decides wether data should be handled as a
	LSA related message - from other routing Daemons
	or as a	
	request from an exisitng flask client	

*/

void recv_handler( int read_sock, int *fd_high, int *connection_count,char *objs_file){

	if(read_sock == g_rd_sock){ // incoming LSA or LSA-ACK
		lsa_handler(g_rd_sock);
	}else if(read_sock == g_flask_sock && *connection_count < MAX_CONNECTIONS ){ //new flask client
        	if (new_client_handler(read_sock,fd_high) == SUCCESS){
			(*connection_count)++;
		}
	}else{  // new request from flask client
		if(flask_request_handler(&(connections[read_sock]),read_sock,objs_file)== CLOSED){
			(*connection_count)--;
		}
	}
	
}


/*

called whenever there is data a response to send to the flask
client connected to the other end of the input socket.
sends the data stored in the corresponding connection struct 
*/



void flask_response_handler(int write_sock, connection *flask_connection){

	send(write_sock,flask_connection->resp,strlen(flask_connection->resp),0);
	printf("I sent: %s!!!\n",flask_connection->resp);

	//remove i from the write fd_set
	FD_CLR(write_sock,&g_master_wfds);
	
	//reset this connections in and out bufs
	flask_connection->buf[0]='\0'; 
	flask_connection->resp[0]='\0';


}


/*
char *build_rd_str(){

	char *str;
	char *my_id;
	char my_host[MAX_HOST_LEN];
	char *my_objs;
	char *my_neighs;
	char *neighbor;
	int sock;
	sprintf(str,"%d %s %s %s",my_id,my_host,my_objs,my_neighs);
	
	return str;
}
*/

/*
void rd_update_in(int updater_id,char *updater_host, char *updater_neighs,char *updater_objects){
	char *objects;
	char *obj;
	char * neighbor;
	int cost;
	neighbor=strtok(updater_neighs,"*");

	while(neighbor != NULL){
		update_rt(rt,updater_id,atoi(neighbor));
		neighbor = strtok(NULL,"*");
	}
	
	obj=strtok(updater_objects," ");
	while(obj != NULL){
		if(is_new_obj(ht,updater_id,obj) == 1){
			hash_add(&ht,obj,updater_host,rt[updater_id][2]);
		}		
		obj = strok(NULL," ");
	}			
}
*/

/*
void update_rt( int updater_id, int neighbor_id){
	int curr_cost = rt[neighbor_id][2]; //current cost to get to neighbor_id
	int updater_cost = rt[updater_id][2]; // current cost to get to updater_id
	int new_cost = updater_cost+1; // cost to get to neighbor_id via updater_id
	if(new_cost < curr_cost){
		rt[neighbor_id][1]=updater_id;
		rt[neighbor_id][2]=new_cost;	
	}
}
*/


int main(int argc, char* argv[]){

	if(check_input(argc,argv) == FAIL){
		usage();
		return 0;
	}


	int i;
	int fd_high;
	int ret;
	int connection_count = 0;

	fd_set curr_rfds, curr_wfds;
	if(init_all(&curr_rfds,&curr_wfds,&fd_high,argv[1],argv[2]) == FAIL){
		return FAIL;
	}

	printf("wating for flask connection\n");

	while (1){
		curr_wfds = g_master_wfds;
		curr_rfds = g_master_rfds;
		ret = select(fd_high+1,&curr_rfds, &curr_wfds,NULL,NULL); 

		if (ret == -1){ //error selecting... close? skip? idk
	  		close(g_flask_sock);
			close(g_rd_sock);
			exit(0);	
			//continue;
		}	

		for(i=0;i<=fd_high;i++){
			if(FD_ISSET(i,&curr_rfds)){	
				recv_handler(i,&fd_high, &connection_count,argv[2]);
			}
			if(FD_ISSET(i,&curr_wfds)){ 
				if ( i == g_rd_sock ){
					//rd_response_handler();		
				}else{
					flask_response_handler(i,&(connections[i]));
				}
			}
		}
	}

	return SUCCESS;

}
