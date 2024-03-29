#include <stdio.h>     // printf
#include <stdlib.h>    // Malloc, NULL, exit...
#include <assert.h>
#include <signal.h>    // sigaction
#include <strings.h>   // bzero
#include <time.h>
#include <unistd.h>    // alarm

#include <event-list.h>
#include <pdu.h>
#include <log.h>
#include <probe.h>

struct resetClient_t {
   void * data;
   void (*resetFunc)(void * data);

   struct resetClient_t * next;
};

/*
 * La quantité de données demandée à malloc
 */
unsigned long __totalMallocSize = 0;

/*
 * Caractéristiques d'une instance du simulateur (à voir : ne
 * seraient-ce pas les caractéristiques d'une simulation ?)
 */
struct motsim_t {
#ifdef SYNCHRONIZE_CLOCK	 
   struct timespec        actualStartTime;
   struct probe_t       * clockDrift; // Pour mesurer la concordance des horloges
#endif
   motSimDate_t           currentTime;
   motSimDate_t           finishTime; // Heure simulée de fin prévue
   struct eventList_t   * events;
   int                    nbInsertedEvents;
   int                    nbRanEvents;

   struct probe_t       * dureeSimulation;
   struct resetClient_t * resetClient;
};

struct motsim_t * __motSim;

/*
 * Caractéristiques d'une campagne de simulation. Une campagne sert à
 * instancier à plusieurs reprises une même simulation.
 */
struct motSimCampaign_t {
   int nbSimulations;   // Nombre d'instances de la simulation à
			// répliquer

   // La liste des sondes de moyenne (permettant d'établir des
   // intervalles de confiance sur des sondes de la simulation)

   // La liste des clients à ré-initialiser en début de campagne
   struct resetClient_t * resetClient;
};


void motSim_periodicMessage(void * foo)
{
     printf("\r[%6.2f%%] t = %8.2f, ev : %6d in %6d out %6d pre",
	    100.00*__motSim->currentTime/__motSim->finishTime,
	    __motSim->currentTime,
	    __motSim->nbInsertedEvents,
	    __motSim->nbRanEvents,
	    eventList_getLength(__motSim->events));
     fflush(stdout);
}

void mainHandler(int sig)
{
   if (sig == SIGCHLD) {
   } else { 
      motSim_exit(200+sig);
   }
}

void periodicHandler(int sig)
{
   if (__motSim->currentTime<= __motSim->finishTime) {
      motSim_periodicMessage(NULL);
      alarm(1);
   }
}

/*
 * Terminaison "propre"
 */
void motSim_exit(int retValue)
{
   motSim_printStatus();
   exit(retValue);
}

/*
 * Création d'une instance du simulateur au sein de laquelle on pourra
 * lancer plusieurs simulations consécutives
 */
