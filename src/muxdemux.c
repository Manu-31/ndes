/**
 * @file muxdemux.c
 * @brief A basic multiplxer/demultiplexer
 */

#include <limits.h>   // UINT_MAX
#include <muxdemux.h>
#include <ndesObject.h>
#include <ndesObjectList.h>

/**
 * @brief A multiplexing sender
 *
 */
struct muxDemuxSender_t {
   void * destination;
   processPDU_t destProcessPDU;

   struct ndesObjectList_t * sapList; //<! Sorted in ascending identifier
};

/**
 * @brief A multiplexer service access point
 */
struct muxDemuxSenderSAP_t {
   declareAsNdesObject; //< C'est un ndesObject 
   unsigned int identifier; //<! The SAPI
   struct muxDemuxSender_t * sender; //<! The corresponding sender

   void * src;
   getPDU_t srcGet;
};

/**
 * @brief Définition des fonctions spécifiques liées au ndesObject
 */
defineObjectFunctions(muxDemuxSenderSAP);

/**
 * @brief Les senderSAP sont aussi des ndesObject
 */
struct ndesObjectType_t  muxDemuxSenderSAPType = {
   ndesObjectTypeDefaultValues(muxDemuxSenderSAP)
};

#define SAPI_MAGIC_NB ((long long unsigned)684253148796258036)
//#define SAPI_MAGIC_NB ((long long unsigned)48324832)

/**
 * @brief An encapsulation structure
 */
struct muxDemuxEncaps_t {
   unsigned long long magic;  // WARNING to remove
   unsigned int sapi;
   struct PDU_t * pdu;
};

struct muxDemuxEncaps_t * muxDemuxEncaps_create(struct muxDemuxSenderSAP_t * sap,
						struct PDU_t * pdu)
{
   struct muxDemuxEncaps_t * result = (struct muxDemuxEncaps_t * )sim_malloc(sizeof(struct muxDemuxEncaps_t ));

   printf_debug(DEBUG_MUX, "IN for PDU %d in SAPI %d\n", PDU_id(pdu), sap->identifier);

   result->magic = SAPI_MAGIC_NB;
   result->sapi = sap->identifier;
   result->pdu = pdu;

   return result;
}

void muxDemuxEncaps_delete(struct muxDemuxEncaps_t * encaps)
{
   free(encaps);
}

/**
 * @brief Sender (multiplexer) creator
 */
struct muxDemuxSender_t * muxDemuxSender_create(void * destination,
						processPDU_t destProcessPDU)
{
   struct muxDemuxSender_t * result = (struct muxDemuxSender_t *)sim_malloc(sizeof(struct muxDemuxSender_t ));

   result->destination = destination;
   result->destProcessPDU = destProcessPDU;
   result->sapList = ndesObjectList_create(&muxDemuxSenderSAPType);
 
   return result;
}

/**
 * @brief Creation of a new Service Access Point
 * @param sender A pointer to the multiplexer structure
 * @param newSAPI The desired identifier (non zero) or unspecified
 * (zero)
 * @result A new SAP or NULL (identifier unavailable)
 */
