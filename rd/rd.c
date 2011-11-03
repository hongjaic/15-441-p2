#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>


#include <errno.h>
#include <stdio.h>
#include <unistd.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <arpa/inet.h>


#include "rd.h"


char my_uri[MAX_URI_LEN];
fd_set master_wfds;
fd_set master_rfds;




int hash_add_my_objs(liso_hash *ht,FILE *files_fd, int port){


	int i = 0;
	int j= 0;
	char car = '0';
   
	char name[MAX_OBJ_LEN];

	int go = 1;

  
	

	int my_uri_len = strlen(my_uri);
	j = my_uri_len;

	printf("default URI: %s\n", my_uri);	

	while(go){
		

		while (car != ' '){

			if((car = fgetc(files_fd)) == EOF){
				go = 0;
				break;
			}else{
				if(car !='\r' && car != '\n' && car!=' '){
					name[i++] = car;		
				}
			}
		}

		name[i] = '\0';

		if(go == 0){
			break;
		}

		while (car != '\r' && car!='\n'){
			if((car = fgetc(files_fd)) != EOF){
				my_uri[j++] = car;		
			}else{
				go = 0;
				break;
			}

			//printf("!%c!\n",uri[j-1]);		
		}
		my_uri[j-1] = '\0';

		printf("local object: %s at path:%s\n\n",name,my_uri);
		hash_add(ht,name,my_uri,0);		
		i = 0;
		j = my_uri_len;
		

	}

	return SUCCESS;
}

void init_nodes(node nodes[], FILE *conf_fd,int *count){

	char buf[MAX_READ_LEN];
	char *ptr;
	
	
	while(*count < MAX_NODES && fgets(buf,MAX_READ_LEN,conf_fd) != NULL){

		ptr = strtok(buf," ");
		nodes[*count].id=atoi(ptr);
		
		ptr = strtok(NULL," ");
		strcpy(nodes[*count].host,ptr);

		ptr = strtok(NULL," ");
		nodes[*count].route_p= atoi(ptr);

		ptr = strtok(NULL," ");
		nodes[*count].local_p= atoi(ptr);

		ptr = strtok(NULL," ");
		nodes[*count].server_p = atoi(ptr);
		(*count)++;

	
		
	}

	while(*count < MAX_NODES){

		nodes[*count].id=-1;
		nodes[*count].route_p= -1;
		nodes[*count].local_p= -1;
		nodes[*count].server_p = -1;
		(*count)++;
		
	}


}




void socket_setup(int *sock, int port,int domain,int type){

	struct sockaddr_in addr; 
	int yes = 1;

	/************ SERVER SOCKET SETUP ************/
	if ((*sock = socket(domain, type, 0)) == -1){ //PF_LOCAL
		exit(EXIT_FAILURE);
	}
	 
	if(domain == PF_INET){
		addr.sin_family = AF_INET;
	}else{
		addr.sin_family = AF_LOCAL;
	}		
	addr.sin_port = htons(port);
	addr.sin_addr.s_addr = INADDR_ANY; //NOT ANY	
	// lose the "address already in use" error message
	setsockopt(*sock, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));

	if (bind(*sock, (struct sockaddr *) &addr, sizeof(addr))== -1 ){
		exit(EXIT_FAILURE);
	}

	if (listen(*sock,5)== -1){
		exit(EXIT_FAILURE);
	}
	
}



int new_connection(connection *connections[],int sock,int *fd_high ){

	int redir;
	struct sockaddr_storage client_addr; // client address
	socklen_t cli_size = sizeof(client_addr);
	
	redir = accept(sock,(struct sockaddr *) &client_addr, &cli_size);
	if (redir == FAIL){
		return FAIL;	
	}
	if(connections[redir] == NULL){
		connections[redir] = (connection *)malloc(sizeof(connection));
	}
	


	connections[redir]->buf[0]='\0';
	connections[redir]->resp[0]='\0';
							

	if (redir > *fd_high){    // keep track of the max
		*fd_high = redir;
	}
	FD_SET(redir, &master_rfds);
	
	return SUCCESS;

}

/*
char *build_ospf_str(){

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


void ospf_update_in(hasht *ht, int *rt[MAX_NODES],int updater_id,char *updater_host, char *updater_neighs,char *updater_objects){
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
			hash_add(ht,obj,updater_host,rt[updater_id][2]);
		}		
		obj = strok(NULL," ");
	}
			
	
}


*/

void update_rt(int *rt[MAX_NODES], int updater_id, int neighbor_id){
	int curr_cost = rt[neighbor_id][2]; //current cost to get to neighbor_id
	int updater_cost = rt[updater_id][2]; // current cost to get to updater_id
	int new_cost = updater_cost+1; // cost to get to neighbor_id via updater_id
	if(new_cost < curr_cost){
		rt[neighbor_id][1]=updater_id;
		rt[neighbor_id][2]=new_cost;
	
	}
}








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


