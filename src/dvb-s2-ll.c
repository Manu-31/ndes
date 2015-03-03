/*----------------------------------------------------------------------*/
/* @file dvb-s2-ll.h  */
/* @brief Implantation de la couche liaison DVB-S2.                       */
/*                                                                      */
/*  ATENTION : propagation à voir !!! Ce qui est fait dans endPropag    */
/*  devrait entre dans end transmission et un autre object (genre file) */
/*  pourrait alors gérer la progpatation                                */
/*----------------------------------------------------------------------*/
#include <stdlib.h>    // Malloc, NULL, exit...
#include <assert.h>

#include <motsim.h>
#include <event.h>

#include <dvb-s2-ll.h>

/*
 * Caractérisation d'un MODCOD
 */
struct t_modcod {
   unsigned int      bitLength;      // (codage) Taille de la charge utile
   unsigned int      bitsPerSymbol;  // (modulation) Valence
   struct probe_t *  actualPayloadBitSizeProbe;
};


/*
 * Les MODCODs seront classés dans l'ordre croissant de la capacité
 * donc dans l'ordre décroissant de la robustesse. On peut donc déclasser
 * un paquet de j vers i si i < j
 */
struct DVBS2ll_t {
   unsigned int      FECFrameBitLength; // Taille de la FECFRAME
   unsigned long     symbolPerSecond;
   int               nbModCods;
   struct t_modcod   modcod[NB_MODCOD_MAX];
   int               available; // Le support est-il libre ?

   // Description de la destination
   void * destination; // L'objet auquel sont destinées les PDUs
   processPDU_t send;  // La fonction permettant d'envoyer la PDU

   // Description de la source
   void * source;      // L'objet en question
   getPDU_t getPDU;    // La méthode de récupération des PDU

   struct PDU_t   *  currentPDU; // La PDU en cours d'emission

   // Une probe de datage des DUMMY
   struct probe_t *  dummyFecFrameProbe;
};

/*
 * Réinitalisation. On se remet dans un état connu
 */
void DVBS2ll_reset(struct DVBS2ll_t * dvbs2ll)
{
   if (dvbs2ll->currentPDU) {
      PDU_free(dvbs2ll->currentPDU);
      dvbs2ll->currentPDU = NULL;
   }
   dvbs2ll->available = 1;
}

/*
 * Création d'une entité DVB-S2 couche 2. Attention, elle ne contient
 * aucun MODCOD par défaut, il faut en ajouter.
 * 
 * Le débit est donnée en symboles/seconde
 */
struct DVBS2ll_t * DVBS2ll_create(void * destination,
				  processPDU_t destProcessPDU,
				  unsigned long symbolPerSecond,
				  unsigned int FECFrameBitLength)
{
   struct DVBS2ll_t * result = (struct DVBS2ll_t * )sim_malloc(sizeof(struct DVBS2ll_t));
 
   if (result) {
      result->nbModCods = 0;
      result->destination = destination;
      result->send = destProcessPDU;
      result->symbolPerSecond = symbolPerSecond;
      result->FECFrameBitLength = FECFrameBitLength;
      result->source = NULL;
      result->getPDU = NULL;
      result->dummyFecFrameProbe = NULL;
      result->available = 1;
 
      // Ajout à la liste des choses à réinitialiser avant une prochaine simu
      motsim_addToResetList(result, (void (*)(void *))DVBS2ll_reset);

      printf_debug(DEBUG_DVB, "%p created\n", result);
   }
   return result;
};

/*
 * Attribution d'une source. Attention c'est obligatoire ici car c'est
 * l'entité DVBS2ll qui va solliciter la source lorsque le support
 * sera libre
 */
void DVBS2ll_setSource(struct DVBS2ll_t * dvbs2ll, void * source, getPDU_t getPDU)
{
   dvbs2ll->source = source;
   dvbs2ll->getPDU = getPDU;
}

/*
 * Ajout d'un MODCOD. Le codage est paramétré par le nombre de bits
 * par BBFRAME et la modulation par le nombre de bits par symbole.
 * La valeur retournée est l'indice de ce nouveau MODCOD.
 * 
 * WARNING  il serait bon de cacher la taille et de ne montrer que le codage
 */
int DVBS2ll_addModcod(struct DVBS2ll_t * dvbs2ll, unsigned int bbframeBitLength, unsigned int bitsPerSymbol)
{
   int n;

   assert(dvbs2ll->nbModCods < NB_MODCOD_MAX);

   n = dvbs2ll->nbModCods;

   dvbs2ll->nbModCods++;
   DVBS2ll_setModcod(dvbs2ll, n, bbframeBitLength, bitsPerSymbol);

   return n;
}

/*
 * Modification des propriétés du MODCOD n
 */
