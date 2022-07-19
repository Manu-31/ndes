/** 
 * @file sched_rr.c
 * @brief Un ordonnanceur round robin élémentaire
 */

#include <sched_rr.h>

/**
 * Structure définissant notre ordonanceur
 */
struct rrSched_t {
   //! La destination (typiquement un lien)
   void         * destination;
   //! Fonction de réception de la destination
   processPDU_t   destProcessPDU;

   //! Nombre de sources (files d'entrée)
   int        nbSources;
   //! Les sources (files d'entrée)
   void     * sources[SCHED_RR_NB_INPUT_MAX];
   //! Fonctions d'émission des souces
   getPDU_t   srcGetPDU[SCHED_RR_NB_INPUT_MAX];

   //! La dernière source servie par le tourniquet
   int lastServed;
};

/**
 * Création d'une instance de l'ordonnanceur avec la destination en
 * paramètre 
 * @param destination l'entité aval (un lien)
 * @param destProcessPDU la fonction de réception de la destination
 * @result la structure allouée et initialisée
 */
struct rrSched_t * rrSched_create(void * destination,
				  processPDU_t destProcessPDU)
{
   struct rrSched_t * result = (struct rrSched_t * )sim_malloc(sizeof(struct rrSched_t));

   // Gestion de la destination
   result->destination = destination;
   result->destProcessPDU = destProcessPDU;

   // Pas de source définie
   result->nbSources = 0;

   // On commence quelquepart ...
   result->lastServed = 0;

   return result;
}

/*
 * Ajout d'une source (ce sera par exemple une file)
 */
void rrSched_addSource(struct rrSched_t * sched,
		       void * source,
		       getPDU_t getPDU)
{
   assert(sched->nbSources < SCHED_RR_NB_INPUT_MAX);

   sched->sources[sched->nbSources] = source;
   sched->srcGetPDU[sched->nbSources++] = getPDU;
}

/**
 * @brief : La fonction permettant de demander une PDU à notre scheduler
 * C'est ici qu'est implanté l'algorithme
 */
struct PDU_t * rrSched_getPDU(void * s)
{
   struct rrSched_t * sched = (struct rrSched_t * )s;
   struct PDU_t * result = NULL;

   assert(sched->nbSources > 0);

   int next = sched->lastServed;

   // Quelle est la prochaine source à servir ?
   do {
      // On cherche depuis la prochaine la première source qui a des
      // choses à nous donner
      next = (next + 1)%sched->nbSources;
      result = sched->srcGetPDU[next](sched->sources[next]);
   } while ((result == NULL) && (next != sched->lastServed));

   if (result)
      sched->lastServed = next;

   return result;
}

/*
 * La fonction de soumission d'un paquet à notre ordonnanceur
 */
int rrSched_processPDU(void *s,
		       getPDU_t getPDU,
		       void * source)
{
   int result;
   struct rrSched_t * sched = (struct rrSched_t *)s;

   printf_debug(DEBUG_SCHED, "in\n");

   result = sched->destProcessPDU(sched->destination, rrSched_getPDU, sched);

   printf_debug(DEBUG_SCHED, "out %d\n", result);

   return result;
}
