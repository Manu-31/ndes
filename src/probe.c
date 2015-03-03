/**
 * @file probe.c
 * @brief Implantation des probes
 * Attention à la structure des sondes exhaustives, le parcours en
 * lecture n'est pas forcément intuitif ...
 *
 * De plus, lors du reset, on libère la mémoire, ce qui prend du temps
 * et nécessite de les réallouer pour la simulation suivante. On
 * pourrait les conserver, non ?
 *
 * La gestion périodique est faite sans utiliser les événements
 * périodiques, c'est certainement à revoir, ...
 */


#include <stdio.h>     // printf
#include <stdlib.h>    // Malloc, NULL, exit...
#include <math.h>      // round
#include <string.h>    // strlen
#include <unistd.h>    // write
#include <assert.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <motsim.h>
#include <event.h>
#include <probe.h>

/**
 * @brief Structure permettant la gestion des sondes exhaustives
 */
struct sampleSet_t {
   double samples[PROBE_NB_SAMPLES_MAX]; //!< list of samples
   double dates[PROBE_NB_SAMPLES_MAX];   //!< date for each sample

   // Pour le chainage
   struct sampleSet_t * prev;
   struct sampleSet_t * next;
};

/*
 * Structure permettant la gestion des sondes graphBar
 */
struct graphBar_t {
   double          min;
   double          max;
   unsigned long   nbBar;
   double        * value;      //!< Must be real for normalization
   int             normalized; //!< Probe is read only once normalized
};

/*
 * Structure permettant la gestion des sondes de moyenne
 */
struct mean_t {
   double valueSum;  // La somme cumulée des échantillons
   double firstDate; // Date premier événement
   double lastDate;  // Date dernier événement
};

/*
 * Gestion par tranches temporelles
 */
struct timeSlice_t {
   double           valueSum;  // La somme cumulée des échantillons
   int              nbSamplesInSlice; // Le nombre d'echantillons dans la tranche
   struct probe_t * meanProbe;  // Une probe exhaustive sur la moyenne à chaque fin d'intervalle
   struct probe_t * bwProbe;  // Une probe exhaustive sur le débit à chaque fin d'intervalle
};

/*
 * Pour une sonde périodique
 */
struct periodic_t {
   struct probe_t * data; // Une sonde exhaustive pour stocker un échantillon par période
};

/**
 * @brief Gestion des sondes à fenêtre.
 */
struct slidingWindow_t {
   double * samples;
   double * dates;

   int capacity;   //!< Le nombre d'échantillons conservés
   int length, last;
};

/*
 * Gestion par une moyenne mobile M <- a.M + (1-a).sample
 */
struct EMA_t {
   double a;      // Aging factor
   double avg;    // Current average value
   double bwAvg;  // Current average throughput value

   double value; // Cumulated current value
   double previousTime;
   double previousAvg, previousBwAvg;
};

/*
 * Structure générale d'une sonde
 */
struct probe_t {
   enum probeType_t probeType;
   char           * name;
   unsigned long    nbSamples;
   double           min, max;
   double           lastSample;
   double           lastSampleDate;
   double           period;          // Certaines probes ont des choses à faire 

   union probeData_t { 
      struct sampleSet_t     * sampleSet;
      struct graphBar_t      * graphBar;
      struct mean_t          * mean;
      struct timeSlice_t     * timeSlice;
      struct slidingWindow_t * window;
      struct EMA_t           * ema;
      struct periodic_t      * periodic;
   } data;

   // (Optional) fiter to apply before sampling
   struct PDUFilter_t * filter;

   // Une sonde persistante n'est jamais réinitialisée
   int persistent;

   // Les métas sondes
   struct probe_t * sampleProbe;      // Sur les échantillons
   struct probe_t * meanProbe;        // Sur la moyenne
   struct probe_t * throughputProbe ; // 
				      // Sur le "débit" (cf notes relatives)
   // Les méthodes de manipulation

   // On chaîne localement les probes pour échantilloner d'un coup un seul ev
   struct probe_t * nextProbe;

   // On chaîne globalement les probes pour en garder une trace
   struct probe_t * next;
};


// Pointeur sur la chaine de toutes les probes du système
struct probe_t * firstProbe = NULL;


/**
 * @brief Define a probe as persistent
 * Une sonde persistante ne sera pas réinitialisée en cas de reset (en
 * fin de simulation)
 */
void probe_setPersistent(struct probe_t * p)
{
   p->persistent = 1;
}

/**
 * @brief Chaînage des probes p1 et p2, dans cet ordre.
 * @param p1 première sonde, doit être non NULL
 * @param p2 seconde sonde
 * @result p1
 *
 * Tout échantillon sur
 * p1 sera répercuté sur p2. C'est la seule méthode qui soit
 * répercutée en cascade. Les reset, calcul de moyenne, ... doivent
 * être invoquées sur chaque sonde si nécessaire
 */
struct probe_t * probe_chain(struct probe_t * p1, struct probe_t * p2)
{
   assert(p1 != NULL);
   assert(p1->nextProbe == NULL); // Pour bien faire il faudrait p2->nextProbe <- p1->nextProbe

   p1->nextProbe = p2;
   printf_debug(DEBUG_PROBE, "\"%s\" (%p, type %s) chained after \"%s\" (%p, type %s)\n",
		p2?probe_getName(p2):"(null)", p2, p2?probeTypeName(p2->probeType):"(null)", 
		probe_getName(p1), p1, probeTypeName(p1->probeType));
   return p1;
}

void probe_resetExhaustive(struct probe_t * probe)
{
   struct sampleSet_t * ss;

   while (probe->data.sampleSet) {
      ss = probe->data.sampleSet->prev;
      free(probe->data.sampleSet);
      probe->data.sampleSet = ss;
   }
}

void probe_resetGraphBar(struct probe_t * probe)
{
   int i;
   printf_debug(DEBUG_PROBE, "reseting '%s'\n", probe_getName(probe));

   for (i = 0; i < probe->data.graphBar->nbBar; i++){
      probe->data.graphBar->value[i] = 0.0;
   }
}

void probe_resetMean(struct probe_t * pr)
{
   pr->data.mean->valueSum = 0.0;
   pr->data.mean->firstDate = 0.0;
   pr->data.mean->lastDate = 0.0;
}

void probe_EMAReset(struct probe_t * pr)
{
   pr->data.ema->avg = 0.0;
   pr->data.ema->bwAvg = 0.0;
}


void probe_slidingWindowReset(struct probe_t * pr)
{
   pr->data.window->length = 0;
   pr->data.window->last = 0;
}

void probe_scheduleNextEvent(struct probe_t * tap);

/* WARNING les deux fonctions suivantes sont à fusionner */

void probe_periodicReset(struct probe_t * pr)
{
   struct event_t * ev;

   probe_reset(pr->data.periodic->data);

   // On déclanche un échantillon du cumul à 0 + t
   ev = event_create((void (*)(void *))probe_scheduleNextEvent, pr);
   printf_debug(DEBUG_PROBE, "premiere moyenne a %f ms pour %s\n", pr->period, pr->name);
   motSim_scheduleEvent(ev, pr->period);
}

