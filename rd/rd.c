#include <netinet/in.h>
#include <netinet/ip.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <signal.h>
#include <sys/select.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/sendfile.h>
#include <sys/resource.h>

#include "file_loader.h"
#include "engine_wrapper.h"
#include "engine_actions.h"
#include "flask_engine.h"
#include "flooding_engine.h"
#include "hashset.h"
#include "direct_links.h"
#include "local_objects.h"
#include "routing_table.h"

#define USAGE "\nUsage: %s <CONFIG FILE> <FILES FILE>\n\n"

engine_wrapper engine;
routing_table rt;
direct_links dl;
local_objects ol;
liso_hash gol;
int my_node_id;
int sequence_number;
char my_uri[MAX_URI_LEN];

char *conffile;
char *filefile;

int main(int argc, char *argv[])
{
    if (argc != 3)
    {
        fprintf(stderr, USAGE, argv[0]);
        exit(EXIT_FAILURE);
    }

    conffile = argv[1];
    filefile = argv[2];

    /* Load node configurations settings */
    load_node_conf(conffile, &dl, my_uri);
    my_node_id = ((dl.links)[0]).id;

    /* Load local file information */
    load_node_file(filefile, &ol, &gol, my_node_id);

    sequence_number = 0;

    create_engine(((dl.links)[0]).route_p, ((dl.links)[0]).local_p);

    engine_event();

    return 1;
}
