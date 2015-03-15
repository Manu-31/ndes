/**
 * @file pdu-source.c
 *
 * @brief Source générique de PDU.
 *
 *   Lorsqu'une PDU est produite, elle est fourni immédiatement à la
 *   destination. Si celle-ci ne la consomme pas, elle RESTE
 *   DISPONIBLE jusqu'à la production de la suivante.
 *
 * WARNING il reste des choses à rendre plus propres, en particulier
 * la gestion des sources "déterministes" (qui délivrent une séquence
 * pré établie). Il faudra définir un générateur aléatoire de type
 * séquence pour l'utiliser ici. 
 */
#include <stdlib.h>    // Malloc, NULL, exit...

#include <event.h>
#include <pdu-source.h>

#include <log.h>

/**
 * @brief Une source générique de PDU
 */
struct PDUSource_t {
   declareAsNdesObject;  //!< C'est un ndesObject
   struct dateGenerator_t * dateGen; //!< Le générateur de date de départ
   struct randomGenerator_t * sizeGen;//!< Le générateur de taille

   void * destination; //!< L'objet auquel sont destinées les PDUs
   processPDU_t destProcessPDU; //!< La fonction permettant de signaler la présence de la PDU

   struct probe_t *  PDUGenerationSizeProbe; //!< Une sonde sur la taille des PDU produites
   struct PDU_t * pdu; //!< La dernière PDU créée en cours d'émission
   struct PDU_t * nextPdu; //!< La dernière prochaine PDU à émettre

   struct dateSize * sequence; //!< Pour le cas déterministe
   int detNextIdx; //!< Prochain indice dans le cas déterministe
};

struct ndesObjectType_t PDUSourceType = {
  ndesObjectTypeDefaultValues(PDUSource)
};

/**
 * @brief Définition des fonctions spécifiques liées au ndesObject
 */
defineObjectFunctions(PDUSource);

struct PDUSource_t * PDUSource_create(struct dateGenerator_t * dateGen,
				      void * destination,
				      processPDU_t destProcessPDU)
{
   struct PDUSource_t * result = (struct PDUSource_t *)
              sim_malloc(sizeof(struct PDUSource_t));

   ndesObjectInit(result, PDUSource);

   result->pdu = NULL;
   result->nextPdu = NULL;
   PDUSource_setDateGenerator(result, dateGen);
   result->destProcessPDU = destProcessPDU;
   result->destination = destination;
   result->sizeGen = NULL;
   result->PDUGenerationSizeProbe = NULL;

   // Pour les sources déterministes (à refaire un jour)
   result->sequence = NULL;
   result->detNextIdx = 0;

   // Ajout à la liste des choses à réinitialiser avant une prochaine simu
   motsim_addToResetList(result, (void (*)(void *))PDUSource_start);
   
   return result;
}
/**
 * @brief Change the date generator
 * @param src The PDUSource to modify
 * @param gen The new date generator
 * The previos date generator should be freed by the caller
 */
void PDUSource_setDateGenerator(struct PDUSource_t * src,
                                struct dateGenerator_t * dateGen)
{
   src->dateGen = dateGen;
}

/**
 * @brief Get access to the date generator
 * @param src The PDUSource to query
 * @result The date generator
 */
struct dateGenerator_t * PDUSource_getDateGenerator(struct PDUSource_t * src)
{
   return src->dateGen;
}

/**
 *  @brief Création d'un générateur déterministe
 * 
 *  @param sequence Un tableau de {date, size} définissant chaque PDU
 *  @param destination L'entité aval
 *  @param destProcessPDU La fonction de traitement de la destination
 *  @result Un pointeur sur la source créée/initialisée
 *
 *  Un tel générateur permet de définir explicitement la séquence des
 *  PDUs à générer. Cette séquence est définie par un tableau de
 *  couples {date, taille}. Le dernier élément de la liste doit être
 *  {0.0, 0}. Le tableur n'est pas copié, il ne doit donc pas être
 *  libéré tant que la source peut servir.
 */