void probe_timeSliceReset(struct probe_t * pr)
{
   struct event_t * ev;

   pr->data.timeSlice->valueSum = 0.0;
   pr->data.timeSlice->nbSamplesInSlice = 0;

   probe_reset(pr->data.timeSlice->meanProbe);
   probe_reset(pr->data.timeSlice->bwProbe);

   // On déclanche un échantillon du cumul à 0 + t
   ev = event_create((void (*)(void *))probe_scheduleNextEvent, pr);
   printf_debug(DEBUG_PROBE, "premiere moyenne a %f ms(ev %p)\n", pr->period, ev);
   motSim_scheduleEvent(ev, pr->period);
}

/*
 * Réinitialisation d'une probe (pour permettre de relancer une
 * simulation dans les mêmes conditions). Tout est effacé et doit donc
 * avoir été sauvegardé si besoin.
 */
void probe_reset(struct probe_t * probe)
{
   printf_debug(DEBUG_PROBE, "about to reset \"%s\" (type %d)\n",
		probe_getName(probe), 
		probe->probeType);
   if (probe->persistent){
      return; 
   }
   switch (probe->probeType) {
      case exhaustiveProbeType : 
	 probe_resetExhaustive(probe);
      break;
      case graphBarProbeType : 
	 probe_resetGraphBar(probe);
      break;
      case meanProbeType : 
	 probe_resetMean(probe);
      break;
      case timeSliceAverageProbeType : 
      case timeSliceThroughputProbeType : 
	 probe_timeSliceReset(probe);
      break;
      case EMAProbeType : 
	 probe_EMAReset(probe);
      break;
      case slidingWindowProbeType :
 	 probe_slidingWindowReset(probe);
      break;
      case periodicProbeType :
 	 probe_periodicReset(probe);
      break;
      default :
	 motSim_error(MS_WARN, "No reset for probe \"%s\" (type \"%s\")\n", probe_getName(probe), probeTypeName(probe->probeType));
      break;

   }

   probe->nbSamples = 0;
   probe->min = 0.0;
   probe->max = 0.0;
   probe->lastSample = 0;
   probe->lastSampleDate = 0;

   printf_debug(DEBUG_PROBE, "reset \"%s\"\n", probe_getName(probe));
}

/*
 * Création générale. Attention, toute création de probe doit passer
 * par là.
 */
struct probe_t * probe_createRaw(enum probeType_t probeType)
{
   struct probe_t * result = (struct probe_t * ) sim_malloc(sizeof(struct probe_t));
   printf_debug(DEBUG_PROBE, "in\n");

   //printf_debug(DEBUG_PROBE, "creating a \"%s\"\n" probeTypeName(probeType));

   assert(result);

   result->persistent = 0;
   result->probeType = probeType;
   result->nbSamples = 0;
   result->lastSample = 0.0;
   result->name = strdup("Generic probe");
   result->nextProbe = NULL;
   result->period = 0.0;

   // Default is no filter
   result->filter = NULL;

   // Les métas probes
   result->meanProbe = NULL;
   result->throughputProbe = NULL;
   result->sampleProbe = NULL;

   result->next = firstProbe;
   firstProbe = result;

   // Ajout à la liste des choses à réinitialiser avant une prochaine simu
   motsim_addToResetList(result, (void (*)(void * data)) probe_reset);

   //printf_debug(DEBUG_PROBE, "A \"%s\" has been created\n" probeTypeName(probeType));
   printf_debug(DEBUG_PROBE, "out\n");

   return result;
}

// Conserve un échantillon à la fin de chaque tranche temporelle de durée t
struct probe_t * probe_periodicCreate(double t)
{
   struct event_t * ev;

   struct probe_t * result = probe_createRaw(periodicProbeType);

   result->data.periodic = (struct periodic_t *) sim_malloc(sizeof(struct periodic_t));
   result->data.periodic->data = probe_createExhaustive();
   result->period = t;

   // On déclanche un échantillon du cumul à 0 + t
   ev = event_create((void (*)(void *))probe_scheduleNextEvent, result);
   printf_debug(DEBUG_PROBE, "premiere moyenne a %f ms(ev %p) pour \"%s\"\n", result->period, ev, result->name);
   motSim_scheduleEvent(ev, result->period);

   return result;
}

// Conserve une moyenne mobile M <- a.M + (1-a).sample
struct probe_t * probe_EMACreate(double a)
{
   struct probe_t * result = probe_createRaw(EMAProbeType);

   result->data.ema = (struct EMA_t *) sim_malloc(sizeof(struct EMA_t));

   result->data.ema->a = a;
   result->data.ema->avg = 0.0;   
   result->data.ema->bwAvg = 0.0;
   result->data.ema->value = 0.0;
   result->data.ema->previousTime = 0;
   result->data.ema->previousAvg = 0;
   result->data.ema->previousBwAvg = 0;

   return result;
}

/** 
 * @brief Création d'une sonde à fenêtre glissante
 * @param windowLength Nombre d'échantillons conservés
 *
 */
struct probe_t * probe_slidingWindowCreate(int windowLength)
{
   printf_debug(DEBUG_PROBE, "in\n");

   struct probe_t * result = probe_createRaw(slidingWindowProbeType);

   result->data.window = (struct slidingWindow_t *)sim_malloc(sizeof(struct slidingWindow_t ));
   result->data.window->dates = (double *)sim_malloc(windowLength*sizeof(double));
   result->data.window->samples = (double *)sim_malloc(windowLength*sizeof(double));
   result->data.window->capacity = windowLength;
   result->data.window->length = 0;
   result->data.window->last = 0;

   printf_debug(DEBUG_PROBE, "out\n");
   return result;
}

struct probe_t * probe_createExhaustive()
{
   struct probe_t * result = probe_createRaw(exhaustiveProbeType);

   result->data.sampleSet = NULL;

   return result;
}

/*
 * Echantillonage de la moyenne actuelle d'une sonde par moyennes
 * temporelles
 */
void probe_timeSliceNextEvent(struct probe_t * tap)
{
   // On échantillonne
   probe_sample(tap->data.timeSlice->meanProbe,
                tap->data.timeSlice->valueSum/(tap->data.timeSlice->nbSamplesInSlice?tap->data.timeSlice->nbSamplesInSlice:1.0));
   probe_sample(tap->data.timeSlice->bwProbe,
                8.0*tap->data.timeSlice->valueSum/tap->period);

   // On repart à zéro
   tap->data.timeSlice->valueSum = 0.0 ;
   tap->data.timeSlice->nbSamplesInSlice = 0;

}

/*
 * Une sonde périodique prélève de façon exhaustive un échantillon par
 * tranche de temps. La tranche de temps s'achève, on stoque donc la
 * dernière valeur prélevée.
 */
