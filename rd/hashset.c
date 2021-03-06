/**
 * CS 15-441 Computer Networks
 *
 * Functions and logic for hashing objects for lookup.
 *
 * @file    liso_hash.h
 * @author  Hong Jai Cho <hongjaic>, Raul Gonzalez <rggonzal>
 */

#include "hashset.h"


/* Helper functions */
void free_header_link(pair *ptr);
void free_path_link(path *ptr);
int pathcmp(path *ptr1, path *ptr2);
void remove_obj_from_objset(liso_hash *h, char *obj_name);
int link_contains_paths(pair *ptr, path *paths);
pair *link_contains_obj(pair *ptr, char *obj_name);
uint32_t super_fast_hash(const char *data, int len);
void string_tolower(char *str);


/*
 * init_hash - Initilizes hash table h.
 *
 * @param h Hash table to initialize
 */
void init_hash(liso_hash *h)
{
    int i;
    for (i = 0; i < HASHSIZE; i++)
    {
        (h->hash)[i] = NULL;
    }

    memset(h->obj_names, 0, MAXOBJNUM*MAXOBJNAME);
    h->num_objs = 0;
}


/*
 * contains_object - Determines if hash table h contains object obj_name.
 *
 * @param h         pointer to objects hash table
 * @param obj_name  name of object to lookup for
 * @return          object struct containing object information when successful,
 *                  otherwise NULL
 */
pair *contains_object(liso_hash *h, char *obj_name)
{
    int hash_index;
    pair *ptr;

    if (h->num_objs == 0)
        return NULL;

    hash_index = super_fast_hash(obj_name, (int)strlen(obj_name))%HASHSIZE;

    if ((ptr = (h->hash)[hash_index]) == NULL)
        return NULL;

    return link_contains_obj(ptr, obj_name);
}


/*
 * get_paths - Gets the available next hops for object obj_name.
 *
 * @param h         pointer to objects hash table
 * @param obj_name  name of object to lookup for
 * @return          list of next hops when successful, othersise NULL
 */
path *get_paths(liso_hash *h, char *obj_name)
{
    int hash_index;
    pair *curr;

    if ((h->num_objs) == 0)
        return NULL;

    hash_index = super_fast_hash(obj_name, (int)strlen(obj_name))%HASHSIZE;

    curr = (h->hash)[hash_index];

    while (curr != NULL)
    {
        if (strcmp(obj_name, curr->obj_name) == 0)
        {
            return curr->path_s;
        }
        
        curr = curr->next_pair_s;
    }

    return NULL;
}


/*
 * hash_add - Hashes object obj_name into hash table h.
 *
 * @param h         pointer to objects hash table
 * @param obj_name  name of object to hash
 * @param node_id   next hop for obj_name
 * @param cost      cost to node_id
 * @return          1
 */
int hash_add(liso_hash *h, char *obj_name, int node_id, int cost)
{
    int hash_index = -1;
    pair *dom = NULL;
    path *curr_nearest = NULL;

    hash_index = super_fast_hash(obj_name, (int)strlen(obj_name))%HASHSIZE;

    if ((dom = contains_object(h, obj_name)) != NULL)
    {
        curr_nearest = dom->path_s;
        
        if (curr_nearest == NULL)
        {
            path *newpath = malloc(sizeof(path));
            newpath->node_id = node_id;
            newpath->cost = cost;
            newpath->next_path_s = NULL;
            dom->path_s = newpath;
        }
        else if (curr_nearest->cost > cost)
        {
            path *newpath = malloc(sizeof(path));
            newpath->node_id = node_id;
            newpath->cost = cost;
            newpath->next_path_s = NULL;

            free(curr_nearest);
            dom->path_s = newpath;
        }

        return 1;
    }

    path *newpath = malloc(sizeof(path));
    newpath->node_id = node_id;
    newpath->cost = cost;
    newpath->next_path_s = NULL;

    pair *new = malloc(sizeof(pair));
    strcpy(new->obj_name, obj_name);
    new->path_s = newpath;
    new->next_pair_s = NULL;


    if ((dom = (h->hash)[hash_index]) == NULL)
    {
        (h->hash)[hash_index] = new;
    } 
    else 
    {
        new->next_pair_s = dom;
        (h->hash)[hash_index] = new;
    }

    strcpy((h->obj_names)[h->num_objs], obj_name);
    h->num_objs++;

    return 1;
}