void DVBS2ll_setModcod(struct DVBS2ll_t * dvbs2ll,
                       int n,
		       unsigned int bbframeBitLength,
		       unsigned int bitsPerSymbol)
{
   assert(n>=0);
   assert(n<dvbs2ll->nbModCods);

   // Les MODCODs doivent être "ordonnés" (pour le scheduler avec
   // déclassement, pas génial, il vaudrait mieux que l'algo vérifie
   // mais ce serait plus lourd !)
   if (n>0) {
     assert(bbframeBitLength >= dvbs2ll->modcod[n-1].bitLength);
     assert(bitsPerSymbol >= dvbs2ll->modcod[n-1].bitsPerSymbol);
   }

   if (n<dvbs2ll->nbModCods-1) {
     assert(bbframeBitLength <= dvbs2ll->modcod[n+1].bitLength);
     assert(bitsPerSymbol <= dvbs2ll->modcod[n+1].bitsPerSymbol);
   }


   dvbs2ll->modcod[n].bitLength = bbframeBitLength;
   dvbs2ll->modcod[n].bitsPerSymbol = bitsPerSymbol;
   dvbs2ll->modcod[n].actualPayloadBitSizeProbe = NULL;

}

/*
 * Nombre de MODCODs
 */
int DVBS2ll_nbModcod(struct DVBS2ll_t * dvbs2ll)
{
   return dvbs2ll->nbModCods;
}

/*
 * Capacité d'une BBFRAME associée au MODCOD d'indice fourni
 */ 
unsigned int DVBS2ll_bbframePayloadBitSize(struct DVBS2ll_t * dvbs2ll, int mcIdx)
{
   return dvbs2ll->modcod[mcIdx].bitLength;
}

/*
 * Nombre de bits par symbole d'un modcod
 */
unsigned int DVBS2ll_bitsPerSymbol(struct DVBS2ll_t * dvbs2ll, int mcIdx)
{
   return dvbs2ll->modcod[mcIdx].bitsPerSymbol;
}

/*
 * Temps d'émission d'une BBFRAME associée au MODCOD d'indice fourni
 * Si l'indice n'est pas celui d'un MODCOD géré, le temps donné sera
 * celui de l'émission d'une DUMMY PLFRAME
 */ 
double DVBS2ll_bbframeTransmissionTime(struct DVBS2ll_t * dvbs2ll, int mcIdx)
{
  if (mcIdx < dvbs2ll->nbModCods) {
     return (double) dvbs2ll->FECFrameBitLength / ((double)dvbs2ll->modcod[mcIdx].bitsPerSymbol *(double) dvbs2ll->symbolPerSecond);
  } else {   // Sending a DUMMY PLFRAME
     return 36.0* 90.0 / (double) dvbs2ll->symbolPerSecond;
  }
}

/**
 * @brief Ajout d'une sonde sur la taille de la charge utile des trames émises
 * sur un MODCOD donné
 */
void DVBS2ll_addActualPayloadBitSizeProbe(struct DVBS2ll_t * dvbs2ll, int mc, struct probe_t * pr)
{
   assert(mc < dvbs2ll->nbModCods);

   dvbs2ll->modcod[mc].actualPayloadBitSizeProbe = probe_chain(pr, dvbs2ll->modcod[mc].actualPayloadBitSizeProbe);
}

/**
 * @brief Ajout de la probe sur les DUMMY
 */
void DVBS2ll_addDummyFecFrameProbe(struct DVBS2ll_t * dvbs2ll, struct probe_t * pr)
{
  dvbs2ll->dummyFecFrameProbe = probe_chain(pr, dvbs2ll->dummyFecFrameProbe);
}

/*
 * The function used by the destination to actually get the next PDU
 */
struct PDU_t * DVBS2ll_getPDU(struct DVBS2ll_t * dvbs2ll)
{
   struct PDU_t * pdu = dvbs2ll->currentPDU;

   dvbs2ll->currentPDU = NULL;

   printf_debug(DEBUG_SRC, "releasing PDU %d (size %d)\n", PDU_id(pdu), PDU_size(pdu));

   return pdu;
}

/*
 * Evenement de fin d'emission
 */
void DVBS2ll_endTransmission(struct DVBS2ll_t * dvbs2ll)
{
   struct PDU_t * pdu;

   printf_debug(DEBUG_DVB, "t=%f\n",
		motSim_getCurrentTime());

   // Si PDU != NULL, on passe la PDU à la destination
   if (dvbs2ll->currentPDU) {
      dvbs2ll->send(dvbs2ll->destination, (getPDU_t)DVBS2ll_getPDU, dvbs2ll);
   }//  (sinon c'est une DUMMY, on n'en fait rien)

   // On est pret à remettre le couvert ...
   dvbs2ll->available = 1;
   // On demande une BBFRAME à la source ...
   pdu = dvbs2ll->getPDU(dvbs2ll->source);

   // ... et on la transmet (s'il n'y en a pas, pdu == NULL et c'est
   // une DUMMY qui est émise).
   DVBS2ll_sendPDU(dvbs2ll, pdu);
}

