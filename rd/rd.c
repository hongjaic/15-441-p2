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





int hash_add_my_objs(liso_hash *ht,FILE *files_fd, int port){


	int i = 0;
	int j= 0;
	char car = '0';
	char name[MAX_OBJ_LEN];
	char uri[MAX_URI_LEN];
	int go = 1;
	
	sprintf(uri,"http://127.0.0.1:%d",port);
	int uri_len = strlen(uri);
	j = uri_len;
	

	while(go){
		

		printf("%s\n",uri);			
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

	int redir_sock;
	struct sockaddr_storage client_addr; // client address
	socklen_t cli_size = sizeof(client_addr);
	
	redir_sock = accept(sock,(struct sockaddr *) &client_addr, &cli_size);
	if (redir_sock == FAIL){
		close(redir_sock);						
	}else{
		
		if (redir_sock > *fd_high){    // keep track of the max
			*fd_high = redir_sock;
		}

		FD_SET(redir_sock, master_fd_list);
							
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

	node nodes[MAX_NODES];
	node * my_node;
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
	char tmp_str[MAX_BUF_LEN];
	int my_uri_len;	
	char buf[MAX_BUF_LEN];
	//int rt[150][3];	
	int node_count = 0;
	
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
	liso_hash *ht = (liso_hash *)malloc(sizeof(liso_hash));	
	
	if(check_input(argc,argv) == FAIL){
		usage();
		return 0;
	}

	
	
	conf_fd = fopen(argv[1],"r");
	
	init_nodes(nodes,conf_fd,&node_count);

	fclose(conf_fd);

	
	files_fd = fopen(argv[2],"r");

	my_node = &nodes[0];
	hash_add_my_objs(ht,files_fd,my_node->local_p);

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

	sprintf(my_uri,"http://127.0.0.1:%d",my_node->local_p);
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
					printf("trying to connect\n");
                    			new_connection(i,&fd_high, &master_fd_list);
			
				}else{
					ret = recv(i, buf, MAX_BUF_LEN, 0);
					if( ret > 0){
						
						
						cmd = strtok(buf," ");
					
						printf("%s\n%s\n",buf,cmd);
						if(strcmp(cmd,"RDGET") == 0){
							name = strtok(NULL," ");
							host_path_s = get_paths(ht,name);
							
							if(host_path_s == NULL){
								send(i,"404",3, 0);
							}else{
						
								sprintf(tmp_str,"200 %s",host_path_s->uri);
								send(i,tmp_str,strlen(tmp_str), 0);
								printf("sending:\n\t %s\n",tmp_str);
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
							send(i,"200",3, 0);
							printf("sent OK ADD\n");
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
					} else {
					    close(i);
					    FD_CLR(i,&master_fd_list);
                   			 
					}
					
					memset(buf,0,MAX_BUF_LEN);
				}
			} 
			 
			if(FD_ISSET(i,&write_fd_list)){

						
			} 
			
		}
	}

    return 1;
	
}