/*
 * hash_remove_node - Removes next hop for node_id node for al objects.
 *
 * @param h         pointer to objects hash table
 * @param node_id   node id of node to remove
 * @return          1
 */
int hash_remove_node(liso_hash *h, int node_id)
{
    char *obj_name;
    int i, hash_index, num_objs = h->num_objs;
    pair *iter = NULL;

    if (num_objs == 0)
    {
        return 1;
    }

    for (i = 0; i < num_objs; i++)
    {
        obj_name = (h->obj_names)[i];
        hash_index = super_fast_hash(obj_name, (int)strlen(obj_name))%HASHSIZE;

        iter = (h->hash)[hash_index];

        while (iter != NULL)
        {
            if (strcmp(obj_name, iter->obj_name) == 0)
            { 
                if(iter->path_s != NULL && iter->path_s->node_id == node_id)
                {
                    free(iter->path_s);
                    iter->path_s = NULL;
                }
                break;
            }
            
            iter = iter->next_pair_s;
        }
    }

    return 1;
}

/*
 * hash_remove_object - Removes object obj_name from hash table.
 *
 * @param h         pointer to objects hash table
 * @param obj_name  objecct to remove
 * @return          1 if remove successful, 0 if no match
 */
int hash_remove_object(liso_hash *h, char *obj_name)
{
    pair *iter = NULL;
    pair *prev = NULL;
    int hash_index = super_fast_hash(obj_name, (int)strlen(obj_name))%HASHSIZE;

    if (h->num_objs == 0)
        return 0;

    if ((iter = (h->hash)[hash_index]) == NULL)
    {
        return 0;
    }

    while (iter != NULL)
    {
        if (strcmp(obj_name, iter->obj_name) == 0)
        {
            if (prev == NULL)
            {
                (h->hash)[hash_index] = iter->next_pair_s;
                free(iter->path_s);
                free(iter);
            }
            else
            {
                prev->next_pair_s = iter->next_pair_s;
                free(iter->path_s);
                free(iter);
            }

            h->num_objs--;
            remove_obj_from_objset(h, obj_name);

            return 1;
        }
        prev = iter;
        iter = iter->next_pair_s;
    }
        
    return 0;
}


/*
 * collapse - Frees memeory allocated to hash table h.
 *
 * @param h pointer to object hash table
 * @return  1 if successful, 0 if no objects are in hash table
 */
int collapse(liso_hash *h)
{
    if (h->num_objs == 0)
        return 0;

    int i;
    for (i = 0; i < HASHSIZE; i++)
    {
        free_header_link((h->hash)[i]);
    }

    init_hash(h);
    return 1;
}


/*
 * printPairs - Prints all object->paths relations.
 *
 * @param h pointer to objects hash table
 */
void printPairs(liso_hash *h)
{
    int i;
    path *ptr;

    if (h == NULL)
    {
        return;
    }

    if (h->num_objs == 0)
    {
        return;
    }

    for (i = 0; i < HASHSIZE; i++)
    {
        ptr = get_paths(h, (h->obj_names)[i]);
        
        if(ptr != NULL)
        {
            printf("%s -- \n", (h->obj_names)[i]);
        }

        while (ptr != NULL)
        {
            printf("node_id: %d, cost: %d\n", ptr->node_id, ptr->cost);
            ptr = ptr->next_path_s;
        }
    }
    printf("\n");
}


/*
 * free_header_link - Frees all memory allocated for a specific object
 *
 * @param ptr   pointer to object
 */
void free_header_link(pair *ptr)
{
    pair *prev = NULL;
    pair *curr = ptr;

    while (curr != NULL)
    {
        prev = curr;
        curr = curr->next_pair_s;
        free_path_link(prev->path_s);
        free(prev);
    }
}


/*
 * free_path_link - Frees all memory allocated to define the paths to a specific
 * objects.
 *
 * @param ptr   pointer to nearest path
 */