int recv_data(liso_hash *ht,connection *curr_connection,int sock,char *my_objs_file){

	char * cmd;
	char * name;
	char *tmp_uri;
	char tmp_str[MAX_BUF_LEN];
	FILE * files_fd;

	int my_uri_len;
	
	path * host_path_s;
	int ret;
	char buf[MAX_BUF_LEN];
	memset(buf,0,MAX_BUF_LEN);
	my_uri_len = strlen(my_uri);


	ret = recv(sock, buf, MAX_BUF_LEN, 0);
	if( ret < 0){
		return FAIL;
	}
	
	if( ret == 0){
		close(sock);
		FD_CLR(sock,&master_rfds);
		FD_CLR(sock,&master_wfds);
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
		host_path_s = get_paths(ht,name);
			
		if(host_path_s == NULL){
				strcpy(curr_connection->resp,"404 NF\r\n");
		}else{
			sprintf(tmp_str,"OK %s\r\n",host_path_s->uri);
			strcpy(curr_connection->resp,tmp_str);
		}	
	}else if(strcmp(cmd,"ADDFILE")==0){ // add file to local node
		tmp_uri = strtok(NULL," ");
		tmp_uri[strlen(tmp_uri)-2]='\0';

		files_fd = fopen(my_objs_file,"a");
		fprintf(files_fd,"%s %s\n",name,tmp_uri);
		fclose(files_fd);
		
		strcat(my_uri,tmp_uri);							
		hash_add(ht,name,my_uri,0);
		my_uri[my_uri_len] = '\0';
		
		strcpy(curr_connection->resp,"OK\r\n");
	} 

	FD_SET(sock,&master_wfds); 

			
	return SUCCESS;
	
}


void init_connections(connection *connections[]){
	int i;
	for(i = 0; i< MAX_CONNECTIONS; i++){
		connections[i] = NULL;
	}
}



int main(int argc, char* argv[]){


	if(check_input(argc,argv) == FAIL){
		usage();
		return 0;
	}

	FILE *conf_fd;
	FILE * files_fd;
	node nodes[MAX_NODES];
	node * my_node;
	//int node_count = 0;
	int flask_sock;
	//int ospf_sock;
	int i;
	int fd_high;
	int run = 1;
	int ret;
	int num_connections = 0;	
	//int rt[150][3];	
	int node_count = 0;
    	char hostname[MAX_URI_LEN];
	connection *connections[MAX_CONNECTIONS];
	
	init_connections(connections);
	gethostname(hostname, MAX_URI_LEN);
	

	fd_set curr_rfds;
	fd_set curr_wfds;
	liso_hash *ht = (liso_hash *)malloc(sizeof(liso_hash));	
	
	
	conf_fd = fopen(argv[1],"r");
	init_nodes(nodes,conf_fd,&node_count);
	fclose(conf_fd);

	
	files_fd = fopen(argv[2],"r");
	my_node = &nodes[0];
	
	sprintf(my_uri,"http://%s:%d", hostname, my_node->server_p);
	hash_add_my_objs(ht,files_fd,my_node->server_p);
	fclose(files_fd);


		
	/*
	for(i=0;i<MAX_NODES;i++){
		rt[i][0]=i;
		rt[i][1]=-1;
		rt[i][2]=-1;
	}	

	rt[nodes->id][1]=nodes->id;
	rt[nodes->id][2] = 0;
	*/
	socket_setup(&flask_sock,my_node->local_p,PF_INET,SOCK_STREAM);
	/*
	socket_setup(&ospf_sock,nodes->route_p,PF_INET,SOCK_DGRAM);
	if(ospf_sock > flask_sock){
		fd_high = ospf_sock+1;
	}else{
	*/
	fd_high = flask_sock;
	//}

		FD_ZERO(&curr_wfds);
	FD_ZERO(&master_wfds);
	FD_ZERO(&curr_rfds);
	FD_ZERO(&master_rfds);

	FD_SET(flask_sock,&master_rfds); 	//add sock to master file descriptor list
	//FD_SET(ospf_sock,&master_rfds); 	//add sock to master file descriptor list

	printf("wating for flask connection\n");

	while (run){
		curr_wfds = master_wfds;
		curr_rfds = master_rfds;
		ret = select(fd_high+1,&curr_rfds, &curr_wfds,NULL,NULL);

		if (ret  <0){
			close(flask_sock);
			return FAIL;
		}else if(ret == 0){
			continue;
		}else{
			for(i=0;i<=fd_high;i++){
				if(FD_ISSET(i,&curr_rfds)){	
					if(i == flask_sock && num_connections < MAX_CONNECTIONS ){ //new 
        					new_connection(connections,i,&fd_high);
						if(ret == SUCCESS){
							num_connections++;
						}
					}else{
						ret = recv_data(ht,connections[i],i,argv[2]);
						if(ret == CLOSED){
							num_connections--;
						}
					}
				}
					
				
				if(FD_ISSET(i,&curr_wfds)){
					send(i,	connections[i]->resp,strlen(connections[i]->resp),0);
					printf("I sent: %s!!!\n",connections[i]->resp);
					FD_CLR(i,&master_wfds);
					connections[i]->buf[0]='\0';
					connections[i]->resp[0]='\0';
				}

			}
		}
	}

return SUCCESS;

}
