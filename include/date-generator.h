/**
 * @file date-generator.h
 * @brief Les générateurs de dates
 *
 * Ils sont largement fondés sur les générateurs de
 * nombres aléatoires.
 * Toutes les dates sont toujours exprimées en secondes depuis le début de
 * la simulation.
 *
 * WARNING : supprimer dateGenerator_create() qui pose problème avec
 * l'appel à setStartDate()
 */

#ifndef __DEF_DATE_GENERATOR
#define __DEF_DATE_GENERATOR

#include <probe.h>
#include <random-generator.h>

struct dateGenerator_t;

/**
 * @brief Creation of a date generator
 * @result a struct dateGenerator_t * 
 *
 * The created dateGenerator is unusable for now. It needs to be
 * associated to a random generator
 */
struct dateGenerator_t * dateGenerator_create();

/**
 * Creation d'une source qui genere des evenements a interrarivee
 * exponentielle.
 */
struct dateGenerator_t * dateGenerator_createExp(double lambda);

/**
 * Creation d'une source qui genere des evenements a interrarivee
 * constante. Bref, périodiques !
 */
struct dateGenerator_t * dateGenerator_createPeriodic(double period);

/**
 * @brief Choix du générateur aléatoire des durées entre dates
 *
 * @param dateGen le générateur à modifier
 * @param randGen le générateur de date à affecter
 */
void dateGenerator_setRandomGenerator(struct dateGenerator_t * dateGen,
				      struct randomGenerator_t * randGen);

/**
 * @brief Ajout d'une sonde sur les inter-arrivees
 *
 * @param dateGen le générateur à modifier
 * @param probe la sonde à affecter
 */
void dateGenerator_addInterArrivalProbe(struct dateGenerator_t * dateGen,
					struct probe_t * probe);

/**
 * @brief Obtention de la prochaine date
 * @param dateGen le générateur à utiliser
 */
double dateGenerator_nextDate(struct dateGenerator_t * dateGen);

/**
 * @brief Choix de la date de démarrage
 *
 * Les dates seront générées à partir de la date passée en
 * paramètre. Par défaut, c'est la date d'attribution du
 * randomGenerator qui en fait office, c'est-à-dire 0.0 si c'est fait
 * avant de lancer la simulation.
 */
void dateGenerator_setStartDate(struct dateGenerator_t * dateGen,
				motSimDate_t date);

/*
 * Modification du paramètre lambda
 */
void dateGenerator_setLambda(struct dateGenerator_t * dateGen, double lambda);

/*
 * Prepare for record values in order to replay on each reset
 */
void dateGenerator_recordThenReplay(struct dateGenerator_t *  d);

/**
 * @brief Is this a periodic source ?
 * @param d a date generator
 * @result non null if d is periodic
 */
int dateGenerator_isPeriodic(struct dateGenerator_t *  d);

#endif
