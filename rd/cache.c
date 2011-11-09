/*
 * cache.c
 *
 * <NAME: Hong Jai Cho>
 * <ANDREW: hongjaic>
 */

#include "cache.h"

/*
 * insert - Insert new object in the front and set it as most recently used
 *
 * @param mrup      Most recently used object
 * @param newObj    new object to be iserted
 * @param lrup      Least recently used object
 */
void insert(object **mrup, object *newObj, object **lrup) 
{  
    /* Case: cache is empty - Just set the new object as most recently used
     * object */
    if (*mrup == NULL) 
    {
        newObj->prev = NULL;
        newObj->next = NULL;
        *lrup= newObj;
        *mrup = newObj;
        return;
    }

    /* Case: Cache is not empty - Insert new object in the front and set it as
     * most recently used object */
    newObj->prev = NULL;
    newObj->next = *mrup;
    newObj->next->prev = newObj;
    *mrup = newObj;
}

/*
 * evict - Deletes the least recently used object from the end of the list
 *
 * @param mru   Most recently used object
 * @param lru   Least recently used object
 */
void evict(object **mrup, object **lrup) 
{
    object *curr;
    

    /* Return if cache is empty */
    if(*lrup == NULL)
        return;
    
    /* Only on object in cache - Just empty the cache */
    if(*mrup == *lrup) {
        freeObj(*lrup);
        *mrup = NULL;
        *lrup = NULL;
        return;
    }

    /* Unlink lru with the cache, update the least recently used object and free
     * the evicted object */
    curr = *lrup;
    curr->prev->next = NULL;
    *lrup = curr->prev;
    freeObj(curr); 
}

/*
 * search - Traverses the cache and returns pointer to object if cached, returns
 * NULL otherwise.
 *
 * @param mrup  Most recently use object
 * @param host  Host name
 * @param path  File name
 *
 * @return      pointer to object if cache hit, NULL otherwise
 */
object *search(object **mrup, char *host, char *path) 
{  
    object *curr;
    
    /* Return NULL if cache is empty */
    if(*mrup == NULL)
        return NULL;

    /* Traverse the list */
    curr = *mrup;
    while(curr->next!=NULL)
    {
        /* Return current object if hostname and filename match */
        if(!strcmp(curr->host, host) && !strcmp(curr->path, path)) 
            return curr; 
        curr = curr->next;
    } 

    /* Check for match and return object */
    if(!strcmp(curr->host, host) && !strcmp(curr->path, path))
          return curr;

    /* No match - return NULL */
    return NULL;
}

/*
 * moveToFront - Moves obejct to the front of the list set it as the most
 * recently used object.
 *
 * @param mrup      Most recently used object
 * @param lrup      Lease recently used object
 * @param toMove    Object to be moved to front (used object)
 */
void moveToFront(object **mrup, object **lrup, object
        *toMove) 
{
    /* Don't do anything if object to be moved is wrong */
    if(toMove == NULL)
        return;

    /* Don't do anything if object to be moved is most recently used object */
    if(toMove == *mrup)
        return;
    
    if(toMove == *lrup)
        *lrup = toMove->prev;
    
    /* Move to front */
    toMove->prev->next=toMove->next;
    if(toMove->next!=NULL)
    {
        toMove->next->prev=toMove->prev;
    } 
    toMove->prev=NULL;
    toMove->next = *mrup;
    (*mrup)->prev = toMove;
    *mrup=toMove;  
}

/*
 * exceed_max - Checks whether object size exceeds 100KB
 *
 * @param size  Size of object
 *
 * @return      Returns 1 if object size exeeds 100KB and 0 otherwise
 */
int exceed_max(unsigned size) 
{ 
    return (size > MAX_OBJECT_SIZE);
}

/*
 * cacheIsFull - Checks whether cache is full
 *
 * @param size  size of new boejct
 * @param cache size of cache used
 *
 * @return      1 if cache is full and 0 otherwise
 */
int cacheIsFull(unsigned size, unsigned cache) 
{  
    return (size+cache > CACHE_SIZE);
}

/*
 * freeObj - Frees an object
 *
 * @param eobj Object being freed
 */
void freeObj(object *eobj) 
{ 
    /* Free all malloced components */
    free(eobj->host);
    free(eobj->path);
    free(eobj->obj);
    free(eobj);
}

/*
 * cacheCollapse - Cleans out the cache
 *
 * @param 
 */
void cachCollapse(object *mrup)
{
    object *prev;
    object *curr = mrup;

    while(curr != NULL)
    {
        prev = curr;
        curr = curr->next;
        freeObj(prev);
    }
}
