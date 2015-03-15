/**
 * @file sched_drr.h
 * @brief Définition d'un ordonnanceur Deficit Round Robin
 *
 * En cours de r�alisation, donc pas utilisable pour le moment
 */
#ifndef __SCHED_DEFICIT_ROUND_ROBIN
#define __SCHED_DEFICIT_ROUND_ROBIN

#include <pdu.h>
#include <ndesObject.h>

/**
 * Nombre maximal d'entrées
 */
#define SCHED_DRR_NB_INPUT_MAX 8

/**
 * Structure définissant notre ordonanceur
 */
struct schedDRR_t ;

/**
 * Création d'une instance de l'ordonnanceur avec la destination en
 * paramètre 
 */
struct schedDRR_t * schedDRR_create(void * destination,
				    processPDU_t destProcessPDU);

/**
 * Ajout d'une source (ce sera par exemple une file)
 */
void schedDRR_addSource(struct schedDRR_t * sched,
			unsigned long nbBitPerRound,
			void * source,
			getPDU_t getPDU);
/**
 * La fonction permettant de demander une PDU à notre scheduler
 * C'est ici qu'est implanté l'algorithme
 */
struct PDU_t * schedDRR_getPDU(void * s);

/**
 * La fonction de soumission d'un paquet à notre ordonnanceur
 */
int schedDRR_processPDU(void *s,
			getPDU_t getPDU,
			void * source);

/**
 * @brief schedDRR can also be seen as ndesObjects
 */
declareObjectFunctions(schedDRR);
#endif