void motSim_create()
{
   struct sigaction act;

   __motSim = (struct motsim_t * )sim_malloc(sizeof(struct motsim_t));
   __motSim->currentTime = 0.0;

   printf_debug(DEBUG_MOTSIM, "Initialisation du simulateur ...\n");
   __motSim->events = eventList_create();
   __motSim->nbInsertedEvents = 0;
   __motSim->nbRanEvents = 0;
   __motSim->resetClient = NULL;

   printf_debug(DEBUG_MOTSIM, "gestion des signaux \n");
   // We want to close files on exit, even with ^c
   bzero(&act, sizeof(struct sigaction));
   act.sa_handler = mainHandler;
   act.sa_flags = SA_NOCLDWAIT;

   sigaction(SIGHUP, &act,NULL);
   sigaction(SIGINT, &act,NULL);
   sigaction(SIGQUIT, &act,NULL);
   sigaction(SIGCHLD, &act,NULL);

   // For periodic ping
   act.sa_handler = periodicHandler;
   sigaction(SIGALRM, &act,NULL);

   printf_debug(DEBUG_MOTSIM, "creation des sondes systeme\n");

   // Calcul de la durée moyenne des simulations
   __motSim->dureeSimulation = probe_createExhaustive();
   probe_setPersistent(__motSim->dureeSimulation);

   // Arrive-t-on a garder les horloges cohérentes ?
#ifdef SYNCHRONIZE_CLOCK
   __motSim->clockDrift = probe_createMean();
   probe_setName(__motSim->clockDrift, "clock drift");
#endif
   
   // Les sondes systeme
   PDU_createProbe = probe_createMean();
   probe_setName(PDU_createProbe, "created PDUs");

   PDU_reuseProbe = probe_createMean();
   probe_setName(PDU_reuseProbe, "reused PDUs");

   PDU_mallocProbe = probe_createMean();
   probe_setName(PDU_mallocProbe, "mallocd PDUs");

   PDU_releaseProbe = probe_createMean();
   probe_setName(PDU_releaseProbe, "released PDUs");

   // Intialisation des log
   printf_debug(DEBUG_MOTSIM, "Initialisation des log ...\n");
   ndesLog_init();

   printf_debug(DEBUG_MOTSIM, "Simulateur pret ...\n");
}

/**
 * @brief Schedule an event to be run at a given date
 * @param event a (non NULL) pointer to an initialised event
 * @param date a (non past) date to run the event
 */
void motSim_scheduleEvent(struct event_t * event, motSimDate_t date)
{
 
   printf_debug(DEBUG_EVENT, "New event (%p) at %6.3f (%d ev)\n", event, date, __motSim->nbInsertedEvents);
   assert(__motSim->currentTime <= date);

   eventList_insert(__motSim->events, event, date);
   __motSim->nbInsertedEvents++;
 
}

/**
 * @brief Create then schedule a new event
 * @param run Event function
 * @param data a pointer to use as argument for run
 * @param date The (future) date to execute event
 */
void motSim_scheduleNewEvent(void (*run)(void *data), void * data, motSimDate_t date)
{
  motSim_scheduleEvent(event_create(run, data), date);
}

/**
 * @brief Pause execution until wall clock >= simulatedTime
 * @param simulatedTime date of next event
 * @return wall clock time (in second from start of simulation)
 */
static inline double waitForActualClock(motSimDate_t simulatedTime)
{
   struct timespec actualCurrentTime;
   long nbSec, nbNsec;
   useconds_t avance;
   double dureeReelle, result;
   
   // Depuis combien de temps dure la simuation ?
   //    Date actuelle
   clock_gettime(CLOCK_REALTIME, &actualCurrentTime);

   //    Calcul de la durée écoulée depuis le début 
   nbSec = actualCurrentTime.tv_sec - __motSim->actualStartTime.tv_sec;
   if (actualCurrentTime.tv_nsec >= __motSim->actualStartTime.tv_nsec) {
      nbNsec = actualCurrentTime.tv_nsec - __motSim->actualStartTime.tv_nsec;
   } else {
      nbNsec = 1000000000 - __motSim->actualStartTime.tv_nsec + actualCurrentTime.tv_nsec;
      nbSec--;
   }
   dureeReelle = ((double)nbSec + (double)nbNsec / 1.0e9);
   if (dureeReelle < simulatedTime) {
     printf_debug(DEBUG_CLOCK, "%f réel en avance sur %f simulé\n", dureeReelle, simulatedTime);

     avance = (useconds_t)(1000000.0 * (simulatedTime - dureeReelle));
     printf_debug(DEBUG_CLOCK, "avance de %d\n", avance);

     // Soucis avec usleep(n) si n > 1 000 000
     if (avance > 1000000) {
        printf_debug(DEBUG_CLOCK, "dodo de %d\n", avance/1000000);
        sleep(avance/100000);
	avance = avance % 1000000;
     }        
     printf_debug(DEBUG_CLOCK, "udodo de %d\n", avance);
     usleep((useconds_t)avance);

     printf_debug(DEBUG_CLOCK, "dodo fini\n");
     
     // La suite est pour débuguer !
     clock_gettime(CLOCK_REALTIME, &actualCurrentTime);

     // Calcul de la durée en temps réel
     nbSec = actualCurrentTime.tv_sec - __motSim->actualStartTime.tv_sec;
     if (actualCurrentTime.tv_nsec >= __motSim->actualStartTime.tv_nsec) {
        nbNsec = actualCurrentTime.tv_nsec - __motSim->actualStartTime.tv_nsec;
     } else {
        nbNsec = 1000000000 - __motSim->actualStartTime.tv_nsec + actualCurrentTime.tv_nsec;
        nbSec--;
     }
     // Fin du debug
    
   } else {
     printf_debug(DEBUG_NEVER, "%f réel en retard sur %f simulé\n", ((double)nbSec + (double)nbNsec / 1.0e9), simulatedTime);
   }
   printf_debug(DEBUG_ALWAYS, "out\n");
   // Pour le debug
   result = (double)nbSec + (double)nbNsec / 1.0e9;
   return result;
}

