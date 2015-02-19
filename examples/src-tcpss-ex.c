/**
 * @file src-tcpss-ex.c
 * @brief An example of src-tcpss usage
 *
 * This program simulates the transmission of a file through an ADSL
 * access link. A very long RTT is used to show the slow start window
 * increase.
 */
#include <motsim.h>
#include <event.h>
#include <pdu-sink.h>
#include <ll-simplex.h>
#include <src-tcpss.h>
#include <probe.h>

/**
 * @brief Characteristics of the file, access link and network
 *
 */
#define FILE_SIZE 30000

#define ACCESS_LINK_THROUGHPUT    1000000
#define ACCESS_LINK_TRANSMIT_TIME 0.00001

#define RTT 1.0
#define MTU 1500

void sendOneFile(struct srcTCPSS_t  * src)
{
   srcTCPss_sendFile(src, FILE_SIZE);
}

int main()
{
   struct srcTCPSS_t  * src;
   struct llSimplex_t * link;
   struct PDUSink_t   * sink;
   struct probe_t     * pr;
   int i;

   motSim_create();

   // The packets are sent to a sink
   sink = PDUSink_create();

   // Let's put a probe on the sink
   pr = probe_createExhaustive();
   PDUSink_addInputProbe(sink, pr);

   // The client access network
   link = llSimplex_create(sink,
                           PDUSink_processPDU,
			   ACCESS_LINK_THROUGHPUT, ACCESS_LINK_TRANSMIT_TIME);

   // The source
   src = srcTCPss_create(MTU, RTT, 1, link, llSimplex_processPDU);

   // Send a file
   srcTCPss_sendFile(src, FILE_SIZE);

   // We will later send two other files to test "persistent
   // connections"
   event_add((void (*)(void *data))sendOneFile, (void*)src, 0.20);
   event_add((void (*)(void *data))sendOneFile, (void*)src, 100.0);


   motSim_runUntilTheEnd();

   // Print some info
   printf("%ld packets received\n", probe_nbSamples(pr));
   for (i = 0; i < probe_nbSamples(pr); i++) {
     printf("[%lf] %ld\n", probe_exhaustiveGetDateN(pr, i), (long)probe_exhaustiveGetSampleN(pr, i));
   }
   return 0;
}
