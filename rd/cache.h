/* cache.h
 *
 * <NAME: Hong Jai Cho>
 * <ANDREW: hongjaic>
 */

#ifndef CACHE_H
#define CACHE_H
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#define MAX_OBJECT_SIZE    500* ((1 << 10) * sizeof(char))
#define CACHE_SIZE          ((1 << 30) * sizeof(char))

/* The object struct with different fields 
 * host - used to check for match
 * path - used to check for match
 * obj - actual object information
 * size - size of obejct
 * prev - points to previous object
 * next -  points to next object
 */
typedef struct object 
{
    char *host;
    char *path;
    void *obj;
    unsigned size;
    struct object *prev;
    struct object *next;
} object;

void insert(object **mrup, object *newObj, object **lrup);
void evict(object **mrup, object **lrup);
object *search(object **mrup, char *host, char *path);
int exceed_max(unsigned size);
int cacheIsFull(unsigned size, unsigned cache);
void freeObj(object *eobj);
void moveToFront(object **mrup, object **lrup, object
        *toMove);

#endif
