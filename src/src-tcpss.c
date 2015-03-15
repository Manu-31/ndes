/**
 * @file src-tcpss.c
 * @brief A very basic TCP source model (slow start only)
 *
 * This module implements a very basic TCP source model as described
 * in [1]. This model is relevant to study the consequences of short
 * connections on the client acces link (this is the point of [1]).
 *
 * [1] "cdma2000 Evaluation Methodology" 3GPP2
 * C. R1002-0. Version1.0. December 10, 2004
 */
#include <src-tcpss.h>
#include <event.h>
#include <motsim.h>
#include <file_pdu.h>
#include <event-list.h>

#define TCP_BASE_HEADER_SIZE 40

/**
 * @brief Definition of a source
 */
struct srcTCPSS_t {
   int                windowSize ;
   int                MSS;
   double             RTT;

   int                backlog;   //!< the number of unsent bytes
   int                nbSentSegments;  //!< Number of segments actually sent
   struct filePDU_t * outputQueue; //!< An internal queue where
				   //!segments are queued waiting to
				   //!be actually sent
   void        * destination;    //!< L'objet auquel sont destinées les PDUs
   processPDU_t  destProcessPDU; //!< La fonction permettant de
				 //!signaler à la destination la
				 //!présence de la PDU
  struct eventList_t * EOTEventList; //!< List of events to run at the
				  //!end of transmission
};


/**
 * @brief Creation/initialization of a source
 * @param MTU is the maximum transmission unit of the link
 * @param RTTmd is the Round Trip Time minus transmission time on the access link
 * @param initialWindow is the initial value of cwnd
 * @param destination is a pointer to the destination entity
 * @param destProcessPDU is the PDU processing function of the destination
 */
struct srcTCPSS_t * srcTCPss_create(int MTU,
                                    double RTTmd,
                                    int initialWindow,
                                    void * destination,
				    processPDU_t destProcessPDU)
{
   struct srcTCPSS_t * result = (struct srcTCPSS_t *) sim_malloc(sizeof(struct srcTCPSS_t ));

   result->windowSize = initialWindow;
   result->MSS = MTU - TCP_BASE_HEADER_SIZE ;
   result->RTT = RTTmd;
   result->backlog = 0;
   result->nbSentSegments = 0;
   result->destination = destination;
   result->destProcessPDU = destProcessPDU;
   result->outputQueue = filePDU_create(NULL, NULL);
   result->EOTEventList = eventList_create();
   return result;
}

struct PDU_t * srcTCPss_getPDU(void * src);

/**
 * @brief Build a segment and notify the destination
 * @param src is a pointer to the source
 */
void srcTCPss_sendSegment(struct srcTCPSS_t * src)
{
   int segmentSize; 
   struct PDU_t * segment;

   // Send no more than available and MSS
   segmentSize = min(src->backlog, src->MSS);

   if (segmentSize > 0) {
      printf_debug(DEBUG_SRC, "Will send a segment of %d bytes (out of %d remaining)\n", segmentSize, src->backlog);

      src->backlog -= segmentSize;

      // We need to build this new segment
      segment = PDU_create(segmentSize, NULL);

      // We insert it in the outputQueue 
      filePDU_insert(src->outputQueue, segment);

      printf_debug(DEBUG_SRC, "Segment in the queue, notifying the destination\n");

      // Now we notify the destination 
      src->destProcessPDU(src->destination, srcTCPss_getPDU, src);
   }
}

/**
 * @brief Send several segments
 * @param src is a pointer to the source
 * @param nb is the number of segments to send
 */
void srcTCPss_sendSegments(struct srcTCPSS_t * src, int nb)
{
   int n;

   for (n=0; n< nb; n++){
      srcTCPss_sendSegment(src);
   }
}

/**
 * @brief Send two segments
 * @param s is a pointer to the source
 */
void srcTCPss_send2Segments(void * s)
{
   struct srcTCPSS_t * src = (struct srcTCPSS_t *) s;

   srcTCPss_sendSegments(src, 2);
}

/**
 * @brief Simulate transmission of nbBytes bytes
 * @param src is a pointer to the source
 * @param nbBytes is the number of bytes to transmit
 */
void srcTCPss_sendFile(struct srcTCPSS_t * src,
                       int nbBytes)
{
   printf_debug(DEBUG_SRC, "Will send %d bytes (ws %d)\n", nbBytes, src->windowSize);

   if (srcTCPss_isEmpty(src)) {
      src->backlog = nbBytes;

      printf_debug(DEBUG_SRC, "First burst of %d segments\n",  min(src->windowSize, (nbBytes + src->MSS - 1)/src->MSS));
      // Send as many segments as possible
      srcTCPss_sendSegments(src, min(src->windowSize, (nbBytes + src->MSS - 1)/src->MSS));
   } else {
      printf_debug(DEBUG_SRC, "Persistent use\n");
      // If the source is already active, we just add these new bytes
      src->backlog += nbBytes;
   }
}

/**
 * @brief get a PDU from a source
 * @param s is a pointer to the source
 * @result A PDU if available or NULL
 */
struct PDU_t * srcTCPss_getPDU(void * s)
{
   struct srcTCPSS_t * src = (struct srcTCPSS_t *) s;
   struct PDU_t * result = NULL;

   printf_debug(DEBUG_SRC, "IN\n");

  // If a PDU is available, we send it and (slow start) schedule 2 new
  // PDUs, if available
  if (filePDU_length(src->outputQueue) > 0) {
     printf_debug(DEBUG_SRC, "scheduling two more segments\n");
     motSim_scheduleNewEvent(srcTCPss_send2Segments, src, motSim_getCurrentTime() + src->RTT);
     result = filePDU_extract(src->outputQueue);
     src->nbSentSegments++;
     printf_debug(DEBUG_SRC, "returning a segment (nb %d)\n", src->nbSentSegments);
     if (srcTCPss_isEmpty(src)) {
        printf_debug(DEBUG_SRC, "(last for now)\n");
        // This is the end of the transmission, we need to save window
        // size for the next transmission, if any
        src->windowSize += src->nbSentSegments;
        src->nbSentSegments = 0;

        // Run any event in the list, if any.
        // WARNING : this this the EOT, not the end of reception !
        // You may need to wait for an extra RTT ...
        eventList_runList(src->EOTEventList);
     }
     return result; 
  } else { // Should not occur ...
     printf_debug(DEBUG_SRC, "OUT\n");
     return NULL;
  }
}

/**
 * @brief is the source empty ?
 * @param src is a source
 * @return non null if at least one byte remains to be sent
 *
 * A source is "empty" as soon as every byte has bee sent. Be carefull
 * that an empty source will turn to non empty after a (new) call to
 * srcTCPss_sendFile.
 */
int srcTCPss_isEmpty(struct srcTCPSS_t * src)
{
   return ((src->backlog == 0) && (filePDU_length(src->outputQueue) ==  0));
}

/**
 * @brief Delete and free a source
 * @param src an empty source
 * 
 * The caller MUST ensure this source is empty.
 */
void srcTCPss_free(struct srcTCPSS_t * src)
{
   assert(srcTCPss_isEmpty(src));

   // Delete the outputQueue
   printf_debug(DEBUG_TBD, "filePDU_free unavailable !");

   // Free the structure
   free(src);
}

/**
 * @brief Add an event to be run on EOT
 * @param src is a poiter to the source
 * @param ev is the event to schedule for EOT
 */
void srcTCPss_addEOTEvent(struct srcTCPSS_t * src, struct event_t * ev)
{
   eventList_append(src->EOTEventList, ev);
}
