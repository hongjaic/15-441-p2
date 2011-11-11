#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <time.h>

#include "rd.h"
#include "flooding_engine.h"
#include "hashset.h"

char my_uri[MAX_URI_LEN];
fd_set master_wfds;
fd_set master_rfds;
int flask_sock;
int rd_sock;
int local_obj_count;
int node_count;
direct_links neighbors;
connection  connections[MAX_CONNECTIONS];
liso_hash ht;
routing_table rt;
local_objects my_files;

int check_input(int argc, char *argv[]){

	if(argc != 3)
	{
		return FAIL;
	}
	
	return SUCCESS;
	
}

void usage(){
	printf("Usage: ./rd configuration-file object-file\n");
}





/* adds all of the object names in the files_fd file
to the object  hashtable. key = object name. value = localhost+object path
*/
int hash_add_my_objs(FILE *files_fd, int port)
{

	int count = 0;
	int i = 0;
	int j= 0;
	char car = '0';
  	char name[MAX_OBJ_LEN];
	int my_uri_len = strlen(my_uri);
	j = my_uri_len;
	int quit = 0;

	printf("default URI: %s\n", my_uri);	
	while(1)
	{
		while (car != ' ')
		{
			if((car = fgetc(files_fd)) == EOF)
			{
				name[i] = '\0';
				return count;
			}else
			{
				if(car !='\r' && car != '\n' && car!=' ')
				{
					name[i++] = car;		
				}
			}
		}

		while (car != '\r' && car!='\n')
		{
			if((car = fgetc(files_fd)) != EOF)
			{
				my_uri[j++] = car;		
			}else
			{
				quit = 1;
				break;
			}
			//printf("!%c!\n",uri[j-1]);		
		}
		
		my_uri[j-1] = '\0';
		printf("local object: %s at path:%s\n\n",name,my_uri);
		hash_add(&ht,name,neighbors.link[0].id,0);
		strcpy(my_files.objects[count].name , name);
		strcpy(my_files.objects[count].path , my_uri);		
		i = 0;
		j = my_uri_len;
		count++;
		
		if (quit)
		{
			my_files.num_objects = count;
			return count;
		}
	}
}


/* 
initializes the nodes array, which keeps track of all the nodes in the system
and their information. this initializes the nodes array to include the local node
as nodes[0], as well as all of its direct link nodes as 
nodes[1] ... nodes[j] (assuming local node has j direct_links) 
nodes[j+1] ... nodes[MAX_NODES] are initialized to NULL
*/

int init_neighbors(FILE *conf_fd)
{

	char buf[MAX_READ_LEN];
	char *ptr;
	int tmp;
	int num_neighbors = 0;
	while(num_neighbors < MAX_NODES && fgets(buf,MAX_READ_LEN,conf_fd) != NULL)
	{

		ptr = strtok(buf," ");
		neighbors.link[num_neighbors].id=atoi(ptr);
		
		ptr = strtok(NULL," ");
		strcpy(neighbors.link[num_neighbors].host,ptr);

		ptr = strtok(NULL," ");
		neighbors.link[num_neighbors].route_p= atoi(ptr);

		ptr = strtok(NULL," ");
		neighbors.link[num_neighbors].local_p= atoi(ptr);

		ptr = strtok(NULL," ");
		neighbors.link[num_neighbors].server_p = atoi(ptr);
		
		num_neighbors++;
		
	}
	tmp = num_neighbors;
	while(tmp < MAX_NODES)
	{
		neighbors.link[tmp].id = -1;
		tmp++;
	}
	neighbors.num_link = num_neighbors;
	return num_neighbors;
}


/*
Creates  a socket.
Binds it to the given port.
Listens fo action.
*/

void socket_setup(int *sock, int port,int type)
{

	struct sockaddr_in addr; 
	int yes = 1;

	if (type == TCP)
	{
		*sock = socket(PF_INET, SOCK_STREAM,0);
	}else
	{
		*sock = socket(PF_INET, SOCK_DGRAM,0);
	}
	if (*sock == -1)
	{ 
		exit(EXIT_FAILURE);
	}
	 
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);
	addr.sin_addr.s_addr = INADDR_ANY; //NOT ANY	
	// lose the "address already in use" error message
	setsockopt(*sock, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));

	if (bind(*sock, (struct sockaddr *) &addr, sizeof(addr))== -1 )
	{
		exit(EXIT_FAILURE);
	}

	if(type == TCP)
	{
		if (listen(*sock,5)== -1)
		{
			exit(EXIT_FAILURE);
		}
	}
	
}