void free_path_link(path *ptr)
{
    path *prev = NULL;
    path *curr = ptr;

    while (curr != NULL)
    {
        prev = curr;
        curr = curr->next_path_s;
        free(prev);
    }
}


/*
 * link_contains_paths - Determines wheter a path exists for a specific object.
 *
 * @param ptr   pointer to object
 * @param paths pointer to path
 * @return      1 if paths exists for ptr, otherwise -1
 */
int link_contains_paths(pair *ptr, path *paths)
{
    pair *curr = ptr;

    while (curr  != NULL)
    {
        if (pathcmp(paths, curr->path_s) == 0)
        {
            return 1;
        }

        curr = curr->next_pair_s;
    }

    return 0;
}


/*
 * pathcmp - Compares two paths
 *
 * @param ptr1  pointer to path1
 * @param ptr2  pointer to path2
 * @return      1 if path1 and path2  are different, 0 if they are equal
 */
int pathcmp(path *ptr1, path *ptr2)
{
    path *curr1 = ptr1;
    path *curr2 = ptr2;

    while (curr1 != NULL && curr2 != NULL)
    {
        if (curr1->node_id != curr2->node_id)
        {
            return 1;
        }

        if (curr1->cost != curr2->cost)
        {
            return 1;
        }

        curr1 = curr1->next_path_s;
        curr2 = curr2->next_path_s;
    }

    if (curr1 == NULL && curr2 != NULL)
    {
        return 1;
    }

    if (curr1 != NULL && curr2 == NULL)
    {
        return 1;
    }

    return 0;
}


/*
 * link_contains_obje - Determines if list of objects contains object obj_name
 *
 * @param ptr       pointer to the header of link of objects
 * @param obj_name  name of object to lookup for
 * @return          pointer to obj_name if it is in the list, otherwise NULL
 */
pair *link_contains_obj(pair *ptr, char *obj_name)
{
    pair *curr = ptr;

    while (curr != NULL)
    {
        if (strcmp(obj_name, curr->obj_name) == 0)
        {
            return curr;
        }
        curr = curr->next_pair_s;
    }

    return NULL;
}


/*
 * remove_obj_from_objest - Removes object from objects keyset.
 *
 * @param h         pointer to objects hash table
 * @param obj_name  name of object to remove
 */
void remove_obj_from_objset(liso_hash *h, char *obj_name)
{
    int i;
    for (i = 0; i < HASHSIZE; i++)
    {
        if (strcmp(obj_name, (h->obj_names)[i]) == 0)
        {
            memset((h->obj_names)[i], 0, MAXOBJNAME);
        }
    }
}


/*
 * super_fast_hash - Paul Hsieh's super fast hash function.
 *                   www.azillionmonkeys.com/qed/hash.html
 *
 * @param data  data to be hashed
 * @param len   length of data
 * @return      hash key value
 */
uint32_t super_fast_hash(const char * data, int len)
{
    uint32_t hash = len, tmp;
    int rem;

    if (len <= 0 || data == NULL) return 0;

    rem = len & 3;
    len >>= 2;
    
    for (; len > 0; len--)
    {
        hash += get16bits(data);
        tmp = (get16bits(data+2) << 11) ^ hash;
        hash = (hash << 16) ^ tmp;
        data += 2*sizeof(uint16_t);
        hash += hash >> 11;
    }

    switch (rem)
    {
        case 3: hash += get16bits(data);
                hash ^= hash << 16;
                hash ^= data[sizeof(uint16_t)] << 18;
                hash += hash >> 11;
                break;
        case 2: hash += get16bits(data);
                hash ^= hash << 11;
                hash += hash << 17;
                break;
        case 1: hash += *data;
                hash ^= hash << 10;
                hash += hash >> 11;
    }

    hash ^= hash << 3;
    hash += hash >> 5;
    hash ^= hash << 4;
    hash += hash >> 17;
    hash ^= hash << 25;
    hash += hash >> 16;

    return hash;
}


/*
 * string_tolower - Converts string to all lower cases.
 *
 * @param str   string to convert
 */
void string_tolower(char *str)
{
    int i;
    int len;

    if (str == NULL)
        return;

    len = strlen(str);

    for (i = 0; i < len; i++)
    {
        *(str + i) = tolower(*(str + i));
    }
}
