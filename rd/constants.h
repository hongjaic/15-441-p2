#ifndef CONSTANTS_H
#define CONSTANTS_H

// engine_wrapper.h
#define BUF_SIZE            1024
#define FLOODING_PERIOD     30
#define TIMEOUT             1
#define RETRANSMIT_TIME     2
#define MAX_CONNECTIONS     1024

// file_loader.h
#define MAX_LINE            1024

// flask_engine.h

// flooding_engine.h
#define TYPE_LSA            0x00000000
#define TYPE_ACK            0x00000001
#define TYPE_DOWN           0x00000010
#define DEFAULT_TTL         0x20

#define STATUS_UP           1
#define STATUS_DOWN         0

#define MAX_NODES           150
#define MAX_BUF             1024
#define MAX_OBJ_LEN         64
#define MAX_PATH_LEN        2048
#define MAX_OBJ_NUM         10000
#define MAX_LSA_LEN         1024

#define LSA_SENT            1
#define LSA_NOT_SENT        0

// hashset.h
#define HASHSIZE            9377
#define MAXOBJNUM           9377
#define MAXOBJNAME          64
#define MAXVALUELEN         256
#define MAXURILEN           2048

#define ACK_RECEIVED        1
#define ACK_NOT_RECEIVED    0

#define MAX_URI_LEN     1024

#endif
