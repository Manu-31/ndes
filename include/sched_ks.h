/*
 *   Ordonnancement par un algorithme de r�solution du sac � dos.
 *
 *   Cet ordonnanceur est associ� � une liaison de type DVB-S2 caract�ris�e
 * par un certain nombre de MODCOD. Il consomme des paquets plac�s dans des
 * files. Un ensemble de files est associ� � un MODCOD. Chaque file est
 * ensuite caract�ris�e par une fonction d'utilit�.
 */
#ifndef __SCHED_BACKSACK
#define __SCHED_BACKSACK

#include <schedACM.h>

#define NB_SOUS_CAS_MAX 65536

struct sched_kse_t;

/*
 * Cr�ation d'un scheduler avec sa "destination". Cette derni�re doit
 * �tre de type struct DVBS2ll_t  et avoir d�j� �t� compl�tement
 * construite (tous les MODCODS cr��s).
 * Le nombre de files de QoS diff�rentes par MODCOD est �galement
 * pass� en param�tre.
 * Si exhaustif == 1 alors tous les cas sont envisages, ce qui peut
 * faire beaucoup. Sinon, pour une taille donn�e, on ne poursuit que
 * la meilleure piste.
 */
struct schedACM_t * sched_kse_create(struct DVBS2ll_t * dvbs2ll, int nbQoS, int declOK, int exhaustif);


#endif
