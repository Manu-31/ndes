#include <stdio.h>     // printf
#include <stdlib.h>    // Malloc, NULL, exit...
#include <assert.h>
#include <signal.h>    // sigaction
#include <strings.h>   // bzero
#include <time.h>

#include <event-file.h>

#include <pdu.h>

struct resetClient_t {
   void * data;
   void (*resetFunc)(void * data);

   struct resetClient_t * next;
};

/*
 * La quantit� de donn�es demand�e � malloc
 */
unsigned long __totalMallocSize = 0;

/*
 * Caract�ristiques d'une instance du simulateur (� voir : ne
 * seraient-ce pas les caract�ristiques d'une simulation ?)
 */
struct motsim_t {
   time_t               actualStartTime;
   double               currentTime;
   double               finishTime; // Heure simul�e de fin pr�vue
   struct eventFile_t * events;
   int                  nbInsertedEvents;
   int                  nbRanEvents;

   struct probe_t       * dureeSimulation;
   struct resetClient_t * resetClient;
};

struct motsim_t * __motSim;

/*
 * Caract�ristiques d'une campagne de simulation. Une campagne sert �
 * instancier � plusieurs reprises une m�me simulation.
 */
struct motSimCampaign_t {
   int nbSimulations;   // Nombre d'instances de la simulation �
			// r�pliquer

   // La liste des sondes de moyenne (permettant d'�tablir des
   // intervalles de confiance sur des sondes de la simulation)

   // La liste des clients � r�-initialiser en d�but de campagne
   struct resetClient_t * resetClient;
};


void motSim_periodicMessage(void * foo)
{
     printf("\r[%6.2f%%] t = %8.2f, ev : %6d in %6d out %6d pre",
	    100.00*__motSim->currentTime/__motSim->finishTime,
	    __motSim->currentTime,
	    __motSim->nbInsertedEvents,
	    __motSim->nbRanEvents,
	    eventFile_length(__motSim->events));
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
 * Cr�ation d'une instance du simulateur au sein de laquelle on pourra
 * lancer plusieurs simulations cons�cutives
 */
void motSim_create()
{
   struct sigaction act;

   __motSim = (struct motsim_t * )sim_malloc(sizeof(struct motsim_t));
   __motSim->currentTime = 0.0;

   printf_debug(DEBUG_MOTSIM, "Initialisation du simulateur ...\n");
   __motSim->events = eventFile_create();
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
   // Calcul de la dur�e moyenne des simulations
   __motSim->dureeSimulation = probe_createExhaustive();
   probe_setPersistent(__motSim->dureeSimulation);

   // Les sondes systeme
   PDU_createProbe = probe_createMean();
   probe_setName(PDU_createProbe, "created PDUs");

   PDU_reuseProbe = probe_createMean();
   probe_setName(PDU_reuseProbe, "reused PDUs");

   PDU_mallocProbe = probe_createMean();
   probe_setName(PDU_mallocProbe, "mallocd PDUs");

   PDU_freeProbe = probe_createMean();
   probe_setName(PDU_createProbe, "released PDUs");

   printf_debug(DEBUG_MOTSIM, "Simulateur pret ...\n");
}

void motSim_addEvent(struct event_t * event)
{
 
   printf_debug(DEBUG_EVENT, "New event at %6.3f (%d ev)\n", event_getDate(event), __motSim->nbInsertedEvents);
   assert(__motSim->currentTime <= event_getDate(event));

   eventFile_insert(__motSim->events, event);
   __motSim->nbInsertedEvents++;
 
}

void motSim_runNevents(int nbEvents)
{
   struct event_t * event;

   if (!__motSim->nbRanEvents) {
      __motSim->actualStartTime = time(NULL);
   }
   while (nbEvents) {
      event = eventFile_extract(__motSim->events);
      if (event) {
         nbEvents--;
         printf_debug(DEBUG_EVENT, "next event at %f\n", event_getDate(event));
         assert(__motSim->currentTime <= event_getDate(event));
         __motSim->currentTime = event_getDate(event);
         event_run(event);
         __motSim->nbRanEvents ++;
      } else {
         printf_debug(DEBUG_MOTSIM, "no more event !\n");
         return ;
      }
   }
}

/*
 * Lancement de plusieurs simulations cons�cutives de m�me dur�e
 *
 * L'�tat final (celui des sondes en particulier) est celui
 * correspondant � la fin de la derni�re simulation.
 */
void motSim_runNSimu(double date, int nbSimu)
{
   int n;

   for (n = 0 ; n < nbSimu; n++) {
      // On r�initialise  tous les �l�ments (et on d�marre les
      // sources)
      motSim_reset();

      // On lance la simulation
      motSim_runUntil(date);
   }
}

void motSim_runUntil(double date)
{
   struct event_t * event;

   //event_periodicAdd(motSim_periodicMessage, NULL, 0.0, date/200.0);
   alarm(1);

   __motSim->finishTime=date;
   if (!__motSim->nbRanEvents) {
      __motSim->actualStartTime = time(NULL);
   }
   event = eventFile_nextEvent(__motSim->events);

   while ((event) && (event_getDate(event) <= date)) {
      event = eventFile_extract(__motSim->events);

      printf_debug(DEBUG_EVENT, "next event at %f\n", event_getDate(event));
      assert(__motSim->currentTime <= event_getDate(event));
      __motSim->currentTime = event_getDate(event);
      event_run(event);
      __motSim->nbRanEvents ++;
      /*
afficher le message toutes les 
      n secondes de temps r�el
 ou x % du temps simule passe

   Bof : moins on en rajoute � chaque event, mieux c'est !
      */
      event = eventFile_nextEvent(__motSim->events);
   }
}

/*
 * On vide la liste des �v�nements 
 */
void motSim_purge()
{
   struct event_t * event;
   int warnDone = 0;
   printf_debug(DEBUG_MOTSIM, "about to purge events\n");

   event = eventFile_extract(__motSim->events);

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
      event = eventFile_extract(__motSim->events);
   }
   printf_debug(DEBUG_MOTSIM, "no more event\n");

}