void probe_sampleExhaustive(struct probe_t * probe, double value);
void probe_periodicNextEvent(struct probe_t * pr)
{
   assert (pr->probeType = periodicProbeType);

   printf_debug(DEBUG_PROBE_VERB, "periodic \"%s\" sampling %f\n", probe_getName(pr), pr->lastSample);
   probe_sample(pr->data.periodic->data, pr->lastSample);
}

void probe_scheduleNextEvent(struct probe_t * pr)
{
   printf_debug(DEBUG_PROBE_VERB, "about to schedule next event for \"%s\"  (type %s)\n",
		probe_getName(pr), 
		probeTypeName(pr->probeType));

   switch (pr->probeType) {
      case timeSliceAverageProbeType :
      case timeSliceThroughputProbeType :
 	 probe_timeSliceNextEvent(pr);
      break;
      case periodicProbeType :
 	 probe_periodicNextEvent(pr);
      break;

      default :
	 motSim_error(MS_WARN, "No periodic code for probe \"%s\" (type \"%s\")\n", probe_getName(pr), probeTypeName(pr->probeType));
      break;
   }

   // On programme la prochaine
   motSim_scheduleEvent(event_create((void (*)(void *))probe_scheduleNextEvent, pr),
			motSim_getCurrentTime() + pr->period);

}

// Conserve une moyenne par tranche temporelle de durée t
struct probe_t * probe_createTimeSliceAverage(double t)
{
   struct event_t * ev;

   struct probe_t * result = probe_createRaw(timeSliceAverageProbeType);

   result->data.timeSlice = (struct timeSlice_t *) sim_malloc(sizeof(struct timeSlice_t));
   assert(result->data.timeSlice);

   // On va utiliser une sonde exhaustive sur les moyennes
   result->period = t ;
   result->data.timeSlice->valueSum = 0.0 ;
   result->data.timeSlice->nbSamplesInSlice = 0.0 ;
   result->data.timeSlice->meanProbe = probe_createExhaustive();
   result->data.timeSlice->bwProbe = probe_createExhaustive();

   // On déclanche un échantillon du cumul à 0 + t
   ev = event_create((void (*)(void *))probe_scheduleNextEvent, result);
   printf_debug(DEBUG_PROBE, "premiere moyenne a %f ms(ev %p)\n", result->period, ev);
   motSim_scheduleEvent(ev, result->period);

   return result;
}

// Conserve une moyenne par tranche temporelle de durée t
struct probe_t * probe_createTimeSliceThroughput(double t)
{
   struct probe_t * result = probe_createTimeSliceAverage(t);
   result->probeType = timeSliceThroughputProbeType;


   return result;
}



/*
 * Création d'une sonde qui ne stoque que la moyenne
 */
struct probe_t * probe_createMean()
{
   struct probe_t * result = probe_createRaw(meanProbeType);

   result->data.mean = (struct mean_t *) sim_malloc(sizeof(struct mean_t));
   assert(result->data.mean);

   result->data.mean->valueSum = 0.0;
   result->data.mean->firstDate = 0.0;
   result->data.mean->lastDate = 0.0;

   return result;
}

void probe_sampleMean(struct probe_t * pr, double value)
{
   pr->data.mean->valueSum += value;
   if (pr->nbSamples == 0) {
      pr->data.mean->firstDate = motSim_getCurrentTime();
   }
   pr->data.mean->lastDate = motSim_getCurrentTime();
}

void probe_timeSliceSample(struct probe_t * pr, double value)
{
   pr->data.timeSlice->valueSum += value;
   pr->data.timeSlice->nbSamplesInSlice++;
}

double probe_meanMean(struct probe_t * pr)
{
  return pr->data.mean->valueSum / (double) pr->nbSamples;
}

double probe_IAMeanMean(struct probe_t * pr)
{
  return (pr->data.mean->lastDate - pr->data.mean->firstDate) / (double) pr->nbSamples;
}

double probe_timeSliceMean(struct probe_t * pr)
{
   // On néglige les derniers échantillons WARNING : est-ce raisonable ?
   printf_debug(DEBUG_TBD, "Incomplete average\n");
 
   return 0.0;
}

void probe_delete(struct probe_t * p)
{
   printf_debug(DEBUG_TBD, "Not yet implemented !\n");
}

/*
 * Réinitialisation de  toutes les probes. A priori cette fonction est
 * inutile ! Chaque probe est enregistrée auprès du simulateur pour
 * une réinitialisation entre deux simulations.
 */
void probe_resetAllProbes()
{
   struct probe_t * probe = firstProbe;

   printf_debug(DEBUG_PROBE, "reseting probes ...\n");

   while (probe) {
      probe_reset(probe);
      probe = probe->next;
   }

   printf_debug(DEBUG_PROBE, "probes clean ...\n");
}

void probe_sampleExhaustive(struct probe_t * probe, double value)
{
   struct sampleSet_t * currentSet = probe->data.sampleSet;

   printf_debug(DEBUG_PROBE_VERB, "%p \"%s\" : nbSamples=%ld, value = %f\n", probe, probe_getName(probe), probe->nbSamples, value);

   if(!(probe->nbSamples % PROBE_NB_SAMPLES_MAX)) {
      printf_debug(DEBUG_PROBE_VERB, "building new set\n");
      probe->data.sampleSet = (struct sampleSet_t *) sim_malloc(sizeof(struct sampleSet_t));

      probe->data.sampleSet->prev = currentSet;
      probe->data.sampleSet->next = NULL;
      if (currentSet) {
	 currentSet->next = probe->data.sampleSet;
      }
      currentSet = probe->data.sampleSet;
      //      printf_debug(DEBUG_ALWAYS, "***(pr %p, %s) New set built : %p %p\n", probe, probe_getName(probe), probe->data.sampleSet, probe->data.sampleSet->prev);
   }
  
   currentSet->dates[probe->nbSamples%PROBE_NB_SAMPLES_MAX] = motSim_getCurrentTime();
   currentSet->samples[probe->nbSamples%PROBE_NB_SAMPLES_MAX] = value;
   printf_debug(DEBUG_PROBE_VERB, "OUT\n");
}

/**
 * @brief Reading the nth sample in an exhaustive probe
 * @param probe The exhaustive probe to read from
 * @param n the number of sample to read
 * @result the value of sample n
 */
double probe_exhaustiveGetSampleN(struct probe_t * probe, int n)
{
   assert(probe->probeType == exhaustiveProbeType);
   assert(n<=probe->nbSamples); // WARNING  < or <= !?

   struct sampleSet_t * currentSet = probe->data.sampleSet;
   int numSet = probe->nbSamples / PROBE_NB_SAMPLES_MAX;

   // On va sur le bon set
   while (numSet > n / PROBE_NB_SAMPLES_MAX){
      currentSet = currentSet->prev;
      numSet--;
   }

   return currentSet->samples[n% PROBE_NB_SAMPLES_MAX];
}

/**
 * @brief Echantillon d'une valeur dans une probe à fenêtre glissante
 */
