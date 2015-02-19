#include <motsim.h>
#include <event.h>
#include <ll-simplex.h>
#include <file_pdu.h>
/*
 * Caractéristiques d'une couche liaison simplex
 */
struct llSimplex_t {
   // Les caractérisques
   unsigned long throughput; // Débit en bits/s
   double propagation;       // Temps de propagation en secondes

   // L'état
   int idle; //  Pret à émettre ou pas

   struct PDU_t     * pdu; // La PDU en cours d'émission
   struct filePDU_t * flyingPDUs;  // Les PDUs "en vol"
   struct PDU_t     * pduOut; // Une PDU qui vient de sortir

   // L'entité aval
   void * destination;
   processPDU_t destProcessPDU;

   // La dernière entité amont à se manifester pendant qu'on est occupé
   getPDU_t lastGetPDU;
   void * lastSource;
};

/*
 * Création d'une entité. Les deux paramètres importants sont le
 * débits (en bits/s) et le temps de propagation (en secondes).
 */
struct llSimplex_t * llSimplex_create(void * destination,
				      processPDU_t destProcessPDU,
				      unsigned long throughput,
				      double propagation)
{
   struct llSimplex_t * result = (struct llSimplex_t *) sim_malloc(sizeof(struct llSimplex_t));

   result->destination = destination;
   result->destProcessPDU = destProcessPDU;
   result->throughput = throughput;
   result->propagation = propagation;
   result->lastSource = NULL;

   result->lastGetPDU = NULL;
   result->pduOut = NULL;
   result->flyingPDUs = filePDU_create(NULL, NULL);

   result->idle = 1;

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
 * Fin d'un temps de propagation
 */
void llSimplex_endOfPropagation(void * l)
{
   struct llSimplex_t * lls = (struct llSimplex_t *)l;

   printf_debug(DEBUG_PDU, "in\n");

   // On récupère la première PDU en vol
   lls->pduOut = filePDU_extract(lls->flyingPDUs);
   printf_debug(DEBUG_PDU, "on recupere la PDU %d\n", PDU_id(lls->pduOut));


   // On la donne au destinataire
   lls->destProcessPDU(lls->destination, llSimplex_getPDU, lls);

   // S'il ne l'a pas prise tout de suite, elle est perdue !
   PDU_free(lls->pduOut);

   printf_debug(DEBUG_PDU, "out");
}

/*
 * Fin du temps d'émission
 */
void llSimplex_endOfTransmission(void * l)
{
   struct llSimplex_t * lls = (struct llSimplex_t *)l;

   printf_debug(DEBUG_PDU, "in\n");

   // La PDU est en l'air !
   filePDU_insert(lls->flyingPDUs, lls->pdu);

   // On est dispo du coup !
   lls->idle = 1;

   // Elle est partie !
   lls->pdu = NULL;

   // On prépare son arrivée
   printf_debug(DEBUG_PDU, "On prepare la fin de propagation a %lf\n", motSim_getCurrentTime() + lls->propagation);
   event_add(llSimplex_endOfPropagation,
	     l,
	     motSim_getCurrentTime() + lls->propagation);

   // On va voir en amont si par hasard une nouvelle PDU n'attend pas ...
   if ((lls->lastSource) && (lls->lastGetPDU)){
      (void)llSimplex_processPDU(lls, lls->lastGetPDU, lls->lastSource); 
   }
   printf_debug(DEBUG_PDU, "out\n");
}

/*
 * Gestion d'une PDU soumise par l'amont
 */
int llSimplex_processPDU(void * l,
			 getPDU_t getPDU,
			 void * source)
{
   int result = 0;

   struct llSimplex_t * lls = (struct llSimplex_t * ) l;
   struct PDU_t * pdu;

   printf_debug(DEBUG_PDU, "in\n"); 

   // Si c'est juste pour tester si je suis pret
   if ((getPDU == NULL) || (source == NULL)) {
      result = lls->idle;
   } else {

      if (lls->idle) {   // Si je suis pret, je traite
         // C'est pas pour tester, il y a une PDU à traiter
         pdu = getPDU(source);
         if (pdu) {
            assert(pdu != NULL);
            lls->idle = 0;   // Je ne suis plus pret !
            lls->pdu = pdu; 
            printf_debug(DEBUG_PDU, "On prepare la fin de transmission a %lf\n",
		      motSim_getCurrentTime() + PDU_size(pdu)*8.0/lls->throughput);
            event_add(llSimplex_endOfTransmission,
   		l, 
		motSim_getCurrentTime() + PDU_size(pdu)*8.0/lls->throughput);
	 } else {
            lls->lastSource = NULL;
            lls->lastGetPDU = NULL;
	 }
         result =  1;
      } else {  // Sinon, je me le note pour la fin de transmission
         lls->lastSource = source;
         lls->lastGetPDU = getPDU;
         result = 0;
      }
   }
   printf_debug(DEBUG_PDU, "out %d\n", result); 

   return result;
}

/*
 * Fourniture d'une PDU en aval
 */
struct PDU_t * llSimplex_getPDU(void * l)
{
   struct llSimplex_t * lls = (struct llSimplex_t *)l;

   struct PDU_t * result = lls->pduOut;

   lls->pduOut = NULL ; // Ce n'est plus à nous de la gérer

   return result;
}

