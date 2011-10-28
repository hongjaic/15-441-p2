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



void hash_add(hasht *ht,char *name, char *uri, int cost){
	
}

path *paths_get(char * name){

 	return NULL;
}

int hash_add_my_objs(hasht *ht,FILE *files_fd, int port){


	int i = 0;
	int j= 0;
	char car = '0';
	char name[MAX_OBJ_LEN];
	char uri[MAX_URI_LEN];
	int go = 1;
	int uri_len = strlen(uri);
	sprintf(uri,"http://127.0.0.1:%d",port);
	j = uri_len;
	

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
				uri[j++] = car;		
			}else{
				go = 0;
				break;
			}

			//printf("!%c!\n",uri[j-1]);		
		}
		uri[j-1] = '\0';

		hash_add(ht,name,uri,0);		
		i = 0;
		j = uri_len;


	}

	return SUCCESS;
}


void add_node( node **head,int id, char *host,int route_p, int local_p, int server_p){
	node **new_node = head;
	while(*new_node != NULL){
		*new_node = (*new_node)->next_node;
	}

	(*new_node) = (node *)malloc(sizeof(node));
	(*new_node)->id = id;
	strcpy((*new_node)->host,host);
	(*new_node)->route_p=route_p;
	(*new_node)->local_p = local_p;
	(*new_node)->server_p = server_p;
	(*new_node)->next_node = NULL;

}	

void init_nodes(node **head_node, FILE *conf_fd){


	int id;
	char host[MAX_HOST_LEN];
	int route_p;
	int local_p;
	int server_p;
	char buf[MAX_READ_LEN];
	char *ptr;


	while(fgets(buf,MAX_READ_LEN,conf_fd) != NULL){

		ptr = strtok(buf," ");
		id = atoi(ptr);

		ptr = strtok(NULL," ");
		strcpy(host,ptr);

		ptr = strtok(NULL," ");
		route_p = atoi(ptr);

		ptr = strtok(NULL," ");
		local_p= atoi(ptr);

		ptr = strtok(NULL," ");
		server_p = atoi(ptr);


		add_node(head_node,id,host,route_p,local_p,server_p);
	
		
	}


}




int socket_setup(int *sock, int port,int domain,int type){

	struct sockaddr_in addr; 
	int res = FAIL;
	int yes = 1;

	/************ SERVER SOCKET SETUP ************/
	if ((*sock = socket(domain, type, 0)) != -1){ //PF_LOCAL
	 
		if(domain == PF_INET){
			addr.sin_family = AF_INET;

		}else{
			addr.sin_family = AF_LOCAL;
		}		
		addr.sin_port = htons(port);
		addr.sin_addr.s_addr = INADDR_ANY; //NOT ANY	
		// lose the "address already in use" error message
		setsockopt(*sock, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));

  		if (!bind(*sock, (struct sockaddr *) &addr, sizeof(addr))){
			if (!listen(*sock,5)){
				res = SUCCESS;
			}
		}
	}
	
	 return res;
}