/*
new connection handler is called whenever there is a flask request on the server TCP socket.
accepts the connection.
initializes the buf and resp variables for this newly created connections structure entry.
adds the port to the master_rfds so that our select loop can poll it.
*/

int new_client_handler(int sock,int *fd_high )
{

	int redir;
	struct sockaddr_storage client_addr; // client address
	socklen_t cli_size = sizeof(client_addr);
	
	redir = accept(sock,(struct sockaddr *) &client_addr, &cli_size);
	if (redir == FAIL)
	{
		return FAIL;	
	}

	connections[redir].resp[0]='\0';
	
	if (redir > *fd_high)
	{    // keep track of the max
		*fd_high = redir;
	}
	FD_SET(redir, &master_rfds);
	
	return SUCCESS;
}



int get_next_hop(int target,routing_table *rt)
{
	int i;
	for(i=0;i<rt->num_entry;i++)
	{
		if(rt->entry[i].id == target)
		{
			return rt->entry[i].nexthop;		
		}
	}

	return FAIL;
}

char * get_local_path(char *name, local_objects *my_files)
{

	int i;
	for(i = 0;i<my_files->num_objects;i++)
	{
		if(strcmp(my_files->objects[i].name,name) == 0)
		{
			return my_files->objects[i].path;
		}
	}

	return NULL;
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
add the connections socket to the master_wfds so that the select loop can do its thing on it

*/

int flask_request_handler(connection *curr_connection,int sock,char *objs_file){

	char * cmd;
	char * name;
	char *tmp_uri;
	char tmp_str[MAX_BUF_LEN];
	FILE * files_fd;
	int my_uri_len;	
	path * host_path_s;
	int ret;
	char buf[MAX_BUF_LEN];
	char *file_path;
	memset(buf,0,MAX_BUF_LEN);
	my_uri_len = strlen(my_uri);

	if((ret = recv(sock, buf, MAX_BUF_LEN, 0)) < 0)
	{
		return FAIL;
	}else if(ret == 0)
	{
		close(sock);
		FD_CLR(sock,&master_rfds);
		FD_CLR(sock,&master_wfds);
		return CLOSED ;   
	}

	cmd = strtok(buf," ");
	name = strtok(NULL," ");

	if(strcmp(cmd,"RDGET") == 0)
	{

		name[strlen(name)-2]='\0';
		host_path_s = get_paths(&ht,name);
			
		if(host_path_s == NULL)
		{
			strcpy(curr_connection->resp,"404 NF\r\n");
		}else
		{
			if(host_path_s->node_id == neighbors.link[0].id)
			{
				file_path = my_uri;
				strcat(file_path ,get_local_path(name,&my_files)); 
				
			}else
			{
				file_path = get_link_address(get_next_hop(host_path_s->node_id,&rt),&neighbors); 
				strcat(file_path,name);
			}
			sprintf(tmp_str,"OK %s\r\n",file_path);
			strcpy(curr_connection->resp,tmp_str);
		}	
	}else if(strcmp(cmd,"ADDFILE")==0)
	{ // add file to local node
		tmp_uri = strtok(NULL," ");
		tmp_uri[strlen(tmp_uri)-2]='\0';

		files_fd = fopen(objs_file,"a");
		fprintf(files_fd,"%s %s\n",name,tmp_uri);
		fclose(files_fd);
		
		strcat(my_uri,tmp_uri);							
		hash_add(&ht,name,neighbors.link[0].id,0);
		my_uri[my_uri_len] = '\0';
		local_obj_count++;
		strcpy(curr_connection->resp,"OK\r\n");
	} 

	FD_SET(sock,&master_wfds); 

			
	return SUCCESS;
	
}


int init_all(
		fd_set* curr_rfds,
		fd_set* curr_wfds,
		int* fd_high,
		char* conf_file,
		char* objs_file,
		routing_entry *my_entry
){

	//int i;
	FILE *conf_fd, *objs_fd;
    char hostname[MAX_URI_LEN];
	link_entry my_node = neighbors.link[0];
	gethostname(hostname, MAX_URI_LEN);
	
	conf_fd = fopen(conf_file,"r");
	
	//init localnode as well as all link nodes
	init_neighbors(conf_fd); 
	fclose(conf_fd);

	objs_fd = fopen(objs_file,"r");
	
	sprintf(my_uri,"http://%s:%d", hostname, my_node.server_p);

	 //add all local {object,path} pairs to system hashtable
	local_obj_count = hash_add_my_objs(objs_fd,my_node.server_p);
	fclose(objs_fd);

	
	 //set up flask request TCP listener socket
	socket_setup(&flask_sock,my_node.local_p,TCP);

	//set up routing daemon UDP listener socket
	socket_setup(&rd_sock,my_node.route_p,UDP);
	if(rd_sock > flask_sock)
	{
		*fd_high = rd_sock;
	}else
	{
		*fd_high = flask_sock;
	}

        // zero out all fd_sets
	FD_ZERO(curr_wfds);
	FD_ZERO(&master_wfds);
	FD_ZERO(curr_rfds);
	FD_ZERO(&master_rfds);
       
	// add both TCP and UDP listener sockets to the master read fd sets.
	FD_SET(flask_sock,&master_rfds);
	FD_SET(rd_sock,&master_rfds); 	
	return SUCCESS;
}





/*
called whenever there is any data to read.
Decides wether data should be handled as a
	LSA related message - from other routing Daemons
	or as a	
	request from an exisitng flask client	

*/

void recv_handler(routing_table *rt,direct_links *neighbors,int read_sock, int *fd_high, int *connection_count,char *objs_file){

	if(read_sock == rd_sock)
	{ // incoming LSA or LSA-ACK
		lsa_handler(neighbors,rt,rd_sock);
		return;
	}
	
	if(read_sock == flask_sock && *connection_count < MAX_CONNECTIONS )
	{ //new flask client
       	if (new_client_handler(read_sock,fd_high) == SUCCESS)
       	{
			(*connection_count)++;
		}
		return;
	}		
	 // new request from flask client
	if(flask_request_handler(&(connections[read_sock]),read_sock,objs_file)== CLOSED)
	{
		(*connection_count)--;
		return;
	}
	

	
}


/*

called whenever there is data a response to send to the flask
client connected to the other end of the input socket.
sends the data stored in the corresponding connection struct 
*/



void flask_response_handler(int write_sock, connection *flask_connection)
{

	send(write_sock,flask_connection->resp,strlen(flask_connection->resp),0);
	printf("I sent: %s!!!\n",flask_connection->resp);

	//remove i from the write fd_set
	FD_CLR(write_sock,&master_wfds);
	
	//reset this connections in and out bufs
	flask_connection->resp[0]='\0';


}

void node_dead(direct_links neighbor,int index)
{

}


int main(int argc, char* argv[])
{

	if(check_input(argc,argv) == FAIL)
	{
		usage();
		return 0;
	}


	int i;
	int fd_high;
	int ret;
	int connection_count = 0;
	int iterations = 4;

	routing_entry *my_entry = NULL;
    struct timeval tv;
    tv.tv_sec = 1;
    tv.tv_usec = 0;

	fd_set curr_rfds, curr_wfds;
	LSA * my_lsa = NULL;
	if(init_all(&curr_rfds,&curr_wfds,&fd_high,argv[1],argv[2],my_entry) == FAIL)
	{
		return FAIL;
	}

	
	while (1)
	{
	
		curr_wfds = master_wfds;
		curr_rfds = master_rfds;
		ret = select(fd_high+1,&curr_rfds, &curr_wfds,NULL,&tv); 

		if (ret == -1)
		{ //error selecting... close? skip? idk
	  		close(flask_sock);
			close(rd_sock);
			exit(0);	
			//continue;
		}else if (ret == 0)
		{
			iterations ++;
			if(iterations == 30)
			{
				create_packet(my_lsa, 0,my_entry->sequence_num++, &neighbors, &my_files);
				flood_lsa(&neighbors,my_lsa,rd_sock);
				iterations = 0;
			}
		}	
		
		if (iterations < 3)
		{
			for( i=1; i<neighbors.num_link-1;i++)
			{
				if(neighbors.link[i].ack_received == NO)
				{
					if(neighbors.link[i].retransmits == 2)
					{
						node_dead(neighbors,i);
					}else
					{
						rt_send(rd_sock,my_lsa,neighbors.link[i].id,neighbors.link[i].route_p,sizeof(my_lsa),&neighbors);
						neighbors.link[i].retransmits++;
					}
			
				}
			}
		}
		
		for(i=0;i<=fd_high;i++)
		{
			if(FD_ISSET(i,&curr_rfds))
			{
				recv_handler(&rt,&neighbors,i,&fd_high, &connection_count,argv[2]);
			}
			if(FD_ISSET(i,&curr_wfds))
			{
				flask_response_handler(i,&(connections[i]));
			}
		}
	}

	return SUCCESS;

}
