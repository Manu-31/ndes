/*
 * Les g�n�rateurs de dates. Largement fond� sur les g�n�rateurs de
 * nombres al�atoires.
 * Toutes les dates sont toujours exprim�es en secondes depuis le d�but de
 * la simulation.
 */

#ifndef __DEF_DATE_GENERATOR
#define __DEF_DATE_GENERATOR

#include <probe.h>

struct dateGenerator_t;

struct dateGenerator_t * dateGenerator_create();

/*
 * Ajout d'une sonde sur les inter-arrivees
 */
void dateGenerator_setInterArrivalProbe(struct dateGenerator_t * dateGen,
					struct probe_t * probe);

/*
 * Generation de la prochaine date
 */
double dateGenerator_nextDate(struct dateGenerator_t * dateGen,
			      double currentTime);

/*
 * Creation d'une source qui genere des evenements a interrarivee
 * exponentielle.
 */
struct dateGenerator_t * dateGenerator_createExp(double lambda);

/*
 * Modification du param�tre lambda
 */
void dateGenerator_setLambda(struct dateGenerator_t * dateGen, double lambda);

/*
 * Creation d'une source qui genere des evenements a interrarivee
 * constante. Bref, p�riodiques !
 */
struct dateGenerator_t * dateGenerator_createPeriodic(double period);

/*
 * Prepare for record values in order to replay on each reset
 */
void dateGenerator_recordThenReplay(struct dateGenerator_t *  d);

#endif