void new_connection(int sock,int *fd_high, fd_set *master_fd_list){

	int flask_sock;
	struct sockaddr_storage client_addr; // client address
	socklen_t cli_size = sizeof(client_addr);
	
	flask_sock = accept(sock,(struct sockaddr *) &client_addr, &cli_size);
			
	if (flask_sock == FAIL){
		close(flask_sock);						
	}else{
		
		if (flask_sock > *fd_high){    // keep track of the max
			*fd_high = flask_sock;
		}

		FD_SET(flask_sock, master_fd_list);
							
	}
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



int main(int argc, char* argv[]){

	FILE *conf_fd;
	FILE * files_fd;

	node *nodes;

	//int node_count = 0;
	int flask_sock;
	//int ospf_sock;
	int i;
	
	int fd_high;
	int run = 1;
	int ret;
	char * cmd;
	char * name;
	char my_uri[MAX_URI_LEN];
	char *tmp;
	int my_uri_len;	
	char buf[MAX_BUF_LEN];
	int rt[150][3];	

	
	/*	
	int updater_id;
	char *updater_neighs;
	
	int ack_timeout[MAX_NODES][2];
	for (i = 0; i<MAX_NODES; i++){
		ack_timeout[i][0]= NOT_YET;
		ack_timeout[i][1]=-1;
	}
	*/

	fd_set read_fd_list;
	fd_set master_fd_list;
	fd_set write_fd_list;
	path * host_path_s;
	hasht *ht = (hasht *)malloc(sizeof(hasht));	
	
	if(check_input(argc,argv) == FAIL){
		usage();
		return 0;
	}

	
	
	conf_fd = fopen(argv[1],"r");
	
	init_nodes(&nodes,conf_fd);

	fclose(conf_fd);
	
	files_fd = fopen(argv[2],"r");

	hash_add_my_objs(ht,files_fd,nodes->local_p);

	fclose(files_fd);
		
	for(i=0;i<MAX_NODES;i++){
		rt[i][0]=i;
		rt[i][1]=-1;
		rt[i][2]=-1;
	}	

	rt[nodes->id][1]=nodes->id;
	rt[nodes->id][2] = 0;

	socket_setup(&flask_sock,nodes->local_p,PF_LOCAL,SOCK_STREAM);
	/*
	socket_setup(&ospf_sock,nodes->route_p,PF_INET,SOCK_DGRAM);
	if(ospf_sock > flask_sock){
		fd_high = ospf_sock+1;
	}else{
	*/
		fd_high = flask_sock+1;
	//}

	sprintf(my_uri,"http://127.0.0.1:%d",nodes->local_p);
	my_uri_len = strlen(my_uri);

	FD_ZERO(&write_fd_list);
	FD_ZERO(&master_fd_list);
	FD_ZERO(&read_fd_list);
	FD_SET(flask_sock,&master_fd_list); 	//add sock to master file descriptor list
	//FD_SET(ospf_sock,&master_fd_list); 	//add sock to master file descriptor list

	printf("wating for flask connection\n");

	while (run){

		/*
		if(update_neighbors() == 1){
			str = build_ospf_str();
			
			neighbor=strtok(my_neighs,"*");

			while(neighbor != NULL){
				out_sock = create_socket(UDP,atoi(neighbor));
				if(out_sock > fd_high){
					fd_high = out_sock;
				}
				FD_SET(out_sock,&write_fd_list);
				ack_timeout[atoi(neighbor)][0] = YES;
				ack_timeout[atoi(neighbor)][1] = out_sock;
				
				neighbor = strtok(NULL,"*");

			}


		}

		*/

		read_fd_list = master_fd_list;
		if (select(fd_high+1, &read_fd_list, &write_fd_list, NULL, NULL) == -1){
			close(flask_sock);
			return FAIL;
		}
		
		for(i=0;i<=fd_high;i++){
			if(FD_ISSET(i,&read_fd_list)){		
				if(i == flask_sock /* ||i == ospf_sock */){ //new connection
					new_connection(i,&fd_high, &master_fd_list);
			
				}else{
					memset(buf,0,MAX_BUF_LEN);
					ret = recv(i, buf, MAX_BUF_LEN, 0);
					if( ret > 0){
						
						
						cmd = strtok(buf," ");
						if(strcmp(cmd,"RDGET") == 0){
							name = strtok(NULL," ");
							host_path_s = paths_get(name);
							if(host_path_s == NULL){
								send(i,"NF",2, 0);
							}else{
								sprintf(tmp,"OK %s",host_path_s->uri);
								send(i,tmp,strlen(tmp), 0);
							}
						}else if(strcmp(cmd,"ADDFILE")==0){ // add file to local node
							name = strtok(NULL," ");
							tmp = strtok(NULL," ");
							files_fd = fopen(argv[2],"a");
							fprintf(files_fd,"%s %s\n",name,tmp);
							fclose(files_fd);

							strcat(my_uri,tmp);							
							hash_add(ht,name,my_uri,0);
							my_uri[my_uri_len] = '\0';
							send(i,"OK",2, 0);
						}/* 
						else{ // OSPF message
							updater_id= atoi(cmd);
							updater_host = strtoke(NULL," ");							
							if(strcmp(updater_host,"ACK") != 0){ //new ospf Update

								updater_neighs = strtok(NULL," ");
								updater_objects = strtoke(NULL," ");
								ospf_update_in(rt,updater_id,updater_host_updater_neighs,updater_objects);
							}else{ //ospf ack from neighbor
								FD_CLR(i,&write_fd_list);	

								for(j = 0; j< MAX_NODES;j++){
									if(ack_timeout[j][0]==i){
										break;
									}
								}	
								ack_timeout[j][0]= NOT_YET;
								ack_timeout[j][1] = -1;												

							}

						}
						*/
					}
					
					close(i);
					FD_CLR(i,&master_fd_list);					
				}
			} /*
			else if(FD_ISSET(i,&write_fd_list)){

				
				for(j = 0; j< MAX_NODES;j++){
					if(ack_timeout[j][0]==i){
						try_send = ack_timeout[j][0];
						break;
					}
				}
			
				if(reached_timeout(timer){
					try_send = YES;
				}


				if(try_send == YES){		
					sendto(i,str);
					ack_timeout[i][0]= NOT_YET;
					timer = 0;
					start_timer(timer);
			
				}
				
			} 
			*/
		}
	}

    return 1;
	
}