struct muxDemuxSenderSAP_t * muxDemuxSender_createNewSAP(struct muxDemuxSender_t * sender,
							 unsigned int newSAPI)
{
   unsigned int nextSAPI = 1;

   struct ndesObjectListIterator_t *i;
   struct muxDemuxSenderSAP_t * s; //!< For iteration
   struct muxDemuxSenderSAP_t * p; //!< The SAP after which to insert
   struct muxDemuxSenderSAP_t * result = NULL;

   // If the chosen SAPI is non zero, we check for its availability,
   // else we search the next available value. 
   i = ndesObjectList_createIterator(sender->sapList);
   s = (struct muxDemuxSenderSAP_t *)ndesObjectList_iteratorGetNext(i);
   while (s != NULL) {
      // Looking for an available identifier
      if (s->identifier == nextSAPI) {
         if (nextSAPI == UINT_MAX) {
            ndesObjectList_deleteIterator(i);
            return NULL;
  	 }
         nextSAPI++;
      }
      // Aborting on "collisions"
      if ((newSAPI != 0) && (s->identifier == newSAPI)) {
	 ndesObjectList_deleteIterator(i);
         return NULL;
      }
      s = (struct muxDemuxSenderSAP_t *)ndesObjectList_iteratorGetNext(i);
   }
   ndesObjectList_deleteIterator(i);
   // The new SAP can be created
   if (newSAPI == 0) {
      newSAPI = nextSAPI;
   }

   // Creation and initialisation
   result = (struct muxDemuxSenderSAP_t *)sim_malloc(sizeof(struct muxDemuxSenderSAP_t));
   result->identifier = newSAPI;
   result->sender = sender;
   result->src = NULL;
   result->srcGet = NULL;
   ndesObjectInit(result, muxDemuxSenderSAP);

   // Now we search for its position within the file
   p = NULL; // A priori it is the first
   i = ndesObjectList_createIterator(sender->sapList);
   s = (struct muxDemuxSenderSAP_t *)ndesObjectList_iteratorGetNext(i);
   while ((s != NULL) && (s->identifier < newSAPI)) {
      p=s;
      s = (struct muxDemuxSenderSAP_t *)ndesObjectList_iteratorGetNext(i);
   } 
   ndesObjectList_deleteIterator(i);

   // Insertion
   if (p) {
      ndesObjectList_insertAfter(sender->sapList, p, result);
   } else {
      ndesObjectList_insert(sender->sapList, result);
   }
   return result;
}

/**
 * @brief The sender output fonction
 */
struct PDU_t * muxDemuxSender_getPDU(void * s)
{
   struct PDU_t * pdu, * pduEnc;
   struct muxDemuxSenderSAP_t * sap = (struct muxDemuxSenderSAP_t * )s;
   struct muxDemuxEncaps_t * encaps;

   printf_debug(DEBUG_MUX, "IN\n");

   if ((sap->src == NULL)||(sap->srcGet == NULL)) {
       printf_debug(DEBUG_MUX, "Should not be here\n");
      return NULL;
   }

   // Grab the PDU from the last registered source
   pdu = sap->srcGet(sap->src);

   // Encapsulation (overheadless)
   encaps = muxDemuxEncaps_create(sap, pdu);

   // Embed this in a PDU
   pduEnc = PDU_create(PDU_size(pdu), encaps);
   printf_debug(DEBUG_MUX, "OUT (encapsulated in PDU %d)\n", PDU_id(pduEnc));

   return pduEnc;
}

/**
 * @brief The sender input fonction
 */
int muxDemuxSender_processPDU(void * receiver,
			    getPDU_t getPDU,
			    void * source)
{
   struct muxDemuxSenderSAP_t * sap = (struct muxDemuxSenderSAP_t * )receiver;
   struct muxDemuxSender_t    * sender = sap->sender;
   int result ;

   printf_debug(DEBUG_MUX, "IN\n");

   // Is it just a test ?
   if (source == NULL) {
      printf_debug(DEBUG_MUX, "getPDU/source should not be NULL\n");
      return sender->destProcessPDU(sender->destination, NULL, NULL);
   } else {
      // Prepare the context for the upcoming getPDU
      sap->src = source;
      sap->srcGet = getPDU;

      printf_debug(DEBUG_MUX, "Let the destination process\n");

      // Let the destination process (should call muxDemuxSender_getPDU)
      result = sender->destProcessPDU(sender->destination, muxDemuxSender_getPDU, receiver);

      printf_debug(DEBUG_MUX, "OUT\n");

      return result;
   }
}

int muxDemuxSender_pduMatchesSAP(void* s, struct PDU_t * pdu)
{
   struct muxDemuxSenderSAP_t * sap = (struct muxDemuxSenderSAP_t *)s;
   struct muxDemuxEncaps_t * encaps = (struct muxDemuxEncaps_t * ) PDU_private(pdu);
   
   printf_debug(DEBUG_MUX, "IN (magic (%llu %llu) %s) PDU %d\n", encaps->magic,  SAPI_MAGIC_NB, (encaps->magic == SAPI_MAGIC_NB)?"Yes":"NO", PDU_id(pdu));
   if (encaps->magic == SAPI_MAGIC_NB) {
     printf_debug(DEBUG_MUX, "Match (%d/%d): %s\n", encaps->sapi, sap->identifier, (encaps->sapi == sap->identifier)?"Yes":"NO");
   }

   return ((encaps->magic == SAPI_MAGIC_NB) && (encaps->sapi == sap->identifier));
}

