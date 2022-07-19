/**
 * @file motsim.c
 * @brief Le moteur de simulation.
 * 
 *   Puisqu'il n'y aura jamais qu'un seul moteur de simulation et
 * qu'il faut le passer en paramètre à de nombreuses fonctions qui n'en
 * ont pas directement besoin mais qui vont elles mêmes le passer à 
 * d'autres fonctions pour finalement qu'il soit utilisé par les fonctions
 * qui génèrent des événements, ... bref une variable globale est créée
 * ici et sera utilisée lorsque nécessaire. Ce n'est pas très glorieux
 * mais ça simplifie tellement.
 *
 *   A voir : rendre tout ca multithreadable !
 *
 *   Attention, il faut tout de même l'initialiser ! 
 */
#ifndef __DEF_MOTSIM
#define __DEF_MOTSIM

#include <assert.h>
#include <stdio.h>   // printf
#include <stdlib.h>  // malloc

/**
 * @brief Il peut être parfois intéressant d'essayer de maintenir en
 * cohérence le temps simulé sur le temps de l'horloge.
 */
#define SYNCHRONIZE_CLOCK

#define min(a, b) ((a)<(b)?(a):(b))
#define max(a, b) ((a)>(b)?(a):(b))

//#define MOTSIM_LONG_DATE

#ifdef MOTSIM_LONG_DATE
   typedef long double  motSimDate_t;
#else
   typedef double  motSimDate_t;
#endif

struct motsim_t;
struct event_t;

extern struct motsim_t * __motSim;

/*
 * Initialisation du système
 */
void motSim_create(); 

/*
 * A la fin d'une simulation, certains objets ont besoin d'être
 * réinitialiser (pour remettre des compteurs à 0 par exemple). Ces
 * objets doivent s'enregistrer auprès du simulateur par la fonction
 * suivante
 */
void motsim_addToResetList(void * data, void (*resetFunc)(void * data));

/*
 * Réinitialisation pour une nouvelle exécution
 */
void motSim_reset();

/**
 * @brief Schedule an event to be run at a given date
 * @param event a (non NULL) pointer to an initialised event
 * @param date a (non past) date to run the event
 */
void motSim_scheduleEvent(struct event_t * event, motSimDate_t date);

/**
 * @brief Create then schedule a new event
 * @param run Event function
 * @param data a pointer to use as argument for run
 * @param date The (future) date to execute event
 */
void motSim_scheduleNewEvent(void (*run)(void *data), void * data, motSimDate_t date);

void motSim_runNevents(int nbEvents);

/* 
 * Obtention de la date courante, exprimée en secondes
 */
motSimDate_t motSim_getCurrentTime();

/*
 * Lancement d'une simulation d'une durée max de date
 */
void motSim_runUntil(motSimDate_t date);

/** brief Simulation jusqu'à épuisement des événements
 */
void motSim_runUntilTheEnd();

/*
 * Un petit affichage de l'état actuel de la simulation
 */
void motSim_printStatus();

/*
 * Lancement de nbSimu simulations, chacune d'une durée inférieures ou
 * égale à date.
 */
void motSim_runNSimu(motSimDate_t  date, int nbSimu);

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

#define DEBUG_EVENT       0x00000001
#define DEBUG_MOTSIM      0x00000002
#define DEBUG_GENE        0x00000004
#define DEBUG_TBD         0x00000008
#define DEBUG_FILE        0x00000010
#define DEBUG_SRV         0x00000020
#define DEBUG_SRC         0x00000040
#define DEBUG_GNUPLOT     0x00000080

#define DEBUG_MUX         0x00000100
#define DEBUG_PROBE       0x00000200
#define DEBUG_PROBE_VERB  0x00000400
#define DEBUG_WARN        0x00000800

#define DEBUG_ACM         0x00001000
#define DEBUG_SCHED       0x00002000
#define DEBUG_PDU         0x00004000
#define DEBUG_OBJECT      0x00008000
#define DEBUG_IPV4        0x00010000
#define DEBUG_CLOCK       0x00020000  // Pour synchroniser les horloges
#define DEBUG_TUN         0x00040000  // Le TUN pour communiquer avec le monde
#define DEBUG_DVB         0x10000000
#define DEBUG_KS          0x20000000
#define DEBUG_MALLOC      0x40000000
#define DEBUG_KS_VERB     0x80000000

#define DEBUG_ALWAYS      0xFFFFFFFF
#define DEBUG_NEVER       0x00000000

static unsigned long debug_mask __attribute__ ((unused)) = 0x00000000
  //     | DEBUG_EVENT      // Les événements (lourd !)
  //     | DEBUG_MOTSIM     // Le moteur
  //     | DEBUG_GENE       // Les générateurs de nombre/date/...
  //     | DEBUG_SRV        // Le serveur
  //     | DEBUG_SRC        // La source
  //     | DEBUG_FILE       // La gestion des files
  //     | DEBUG_GNUPLOT
  //     | DEBUG_MUX
  //     | DEBUG_PROBE
  //     | DEBUG_PROBE_VERB
  //     | DEBUG_DVB        // Les outils DVB
  //     | DEBUG_KS         // L'algorithme Knapsack
  //     | DEBUG_KS_VERB    // L'algorithme Knapsack verbeux
  //     | DEBUG_WARN       // Des infos qui peuvent aider à debuger la SIMU
  //     | DEBUG_ACM
  //     | DEBUG_SCHED      // Les ordonnanceurs
  //     | DEBUG_PDU
  //     | DEBUG_OBJECT
  //     | DEBUG_MALLOC     // L'utilisation de malloc
  //     | DEBUG_CLOCK        // Synchronisation des horloges
  //     | DEBUG_IPV4       // Ma tentative de IPv4
  //       | DEBUG_ALWAYS
       | DEBUG_TBD          // Le code pas implanté
  ;

#else
#   define printf_debug(lvl, fmt, args...)
#endif

#define MS_FATAL 1
#define MS_WARN  2

#define motSim_error(lvl, fmt, args...)                             \
   printf("\n------- Error report -------\n");                      \
   printf("In file  %s\nAt line  %d\nFunction %s\n", __FILE__, __LINE__, __FUNCTION__); \
   printf("Message : "  fmt , ## args);                             \
   printf("\n------- Error report -------\n");                      \
   if (lvl == MS_FATAL) motSim_exit(1);

extern unsigned long __totalMallocSize;

#define sim_malloc(l)				\
  ({ void * __tmpMallocRes = malloc(l);\
   assert(__tmpMallocRes); \
   __totalMallocSize += l; \
    __tmpMallocRes; \
  })

#define sim_free(p)            \
  ({assert(p);                 \
  free(p);                     \
  })

#define sim_malloc_avec_printf_qui_foire(l)				\
  ({ void * __tmpMallocRes = malloc(l);\
   assert(__tmpMallocRes); \
   __totalMallocSize += l; \
   printf_debug(DEBUG_MALLOC, "MALLOC %p (size %zu)\n", __tmpMallocRes, l); \
    __tmpMallocRes; \
  })

#endif