void probe_slidingWindowSample(struct probe_t * pr, double v)
{
   printf_debug(DEBUG_PROBE_VERB, "v = %f\n", v);

   // On incrémente le pointeur vers le dernier
   pr->data.window->last++;
   if (pr->data.window->last == pr->data.window->capacity) 
      pr->data.window->last = 0;

   // On incrémente la taille
   pr->data.window->length++;
   if (pr->data.window->length > pr->data.window->capacity) 
      pr->data.window->length = pr->data.window->capacity;

   // On met le truc dans le machin
   pr->data.window->samples[pr->data.window->last] = v;
   pr->data.window->dates[pr->data.window->last] = motSim_getCurrentTime();
}

/*
 * Sur une fenêtre, on fait la moyenne de tous les éléments présents
 */
double probe_slidingWindowMean(struct probe_t * p)
{
   double result = 0.0;
   int n;

   for (n = 0; n < p->data.window->length; n++) {
      result += p->data.window->samples[n];
   }

   return result/p->data.window->length;
}

double probe_meanExhaustive(struct probe_t * probe)
{
   unsigned long n;
   double sum = 0.0;
   struct sampleSet_t * currentSet = probe->data.sampleSet;

   n = probe->nbSamples;
   do {
      n--;
      if (((n%PROBE_NB_SAMPLES_MAX) == PROBE_NB_SAMPLES_MAX -1) && (n != probe->nbSamples -1)) {
         currentSet = currentSet->prev;
      }
      assert(currentSet != NULL);
      sum += currentSet->samples[n%PROBE_NB_SAMPLES_MAX];
   } while (n != 0);

   return sum / probe->nbSamples;
}

/*
 * Moyenne des interarrivées
 */
double probe_IAMeanExhaustive(struct probe_t * probe)
{
   struct sampleSet_t * currentSet = probe->data.sampleSet;
   unsigned long n = probe->nbSamples -1;
   double last, result;

   // Si on vient de changer de bloc
   if ((n % PROBE_NB_SAMPLES_MAX) == PROBE_NB_SAMPLES_MAX -1 ) {
      currentSet = currentSet->prev;
   }

   last = currentSet->dates[n%PROBE_NB_SAMPLES_MAX] ;

   while (currentSet->prev) {
      currentSet = currentSet->prev;
   }
   result = (last - currentSet->dates[0]) / (probe->nbSamples -1);
  
   //   printf("IAMeanExhaustive : last = %f, first = %f, nb = %ld => %f\n", last, currentSet->dates[0], probe->nbSamples, result);

   return result;
}

/*
double probe_IAVarianceExhaustive(struct probe_t * probe)
{
   double mean, ia, sum = 0.0;

   unsigned long n;
   struct sampleSet_t * currentSet = probe->data.sampleSet;

   // WARNING : probe->nbSamples -1 peut nous ramener dans le set précédent !!!

   double date = currentSet->dates[(probe->nbSamples -1)%PROBE_NB_SAMPLES_MAX];

   mean = probe_IAMeanExhaustive(probe);

   // Attention, le calcul date - datePrécédent n'est pas si trivial
   // car ils peuvent être sur deux set différents !!!


   for (n = probe->nbSamples - 2; n >= 0; n--) {
      if ((n%PROBE_NB_SAMPLES_MAX) == PROBE_NB_SAMPLES_MAX -1) {
         currentSet = currentSet->prev;
      }
      assert(currentSet != NULL);
      ia = date
      sum += (mean - ia)
            *(mean - ia]);
   }

   return sum / (probe->nbSamples -1);
}
*/

struct probe_t * probe_createGraphBar(double min, double max, unsigned long nbBar)
{
   unsigned long i;
   struct probe_t * result = probe_createRaw(graphBarProbeType);

   result->data.graphBar = (struct graphBar_t *)sim_malloc(sizeof(struct graphBar_t));
   assert(result->data.graphBar);

   assert(max > min);
   assert (nbBar != 0);

   result->data.graphBar->normalized = 0;
   result->data.graphBar->min = min;
   result->data.graphBar->max = max;
   result->data.graphBar->nbBar = nbBar;
   result->data.graphBar->value = (double *)sim_malloc(sizeof(double)*(size_t)nbBar);
   assert(result->data.graphBar->value);

   for (i = 0; i < nbBar; i++){
      result->data.graphBar->value[i] = 0.0;
   }
 
   printf_debug(DEBUG_PROBE, "graphBar[%f, %f] with %ld bars\n", min, max, nbBar);

   return result;
}

void probe_sampleGraphBar(struct probe_t * probe, double value)
{
   struct graphBar_t * gb = probe->data.graphBar;
   unsigned long bar;

   if (gb->normalized) {
      motSim_error(MS_WARN, "Normalized graphar '%s' is read only\n", probe_getName(probe));
      return;
   }

   if ((value < gb->max) && (value > gb->min)) {
      bar = (unsigned long)trunc((double)gb->nbBar*(value - gb->min)/(gb->max - gb->min));

      assert(bar >= 0);
      assert(bar < gb->nbBar);

      gb->value[bar]++;
      printf_debug(DEBUG_PROBE, "gb->v[%ld]=%f (%f)\n", bar, gb->value[bar], value);
   }
}

/*
 * La moyenne est obtenue en prenant la moyenne pondérée des médianes
 * de chaque barre.
 */
double probe_meanGraphBar(struct probe_t * probe)
{
   unsigned long n;
   double result = 0.0;
   struct graphBar_t * gb = probe->data.graphBar;
   
   for (n = 0 ; n < gb->nbBar; n++) {
     //     printf("[%f, %f] Contribution %d (%f) = %d sur %d\n", gb->min, gb->max, n, gb->min + (gb->max - gb->min)*(n + 0.5)/ gb->nbBar, gb->value[n], probe->nbSamples);
     result += (gb->min + (gb->max - gb->min)*(n + 0.5) / gb->nbBar ) * (gb->normalized? gb->value[n]:(gb->value[n]/probe->nbSamples));
   }

   return result;
}

/**
 * @brief Normalization of a graphBar probe
 */
void probe_graphBarNormalize(struct probe_t * pr)
{
   unsigned long n;
   struct graphBar_t * gb = pr->data.graphBar;

   for (n = 0 ; n < gb->nbBar; n++) {
      gb->value[n] = gb->value[n]/pr->nbSamples;
   }

   gb->normalized = 1;
}