struct PDUSource_t * PDUSource_createDeterministic(struct dateSize * sequence,
						   void * destination,
						   processPDU_t destProcessPDU)
{
   struct PDUSource_t * result = PDUSource_create(NULL,
						  destination,
						  destProcessPDU);
   printf_debug(DEBUG_SRC, "IN, dest=%p, proc=%p\n", destination, destProcessPDU);

   result->sizeGen = NULL; // Pour le moment, c'est implanté par
			   // quelques lignes spécifiques fondées sur
			   // l'absence de générateur de date, pas
			   // terrible mais vite fait !
   result->sequence = sequence;
   result->detNextIdx = 0;

   return result;

}

/**
 * @brief Création d'une source périodique
 * @param period période de génération des PDU
 * @param destination l'entité aval
 * @param destProcessPDU fonction de traitement des PDU par l'aval
 * @result une source périodique initialisée
 *
 * Création d'une source de PDU périodique. La période est fournie en
 * paramètre. La taille des PDU est nulle. La première PDU est
 * générée à la date d'invocation de PDUSource_start.
 */
struct PDUSource_t * PDUSource_createPeriodic(double period,
					      void * destination,
					      processPDU_t destProcessPDU)
{
   return PDUSource_createCBR(period, 0, destination, destProcessPDU);
}

/**
 * @brief Création d'une source CBR
 * @param period période de génération des PDU
 * @param size taille des PDU
 * @param destination l'entité aval
 * @param destProcessPDU fonction de traitement des PDU par l'aval
 * @result une source périodique initialisée
 *
 * Création d'une source de PDU CBR. La période est fournie en
 * paramètre ainsi que la taille des PDU. La première PDU est
 * générée à la date d'invocation de PDUSource_start.
 */
struct PDUSource_t * PDUSource_createCBR(double period,
                                         unsigned int size,
					 void * destination,
					 processPDU_t destProcessPDU)
{
   struct PDUSource_t * result = PDUSource_create(NULL,
						  destination,
						  destProcessPDU);

   PDUSource_setDateGenerator(result, dateGenerator_createPeriodic(period));
   PDUSource_setPDUSizeGenerator(result, randomGenerator_createUIntConstant(size));

   return result;   
}

/*
 * Positionnement d'une sonde sur la taille des PDUs produites. Toutes
 * les PDUs créées sont concernées, même si elles ne sont pas
 * récupérées par la destination.
 */
void PDUSource_addPDUGenerationSizeProbe(struct PDUSource_t * src, struct probe_t *  newProbe)
{
   probe_chain(newProbe, src->PDUGenerationSizeProbe);
   src->PDUGenerationSizeProbe = newProbe;
}

/*
 * Spécification du générateur de taille de PDU associé
 */
void PDUSource_setPDUSizeGenerator(struct PDUSource_t * src, struct randomGenerator_t * rg)
{
   src->sizeGen = rg;
}

/**
 * @brief émission de la PDU courante et création de la prochaine
 * 
 * C'est cette fonction qui émet la PDU courante. Elle doit donc être
 * invoquée au bon moment (au travers d'un événement qu'elle aura
 * elle-même créé, en fait !)
 * On crée ensuite la prochaine PDU à émettre et on détermine la date à
 * laquelle elle doit être transmise. C'es comme ça qu'on assure qu'on
 * sera invoqué au bon moment.
 * Pour amorcer le système, une première invocation est réalisée à
 * l'activation de la source.
 */