/*
 * Fin de propagation
 */
void  DVBS2ll_endPropagation(struct DVBS2ll_t * dvbs2ll)
{
   printf_debug(DEBUG_DVB, "t=%f\n",
		motSim_getCurrentTime());

}

/*
 * Emission d'une PDU au travers d'un MODCOD sélectionné. La PDU doit
 * être d'une taille inférieure ou égale à la taille de charge utile du
 * MODCOD choisi.
 * 
 * L'indice du MODCOD a utiliser est passe dans le champ prive de la PDU
 */
void DVBS2ll_sendPDU(struct DVBS2ll_t * dvbs2ll, struct PDU_t * pdu)
{
   double transmissionTime;
   //   double propagationTime = 0.0; // WARNING on ne peut pas mettre plus, on ne sait pas garder plus d'une PDU !!!

   int mc = dvbs2ll->nbModCods; // Représente une DUMMY PLFRAME
   unsigned int bitLength;
   //   unsigned int bitsPerSymbol;

   // On doit être dispo
   assert(dvbs2ll->available);

   // Le cas pdu == NULL est toléré et correspond à une
   // demande d'émission d'une trame de bourage (DUMMY PLFRAME)
   if (pdu) {
     mc  = (int)(long)PDU_private(pdu);
  
      bitLength = dvbs2ll->modcod[mc].bitLength;
      //bitsPerSymbol = dvbs2ll->modcod[mc].bitsPerSymbol;

      // Les tailles de PDU sont en octets, celle du DVB en bits
      assert(8*PDU_size(pdu) <= bitLength);

      if (dvbs2ll->modcod[mc].actualPayloadBitSizeProbe) {
         probe_sample(dvbs2ll->modcod[mc].actualPayloadBitSizeProbe, 8.0*(double)PDU_size(pdu));
      }
   } else { // On insère une DUMMY 
      mc = dvbs2ll->nbModCods; // Représente une DUMMY PLFRAME,
      if( dvbs2ll->dummyFecFrameProbe) {
	 probe_sampleEvent(dvbs2ll->dummyFecFrameProbe);
      }
   }

   dvbs2ll->available = 0;

   if (dvbs2ll->currentPDU) {
      PDU_free(dvbs2ll->currentPDU);
   }
   dvbs2ll->currentPDU = pdu;

   transmissionTime = DVBS2ll_bbframeTransmissionTime(dvbs2ll, mc);

   printf_debug(DEBUG_DVB, "t=%f : size %u/%u (BYTES) Modcod %d, tt = %f ms\n",
		motSim_getCurrentTime(), pdu?PDU_size(pdu):0, bitLength/8, mc, transmissionTime * 1000.0);

   /* Au bout d'un temps d'émission, le support est libre */
   motSim_scheduleNewEvent((eventAction_t)DVBS2ll_endTransmission, dvbs2ll,
			 motSim_getCurrentTime() + transmissionTime);

   /* Au bout d'un temps d'émission plus propagation, le récepteur reçoit */
   //   motSim_insertNewEvent((eventAction_t)DVBS2ll_endPropagation, dvbs2ll,
   //			 motSim_getCurrentTime() + propagationTime);
}

/*
 * Le support est-il disponible ?
 */
int DVBS2ll_available(struct DVBS2ll_t * dvbs2ll)
{
   printf_debug(DEBUG_DVB, "am i (%p) available ?\n", dvbs2ll);
   return dvbs2ll->available;
}

/*
 * Fonction invoquée pour fournir une nouvelle PDU
 */
int DVBS2ll_processPDU(struct DVBS2ll_t * dvbs2ll,
                        getPDU_t getPDU,
                        void * source)
{
   struct PDU_t * pdu;

   // Si c'est juste pour tester si je suis pret
   if ((getPDU == NULL) || (source == NULL)) {
      printf_debug(DEBUG_ALWAYS, "getPDU/source should not be NULL\n");
      return DVBS2ll_available(dvbs2ll); // WARNING il vaudrait mieux un DVBS2ll_processPDU
   }

   // Si on n'est pas dispo, on ne fait rien !
   // On reviendra voir plus tard (à la fin de la transmission) et
   // tant pis si on ne trouve plus rien !!!
   if (DVBS2ll_available(dvbs2ll)) {
      assert(getPDU != NULL);
      assert(source != NULL);

      pdu = getPDU(source);
  
      DVBS2ll_sendPDU(dvbs2ll, pdu);
   }
   return 1;
}