void probe_EMASample(struct probe_t * probe, double value)
{
   assert(probe->probeType == EMAProbeType);

   //ici, cumuler et mémoriser lastValue, ... pas tres beau, mais faut avancer !

   // Tant que le temps n'avance pas, on cumule les valeurs
   if (motSim_getCurrentTime() == probe->lastSampleDate) {
      probe->data.ema->value += value;
   } else {
      // Le temps a avancé, on redémarre le cumul à la date actuelle
      probe->data.ema->value = value;

      // On sauvegarde les valeurs finales au temps précédent
      probe->data.ema->previousTime = probe->lastSampleDate;
      probe->data.ema->previousAvg = probe->data.ema->avg;
      probe->data.ema->previousBwAvg = probe->data.ema->bwAvg;


      // Le calcul suivant doit pouvoir être fait même si plusieurs
      // values sont échantillonnées à la même date. C'est pourquoi on
      // les cumule (sinon, durée nulle => débit infini)
      if (probe->data.ema->previousTime == 0.0) {
         probe->data.ema->avg = probe->data.ema->value;
         probe->data.ema->bwAvg =  (motSim_getCurrentTime()-probe->data.ema->previousTime);
	 //         probe->data.ema->bwAvg = probe->data.ema->value * 8.0 / (motSim_getCurrentTime()-probe->data.ema->previousTime);
      } else {
         probe->data.ema->avg = probe->data.ema->a * probe->data.ema->previousAvg + (1.0 - probe->data.ema->a) * probe->data.ema->value;
         probe->data.ema->bwAvg = probe->data.ema->a * probe->data.ema->previousBwAvg + (1.0 - probe->data.ema->a) *(motSim_getCurrentTime()-probe->data.ema->previousTime);
	 //         probe->data.ema->bwAvg = probe->data.ema->a * probe->data.ema->previousBwAvg + (1.0 - probe->data.ema->a) * probe->data.ema->value * 8.0 / (motSim_getCurrentTime()-probe->data.ema->previousTime);
      }
      printf_debug(DEBUG_PROBE_VERB, "in \"%s\" (sample %f): %f b/ %f s (ov=%f/v=%f/b=%f/ob=%f)\n",
	 	probe_getName(probe),
		value,
		probe->data.ema->value,
                motSim_getCurrentTime()-probe->data.ema->previousTime,
		probe->data.ema->avg,
		probe->data.ema->previousAvg,
		probe->data.ema->bwAvg,
		probe->data.ema->previousBwAvg);

   }

}

/*
 * Par définition !probe->data.ema->
 */
double probe_EMAMean(struct probe_t * probe)
{
   assert(probe->probeType == EMAProbeType);

   return probe->data.ema->avg;
}
double probe_EMAThroughput(struct probe_t * probe)
{
   assert(probe->probeType == EMAProbeType);
#ifdef DEBUG_NDES
   if (!strncmp(probe_getName(probe), "[DB]", 4)) {
      printf_debug(DEBUG_ALWAYS, "average %f / bwAvg %f in \"%s\" (%p, type %s)\n",
		   probe->data.ema->avg,probe->data.ema->bwAvg,
		probe_getName(probe), probe,
		probeTypeName(probe->probeType));
   }
#endif
   return 8.0*probe->data.ema->avg/probe->data.ema->bwAvg;
   //   return probe->data.ema->bwAvg;
}

/**
 * @brief Do the actual sample, without filtering or chaining
 */
void probe_doSample(struct probe_t * probe, double value)
{
   if (probe==NULL)
      return;
   printf_debug(DEBUG_PROBE_VERB, "about to sample %f in \"%s\" (%p, type %s, %lu samples)\n",
		value,
		probe_getName(probe), probe,
		probeTypeName(probe->probeType),
		probe->nbSamples);
#ifdef DEBUG_NDES
   if (!strncmp(probe_getName(probe), "[DB]", 4)) {
     printf_debug(DEBUG_ALWAYS, "about to sample %f in \"%s\" (%p, type %s, %lu samples)\n",
		  value,
		  probe_getName(probe), probe,
		  probeTypeName(probe->probeType),
		  probe->nbSamples);
   }
#endif
   switch (probe->probeType) {
      case exhaustiveProbeType : 
	probe_sampleExhaustive(probe, value);
      break;
      case meanProbeType : 
	probe_sampleMean(probe, value);
      break;
      case graphBarProbeType : 
	probe_sampleGraphBar(probe, value);
      break;
      case timeSliceAverageProbeType : 
      case timeSliceThroughputProbeType : 
	probe_timeSliceSample(probe, value);
      break;
      case slidingWindowProbeType : 
	probe_slidingWindowSample(probe, value);
      break;
      case EMAProbeType : 
	probe_EMASample(probe, value);
      break;
      case periodicProbeType :
        /* On ne fait que sauvegarder la dernière valeur
         * ce qui va être fait ci-dessous dans le code
         * commun */
      break;
      default :
	 motSim_error(MS_WARN, "No sample for probe \"%s\" (type \"%s\")\n", probe_getName(probe), probeTypeName(probe->probeType));
      break;

   }
   probe->lastSample = value;
   probe->lastSampleDate = motSim_getCurrentTime();

   if (probe->nbSamples == 0) {
      probe->min = value;
      probe->max = value;
   } else {
      probe->min =(value>probe->min)?probe->min:value;
      probe->max =(value<probe->max)?probe->max:value;
   }
   probe->nbSamples++;

   // Gestion des méta probes
   if (probe->sampleProbe) {
      probe_sample(probe->sampleProbe, value);
   }
   if (probe->meanProbe) {
      probe_sample(probe->meanProbe, probe_mean(probe));
   }
   if (probe->throughputProbe) {
     printf_debug(DEBUG_PROBE_VERB, "Throughput %f from probe \"%s\" (type \"%s\") \n", probe_throughput(probe), probe_getName(probe), probeTypeName(probe->probeType));
      probe_sample(probe->throughputProbe, probe_throughput(probe));
   }
}

void probe_sample(struct probe_t * probe, double value)
{
   probe_doSample(probe, value);

   // On chaine si nécessaire 
   if (probe->nextProbe != NULL){
      probe_sample(probe->nextProbe, value);
   }

}

/*
 * Obtention du neme echantillon. Ce n'est pas completement trivial a
 * cause des "sets".
 */
double probe_exhaustiveGetSample(struct probe_t * probe, unsigned long n)
{
   unsigned long nbSet = (probe->nbSamples +PROBE_NB_SAMPLES_MAX-1)/ PROBE_NB_SAMPLES_MAX
                         - n / PROBE_NB_SAMPLES_MAX -1;
   struct sampleSet_t * currentSet = probe->data.sampleSet;

   printf_debug(DEBUG_PROBE, "sample %ld in set %ld/%ld\n",
          n, n / PROBE_NB_SAMPLES_MAX, (probe->nbSamples +PROBE_NB_SAMPLES_MAX-1)/ PROBE_NB_SAMPLES_MAX);
   printf_debug(DEBUG_PROBE, "rewinding %ld sets\n", nbSet);
   while (nbSet--) {
      printf_debug(DEBUG_PROBE, "one set\n");
      currentSet = currentSet->prev;
   }
   printf_debug(DEBUG_PROBE, "got the key\n");

   return currentSet->samples[n%PROBE_NB_SAMPLES_MAX];
   
}

/*
 *  Pour une time slice, on fait la moyenne des moyennes par tranche
 *  temporelle. On ne prend  donc pas en compte la tranche actuelle.
 */
double probe_timeSliceAverageMean(struct probe_t * probe)
{
  return probe_meanExhaustive(probe->data.timeSlice->meanProbe);
}

/*
 *  Pour une time slice, on fait la moyenne des moyennes par tranche
 *  temporelle. On ne prend  donc pas en compte la tranche actuelle.
 */
