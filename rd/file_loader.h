/**
 * CS 15-441 Computer Networks
 *
 * Function definitions for file_loader.c
 *
 * @file    file_loader.h
 * @author  Hong Jai Cho <hongjaic>, Raul Gonzalez <rggonzal>
 */

#ifndef FILE_LOADER_H
#define FILE_LOADER_H

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include "hashset.h"
#include "direct_links.h"
#include "local_objects.h"
#include "constants.h"
#include "lsa.h"
#include "routing_table.h"

int load_node_conf(char *path, direct_links *dl, routing_table *rt, char *my_uri);
int load_node_file(char *path, local_objects *ol, liso_hash *gol, int my_node_id);

#endif