/**
 * @brief Create a filter based on a SAP
 */
struct PDUFilter_t * muxDemuxSender_createFilterFromSAP(struct muxDemuxSenderSAP_t * sap)
{
   struct PDUFilter_t * result;

   result = PDUFilter_create();

   PDUFilter_setPrivate(result, (void *)sap);
   PDUFilter_setTestFunction(result, muxDemuxSender_pduMatchesSAP);
 
   return result;
}

/*********************************************************************************
            RECEIVER SIDE
 */

/**
 * @brief A demultiplexing receiver
 */
struct muxDemuxReceiver_t {
   struct ndesObjectList_t * sapList; //<! Sorted in ascending identifier
};

/**
 * @brief A multiplexer service access point
 */
struct muxDemuxReceiverSAP_t {
   declareAsNdesObject;                  //<! C'est un ndesObject 
   unsigned int identifier;              //<! The SAPI
   struct muxDemuxReceiver_t * receiver; //<! The corresponding receiver
   void * destination;
   processPDU_t destProcessPDU;
   struct PDU_t * pdu;
};


/**
 * @brief Définition des fonctions spécifiques liées au ndesObject
 */
defineObjectFunctions(muxDemuxReceiverSAP);

/**
 * @brief Les receiverSAP sont aussi des ndesObject
 */
struct ndesObjectType_t  muxDemuxReceiverSAPType = {
   ndesObjectTypeDefaultValues(muxDemuxReceiverSAP)
};

/**
 * @brief Receiver (demultiplexer) creator
 */
struct muxDemuxReceiver_t * muxDemuxReceiver_create()
{
   struct muxDemuxReceiver_t * result = (struct muxDemuxReceiver_t *)sim_malloc(sizeof(struct muxDemuxReceiver_t ));

   result->sapList = ndesObjectList_create(&muxDemuxReceiverSAPType);
 
   return result;
}

/**
 * @brief Creation of a new Service Access Point
 * @param receiver A pointer to the demultiplexer structure
 * @param newSAPI The desired identifier (non zero) or unspecified
 * (zero)
 * @result A new SAP or NULL (identifier unavailable)
 */
struct muxDemuxReceiverSAP_t * muxDemuxReceiver_createNewSAP(struct muxDemuxReceiver_t * receiver,
						             unsigned int newSAPI,
							     void * destination,
							     processPDU_t destProcessPDU)
{
   unsigned int nextSAPI = 1;

   struct ndesObjectListIterator_t * i;
   struct muxDemuxReceiverSAP_t * s; //!< For iteration
   struct muxDemuxReceiverSAP_t * p; //!< The SAP after which to insert
   struct muxDemuxReceiverSAP_t * result = NULL;

   printf_debug(DEBUG_MUX, "IN\n");

   // If the chosen SAPI is non zero, we check for its availability,
   // else we search the next available value. 
   i = ndesObjectList_createIterator(receiver->sapList);
   printf_debug(DEBUG_MUX, "STEADY\n");
   s = (struct muxDemuxReceiverSAP_t *)ndesObjectList_iteratorGetNext(i);
   printf_debug(DEBUG_MUX, "GO\n");
   while (s != NULL) {
     printf_debug(DEBUG_MUX, "id %d\n", s->identifier);
      // Looking for an available identifier
      if (s->identifier == nextSAPI) {
         if (nextSAPI == UINT_MAX) {
            ndesObjectList_deleteIterator(i);
            return NULL;
  	 }
         nextSAPI++;
      }
      // Aborting on "collisions"
      if ((newSAPI != 0) && (s->identifier == newSAPI)) {
	 ndesObjectList_deleteIterator(i);
         return NULL;
      }
      s = (struct muxDemuxReceiverSAP_t *)ndesObjectList_iteratorGetNext(i);
   }
   printf_debug(DEBUG_MUX, "DONE\n");
   ndesObjectList_deleteIterator(i);
   printf_debug(DEBUG_MUX, "Search over\n");

   // The new SAP can be created
   if (newSAPI == 0) {
      newSAPI = nextSAPI;
   }
   printf_debug(DEBUG_MUX, "newSAPI %d\n", newSAPI);

   // Creation and initilisation
   result = (struct muxDemuxReceiverSAP_t *)sim_malloc(sizeof(struct muxDemuxReceiverSAP_t));
   result->identifier = newSAPI;
   result->receiver = receiver;
   result->destination = destination;
   result->destProcessPDU = destProcessPDU;
   result->pdu = NULL;
   ndesObjectInit(result, muxDemuxReceiverSAP);

   printf_debug(DEBUG_MUX, "Created\n");

   // Now we search for its position within the file
   p = NULL; // A priori it is the first
   i = ndesObjectList_createIterator(receiver->sapList);
   s = (struct muxDemuxReceiverSAP_t *)ndesObjectList_iteratorGetNext(i);
   while ((s != NULL) && (s->identifier < newSAPI)) {
      p=s;
      s = (struct muxDemuxReceiverSAP_t *)ndesObjectList_iteratorGetNext(i);
   } 
   ndesObjectList_deleteIterator(i);
   printf_debug(DEBUG_MUX, "Ready to insert\n");

   // Insertion
   if (p) {
      printf_debug(DEBUG_MUX, "After\n");
      ndesObjectList_insertAfter(receiver->sapList, p, result);
   } else {
      printf_debug(DEBUG_MUX, "First\n");
      ndesObjectList_insert(receiver->sapList, result);
   }
   printf_debug(DEBUG_MUX, "OUT\n");

   return result;
}