double probe_timeSliceThroughputMean(struct probe_t * probe)
{
  return probe_meanExhaustive(probe->data.timeSlice->bwProbe);
}

double probe_mean(struct probe_t * probe)
{
   switch (probe->probeType) {
      case exhaustiveProbeType : 
	 return probe_meanExhaustive(probe);
      break;
      case meanProbeType : 
	 return probe_meanMean(probe);
      break;
      case graphBarProbeType : 
	 return probe_meanGraphBar(probe);
      break;
      case slidingWindowProbeType : 
	 return probe_slidingWindowMean(probe);
      break;
      case timeSliceAverageProbeType : 
	return probe_timeSliceAverageMean(probe);
      break;
      case timeSliceThroughputProbeType : 
	return probe_timeSliceThroughputMean(probe);
      break;
      case EMAProbeType : 
	return probe_EMAMean(probe);
      break;

      default :
	 motSim_error(MS_FATAL, "No mean for probe \"%s\" (type \"%s\")\n", probe_getName(probe), probeTypeName(probe->probeType));
         return 0.0; // Contre les warning
      break;
   }

//   probe->nbSamples++; /* !?!*/
}

double probe_IAMean(struct probe_t * probe)
{
   switch (probe->probeType) {
      case exhaustiveProbeType : 
	 return probe_IAMeanExhaustive(probe);
      break;
      case meanProbeType : 
	 return probe_IAMeanMean(probe);
      break;

      case graphBarProbeType :
	 printf_debug(DEBUG_TBD, "case graphBarProbeType not implemented !\n");
      default :
         motSim_error(MS_FATAL, "No IAMean for probe \"%s\"\n", probe_getName(probe));
         return 0.0; // Contre les warning
      break;
   }
}

double probe_min(struct probe_t * probe)
{
   return probe->min;
}

double probe_max(struct probe_t * probe)
{
   return probe->max;
}

void probe_sampleEvent(struct probe_t * probe)
{
   probe_sample(probe, 0.0); // WARNING, c'est nul  !!
}

unsigned long probe_nbSamples(struct probe_t * probe)
{
   return probe->nbSamples;
}


double probe_varianceGraphBar(struct probe_t * probe)
{
  return 0.0; // WARNING
}

#define BUFFER_LENGTH 512
void probe_exhaustiveDumpFd(struct probe_t * ep, int fd, int format)
{
   unsigned long n;
   char buffer[BUFFER_LENGTH]; //WARNING
   struct sampleSet_t * set;

   assert(ep->probeType == exhaustiveProbeType);

#ifdef DEBUG_NDES
   if (!strncmp(probe_getName(ep), "[DB]", 4)) {
      printf_debug(DEBUG_ALWAYS, "about to dump %s (type \"%s\") : %lu samples\n",
	   	probe_getName(ep), 
		probeTypeName(ep->probeType),
		ep->nbSamples);
   }
#endif
   printf_debug(DEBUG_PROBE, "about to dump %s (type \"%s\") : %lu samples\n",
		probe_getName(ep), 
		probeTypeName(ep->probeType),
		ep->nbSamples);

   // A la recherche du premier set
   for (set = ep->data.sampleSet; set->prev != NULL ; set = set->prev){};

   // On prend tous les échantillons depuis le premier
   for (n = 0 ; n < probe_nbSamples(ep); n++) {
      sprintf(buffer, "%f %f\n", set->dates[n%PROBE_NB_SAMPLES_MAX], set->samples[n%PROBE_NB_SAMPLES_MAX]);
      write(fd, buffer, strlen(buffer));

      // Si on est au bout d'un set, on décale
      if ((n+1) % PROBE_NB_SAMPLES_MAX == 0) {
	set = set->next;//printf("*** %d -> fin de set\n", n);
      }
   }
}

void probe_timeSliceAverageDumpFd(struct probe_t * p, int fd, int format)
{
   assert(p->probeType == timeSliceAverageProbeType);
   probe_exhaustiveDumpFd(p->data.timeSlice->meanProbe, fd, format);
}

void probe_timeSliceThroughputDumpFd(struct probe_t * p, int fd, int format)
{
   assert(p->probeType == timeSliceThroughputProbeType);
   probe_exhaustiveDumpFd(p->data.timeSlice->bwProbe, fd, format);
}

void probe_periodicProbeDumpFd(struct probe_t * p, int fd, int format)
{
   assert(p->probeType == periodicProbeType);
   probe_exhaustiveDumpFd(p->data.periodic->data, fd, format);
}

/*
 * Dump d'une sonde dans un fichier
 */
void probe_dumpFd(struct probe_t * probe, int fd, int format)
{

   printf_debug(DEBUG_PROBE, "about to dump %s (type \"%s\") : %lu samples\n",
		probe_getName(probe), 
		probeTypeName(probe->probeType),
		probe->nbSamples);

   switch (probe->probeType) {
      case exhaustiveProbeType : 
	 probe_exhaustiveDumpFd(probe, fd, format);
      break;
      case graphBarProbeType : 
	 probe_graphBarDumpFd(probe, fd, format);
      break;
      case timeSliceAverageProbeType : 
	 probe_timeSliceAverageDumpFd(probe, fd, format);
      break;
      case timeSliceThroughputProbeType : 
	 probe_timeSliceThroughputDumpFd(probe, fd, format);
      break;
      case periodicProbeType :
         probe_periodicProbeDumpFd(probe, fd, format);
      break;

      default :
         motSim_error(MS_FATAL, "No dump method for probe \"%s\" (type \"%s\")\n", probe_getName(probe),probeTypeName(probe->probeType));
      break;
   }
}

double probe_exhaustiveThroughput(struct probe_t * probe)
{
   motSim_error(MS_FATAL, "A FAIRE !\n");
   return 0.0;
}

double probe_meanThroughput(struct probe_t * probe)
{
   return probe->data.mean->valueSum/ (probe->data.mean->lastDate - probe->data.mean->firstDate);
   motSim_error(MS_FATAL, "A FAIRE !\n");
   return 0.0;
}

double probe_graphBarThroughput(struct probe_t * probe)
{
   motSim_error(MS_FATAL, "A FAIRE !\n");
   return 0.0;
}

/*
 * Le débit est estimé par le volume reçu divisé par la durée de la
 * période
 */
double probe_timeSliceThroughput(struct probe_t * probe)
{
   double result = 0.0;

   motSim_error(MS_FATAL, "A FAIRE !\n");
   //   result = probe->data.timeSlice->averageProbe->lastSample/probe->data.timeSlice->period;

   return result;
}

