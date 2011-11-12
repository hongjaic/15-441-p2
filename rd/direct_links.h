#ifndef LINK_ENTRY_H
#define LINK_ENTRY_H

#include "constants.h"

typedef struct _link_entry
{
	int id;
	char host[MAX_BUF];
	int local_p;
	int route_p;
	int server_p;
	int retransmits;
	int ack_received;
} link_entry;

typedef struct _direct_links
{
	int num_links;
	link_entry links[MAX_NODES];
} direct_links;

#endif
