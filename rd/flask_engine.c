/**
 * CS 15-441 Computer Networks
 *
 * Functions to handle flask requests and responses.
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
#include <sys/types.h>
#include <sys/select.h>
#include <errno.h>
#include <netdb.h>

void process_buffer(tcp_connection *connection, int i);
void build_response(tcp_connection *connection);

/*
 * flask_engine_create - Creates the server socket for routing daemon <-> flask
 * communication.
 *
 * @return  1 on success. Simply exits on failure.
 */
int flask_engine_create()
{
    struct sockaddr_in addr;
    struct hostent *h;
    int yes = 1;
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
    //addr.sin_addr.s_addr =  *(in_addr_t *)h->h_addr;
    addr.sin_addr.s_addr = INADDR_ANY;
    setsockopt(engine.tcp_sock, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));

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

    printf("listening on %d\n",engine.tcp_port);
    return 1;
}


/*
 * new_client_handler - Sets up new tcp (flask) connections.
 *
 * @param sock  tcp server socket identifier
 * @return      1 when successful, -1 otherwise
 */
int new_client_handler(int sock)
{
    // !!!!! for some reason the sock_addr_in was breaking CP1 functionality. I changed it to sock_Addr_storage jsut to check, cus I wasn't sure
    // what was going on.... it works with storage, but not with in.
    //struct sockaddr_in cli_addr;
    struct sockaddr_storage cli_addr;

    socklen_t cli_size = sizeof(cli_addr);
    int client_sock;

    if (engine.nfds <= MAX_CONNECTIONS - 1)
    {
        client_sock = accept(engine.tcp_sock, (struct sockaddr *) &cli_addr, &cli_size);

        if (client_sock < 0)
        {
            printf("%d and %s\n",errno,strerror(errno));

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


/*
 * flask_request_handler - Processes incoming flask request. Pareses the
 * request and builds response depending on process results.
 *
 * @param i client socket identifier
 * @return  1 when successful, 0 otherwise
 */
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
        FD_CLR(i,&(engine.rfds));
        FD_CLR(i,&(engine.wfds));
        return 0;
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
        printf("received request: %s\n",curr_connection->request);
        curr_connection->request_index = readret;
        curr_connection->status = 200;
        process_buffer(curr_connection, i);
        build_response(curr_connection);
        FD_SET(i, &(engine.wfds));
    }

    return 1;
}


/*
 * flask_response_handler - Handles sending response to client and does
 * necessary clean up.
 *
 * @param i client socket identifier
 * @return  1 when successful. Simply exits when fatal error occurs.
 */
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
            printf("fatal error\n");
            exit(EXIT_FAILURE);
        }
    }

    init_connection(curr_connection);
    //  FD_CLR(i, &(engine.rfds));
    //  FD_CLR(i, &(engine.wfds));
    //close(i);

    return 1;
}


/*
 * process_buffer - Processes flask request and parses out the request method,
 * object name length, and object names.
 *
 * @param connection    flask connection struct containing all necessary buffer
 *                      for parsing
 * @param i             client socket identifier
 */
void process_buffer(tcp_connection *connection, int i)
{
    char *tokens[5];
    // int j;

    tokens[0] = strtok(connection->request, " ");

    if (tokens[0] == NULL)
    {
        connection->status = 500;
        FD_SET(i, &(engine.wfds));
        return;
    }

    tokens[1] = strtok(NULL," ");
    tokens[2] = strtok(NULL," ");
    tokens[3] = strtok(NULL," ");
    tokens[4] = strtok(NULL," ");
    /* !!!!!
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
       */
    strcpy(connection->method, tokens[0]);
    strcpy(connection->name_length, tokens[1]);
    strncpy(connection->name,tokens[2],atoi(tokens[1]));
    if (strcmp(tokens[0], "ADDFILE") == 0)
    {
        strcpy(connection->path_length, tokens[3]);
        strncpy(connection->path, tokens[4],atoi(tokens[3]));
    }
}


/*
 * get_local_file_path - 
 *
 * @param connection   
 *
 */
void get_local_file_path(tcp_connection *connection)
{
    int i;
    for (i = 0; i<ol.num_objects;i++)
    {
        if(strcmp(connection->name,ol.objects[i].name) == 0)
        {
            strcpy(connection->path,ol.objects[i].path);
            return;
        }
    }
    connection->path[0] = '\0';
}


/*
 * build_response - Builds response to send  back to flask.
 *  
 * @param connection    flask connection struct containing all necessary buffer
 *                      for parsing
 */
void build_response(tcp_connection *connection)
{
    FILE *fp;
    path *obj;
    pair *p;
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

    if (strcmp(connection->method, "RDGET")== 0)
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
                get_local_file_path(connection);
                if(connection->path[0] == '\0'){
                    connection->status = 500;
                    sprintf(connection->response, "ERROR found object's host, but could not retrieve  path ");
                }else{
                    sprintf(uri, "%s%s", my_uri, connection->path);
                }
            }
            else
            {

                r_entry = get_routing_entry(&rt, node_id);
                l_entry = lookup_link_entry_node_id(&dl, r_entry->nexthop);

                sprintf(uri, "http://%s:%d/static/f/%s", l_entry->host, l_entry->server_p, connection->name);

            }

            connection->status = 200;
            sprintf(connection->response, "OK %d %s ", (int)(strlen(uri)), uri);
        }

    }
    else if (strcmp(connection->method, "ADDFILE")== 0)
    {
        p = contains_object(&gol, connection->name);

        if(p!= NULL )
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

            fclose(fp);
            connection->status = 200;
            sprintf(connection->response, "OK 0 ");
        }
    }
    printf("response is %s\n",connection->response);
    connection->response_index = strlen(connection->response);
}
