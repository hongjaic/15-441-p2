#include "tcp_connection.h"
#include <stdlib.h>
#include <string.h>

void init_connection(tcp_connection *connection)
{
    memset(connection->request, 0, MAX_BUF);
    connection->request_index = 0;
    memset(connection->response, 0 , MAX_BUF);
    connection->request_index = 0;
    memset(connection->method, 0, MAX_BUF);
    memset(connection->name_length, 0, MAX_BUF);
    memset(connection->name, 0, MAX_BUF);
    memset(connection->path_length, 0, MAX_BUF);
    memset(connection->path, 0 , MAX_BUF);
    connection->status = 0;
}