void PDUSource_buildNewPDU(struct PDUSource_t * source)
{
   motSimDate_t date;
   struct event_t * event;
   unsigned int size = 0; 

   printf_debug(DEBUG_SRC, " IN\n");

   // 1 - Emission de la nextPDU
   /////////////////////////////
   // Suppression de la PDU précédente si pas consommée
   if (source->pdu) {
      printf_debug(DEBUG_SRC, " Destruction de %d\n", PDU_id(source->pdu));
      PDU_free(source->pdu);
   }

   // La prochaine devient la nouvelle
   source->pdu = source->nextPdu;

   if (source->pdu) { // La première fois, c'est un coup à blanc
      // Transmission de la nouvelle PDU
      printf_debug(DEBUG_SRC, " PDU %d (size %u) sent at %6.3f\n",
                   PDU_id(source->pdu), PDU_size(source->pdu),motSim_getCurrentTime());

      // On enregistre dans la probe s'il y en a une
      if (source->PDUGenerationSizeProbe) {
         probe_sample(source->PDUGenerationSizeProbe, (double)PDU_size(source->pdu));
      }

      printf_debug(DEBUG_SRC, " COUCOU\n");
      // On passe la PDU au suivant  
      if ((source->destProcessPDU) && (source->destination)) {
         printf_debug(DEBUG_SRC, " On passe\n");
         // On logue cet événement
         ndesLog_logLineF(PDU_getObject(source->pdu),
                         "CREATED_BY %d", PDUSource_getObjectId(source));
         (void)source->destProcessPDU(source->destination,
                                     (getPDU_t)PDUSource_getPDU,
                                     source);
      }
   }

   // 2 - Creation de la nextPDU
   /////////////////////////////
   printf_debug(DEBUG_SRC, " building next PDU ...\n");

   // Gestion de la version "déterministe" par une valeur spéciale du
   // pointeur. Je n'aime pas ça, mais en attendant mieux, ...
   if (source->dateGen == NULL) {
      printf_debug(DEBUG_SRC, " deterministic source\n");
      printf_debug(DEBUG_SRC, " next idx is  %d\n", source->detNextIdx);

      date = source->sequence[source->detNextIdx].date;
      size = source->sequence[source->detNextIdx].size;

      source->detNextIdx++; // Si la prochaine a une date nulle, alors
			    // l'événement ne sera pas créé, donc pas
			    // la peine de prendre des précautions, on
			    // ne viendra pas incrémenter un coup de
			    // trop si on fait bien attention à finir
			    // la liste par une date nulle.

   } else {
      // On détermine la date de prochaine transmission
      date = dateGenerator_nextDate(source->dateGen);
      // On choisi la taille
      size = source->sizeGen?randomGenerator_getNextUInt(source->sizeGen):0;
   }

   printf_debug(DEBUG_SRC, " next PDU to be sent at %f\n", date);

   // 3 - Création de la PDU et programmation de sa transmission si la
   // date n'est pas dépassée. Si elle l'est, la source s'arrète
   if (date >= motSim_getCurrentTime()) {
      // Création de la prochaine PDU
      source->nextPdu = PDU_create(size, NULL); 

      printf_debug(DEBUG_SRC, " next PDU %d (size %u) created at %6.3f\n",  
                PDU_id(source->nextPdu), size,motSim_getCurrentTime());

      // On crée un événement
      event = event_create((eventAction_t)PDUSource_buildNewPDU, source);

      // On ajoute cet événement au simulateur  pour cette date
      motSim_scheduleEvent(event, date);
   } else {
     printf_debug(DEBUG_SRC, " Aborted (too late) !\n");
   }
   printf_debug(DEBUG_SRC, " OUT\n");
}

/*
 * The function used by the destination to actually get the next PDU
 */
struct PDU_t * PDUSource_getPDU(void * src)
{
   struct PDUSource_t * source = (struct PDUSource_t *)src;
   struct PDU_t * pdu = source->pdu;

   source->pdu = NULL;

   //   if (PDUSource_getName(source) != NULL)
      printf_debug(DEBUG_SRC, "'%s' releasing PDU %d (size %d)\n",
		PDUSource_getObjectName(source),
		PDU_id(pdu),
		PDU_size(pdu));

   ndesLog_logLineF(PDU_getObject(pdu), "OUT %d", PDUSource_getObjectId(source));

   return pdu;
}

/**
 * @brief Démarrage d'une source dans le cadre d'un simulateur
 *
 * A partir de cet instant, elle peut produire des PDUs.
 */
void PDUSource_start(struct PDUSource_t * source)
{
   // On ne sait jamais (cette fonction sert de reset)
   if (source->pdu) {
      PDU_free(source->pdu);
      source->pdu = NULL;
   }

   // On lance la machine
   PDUSource_buildNewPDU(source);
}

