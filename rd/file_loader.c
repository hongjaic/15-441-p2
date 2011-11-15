#include "file_loader.h"


int load_node_conf(char *path, direct_links *dl, routing_table *rt, char *my_uri)
{
    FILE *fp;
    char line[MAX_LINE];
    char *tokens[5];
    int i, j  = 0,k;

    int num_entry;
    ack_checkers *checkers_j;

    char hostname[MAX_LINE];

    if (path == NULL)
    {
        return -1;
    }

    if (dl == NULL)
    {
        return -1;
    }

    dl->num_links = 0;
    rt->num_entry = 0;

    fp = fopen(path, "r");

    while (fgets(line, MAX_LINE, fp) != NULL)
    {
        i = 0;
        tokens[i] = strtok(line, " ");

        while (i < 5)
        {
            i++;
	    
            tokens[i] = strtok(NULL, " ");
        }

        ((dl->links)[j]).id = atoi(tokens[0]);
        strcpy(((dl->links)[j]).host, tokens[1]);

        ((dl->links)[j]).route_p = atoi(tokens[2]);
        ((dl->links)[j]).local_p = atoi(tokens[3]);
        ((dl->links)[j]).server_p = atoi(tokens[4]);
        dl->num_links++;

        ((rt->table)[j]).id = atoi(tokens[0]);
        ((rt->table)[j]).nexthop = atoi(tokens[0]);

		k =1;
        if (j == 0){
        	k = 0;
        }

        ((rt->table)[j]).cost = k;
        ((rt->table)[j]).node_status = 3;
        ((rt->table)[j]).lsa = NULL;
        ((rt->table)[i]).lsa_is_new = 0;
        ((rt->table)[i]).lsa_size = 0;
        ((rt->table)[i]).forwarder_id =((dl->links)[0]).id;
        ((rt->table)[j]).checkers_list = NULL;
        rt->num_entry++;

        memset(line, 0, MAX_LINE);
        j++;
    }

    num_entry = rt->num_entry;
    for(j = 0; j < num_entry; j++)
    {
        ((rt->table)[j]).checkers_list = (ack_checkers *)malloc(sizeof(ack_checkers) + num_entry*sizeof(ack_checker));
        checkers_j = ((rt->table)[j]).checkers_list;
        checkers_j->num_links = dl->num_links;

        for(i = 0; i < checkers_j->num_links; i++)
        {
            ((checkers_j->checkers)[i]).id = ((dl->links)[i]).id;
            ((checkers_j->checkers)[i]).ack_received = ACK_RECEIVED;
            ((checkers_j->checkers)[i]).retransmit = 0;
        }
    }

    fclose(fp);

    gethostname(hostname, MAX_LINE);
    sprintf(my_uri, "http://%s:%d", hostname, ((dl->links)[0]).server_p);

    return 1;
}

int load_node_file(char *path, local_objects *ol, liso_hash *gol, int my_node_id)
{
    FILE *fp;
    char line[MAX_LINE];
    char *tokens[2];
    int  j = 0;

    if (path == NULL)
    {
        return -1;
    }

    if (path == NULL)
    {
        return -1;
    }

    fp = fopen(path, "r");

    while (fgets(line, MAX_LINE, fp) != NULL)
    {

        tokens[0] = strtok(line, " ");
        tokens[1] = strtok(NULL, " ");


        strcpy(((ol->objects)[j]).name, tokens[0]);
        strcpy(((ol->objects)[j]).path, tokens[1]);

        ol->num_objects++;

        hash_add(gol, tokens[0], my_node_id, 0);

        memset(line, 0, MAX_LINE);
        j++;
    }

    fclose(fp);

    return 1;
}
