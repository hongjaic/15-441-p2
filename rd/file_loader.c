#include "file_loader.h"


int load_node_conf(char *path, direct_links *dl, char *my_uri)
{
    FILE *fp;
    char line[MAX_LINE];
    char *tokens[5];
    int i, j = 0;

    char hostname[MAX_LINE];

    if (path == NULL)
    {
        return -1;
    }

    if (dl == NULL)
    {
        return -1;
    }

    fp = fopen(path, "r");

    while (fgets(line, MAX_LINE, fp) != NULL)
    {
        i = 0;
        tokens[i] = strtok(line, " ");

        while (tokens[i] != NULL)
        {
            i++;
            tokens[i] = strtok(NULL, " ");
        }

        ((dl->links)[j]).id = atoi(tokens[0]);
        strcpy(((dl->links)[j]).host, tokens[1]);
        ((dl->links)[j]).local_p = atoi(tokens[2]);
        ((dl->links)[j]).route_p = atoi(tokens[3]);
        ((dl->links)[j]).server_p = atoi(tokens[4]);
        ((dl->links)[j]).ack_received = 0;
        
        dl->num_links++;

        memset(line, 0, MAX_LINE);
        j++;
    }

    fclose(fp);

    gethostname(hostname, MAX_LINE);
    sprintf(my_uri, "http://%s:%d", hostname, ((dl->links)[i]).server_p);

    return 1;
}

int load_node_file(char *path, local_objects *ol, liso_hash *gol, int my_node_id)
{
    FILE *fp;
    char line[MAX_LINE];
    char *tokens[2];
    int i, j = 0;

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
        i = 0;
        tokens[i] = strtok(line, " ");

        while (tokens[i] != NULL)
        {
            i++;
            tokens[i] = strtok(NULL, " ");
        }

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