/**
 * @brief The receiver output fonction
 */
struct PDU_t * muxDemuxReceiver_getPDU(void * s)
{
   struct muxDemuxReceiverSAP_t * rcv = (struct muxDemuxReceiverSAP_t * )s;
   struct PDU_t * pdu = rcv->pdu;

   printf_debug(DEBUG_MUX, "IN\n");

   rcv->pdu=NULL;
   printf_debug(DEBUG_MUX, "OUT\n");

   return pdu;
}

/**
 * @brief The receiver input fonction
 */
int muxDemuxReceiver_processPDU(void * rcv,
   			        getPDU_t getPDU,
			        void * source)
{
   struct muxDemuxReceiver_t       * receiver = (struct muxDemuxReceiver_t    *)rcv;
   struct muxDemuxReceiverSAP_t    * s;
   struct muxDemuxEncaps_t         * encaps;
   struct ndesObjectListIterator_t * i;
   int sapi;
   struct PDU_t * pdu;
   int result;

   printf_debug(DEBUG_MUX, "IN\n");

   //attention, bug a corriger : il faut paser un getPDU null, pas une pdu nulle pour le test
   if ((getPDU == NULL) || (source == NULL)) {
      printf_debug(DEBUG_MUX, "getPDU/source should not be NULL\n");
      return 1;
   }

   // First, let's get the PDU
   pdu = getPDU(source);
   printf_debug(DEBUG_MUX, "got the PDU\n");

   // De-encapsulation
   encaps = (struct muxDemuxEncaps_t*)PDU_private(pdu);
   sapi = encaps->sapi;

   // Then we need to find the output SAP
   i = ndesObjectList_createIterator(receiver->sapList);
   s = (struct muxDemuxReceiverSAP_t *)ndesObjectList_iteratorGetNext(i);
   while ((s != NULL) && (s->identifier != sapi)) {
      s = (struct muxDemuxReceiverSAP_t *)ndesObjectList_iteratorGetNext(i);
   } 
   ndesObjectList_deleteIterator(i);

   if ((s == NULL) || (s->identifier != sapi)) {
      motSim_error(MS_WARN, "SAPI not found\n");
      return 0;
   }

   // Prepare the context for getPDU
   s->pdu = encaps->pdu;

   // Let the destination process the PDU
   printf_debug(DEBUG_MUX, "Let the destination process\n");
   result = s->destProcessPDU(s->destination, muxDemuxReceiver_getPDU, (void *)s);

   printf_debug(DEBUG_MUX, "OUT\n");

   return result;
}

