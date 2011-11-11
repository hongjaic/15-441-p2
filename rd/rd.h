#ifndef RD_H_
#define RD_H_



#define CLOSED 0


#define MAX_OBJ_LEN 64
#define MAX_URI_LEN 2048
#define MAX_HOST_LEN 100
#define MAX_READ_LEN 666
#define MAX_BUF_LEN MAX_OBJ_LEN+MAX_URI_LEN+9

#define MAX_RESP_LEN MAX_URI_LEN+3
#define MAX_LSA_LEN 2048
#define MAX_NODES 150
#define OSPF_INTERVAL 6000
#define flask_port 8888
#define MAX_CONNECTIONS 1000
#define TCP 1
#define UDP 0
#define YES 1
#define NO 0

#define MAX_OBJS 15000




typedef struct _object_entry{
	char name[MAX_OBJ_LEN];
	char path[MAX_URI_LEN];
}object_entry;

typedef struct _local_objects{
	int num_objects;
	object_entry objects[];

}local_objects;


typedef struct _connection{

	char *buf[MAX_BUF_LEN];
	char resp[MAX_RESP_LEN];

}connection;



#endif /*RD_H_*/





