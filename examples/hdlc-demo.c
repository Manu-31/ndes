/**
 * @brief A simple demo program to test HDLC
 */
#include <motsim.h>
#include <hdlc.h>
#include <ll-simplex.h>
#include <pdu-source.h>

int main()
{
   struct hdlc_t * client, * server;
   struct llSimplex_t *c2s, *s2c;
   struct PDUSource_t * src;
   struct dateSize sequence[] = {{1.0, 100}, {2.0, 100}, {0.0, 0}};

   motSim_create();

   // Creation of both entities
   server = hdlc_create();
   client = hdlc_create();

   // The link from client to server
   c2s = llSimplex_create(server,
			  hdlc_processPDU,
			  64000, 0.01);
   hdlc_setOutLink(client, c2s, llSimplex_processPDU);

   // The link from server to client
   s2c = llSimplex_create(client,
			  hdlc_processPDU,
			  64000, 0.01);
   hdlc_setOutLink(server, s2c, llSimplex_processPDU);

   // The client side source
   src = PDUSource_createDeterministic(sequence, client, hdlc_processSDU);

   hdlc_connectRequest(client);
   PDUSource_start(src);

   //   hdlc_send();
   motSim_runUntilTheEnd();

   return 0;
}
