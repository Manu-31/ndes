/**
 * @file pdu-sink.c
 * @brief Implantation d'un puits de PDU
 *
 */
#include <stdio.h>     // printf
#include <stdlib.h>    // Malloc, NULL, exit...

#include <pdu-sink.h>

#include <ndesObject.h>
#include <log.h>

/**
 * @brief structure d'un puits
 */
struct PDUSink_t {
   declareAsNdesObject; //< C'est un ndesObject 
   struct probe_t * insertProbe;
};

/**
 * @brief Les entrées de log sont des ndesObject
 */
struct ndesObjectType_t PDUSinkType = {
  ndesObjectTypeDefaultValues(PDUSink)
};

/**
 * @brief Définition des fonctions spécifiques liées au ndesObject
 */
defineObjectFunctions(PDUSink);

struct PDUSink_t * PDUSink_create()
{
   struct PDUSink_t * result = (struct PDUSink_t *)sim_malloc(sizeof(struct PDUSink_t));

   ndesObjectInit(result, PDUSink);

   result->insertProbe = NULL;

   return result;
}

/**
 * @brief Affectation d'une sonde sur les evenements d'insertion
 */
void PDUSink_addInputProbe(struct PDUSink_t * sink, struct probe_t * insertProbe)
{
   sink->insertProbe = probe_chain(insertProbe, sink->insertProbe);
}


int PDUSink_processPDU(void * s, getPDU_t getPDU, void * source)
{
   printf_debug(DEBUG_PDU, "in\n");
  
   struct PDUSink_t * pduSink = (struct PDUSink_t * )s;
   struct PDU_t * pdu = getPDU(source);

   // Si c'est juste pour tester si je suis pret
   if ((getPDU == NULL) || (source == NULL)) { 
      printf_debug(DEBUG_ALWAYS, "getPDU and source should now be non NULL\n");
      return 1;
   }

   if (pduSink->insertProbe){
     //      probe_sample(pduSink->insertProbe, PDU_id(pdu));
      probe_sample(pduSink->insertProbe, PDU_size(pdu));
   }

   if (pdu) {
      ndesLog_logLineF(PDU_getObject(pdu), "IN %d", PDUSink_getObjectId(s));

      PDU_free(pdu);
   }

   printf_debug(DEBUG_PDU, "out\n");
   return 1;
}


