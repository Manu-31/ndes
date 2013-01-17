/*
 *   Ordonnancement par un algorithme simple fond� sur des fnctions d'utilit� 
 *
 */
#ifndef __SCHED_UTILITY
#define __SCHED_UTILITY

#include <schedACM.h>

/*
 * Cr�ation d'un scheduler avec sa "destination". Cette derni�re doit
 * �tre de type struct DVBS2ll_t  et avoir d�j� �t� compl�tement
 * construite (tous les MODCODS cr��s).
 * Le nombre de files de QoS diff�rentes par MODCOD est �galement
 * pass� en param�tre.
 */
struct schedACM_t * schedUtility_create(struct DVBS2ll_t * dvbs2ll, int nbQoS, int declOK);


#endif
