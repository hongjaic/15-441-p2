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

   FD_ZERO(&(engine.wfds));
   FD_ZERO(&(engine.rfds));
   FD_SET(engine.udp_sock, &(engine.rfds));
   FD_SET(engine.tcp_sock, &(engine.rfds));

   engine.nfds = 5;

   engine.fdmax = engine.udp_sock >= engine.tcp_sock ? engine.udp_sock : engine.tcp_sock;

   return 1;
}

int engine_event()
{
   int i, iterations = RETRANSMIT_TIME+1;
   int sel_return = -1;
   int down_id;
   struct timeval tv;
   tv.tv_sec = TIMEOUT;
   tv.tv_usec = 0;

   routing_entry entry;
   printf("listening...\n");
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
            //Flood our own LSA
            flood(TYPE_LSA,engine.udp_sock, &dl, &ol, &rt, my_node_id, sequence_number);
            sequence_number++;

            //!!!! for all neighbors, flood a type-3 lsa for all down neighbors
            for(i =1; i < dl.num_links; i++)
            {
               down_id = dl.links[i].id;
               if (get_routing_entry(&rt,down_id)->node_status == STATUS_DOWN){
                  printf("flooding node %d is down\n",down_id);
                  flood(TYPE_DOWN,engine.udp_sock, &dl, &ol, &rt, down_id, sequence_number);
                  sequence_number++;
               }
            }

            //for all nodes, forward/flood their received LSAs....
            for (i = 1; i < rt.num_entry; i++)
            {
               entry = (rt.table[i]);

               if (entry.lsa_is_new == 1)
               {
                  flood_received_lsa(engine.udp_sock, entry.lsa, &dl, &rt, entry.lsa_size, entry.forwarder_id);
               }
            }
            printf("reset!!!\n");
            iterations = 0;
         }

         /* If an ACK has not been received by not, retransmit */
         // !!!! changed == to <= .. want to retransmit first 3 iterations after flooding
         if (iterations <= RETRANSMIT_TIME)
         {

            for (i = 0; i < rt.num_entry; i++)
            {
               entry = rt.table[i];
               if(entry.lsa !=NULL){
                  printf("node %d has LSA\n",entry.id);
                  retransmit_missing(engine.udp_sock,entry.lsa,&dl,&rt,entry.lsa_size,entry.forwarder_id);
               }else{
                  printf("node %d has NULL LSA\n",entry.id);
               }
            }


         }

         iterations++;
      }else{

         for (i = 0; i < engine.fdmax + 1; i++)
         {
            if (FD_ISSET(i, &engine.temp_rfds))
            {
               if (recv_handler(i) < 1)
               {
                  continue;
               }
               FD_SET(i,&engine.wfds);
               engine.temp_wfds = engine.wfds;
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
