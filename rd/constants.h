/**
 * CS 15-441 Computer networks
 *
 * Collection of all constants.
 *
 * @file    constants.h
 * @author  Hong Jai Cho <hongjaic>, Raul Gonzalez <rggonzal>
 */

#ifndef CONSTANTS_H
#define CONSTANTS_H

#define BUF_SIZE            1024
#define FLOODING_PERIOD     10
#define TIMEOUT             3
#define RETRANSMIT_TIME     4
#define MAX_CONNECTIONS     1024

#define MAX_LINE            1024

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

#define HASHSIZE            9377
#define MAXOBJNUM           9377
#define MAXOBJNAME          64
#define MAXVALUELEN         256
#define MAXURILEN           2048

#define ACK_RECEIVED        1
#define ACK_NOT_RECEIVED    0

#define MAX_URI_LEN     1024

#endif
