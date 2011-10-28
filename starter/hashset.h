/**
 * CS 15-441 Computer Networks
 *
 * @file    liso_hash.h
 * @author  Hong Jai Cho <hongjaic@andrew.cmu.edu>
 */

#ifndef HASHSET_H
#define HASHSET_H
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <ctype.h>

#undef get16bits
#if (defined(__GNUC__) && defined (__i386__)) || defined(__WATCOMC__) \
    || defined(_MSC_VER) || defined (__BORLANDC__) || defined (__TURBOC__)
#define get16bits(d) (*((const uint16_t *) (d)))
#endif

#if !defined(get16bits)
#define get16bits(d) ((((uint32_t)(((const uint8_t *)(d))[1])) << 8) \
        +(uint32_t)(((const uint8_t *)(d))[0]))
#endif

#define HASHSIZE        9377
#define MAXOBJNUM       9377
#define MAXOBJNAME      64
#define MAXVALUELEN     256
#define MAXURILEN       2048

typedef struct path
{
    char uri[MAXURILEN];
    int cost;
    struct path *next_path_s; 
} path;

typedef struct pair
{
    char obj_name[MAXOBJNAME];
    path *path_s;
    struct pair *next_pair_s; 
} pair;

typedef struct liso_hash
{
    pair *hash[HASHSIZE];
    char obj_names[MAXOBJNUM][MAXOBJNAME];
    int num_objs;
} liso_hash;

liso_hash headers_hash;

void init_hash(liso_hash *h);
int contains_object(liso_hash *h, char *obj_name);
path *get_paths(liso_hash *h, char *obj_name);
int hash_add(liso_hash *h, char *obj_name, char *uri, int cost);
int hash_remove(liso_hash *h, char *obj_name);
int collapse(liso_hash *h);
void printPairs(liso_hash *h);

#endif
