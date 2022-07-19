/**
 * @file : muxfcfs.c
 * @brief : Implantation d'un multiplexeur FCFS
 *
 * Un "multiplexeur FCFS" permet d'associer une destination unique à
 * un nombre quelconque de sources.
 * 
 * Les PDUs soumises par les sources sont transmises immédiatement à
 * la destination dans l'ordre où elles arrivent.
 */

#include <stdlib.h>    // Malloc, NULL, exit...

#include <motsim.h>
#include <muxfcfs.h>

struct muxfcfs_t {

   // Sauvegarde de l'origine de la dernière soumission
   void * source;
   getPDU_t source_getPDU;

   void * destination;          // L'objet auquel sont destinées les PDUs
   processPDU_t destProcessPDU; // La fonction permettant d'envoyer la PDU
};


/*
 * Création d'un multiplexeur
 */
struct muxfcfs_t * muxfcfs_create(void * destination,
			    processPDU_t destProcessPDU)
{
   struct muxfcfs_t * result = (struct muxfcfs_t *)sim_malloc(sizeof(struct muxfcfs_t));

   printf_debug(DEBUG_MUX, "IN\n");

   result->source = NULL;
   result->source_getPDU = NULL;
   result->destination = destination;
   result->destProcessPDU = destProcessPDU;

   printf_debug(DEBUG_MUX, "OUT\n");

   return result;
}

/**
 * @brief Demande d'une PDU par la destination
 */
struct PDU_t * muxfcfs_getPDU(void * vm)
{
   struct muxfcfs_t * mux = (struct muxfcfs_t *) vm;
   struct PDU_t * pdu;

   printf_debug(DEBUG_MUX, "IN\n");

   if ((mux->source == NULL) || (mux->source_getPDU == NULL)) {
      printf_debug(DEBUG_MUX, "pas de source, OUT\n");
      return NULL;
   }

   // On récupère
   pdu = mux->source_getPDU(mux->source);

   // On oublie
   mux->source = NULL;
   mux->source_getPDU = NULL;

   printf_debug(DEBUG_MUX, "OUT\n");

   // On donne !
   return pdu;
}

/**
 * @brief Soumission d'une PDU par une source
 */
int muxfcfs_processPDU(void * vm,
                       getPDU_t getPDU,
                       void * source)
{
   struct muxfcfs_t * mux = (struct muxfcfs_t *) vm;
   //   struct PDU_t * pdu;
   int result = 0;

   printf_debug(DEBUG_MUX, "IN\n");

   // Si c'est juste pour tester si je suis pret
   if ((getPDU == NULL) || (source == NULL)) {
      printf_debug(DEBUG_ALWAYS, "getPDU and source should now be non NULL\n");
      return (source == NULL);
   }

   // Si la précédente n'a pas été consommée, elle est détruite
   //   pdu = muxfcfs_getPDU(vm); // WARNING !?!?

   printf_debug(DEBUG_MUX, "on note la source\n");

   // On note l'origine
   mux->source = source;
   mux->source_getPDU = getPDU;

   // On prévient la destination
   if (mux->destProcessPDU && mux->destination) {
      printf_debug(DEBUG_MUX, "on previent la destination\n");
      result = mux->destProcessPDU(mux->destination, muxfcfs_getPDU, mux);
   }

   printf_debug(DEBUG_MUX, "OUT\n");
   return result;
}
