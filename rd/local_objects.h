#ifndef LOCAL_OBJECTS_H
#define LOCAL_OBJECTS_H

#include "constants.h"

typedef struct _object_entry
{
	char name[MAX_OBJ_LEN];
	char path[MAX_PATH_LEN];
} object_entry;

typedef struct _local_objects
{
	int num_objects;
	object_entry objects[MAX_OBJ_NUM];
} local_objects;

#endif
