/**
 * CS 15-441 Computer Networks
 *
 * Table of direct neighbors.
 *
 * @file    direct_links.h
 * @author  Hong Jai Cho <hongjaic>, Raul Gonzalez <rggonzal>
 */

#ifndef LINK_ENTRY_H
#define LINK_ENTRY_H

#include "constants.h"

/*
 * Generic struct for entry in direct neighbors table
 */
typedef struct _link_entry
{
	int id;
	char host[MAX_BUF];
	int local_p;
	int route_p;
	int server_p;
} link_entry;

/*
 * Direct neighbors table
 */
typedef struct _direct_links
{
	int num_links;
	link_entry links[MAX_NODES];
} direct_links;

#endif