/*
 * A la fin d'une simulation, certains objets ont besoin d'�tre
 * r�initialiser (pour remettre des compteurs � 0 par exemple). Ces
 * objets doivent s'enregistrer aupr�s du simulateur par la fonction
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
 * R�initialisation du simulateur pour une nouvelle
 * simulation. Attention, il serait pr�f�rable d'invoquer une liste de
 * sous-programmes enregistr�s par autant de modules concern�s.
 */
void motSim_reset()
{
   struct resetClient_t * resetClient;

   // Les �v�nements
   motSim_purge();

   // Le simulateur lui-m�me
   printf_debug(DEBUG_MOTSIM, "ho yes, once again !\n");
   __motSim->currentTime = 0.0;
   __motSim->nbInsertedEvents = 0;
   __motSim->nbRanEvents = 0;

   // Les clients identifi�s (probes et autres)
   // Attention, ils vont �ventuellement ins�rer de nouveaux �v�nements
   printf_debug(DEBUG_MOTSIM, "about to reset clients\n");
   for (resetClient = __motSim->resetClient; resetClient; resetClient = resetClient->next) {
      resetClient->resetFunc(resetClient->data);
   }

   // La simulation est consid�r�e finie
   probe_sample(__motSim->dureeSimulation , time(NULL) - __motSim->actualStartTime);
}


double motSim_getCurrentTime()
{
   return __motSim->currentTime;
};

/*
 * Initialisation puis insertion d'un evenement
 */
void motSim_insertNewEvent(void (*run)(void *data), void * data, double date)
{
  motSim_addEvent(event_create(run, data, date));
}

void motSim_printStatus()
{
   printf("[MOTSI] Date = %f\n", __motSim->currentTime);
   printf("[MOTSI] Events : %ld created (%ld m + %ld r)/%ld freed\n", 
	  event_nbCreate, event_nbMalloc, event_nbReuse, event_nbFree);
   printf("[MOTSI] Simulated events : %d in, %d out, %d pr.\n",
	  __motSim->nbInsertedEvents, __motSim->nbRanEvents, eventFile_length(__motSim->events));
   printf("[MOTSI] PDU : %ld created (%ld m + %ld r)/%ld freed\n",
	  probe_nbSamples(PDU_createProbe),
	  probe_nbSamples(PDU_mallocProbe),
	  probe_nbSamples(PDU_reuseProbe),
	  probe_nbSamples(PDU_freeProbe));
   printf("[MOTSI] Total malloc'ed memory : %ld bytes\n",
	  __totalMallocSize);
   printf("[MOTSI] Realtime duration : %ld sec\n", time(NULL) - __motSim->actualStartTime);
}


/*==========================================================================*/
/*      Mise en oeuvre de la notion de campagne.                            */ 
/*==========================================================================*/
void motSim_campaignRun(struct motSimCampaign_t * c)
{
   int n;
   struct resetClient_t * resetClient;

   for (n = 0 ; n < c->nbSimulations; n++){
      // On r�initialise  tous les �l�ments li�s � la campagne dans sa
      // globalit�. A priori il ne s'agit que des sondes
      // inter-simulation utilis�es pour les intervalles de confiance


      for (resetClient = c->resetClient; resetClient; resetClient = resetClient->next) {
         resetClient->resetFunc(resetClient->data);
      }


      // On lance l'instance de simulation
 
      // On �chantillonne les mesures inter-simu
   }
}

void motSim_campaignStat()
{
   printf("[MOTSI] Number of simulations : %l\n", probe_nbSamples(__motSim->dureeSimulation));
   printf("[MOTSI] Mean duration         : %f sec\n", probe_mean(__motSim->dureeSimulation));
}
