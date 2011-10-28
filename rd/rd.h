#ifndef RD_H_
#define RD_H_

#include "hashset.h"

#define SUCCESS 1
#define FAIL -1

#define MAX_OBJ_LEN 64
#define MAX_URI_LEN 2048
#define MAX_HOST_LEN 100
#define MAX_BUF_LEN MAX_OBJ_LEN+MAX_URI_LEN+9
#define MAX_READ_LEN MAX_HOST_LEN+30
#define MAX_NODES 150
#define OSPF_INTERVAL 6000
#define flask_port 8888

typedef struct _node{

	int id;
	char host[MAX_HOST_LEN];
	int local_p;
	int route_p;
	int server_p;
	int sequence_num;
	struct _node * next_node;

}node;
#endif /*LISOD_H_*/





