/**
 * CS 15-441 Computer Networks
 *
 * @file    flask_engine.h
 * @author  Hong Jai Cho <hongjaic>, Raul Gonzales <rggonzal>
 */

#include "engine_actions.h"


int recv_handler(int i);
int send_handler(int i);

int create_engine(int udp_port, int tcp_port)
{
    engine.udp_port = udp_port;
    engine.tcp_port = tcp_port;

    flooding_engine_create();
    flask_engine_create();

    FD_ZERO(&(engine.rfds));
    FD_SET(engine.udp_sock, &(engine.rfds));
    FD_SET(engine.tcp_sock, &(engine.rfds));
    FD_ZERO(&(engine.wfds));

    engine.nfds = 5;

    engine.fdmax = engine.udp_sock >= engine.tcp_sock ? engine.udp_sock : engine.tcp_sock;

    return 1;
}

int engine_event()
{
    int i, iterations = 0;
    int sel_return = -1;

    struct timeval tv;
    tv.tv_sec = TIMEOUT;
    tv.tv_usec = 0;

    //LSA my_lsa;
    int lsa_sent = 0;

    while (1)
    {
        engine.temp_rfds = engine.rfds;
        engine.temp_wfds = engine.wfds;

        sel_return = select(engine.fdmax + 1, &(engine.temp_rfds), &(engine.temp_wfds), NULL, &tv);

        if (sel_return == -1)
        {
            close(engine.udp_sock);
            close(engine.tcp_sock);
            fprintf(stderr, "Select returned -1");
            collapse(&gol);
            exit(EXIT_FAILURE);
        }

        if (sel_return == 0)
        {
            iterations++;

            /* It's time to flood */
            if (iterations == FLOODING_PERIOD)
            {
                flood_lsa(engine.udp_sock, &dl, &ol, &rt, my_node_id, sequence_number);
                sequence_number++;
                iterations = 0;
                lsa_sent = 1;
            }

            /* If an ACK has not been received by not, retransmit */
            else if (iterations == RETRANSMIT_TIME)
            {
                lsa_sent = 0;
            }
        }

        for (i = 0; i < engine.fdmax + 1; i++)
        {
            if (FD_ISSET(i, &engine.temp_rfds))
            {
                if (recv_handler(i) < 1)
                {
                    continue;
                }
            }

            if (FD_ISSET(i, &engine.temp_wfds))
            {
                if (send_handler(i) < 1)
                {
                    continue;
                }
            }
        }
    }
}

int recv_handler(int i)
{
    if (i == engine.udp_sock)
    {
        return lsa_handler(i, &dl, &rt);
    }
    else if (i == engine.tcp_sock)
    {
       return new_client_handler(i); 
    }
    
    return flask_request_handler(i);
}

int send_handler(int i)
{
    return flask_response_handler(i);
}