void motSim_runNevents(int nbEvents)
{
   struct event_t * event;
   double wallCl;
   
   if (!__motSim->nbRanEvents) {
      //      __motSim->actualStartTime = time(NULL);
      clock_gettime(CLOCK_REALTIME, &__motSim->actualStartTime);
   }
   while (nbEvents) {
      event = eventList_extractFirst(__motSim->events);
      if (event) {
         nbEvents--;
         printf_debug(DEBUG_EVENT, "next event (%p) at %f\n", event, event_getDate(event));
         assert(__motSim->currentTime <= event_getDate(event));

	 // On avance l'horloge virtuelle du moteur
         __motSim->currentTime = event_getDate(event);

#ifdef SYNCHRONIZE_CLOCK	 
	 // Essayons d'accorder les horloges ...
	 wallCl = waitForActualClock(__motSim->currentTime);
         printf_debug(DEBUG_CLOCK, "run event (%p) at %f (clock %f)\n", event, event_getDate(event), wallCl);
         probe_sample(__motSim->clockDrift , __motSim->currentTime - wallCl);
#endif
	 // On lance un événement de plus
         event_run(event);
         __motSim->nbRanEvents ++;
	 
      } else {
         printf_debug(DEBUG_MOTSIM, "no more event !\n");
         return ;
      }
   }
}

/** brief Simulation jusqu'à épuisement des événements
 */
void motSim_runUntilTheEnd()
{
   struct event_t * event;
   double wallCl __attribute__((unused));
   
   if (!__motSim->nbRanEvents) {
      //      __motSim->actualStartTime = time(NULL);
      clock_gettime(CLOCK_REALTIME, &__motSim->actualStartTime); 
   }
   while (1) {
      event = eventList_extractFirst(__motSim->events);
      if (event) {
         printf_debug(DEBUG_EVENT, "next event (out of %d) at %f\n", eventList_getLength(__motSim->events), event_getDate(event));
         assert(__motSim->currentTime <= event_getDate(event));

	 // On avance l'horloge virtuelle du moteur
         __motSim->currentTime = event_getDate(event);

#ifdef SYNCHRONIZE_CLOCK	 
	 // Essayons d'accorder les horloges ...
	 wallCl = waitForActualClock(__motSim->currentTime);
         probe_sample(__motSim->clockDrift , __motSim->currentTime - wallCl);
#endif
         event_run(event);
         __motSim->nbRanEvents ++;
         printf_debug(DEBUG_EVENT, "now %d events left\n", eventList_getLength(__motSim->events));
      } else {
         printf_debug(DEBUG_MOTSIM, "no more event !\n");
         return ;
      }
   }
}

/*
 * Lancement de plusieurs simulations consécutives de même durée
 *
 * L'état final (celui des sondes en particulier) est celui
 * correspondant à la fin de la dernière simulation.
 */
