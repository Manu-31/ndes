/*
 *   Le moteur de simulation.
 * 
 *   Puisqu'il n'y aura jamais qu'un seul moteur de simulation et
 * qu'il faut le passer en param�tre � de nombreuses fonctions qui n'en
 * ont pas directement besoin mais qui vont elles m�mes le passer � 
 * d'autres fonctions pour finalement qu'il soit utilis� par les fonctions
 * qui g�n�rent des �v�nements, ... bref une variable globale est cr��e
 * ici et sera utilis�e lorsque n�cessaire. Ce n'est pas tr�s glorieux
 * mais �a simplifie tellement.
 *
 *   A voir : rendre tout ca multithreadable !
 *
 *   Attention, il faut tout de m�me l'initialiser ! 
 */
#ifndef __DEF_MOTSIM
#define __DEF_MOTSIM

#include <assert.h>
#include <stdlib.h>  // malloc


typedef double  motSimDate_t;

struct motsim_t;
struct event_t;

extern struct motsim_t * __motSim;

/*
 * Initialisation du syst�me
 */
void motSim_create(); 

/*
 * A la fin d'une simulation, certains objets ont besoin d'�tre
 * r�initialiser (pour remettre des compteurs � 0 par exemple). Ces
 * objets doivent s'enregistrer aupr�s du simulateur par la fonction
 * suivante
 */
void motsim_addToResetList(void * data, void (*resetFunc)(void * data));

/*
 * R�initialisation pour une nouvelle ex�cution
 */
void motSim_reset();

/*
 * Insertion d'un evenement initialise
 */
void motSim_addEvent(struct event_t * event);

/*
 * Initialisation puis insertion d'un evenement
 */
void motSim_insertNewEvent(void (*run)(void *data), void * data, double date);

void motSim_runNevents(int nbEvents);

/* 
 * Obtention de la date courante, exprim�e en secondes
 */
motSimDate_t motSim_getCurrentTime();

/*
 * Lancement d'une simulation d'une dur�e max de date
 */
void motSim_runUntil(double date);

/*
 * Un petit affichage de l'�tat actuel de la simulation
 */
void motSim_printStatus();

/*
 * Lancement de nbSimu simulations, chacune d'une dur�e inf�rieures ou
 * �gale � date.
 */
void motSim_runNSimu(double date, int nbSimu);

void motSim_printCampaignStat();

/*
 * Terminaison "propre"
 */
void motSim_exit(int retValue);

/*
 * Les outils de debogage

 */
#ifdef  DEBUG_NDES
#include <stdio.h>

#   define printf_debug(lvl, fmt, args...)	\
   if ((lvl)& debug_mask)                    \
      printf("[%6.3f ms] %s - " fmt, 1000.0*motSim_getCurrentTime() , __FUNCTION__ , ## args)

#define DEBUG_EVENT    0x00000001
#define DEBUG_MOTSIM   0x00000002
#define DEBUG_GENE     0x00000004
#define DEBUG_TBD      0x00000008
#define DEBUG_FILE     0x00000010
#define DEBUG_SRV      0x00000020
#define DEBUG_SRC      0x00000040
#define DEBUG_GNUPLOT  0x00000080

#define DEBUG_MUX      0x00000100
#define DEBUG_PROBE         0x00000200
#define DEBUG_PROBE_VERB    0x00000400
#define DEBUG_WARN     0x00000800

#define DEBUG_ACM      0x00001000
#define DEBUG_SCHED    0x00002000

#define DEBUG_DVB      0x10000000
#define DEBUG_KS       0x20000000
#define DEBUG_MALLOC   0x40000000
#define DEBUG_KS_VERB  0x80000000

#define DEBUG_ALWAYS   0xFFFFFFFF

static unsigned long debug_mask = 0x00000000
  //     | DEBUG_EVENT     // Les �v�nements (lourd !)
  //       | DEBUG_MOTSIM    // Le moteur
  //     | DEBUG_GENE      // Les g�n�rateurs de nombre/date/...
  //     | DEBUG_SRV       // Le serveur
  //     | DEBUG_SRC       // La source
  //      | DEBUG_FILE      // La gestion des files
  //      | DEBUG_GNUPLOT
  //     | DEBUG_MUX
  //         | DEBUG_PROBE
  //          | DEBUG_PROBE_VERB
  //     | DEBUG_DVB       // Les outils DVB
  //      | DEBUG_KS        // L'algorithme Knapsack
  //   | DEBUG_KS_VERB   // L'algorithme Knapsack verbeux
  //     | DEBUG_WARN      // Des infos qui peuvent aider � debuger la SIMU
  //       | DEBUG_ACM
  //       | DEBUG_SCHED
  //     | DEBUG_MALLOC    // L'utilisation de malloc
       | DEBUG_TBD       // Le code pas implant�
  //     | DEBUG_ALWAYS
  ;

#else
#   define printf_debug(lvl, fmt, args...)
#endif

#define MS_FATAL 1
#define MS_WARN  2

#define motSim_error(lvl, fmt, args...) \
   printf("\n------- Error report -------\n");                      \
   printf("In file  %s\nAt line  %d\nFunction %s\n", __FILE__, __LINE__, __FUNCTION__); \
   printf("Message : "  fmt , ## args);\
   printf("\n------- Error report -------\n");  \
   if (lvl == MS_FATAL) motSim_exit(1);

extern unsigned long __totalMallocSize;

#define sim_malloc(l)				\
  ({ void * __tmpMallocRes = malloc(l);\
   assert(__tmpMallocRes); \
   __totalMallocSize += l; \
    __tmpMallocRes; \
  })

#define sim_malloc_avec_printf_qui_foire(l)				\
  ({ void * __tmpMallocRes = malloc(l);\
   assert(__tmpMallocRes); \
   __totalMallocSize += l; \
   printf_debug(DEBUG_MALLOC, "MALLOC %p (size %zu)\n", __tmpMallocRes, l); \
    __tmpMallocRes; \
  })

#endif
