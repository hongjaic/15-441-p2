#ifndef RD_H_
#define RD_H_

#include "hashset.h"

#define SUCCESS 1
#define FAIL -1
#define CLOSED 0


#define MAX_OBJ_LEN 64
#define MAX_URI_LEN 2048
#define MAX_HOST_LEN 100
#define MAX_BUF_LEN MAX_OBJ_LEN+MAX_URI_LEN+9
#define MAX_READ_LEN MAX_HOST_LEN+30
#define MAX_RESP_LEN MAX_URI_LEN+3
#define MAX_LSA_LEN 2048
#define MAX_NODES 150
#define OSPF_INTERVAL 6000
#define flask_port 8888
#define MAX_CONNECTIONS 1000
#define TCP 1
#define UDP 0
#define UP 1
#define DOWN 0
#define MAX_OBJS 15000

#define VERSION '1'
#define TYPE_LSA "00"
#define TYPE_LSA_ACK "01" 
#define DEF_TTL ' ' //aschii char 'space' is decimal 32 



struct packet {

	char version;
	char ttl;
	char type[2];
	int src_id;
	int seq_num;
	int link_count;
	int obj_count;
	int links[MAX_NODES];
	char objs[MAX_OBJS][MAX_OBJ_LEN];	
};

typedef struct _node{

	int id;
	char host[MAX_HOST_LEN];
	int local_p;
	int route_p;
	int server_p;
	int seq_num;
	struct packet lsa;
	char lsa_buf[MAX_LSA_LEN];	
	int state;
}node;


typedef struct _connection{

	char buf[MAX_BUF_LEN];
	char resp[MAX_RESP_LEN];

}connection;


node nodes[MAX_NODES];

#endif /*RD_H_*/





