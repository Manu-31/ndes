/**
 * @brief Test de la synchro sur l'horloge du système
 * 
 * On lance NB_EVENTS occurences d'un événement périodique de période
 * EVENT_PERIOD. Le but est de voir (dans le bilan du simulateur) si
 * on cale assez bien à l'horloge système.
 */
#include <stdlib.h>    // Malloc, NULL, exit, ...
#include <assert.h>
#include <strings.h>   // bzero, bcopy, ...
#include <stdio.h>     // printf, ...
#include <math.h>      // fabs

#include <motsim.h>
#include <event.h>

// Combien d'événements lancer ?
#define NB_EVENTS 100

// Quelle périodicité (en secondes) ?
#define EVENT_PERIOD 0.1

/**
 * @brief il faut une fonction pour chaque événement
 */
void rien(void * non)
{
}

/**
 * 
 */
int main() {
   motSim_create(); // Création du simulateur

   // On va simplement créer un événement périodique
   event_periodicAdd(rien, NULL, 0.0, EVENT_PERIOD);

   // On lance la simulation
   motSim_runNevents(NB_EVENTS);

   
   motSim_printStatus();
}