void motSim_runNSimu(motSimDate_t date, int nbSimu)
{
   int n;

   for (n = 0 ; n < nbSimu; n++) {
      // On réinitialise  tous les éléments (et on démarre les
      // sources)
      motSim_reset();

      // On lance la simulation
      motSim_runUntil(date);
   }
}

void motSim_runUntil(motSimDate_t date)
{
   struct event_t * event;

   //event_periodicAdd(motSim_periodicMessage, NULL, 0.0, date/200.0);
   alarm(1);

   __motSim->finishTime=date;
   if (!__motSim->nbRanEvents) {
      //      __motSim->actualStartTime = time(NULL);
      clock_gettime(CLOCK_REALTIME, &__motSim->actualStartTime);
   }
   event = eventList_nextEvent(__motSim->events);

   while ((event) && (event_getDate(event) <= date)) {
      event = eventList_extractFirst(__motSim->events);

      printf_debug(DEBUG_EVENT, "next event at %f\n", event_getDate(event));
      assert(__motSim->currentTime <= event_getDate(event));
      __motSim->currentTime = event_getDate(event);
      event_run(event);
      __motSim->nbRanEvents ++;
      /*
afficher le message toutes les 
      n secondes de temps réel
 ou x % du temps simule passe

   Bof : moins on en rajoute à chaque event, mieux c'est !
      */
      event = eventList_nextEvent(__motSim->events);
   }
}

/*
 * On vide la liste des événements 
 */
void motSim_purge()
{
   struct event_t * event;
   int warnDone = 0;
   printf_debug(DEBUG_MOTSIM, "about to purge events\n");

   event = eventList_extractFirst(__motSim->events);

   while (event){
      printf_debug(DEBUG_MOTSIM, "next event at %f\n", event_getDate(event));
      assert(__motSim->currentTime <= event_getDate(event));
      __motSim->currentTime = event_getDate(event);
      //      event_run(event);
      if (!warnDone) {
	 warnDone = 1;
         printf_debug(DEBUG_TBD, "Some events have been purged !!\n");
      };
      __motSim->nbRanEvents ++;
      event = eventList_extractFirst(__motSim->events);
   }
   printf_debug(DEBUG_MOTSIM, "no more event\n");
}

/*
 * A la fin d'une simulation, certains objets ont besoin d'être
 * réinitialiser (pour remettre des compteurs à 0 par exemple). Ces
 * objets doivent s'enregistrer auprès du simulateur par la fonction
 * suivante
 */
void motsim_addToResetList(void * data, void (*resetFunc)(void * data))
{
   struct resetClient_t * resetClient = (struct resetClient_t *)sim_malloc(sizeof(struct resetClient_t));

   assert (resetClient);

   resetClient->next = __motSim->resetClient;
   resetClient->data = data;
   resetClient->resetFunc = resetFunc;

   __motSim->resetClient = resetClient;
}


/*
 * Réinitialisation du simulateur pour une nouvelle
 * simulation. Attention, il serait préférable d'invoquer une liste de
 * sous-programmes enregistrés par autant de modules concernés.
 */
