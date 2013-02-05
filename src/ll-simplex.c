#include <motsim.h>
#include <ll-simplex.h>
#include <file-pdu.h>
/*
 * Caract�ristiques d'une couche liaison simplex
 */
struct llSimplex_t {
   // Les caract�risques
   unsigned long throughput; // D�bit en bits/s
   double propagation;       // Temps de propagation en secondes

   // L'�tat
   int idle; //  Pret � �mettre ou pas

   struct PDU_t     * pdu; // La PDU en cours d'�mission
   struct filePDU_t * flyingPDUs;  // Les PDUs "en vol"

   // L'entit� aval
   void * destination;
   processPDU_t destProcessPDU;
};

/*
 * Cr�ation d'une entit�. Les deux param�tres importants sont le
 * d�bits (en bits/s) et le temps de propagation (en secondes).
 */
struct llSimplex_t * llSimplex_create(void * destination,
				      processPDU_t destProcessPDU,
				      unsigned long throughput,
				      double propagation)
{
   struct llSimplex_t * result = (struct llSimplex_t *) sim_malloc(sieof(struct llSimplex_t));

   result->destination = destination;
   result->destProcessPDU = destProcessPDU;
   result->throughput = throughput;
   result->propagation = propagation;

   return result;
}

/*
 * Destruction
 */
void llSimplex_delete(struct llSimplex_t * lls)
{
   motSim_error(MS_WARN, "Who cares ?");
}

/*
 * Gestion d'une PDU soumise par l'amont
 */
int llSimplex_processPDU(struct llSimplex_t * lls,
			 getPDU_t getPDU,
			 void * source)
{
   struct PDU_t * pdu;

   // Si c'est juste pour tester si je suis pret
   if ((getPDU == NULL) || (source == NULL)) {
      return lls->idle;
   }

   // C'est pas pour tester, il y a une PDU � traiter
   pdu = getPDU(source);

   assert(pdu != NULL);

   if (lls->idle) {   // Si je suis pret, je traite
      lls->idle = 0;   // Je ne suis plus pret !
      lls->pdu = pdu;
      event_add(llSimplex_endOfTransmission,
		lls,
		motSim_getCurrentTime() + PDU_size(pdu)*8.0/lls->throughput);
      return 1;
   } else {  // Sinon, tant pis !
      pdu_free(pdu);
      return 0;
   }
}

/*
 * Fin du temps d'�mission
 */
void llSimplex_endOfTransmission(void * l)
{
   struct llSimplex_t * lls = (struct llSimplex_t *)l;

   // La PDU est en l'air !
   filePDU_insert(lls->flyingPDUs, lls->pdu);

   // Elle est partie !
   lls->pdu = NULL;

   // On est dispo du coup !
   lls->idle = 1;

   // On va voir en amont ...
   lls->source(lls->source,...
}

/*
 * Fourniture d'une PDU en aval
 */
struct PDU_t * llSimplex_getPDU(struct llSimplex_t * lls)
{
vcouco
}

