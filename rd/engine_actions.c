/**
 * CS 15-441 Computer Networks
 *
 * Defines main wrapper logic and functions necessary for routing and handling
 * flask <-> routing daemon communication.
 *
 * @file    flask_engine.h
 * @author  Hong Jai Cho <hongjaic>, Raul Gonzales <rggonzal>
 */

#include "engine_actions.h"


int recv_handler(int i);
int send_handler(int i);

/*
 * create_engine    creates and sets up udp socket for routing and tcp socket
 * for flask<->routing daemon communication
 *
 * @param udp_port  routing daemon socket identifier
 * @param tcp_port  tcp socket identifier
 * @return          1
 */
int create_engine(int udp_port, int tcp_port)
{
    engine.udp_port = udp_port;
    engine.tcp_port = tcp_port;

    flooding_engine_create();
    flask_engine_create();

    FD_ZERO(&(engine.wfds));
    FD_ZERO(&(engine.rfds));
    FD_SET(engine.udp_sock, &(engine.rfds));
    FD_SET(engine.tcp_sock, &(engine.rfds));

    engine.nfds = 5;

    engine.fdmax = engine.udp_sock >= engine.tcp_sock ? engine.udp_sock : engine.tcp_sock;

    return 1;
}


/*
 * engine_event - main enging event loop that handles routing and
 * routing daemon <-> flask communication
 */
int engine_event()
{
    int i, iterations = RETRANSMIT_TIME+1;
    int sel_return = -1;
    struct timeval tv;
    tv.tv_sec = TIMEOUT;
    tv.tv_usec = 0;

    routing_entry entry;
    printf("my node id %d\n",my_node_id);
    while (1)
    {
        engine.temp_rfds = engine.rfds;
        engine.temp_wfds = engine.wfds;
        sel_return = select(engine.fdmax + 1, &(engine.temp_rfds), &(engine.temp_wfds), NULL, &tv);
        //sel_return = select(engine.fdmax + 1, &(engine.temp_rfds), &(engine.temp_wfds), NULL, NULL);


        if (sel_return == -1)
        {
            close(engine.udp_sock);
            close(engine.tcp_sock);
            fprintf(stderr, "Select returned -1\n");
            collapse(&gol);
            exit(EXIT_FAILURE);
        }

        if (sel_return == 0)
        {
            //!!!!!!!!!!!!! big issue, we are flooding up to three different types of LSA in this loop. this means,
            //we should be "saving" all three LSAs... this is a problem...

            /* It's time to flood */
            if (iterations == FLOODING_PERIOD)
            {
                printf("!\n!!!!\n!!!!!!\n!!!!!!\n!!!!!!!!!\n!!!!!!!!!!\n!!!!!!!!!\n!!!!!!\n");
                //Flood our own LSA
                flood(TYPE_LSA,engine.udp_sock, &dl, &ol, &rt, my_node_id, sequence_number);
                sequence_number++;

                //TIMEOUT;
                /*
                //!!!! for all neighbors, flood a type-3 lsa for all down neighbors
                for(i =1; i < dl.num_links; i++)
                {
                down_id = dl.links[i].id;
                if (get_routing_entry(&rt,down_id)->node_status == STATUS_DOWN){
                flood(TYPE_DOWN,engine.udp_sock, &dl, &ol, &rt, down_id, sequence_number);
                sequence_number++;
                }
                }
                */
                //for all nodes, forward/flood their received LSAs....
                for (i = 1; i < rt.num_entry; i++)
                {
                    entry = (rt.table[i]);

                    if (entry.lsa_is_new == 1 && entry.node_status != STATUS_DOWN)
                    {
                        flood_received_lsa(engine.udp_sock, entry.lsa, &dl, &rt, entry.lsa_size, entry.forwarder_id);
                    }
                    if (entry.node_status != STATUS_DOWN)
                    {
                        rt.table[i].node_status--;
                        if(rt.table[i].node_status == STATUS_DOWN)
                        {
                            hash_remove_node(&gol,rt.table[i].id);
                            if(rt.table[i].lsa != NULL)
                            {
                                free(rt.table[i].lsa);
                            }   
                            rt.table[i].lsa = NULL;
                            printf("NODE %d is DOWN!!!\n",rt.table[i].id);
                        }
                    }


                }
                iterations = 0;

            }

            /* If an ACK has not been received by not, retransmit */
            // !!!! changed == to <= .. want to retransmit first 3 iterations after flooding
            if (iterations != 0 && iterations <= RETRANSMIT_TIME)
            {

                for (i = 0; i < rt.num_entry; i++)
                {
                    entry = rt.table[i];

                    retransmit_missing(engine.udp_sock,entry.lsa,&dl,&rt,entry.lsa_size,entry.forwarder_id,&gol);

                }
            }

            iterations++;

            tv.tv_sec = TIMEOUT;
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
                }else{
                    FD_CLR(i,&engine.wfds);
                }
            }
        }

    }
}

/*
 * recv_handler - handler to handle any incoming message
 * @param i incoming request socket identifier
 * @return  1 when successful, otherwise -1
 */
int recv_handler(int i)
{
    if (i == engine.udp_sock)
    {
        return lsa_handler(i, &dl, &rt, &gol);
    }
    else if (i == engine.tcp_sock)
    {
        return new_client_handler(i);
    }

    return flask_request_handler(i);
}

/*
 * send_handler - handler to handle sending anything
 * @param i socket identifier of the receiving side
 * @return  1 when successful, otherwise -1
 */
int send_handler(int i)
{
    return flask_response_handler(i);
}