void motSim_reset()
{
   struct resetClient_t * resetClient;
   struct timespec actualEndTime;
   long nbSec, nbNsec;
   
   // Les événements
   motSim_purge();

   // Le simulateur lui-même
   printf_debug(DEBUG_MOTSIM, "ho yes, once again !\n");
   __motSim->currentTime = 0.0;
   __motSim->nbInsertedEvents = 0;
   __motSim->nbRanEvents = 0;

   // Les clients identifiés (probes et autres)
   // Attention, ils vont éventuellement insérer de nouveaux événements
   printf_debug(DEBUG_MOTSIM, "about to reset clients\n");
   for (resetClient = __motSim->resetClient; resetClient; resetClient = resetClient->next) {
      resetClient->resetFunc(resetClient->data);
   }

   // La simulation est considérée finie
   clock_gettime(CLOCK_REALTIME, &actualEndTime);

   // Calcul de la durée en temps réel
   nbSec = actualEndTime.tv_sec - __motSim->actualStartTime.tv_sec;
   if (actualEndTime.tv_nsec > __motSim->actualStartTime.tv_nsec) {
     nbNsec = actualEndTime.tv_nsec - __motSim->actualStartTime.tv_nsec;
   } else {
     nbNsec =  1000000000 - __motSim->actualStartTime.tv_nsec + actualEndTime.tv_nsec;
     nbSec--;
   }
   
   //probe_sample(__motSim->dureeSimulation , time(NULL) - __motSim->actualStartTime);
   probe_sample(__motSim->dureeSimulation , (double)nbSec + (double)nbNsec / 1.0e9);
}


motSimDate_t motSim_getCurrentTime()
{
   return __motSim->currentTime;
};

void motSim_printStatus()
{
   struct timespec actualEndTime;
   long nbSec, nbNsec;

   printf("[MOTSI] Date = %f\n", __motSim->currentTime);
   printf("[MOTSI] Events : %ld created (%ld m + %ld r)/%ld freed\n", 
	  event_nbCreate, event_nbMalloc, event_nbReuse, event_nbFree);
   printf("[MOTSI] Simulated events : %d in, %d out, %d pr.\n",
	  __motSim->nbInsertedEvents, __motSim->nbRanEvents, eventList_getLength(__motSim->events));
   printf("[MOTSI] PDU : %ld created (%ld m + %ld r)/%ld released\n",
	  probe_nbSamples(PDU_createProbe),
	  probe_nbSamples(PDU_mallocProbe),
	  probe_nbSamples(PDU_reuseProbe),
	  probe_nbSamples(PDU_releaseProbe));
   printf("[MOTSI] Total malloc'ed memory : %ld bytes\n",
	  __totalMallocSize);

   // Calcul de la durée en temps réel
   clock_gettime(CLOCK_REALTIME, &actualEndTime);
   nbSec = actualEndTime.tv_sec - __motSim->actualStartTime.tv_sec;
   if (actualEndTime.tv_nsec > __motSim->actualStartTime.tv_nsec) {
     nbNsec = actualEndTime.tv_nsec - __motSim->actualStartTime.tv_nsec;
   } else {
     nbNsec = 1000000000 - __motSim->actualStartTime.tv_nsec + actualEndTime.tv_nsec;
     nbSec--;
   }

   printf("[MOTSI] Realtime duration : %ld sec %ld ns\n", nbSec, nbNsec);
   // Arrive-t-on a garder les horloges cohérentes ?
#ifdef SYNCHRONIZE_CLOCK
   printf("[MOTSI] Clock drift %f ms\n", 1000.0*probe_mean(__motSim->clockDrift));
#endif
}


/*==========================================================================*/
/*      Mise en oeuvre de la notion de campagne.                            */ 
/*==========================================================================*/
void motSim_campaignRun(struct motSimCampaign_t * c)
{
   int n;
   struct resetClient_t * resetClient;

   for (n = 0 ; n < c->nbSimulations; n++){
      // On réinitialise  tous les éléments liés à la campagne dans sa
      // globalité. A priori il ne s'agit que des sondes
      // inter-simulation utilisées pour les intervalles de confiance


      for (resetClient = c->resetClient; resetClient; resetClient = resetClient->next) {
         resetClient->resetFunc(resetClient->data);
      }


      // On lance l'instance de simulation
 
      // On échantillonne les mesures inter-simu
   }
}

void motSim_campaignStat()
{
   printf("[MOTSI] Number of simulations : %ld\n", probe_nbSamples(__motSim->dureeSimulation));
   printf("[MOTSI] Mean duration         : %f sec\n", probe_mean(__motSim->dureeSimulation));
}
