/*
 * Un "multiplexeur FCFS" permet d'associer une destination unique �
 * un nombre quelconque de sources.
 * 
 * Les PDUs soumises par les sources sont transmises imm�diatement �
 * la destination dans l'ordre o� elles arrivent.
 */

#include <stdlib.h>    // Malloc, NULL, exit...

#include <motsim.h>
#include <muxfcfs.h>

struct muxfcfs_t {

   // Sauvegarde de l'origine de la derni�re soumission
   void * source;
   getPDU_t source_getPDU;

   void * destination;          // L'objet auquel sont destin�es les PDUs
   processPDU_t destProcessPDU; // La fonction permettant d'envoyer la PDU
};


/*
 * Cr�ation d'un multiplexeur
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

/*
 * Demande d'une PDU par la destination
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

   // On r�cup�re
   pdu = mux->source_getPDU(mux->source);

   // On oublie
   mux->source = NULL;
   mux->source_getPDU = NULL;

   printf_debug(DEBUG_MUX, "OUT\n");

   // On donne !
   return pdu;
}

/*
 * Soumission d'une PDU par une source
 */
void muxfcfs_processPDU(void * vm,
                   getPDU_t getPDU,
                   void * source)
{
   struct muxfcfs_t * mux = (struct muxfcfs_t *) vm;
   struct PDU_t * pdu;

   printf_debug(DEBUG_MUX, "IN\n");

   // Si la pr�c�dente n'a pas �t� consomm�e, elle est d�truite
   pdu = muxfcfs_getPDU(vm);

   printf_debug(DEBUG_MUX, "on note la source\n");

   // On note l'origine
   mux->source = source;
   mux->source_getPDU = getPDU;


   // On pr�vient la destination
   if (mux->destProcessPDU && mux->destination) {
      printf_debug(DEBUG_MUX, "on previent la destination\n");
      mux->destProcessPDU(mux->destination, muxfcfs_getPDU, mux);
   }

   printf_debug(DEBUG_MUX, "OUT\n");

}
