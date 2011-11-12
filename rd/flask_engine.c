/**
 * CS 15-441 Computer Networks
 *
 * @file    flask_engine.c
 * @author  Hong Jai Cho <hongjaic>, Raul Gonzales <rggonzal>
 */

#include "flask_engine.h"
#include "flooding_engine.h"
#include "tcp_connection.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <errno.h>
#include <netdb.h>

void process_buffer(tcp_connection *connection, int i);
void build_response(tcp_connection *connection);

int flask_engine_create()
{
    struct sockaddr_in addr;
    struct hostent *h;

    engine.tcp_sock = socket(PF_INET, SOCK_STREAM, 0);

    if (engine.tcp_sock == -1)
    {
        collapse(&gol);
        fprintf(stderr, "Failed to create TCP socket.\n");
        exit(EXIT_FAILURE);
    }

    h = gethostbyname("localhost");

    addr.sin_family = AF_INET;
    addr.sin_port = htons(engine.tcp_port);
    addr.sin_addr.s_addr = *(in_addr_t *)h->h_addr;

    if (bind(engine.tcp_sock, (struct sockaddr *) &addr, sizeof(addr)))
    {
        close(engine.tcp_sock);
        close(engine.udp_sock);
        collapse(&gol);
        fprintf(stderr, "Failed to bind TCP socket.\n");
        exit(EXIT_FAILURE);
    }

    if (listen(engine.tcp_sock, 5))
    {
        close(engine.tcp_sock);
        close(engine.udp_sock);
        collapse(&gol);
        fprintf(stderr, "failed to listen.\n");
        exit(EXIT_FAILURE);
    }
    return 1;
}

int new_client_handler(int sock)
{
    socklen_t cli_size;
    struct sockaddr_in cli_addr;
    int client_sock;

    if (engine.nfds <= MAX_CONNECTIONS - 1)
    {
        client_sock = accept(engine.tcp_sock, (struct sockaddr *) &cli_addr, &cli_size);

        if (client_sock < 0)
        {
            return -1;
        }

        init_connection(&((engine.connections)[client_sock]));

        if (client_sock > engine.fdmax)
        {
            engine.fdmax = client_sock;
        }

        engine.nfds++;

        FD_SET(client_sock, &(engine.rfds));

        if (engine.nfds == MAX_CONNECTIONS - 1)
        {
            FD_SET(client_sock, &(engine.wfds));
            ((engine.connections)[client_sock]).status = 503;
        }

        return 1;
    }

    return -1;
}

int flask_request_handler(int i)
{
    int no_data_read = 0;
    int readret;
    int errnoSave;
    tcp_connection *curr_connection = &((engine.connections)[i]);

    memset(engine.buf, 0, BUF_SIZE);
    readret = recv(i, engine.buf, BUF_SIZE - curr_connection->request_index, 0);

    if (readret == 0)
    {
        init_connection(curr_connection);
        close(i);
    }
    else if (readret < 1)
    {
        errnoSave = errno;

        if (errnoSave == ECONNRESET)
        {
            init_connection(curr_connection);
            close(i);
            fprintf(stderr, "Client disconneted.\n");
            return -1;
        }
        else
        {
            curr_connection->status = 500;
            build_response(curr_connection);
            FD_SET(i, &(engine.wfds));
            no_data_read = 1;
        }
    }

    if (no_data_read == 0)
    {
        memcpy(curr_connection->request, engine.buf, readret);
        curr_connection->request_index = readret;
        process_buffer(curr_connection, i);
        build_response(curr_connection);
    }

    return 1;
}

int flask_response_handler(int i)
{
    int errnoSave;
    int ret;
    tcp_connection *curr_connection = &((engine.connections)[i]);

    ret = send(i, curr_connection->response, curr_connection->response_index, MSG_NOSIGNAL);

    if (ret < 1)
    {
        errnoSave = errno;

        if (errnoSave != EPIPE && errnoSave != ECONNRESET)
        {
            close(i);
            close(engine.udp_sock);
            close(engine.tcp_sock);
            collapse(&gol);
            exit(EXIT_FAILURE);
        }
    }

    init_connection(curr_connection);
    FD_CLR(i, &(engine.rfds));
    FD_CLR(i, &(engine.wfds));
    close(i);

    return 1;
}

void process_buffer(tcp_connection *connection, int i)
{
    char *tokens[5];
    int j;

    tokens[0] = strtok(connection->request, " ");

    if (tokens[0] == NULL)
    {
        connection->status = 500;
        FD_SET(i, &(engine.wfds));
        return;
    }

    for (j = 1; j < 5; j++)
    {
        tokens[j] = strtok(NULL, " ");
    }

    if (strcmp(tokens[0], "RDGET") == 0 || strcmp(tokens[0], "ADDFILE") == 0)
    {
        if (tokens[1] == NULL || (atoi(tokens[1]) != strlen(tokens[2])))
        {
            connection->status  = 500;
            FD_SET(i, &(engine.wfds));
            return;
        }

        if (strcmp(tokens[0], "ADDFILE") == 0)
        {
            if (tokens[3] == NULL || (atoi(tokens[3]) != strlen(tokens[4])))
            {
                connection->status = 500;
                FD_SET(i, &(engine.wfds));
                return;
            }
        }
    }

    strcpy(connection->method, tokens[0]);
    strcpy(connection->name_length, tokens[1]);
    strcpy(connection->name, tokens[2]);

    if (strcmp(tokens[0], "ADDFILE") == 0)
    {
        strcpy(connection->path_length, tokens[3]);
        strcpy(connection->path, tokens[4]);
    }
}

void build_response(tcp_connection *connection)
{
    FILE *fp;
    path *obj;
    routing_entry * r_entry;
    link_entry * l_entry;
    char uri[BUF_SIZE];
    int node_id;

    if (connection->status != 200)
    {
        sprintf(connection->response, "ERROR 21 Internal Server Error ");
        connection->response_index = strlen(connection->response);
        return;
    }

    if (strcmp(connection->method, "RDGET"))
    {
        obj = get_paths(&gol, connection->name);

        if (obj == NULL)
        {
            connection->status = 404;
            sprintf(connection->response, "NOTFOUND 0 ");
        }
        else
        {
            node_id = obj->node_id;

            if (node_id == my_node_id)
            {
                sprintf(uri, "%s/static/%s", my_uri, connection->name);
            }
            else
            {

                r_entry = get_routing_entry(&rt, node_id);
                l_entry = lookup_link_entry_node_id(&dl, r_entry->nexthop);

                sprintf(uri, "http://%s:%d/rd/%d/%s", l_entry->host, l_entry->server_p, l_entry->local_p, connection->name);

            }

            connection->status = 200;
            sprintf(connection->response, "OK %d %s ", (int)(strlen(uri)), uri);
            
        }
    }
    else if (strcmp(connection->method, "ADDFILE"))
    {
        obj = get_paths(&gol, connection->name);

        if(obj != NULL)
        {
            connection->status = 500;
            sprintf(connection->response, "ERROR 19 File already exists ");
        }
        else
        {
            hash_add(&gol, connection->name, my_node_id, 0);
            strcpy(((ol.objects)[ol.num_objects]).name,  connection->name);
            strcpy(((ol.objects)[ol.num_objects]).path,  connection->path);
            ol.num_objects++;

            fp = fopen(filefile, "a");
            fprintf(fp, "%s %s\n", connection->name, connection->path);
                
            connection->status = 200;
            sprintf(connection->response, "OK 0 ");
        }
    }

    connection->response_index = strlen(connection->response);
}