double probe_slidingWindowThroughput(struct probe_t * pr)
{
   double result = 0.0;
   double duree;
   int n;

   if (pr->data.window->length >= pr->data.window->capacity) {
      // Volume reçu depuis la première PDU
      for (n = 0; n < pr->data.window->length; n++) {
         result += pr->data.window->samples[n];
      }

      // On ne prend pas en compte la première (on s'interesse au volume
      // reçu DEPUIS elle)
      result -= pr->data.window->samples[(pr->data.window->last +1  - pr->data.window->length + pr->data.window->capacity)%pr->data.window->capacity];

      // On divise par le temps entre la première et la dernière
      // . la dernière est donnée par last
      duree = pr->data.window->dates[pr->data.window->last];

      // . la première est à l'indice 1 si on n'a pas encore rempli, à
      // last+1 si un tour est fait
      duree = duree - pr->data.window->dates[(pr->data.window->last+1)%pr->data.window->capacity];
      //      printf_debug(DEBUG_ALWAYS, "%f DE %f A %f\n", result, pr->data.window->dates[(pr->data.window->last+1)%pr->data.window->capacity], pr->data.window->dates[pr->data.window->last] );
   } else { // Si on n'a pas encore fait un tour ...
      // Volume reçu depuis la première PDU (non inclue)
      for (n = 2; n <= pr->data.window->length; n++) {
         result += pr->data.window->samples[n];
      }
      // On divise par le temps entre la première et la dernière
      // . la dernière est donnée par last
      duree = pr->data.window->dates[pr->data.window->last];

      duree = duree - pr->data.window->dates[1];
      //      printf_debug(DEBUG_ALWAYS, "%f DE0 %f A %f\n", result, pr->data.window->dates[1], pr->data.window->dates[pr->data.window->last]);

   }

   // Les tailles sont en octets, les durées en secondes
   // Mais les débits en bit/s !
   return 8.0*result/duree;
}

/*
 * Consultation du débit. On considère ici chaque nouvelle valeur
 * comme la taille d'une nouvelle PDU. La fonction suivante permet
 * alors de connaitre le débit qui en  découle. 
 * La méthode de calcul est évidemment dépendante de la nature de la
 * sonde et sa précision est donc variable
 */
double probe_throughput(struct probe_t * probe)
{
   switch (probe->probeType) {
      case exhaustiveProbeType : 
	 return probe_exhaustiveThroughput(probe);
      break;
      case meanProbeType : 
	 return probe_meanThroughput(probe);
      break;
      case graphBarProbeType : 
	 return probe_graphBarThroughput(probe);
      break;
      case slidingWindowProbeType : 
	 return probe_slidingWindowThroughput(probe);  
      break;
      case EMAProbeType :
         return probe_EMAThroughput(probe);
      break;

      default :
         motSim_error(MS_FATAL, "No throughput for probe \"%s\" (type \"%s\")\n", probe_getName(probe), probeTypeName(probe->probeType));
         return 0.0; // Contre les warning
      break;
   }
}

void probe_graphBarDumpFd(struct probe_t * probe, int fd, int format)
{
   unsigned long n;
   struct graphBar_t * gb = probe->data.graphBar;
   char buffer[BUFFER_LENGTH]; //WARNING

   assert(probe->probeType == graphBarProbeType);

   for (n = 0; n < gb->nbBar; n++){
     //      printf("%f %d\n", gb->min+(n+0.5)*(gb->max-gb->min)/gb->nbBar, gb->value[n]);
      sprintf(buffer, "%f %f\n", gb->min+(n+0.5)*(gb->max-gb->min)/gb->nbBar, gb->value[n]);
      write(fd, buffer, strlen(buffer));
   }
}

double probe_varianceExhaustive(struct probe_t * probe)
{
   double mean;

   unsigned long n;
   double sum = 0.0;
   struct sampleSet_t * currentSet = probe->data.sampleSet;

   mean = probe_meanExhaustive(probe);

   n = probe->nbSamples;
   do {
      n--;
      if (((n%PROBE_NB_SAMPLES_MAX) == PROBE_NB_SAMPLES_MAX -1) && (n != probe->nbSamples -1)) {
         currentSet = currentSet->prev;
      }
      assert(currentSet != NULL);
      sum += (mean - currentSet->samples[n%PROBE_NB_SAMPLES_MAX])
            *(mean - currentSet->samples[n%PROBE_NB_SAMPLES_MAX]);
   } while (n != 0);

   return sum / (probe->nbSamples - 1);
}

double probe_variance(struct probe_t * probe)
{
   switch (probe->probeType) {
      case exhaustiveProbeType : 
	return probe_varianceExhaustive(probe);
      break;
      case graphBarProbeType : 
	return probe_varianceGraphBar(probe);
      break;
      default :
         motSim_error(MS_FATAL, "No variance for probe \"%s\" (type \"%s\")\n", probe_getName(probe), probeTypeName(probe->probeType));
	 return 0.0; // Contre les warning
      break;
   }
}

double probe_ecartType(struct probe_t * probe)
{
   return sqrt(probe_variance(probe));
}

double probe_stdDev(struct probe_t * probe)
{
   return sqrt(probe_variance(probe));
}

/**
 * @brief Experimental coefficient of variation
 */
double probe_coefficientOfVariation(struct probe_t * probe)
{
   int n;
   double mean = probe_mean(probe);
   double result = 0.0;

   if (probe->probeType != exhaustiveProbeType) {
      return NAN;
   }

   for (n = 0; n < probe_nbSamples(probe); n++) {
     result += (probe_exhaustiveGetSampleN(probe, n)-mean)*(probe_exhaustiveGetSampleN(probe, n)-mean);
   }

   result = sqrt(result/probe_nbSamples(probe)) / mean;
   return result;
}


/*
 * Demi largeur de l'intervalle de confiance à 5%
 */
double probe_exhaustiveDemiIntervalleConfiance5pc(struct probe_t * p)
{
   double result;

   assert(p->probeType == exhaustiveProbeType);

   result = 1.96*probe_ecartType(p)/sqrt((double)p->nbSamples);

   return result;
}

double probe_timeSliceAverageDemiIntervalleConfiance5pc(struct probe_t * p)
{
   assert(p->probeType == timeSliceAverageProbeType);

   return probe_demiIntervalleConfiance5pc(p->data.timeSlice->meanProbe);
}

/*
 * Demi largeur de l'intervalle de confiance à 5%
 */
double probe_demiIntervalleConfiance5pc(struct probe_t * p)
{
   double result = 0.0;

   switch (p->probeType) {
      case exhaustiveProbeType : 
         result = probe_exhaustiveDemiIntervalleConfiance5pc(p);
      break;
      case  timeSliceAverageProbeType: 
         result = probe_timeSliceAverageDemiIntervalleConfiance5pc(p);
      break;
      default :
	motSim_error(MS_FATAL, "No confidence interval for probe \"%s\" (type \"%s\")\n", probe_getName(p),probeTypeName(p->probeType));
	 return 0.0; // Contre les warning
      break;
   }

   return result;
}

/*
 * Tentative de calcul de l'IC à 5% par la méthode des coupes. C'est
 * très probablement faux ! Combien de blocs de quelle taille par
 * exemple ?
 */
double probe_demiIntervalleConfiance5pcCoupes(struct probe_t * p)
{
   double result;
   struct probe_t * pb;

   assert(p->probeType == exhaustiveProbeType);

   pb = probe_createExhaustive();

   probe_exhaustiveToBlockMean(p, pb, probe_nbSamples(p)/50);

   result = probe_demiIntervalleConfiance5pc(pb);

   probe_delete(pb);

   return result;
}

