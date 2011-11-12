#ifndef LSA_H
#define LSA_H

#pragma pack(1)
typedef struct LSA
{
    int version_ttl_type;
    int sender_node_id;
    int sequence_num;
    int num_links;
    int num_objects;
    char links_objects[];
} LSA;
#pragma pack()

#endif
