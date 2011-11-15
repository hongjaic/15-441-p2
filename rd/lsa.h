/**
 * CS 15-441 Computer Networks
 *
 * Struct definition for an LSA packet.
 *
 * @file    lca.h
 * @author  Hong Jai Cho <hongjaic>, Raul Gonzalez <rggonzal>
 */

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


typedef struct store
{
   int num_lsa;
   LSA *lsa;
   LSA *next;
} lsa_store;
#pragma pack()

#endif