/**
 * @brief Conversion d'une sonde exhaustive en une graphBar
 */
void probe_exhaustiveToGraphBar(struct probe_t * ep, struct probe_t * gbp)
{
   unsigned long n;
   struct sampleSet_t * currentSet = ep->data.sampleSet;

   assert(ep != NULL);
   assert(gbp != NULL);
   assert(ep->probeType == exhaustiveProbeType);
   assert(gbp->probeType == graphBarProbeType);

   // On remonte les echantillons du dernier au premier
   n = ep->nbSamples ;
   do {
      n--;    // on recule pour le prochain
      if (((n%PROBE_NB_SAMPLES_MAX) == PROBE_NB_SAMPLES_MAX -1) && (n != ep->nbSamples - 1)) {  // Changement de set ?
         currentSet = currentSet->prev;
	 printf_debug(DEBUG_PROBE, "previous set\n");
      }
      assert(currentSet != NULL);
      printf_debug(DEBUG_PROBE, "ep[%ld]=%f\n", n, currentSet->samples[n%PROBE_NB_SAMPLES_MAX]);
      probe_sample(gbp, currentSet->samples[n%PROBE_NB_SAMPLES_MAX]);
   } while (n != 0);
}

/*
 * Réduction du nombre d'échantillons d'une sonde exhaustive en
 * remplaçant blockSize échantillons consécutifs par leur moyenne
 */
void probe_exhaustiveToBlockMean(struct probe_t * ep, struct probe_t * bmp, unsigned long blockSize)
{
   unsigned long  n;
   double sum = 0.0;
   struct sampleSet_t * set;

   assert(ep != NULL);
   assert(bmp != NULL);
   assert(ep->probeType == exhaustiveProbeType);
   assert(ep->nbSamples != 0);
   assert(ep->data.sampleSet != NULL);
   /*
   printf("*** [pr %p : %s] %d samples\n", ep, probe_getName(ep), ep->nbSamples);
   printf("*** %p\n", ep->data.sampleSet);
   printf("*** %p\n", ep->data.sampleSet->prev);

   */
   // A la recherche du premier set
   for (set = ep->data.sampleSet; set->prev != NULL ; set = set->prev){};

   // On prend tous les échantillons depuis le premier
   for (n = 0 ; n < probe_nbSamples(ep); n++) {
      sum += set->samples[n%PROBE_NB_SAMPLES_MAX];
      // Si on est au bout d'un set, on décale
      if ((n+1) % PROBE_NB_SAMPLES_MAX == 0) {
	set = set->next;//printf("*** %d -> fin de set\n", n);
      }
      // Si on a assez d'échantillons, on stoque la moyenne
      if ((n+1) % blockSize == 0) {
	//printf("*** %d -> On sample\n", n);
         probe_sample(bmp, sum/(double)blockSize);
	 sum = 0.0;
      }
   }
}

/*
 * Modification du nom, il est copié depuis le paramètre
 * qui peut donc être détruit ensuite
 */
void probe_setName(struct probe_t * p, char * name)
{
   char n[1024];

   printf_debug(DEBUG_PROBE, "\"%s\" (%p)\n", name, p);

   free(p->name);

   p->name = strdup(name);
   switch (p->probeType) {
      case periodicProbeType :
        sprintf(n, "%s (ex. sub-probe)", name);
 	probe_setName(p->data.periodic->data, n);
      break;

      default :
      break;
   }
}


/*
 * Lecture du nom. C'est un pointeur sur le nom qui
 * est retourné, il doit donc être copié avant toute
 * modification/destruction.
 */
char * probe_getName(struct probe_t * p)
{
   return p->name;
}

/*
 * Lecture du nombre min d'échantillons dans un graphBar
  */
int probe_graphBarGetMinValue(struct probe_t * p)
{
   unsigned n;
   struct graphBar_t  * graphBar = p->data.graphBar;
   int result = graphBar->value[0];

   for (n = 1; n < graphBar->nbBar; n++) {
      result = (result < graphBar->value[n])?result:graphBar->value[n];
   };

   return result;
} 

/*
 * Lecture du nombre max d'échantillons dans un graphBar
 */
int probe_graphBarGetMaxValue(struct probe_t * p)
{
   unsigned long n;
   struct graphBar_t  * graphBar = p->data.graphBar;
   int result = graphBar->value[0];

   for (n = 1; n < graphBar->nbBar; n++) {
      result = (result > graphBar->value[n])?result:graphBar->value[n];
   };

   return result;
} 

int probe_graphBarGetValue(struct probe_t * p, int n)
{
   struct graphBar_t  * graphBar = p->data.graphBar;
   int result = graphBar->value[n];

   return result;
}

/*
 * Les méta sondes !!
 * 
 * La sonde p2 observe une valeur de la sonde 1. p2 sera typiquement
 * une sonde périodique et p1 une sonde non exhaustive !
 */
void probe_addMeanProbe(struct probe_t * p1, struct probe_t * p2)
{
  addProbe(p1->meanProbe, p2);
}

void probe_addThroughputProbe(struct probe_t * p1, struct probe_t * p2)
{
  addProbe(p1->throughputProbe, p2);
}

void probe_addSampleProbe(struct probe_t * p1, struct probe_t * p2)
{
  addProbe(p1->sampleProbe, p2);
}


/*****************************************************************************
       Probes and filters
 */

/**
 * @brief Set a filter to a probe
 *
 * If the probe is used through probe_sampleValuePDUFilter, the sample
 * will be done iif the PDU validates the filter
 */
void probe_setFilter(struct probe_t * probe, 
		     struct PDUFilter_t * filter)
{
   probe->filter = filter;
}

/**
 * @brief Echantillonage d'une valeur
 * @param probe La sonde dans laquelle on veut enregistrer
 * @param value La valeur à enregistrer
 * @param pdu une PDU sur laquelle le filtre sera appliqué
 * La probe peut être NULL, auquel cas rien n'est enregistré,
 * naturellement.
 * Attention, l'échantillonage se fera SI ET SEULEMENT SI la PDU passe
 * le fitre.
 */
void probe_sampleValuePDUFilter(struct probe_t * probe, 
			        double value, struct PDU_t* pdu)
{
   printf_debug(DEBUG_PROBE_VERB, "IN '%s' (filter %p pdu %p)\n", probe_getName(probe), probe->filter, pdu);

   if ((probe->filter == NULL) || PDUFilter_filterPDU(probe->filter, pdu)) {
      printf_debug(DEBUG_PROBE_VERB, "Ca passe\n");
      probe_doSample(probe, value);
   }else {
      printf_debug(DEBUG_PROBE_VERB, "Ca FOIRE\n");
   }
   // Chaining ...
   if (probe->nextProbe != NULL){
      printf_debug(DEBUG_PROBE_VERB, "About to chain\n");
      probe_sampleValuePDUFilter(probe->nextProbe, value, pdu);
   }
   printf_debug(DEBUG_PROBE_VERB, "OUT\n");
}


