/**
 * @file probe.h
 * @brief Gestion des probes
 */
#ifndef __DEF_PROBE
#define __DEF_PROBE

#include <pdu.h>
#include <pdu-filter.h>

struct probe_t;

enum probeType_t {
   exhaustiveProbeType,           // Conserve tous les échantillons
   meanProbeType,                 // Conserve la moyenne
   timeSliceAverageProbeType,     // Conserve des moyennes temporelles
   timeSliceThroughputProbeType,  // Conserve le débit moyen par tranche de temps
   graphBarProbeType,             // Conserve un histogramme
   EMAProbeType,                  // Exponential Moving Average AFAIRE
   slidingWindowProbeType,        // Conserve une fenêtre de valeurs AFAIRE
   periodicProbeType              // Enregistre périodiquement une valeur
};


#define probeTypeName(t) \
(t == exhaustiveProbeType)?"exhaustive":(\
(t == meanProbeType)?"mean":(\
(t == timeSliceAverageProbeType)?"timeSliceAverage":(\
(t == timeSliceThroughputProbeType)?"timeSliceThroughput":(\
(t == graphBarProbeType)?"graphBar":(\
(t == EMAProbeType)?"EMA":(\
(t == periodicProbeType)?"periodic":(\
(t == slidingWindowProbeType)?"slidingWindow":"???"))))))) 

/*
 * Pour le moment, c'est forcément des doubles
 */
/**
 * @brief Création d'une sonde exhaustive
 * Attention, une telle sonde conserve tous les échantillons
 */
struct probe_t * probe_createExhaustive();  

/**
 * Une telle sonde conserve des échantillons sur une fenêtre
 */
struct probe_t * probe_slidingWindowCreate(int windowLength);

// Ne conserve aucun échantillon, juste la somme et le nombre
struct probe_t * probe_createMean();

// Conserve une moyenne sur chaque tranche temporelle de durée t
struct probe_t * probe_createTimeSliceAverage(double t);

// Conserve un débit moyen par tranche temporelle de durée t
struct probe_t * probe_createTimeSliceThroughput(double t);

/** 
 * @brief Création d'une sonde périodique
 * @param t Période de l'échantillonage
 *
 * Une telle sonde conserve un échantillon à la fin de chaque tranche
 * temporelle de durée t 
 */
struct probe_t * probe_periodicCreate(double t);

// Conserve une moyenne mobile M <- a.M + (1-a).sample
struct probe_t * probe_EMACreate(double a);

/**
 * @brief Création d'une sonde qui compte le nombre d'occurences par
 * intervalle
 * @param min Minimal value
 * @param max Maximal value
 * @param nbInt Number of bars
 * @result Created graphBar probe
 */
struct probe_t * probe_createGraphBar(double min,
				      double max,
				      unsigned long nbInt);

/**
 * @brief Normalization of a graphBar probe
 */
void probe_graphBarNormalize(struct probe_t * pr);

/**
 * @brief Destruction d'une probe
 */
void probe_delete(struct probe_t * p);

/*
 * Chaînage des probes p1 et p2, dans cet ordre. Tout échantillon sur
 * p1 sera répercuté sur p2. C'est la seule méthode qui soit
 * répercutée en cascade. Les reset, calcul de moyenne, ... doivent
 * être invoquées sur chaque sonde si nécessaire
 */
struct probe_t * probe_chain(struct probe_t * p1, struct probe_t * p2);

/*
 * Réinitialisation d'une probe (pour permettre de relancer une
 * simulation dans les mêmes conditions). Tout est effacé et doit donc
 * avoir été sauvegardé si besoin.
 */
void probe_reset(struct probe_t * probe);

/**
 * @brief Define a probe as persistent
 * Une sonde persistante ne sera pas réinitialisée en cas de reset (en
 * fin de simulation)
 */
void probe_setPersistent(struct probe_t * p);

void probe_resetAllProbes();

/*
 * Modification du nom, il est copié depuis le paramètre
 * qui peut donc être détruit ensuite
 */
void probe_setName(struct probe_t * p, char * name);

/*
 * Lecture du nom. C'est un pointeur sur le nom qui
 * est retourné, il doit donc être copié avent toute
 * modification/destruction.
 */
char * probe_getName(struct probe_t * p);

/**
 * @brief Echantillonage d'une valeur
 * @param probe La sonde dans laquelle on veut enregistrer
 * @param value La valeur à enregistrer
 * La probe peut être NULL, auquel cas rien n'est enregistré,
 * naturellement 
 */
void probe_sample(struct probe_t * probe, double value);

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
		     struct PDUFilter_t * filter);

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
			   double value, struct PDU_t* pdu);

/*
 * Echantillonage de la date d'occurence d'un evenement
 */
void probe_sampleEvent(struct probe_t * probe);

/*
 * Lecture d'un echantillon
 */

double probe_exhaustiveGetSample(struct probe_t * probe, unsigned long n);

/*****************************************************************************
       Consultation des métriques offertes par une sonde
 */

/**
 * @brief Nombre d'echantillons
 */
unsigned long probe_nbSamples(struct probe_t * probe);

/**
 * @brief Maximal sampled value
 */
double probe_max(struct probe_t * probe);

/**
 * @brief Minimal sampled value
 */
double probe_min(struct probe_t * probe);

/*
 * Valeur moyenne, variance, écart type, ... empriques !
 */
double probe_mean(struct probe_t * probe);
double probe_variance(struct probe_t * probe);
double probe_stdDev(struct probe_t * probe);

/**
 * @brief Experimental coefficient of variation
 */
double probe_coefficientOfVariation(struct probe_t * probe);

/*
 * Demi largeur de l'intervalle de confiance à 5%
 */
double probe_demiIntervalleConfiance5pc(struct probe_t * p);
/*
 * Tentative de calcul de l'IC à 5% par la méthode des coupes. C'est
 * très probablement faux ! Combien de blocs de quelle taille par
 * exemple ?
 */
double probe_demiIntervalleConfiance5pcCoupes(struct probe_t * p);

/*
 * Les moments de la loi d'inter-arrivée des événements de sondage
 */
double probe_IAMean(struct probe_t * probe);
double probe_IAVariance(struct probe_t * probe);
double probe_IAStdDev(struct probe_t * probe);

/*
 * Consultation du débit. On considère ici chaque nouvelle valeur
 * comme la taille d'une nouvelle PDU. La fonction suivante permet
 * alors de connaitre le débit qui en  découle. 
 *
 * Il s'agit d'une valeur "instantanée"
 *
 * La méthode de calcul est évidemment dépendante de la nature de la
 * sonde et sa précision est donc variable
 *
 * - Fenêtre glissante : le débit est, à tout moment, le rapport entre
 * la taille cumulée reçue et la durée depuis la première PDU reçue
 * non inclue pour la taille).
 * - Fenêtre temporelle : rapport entre la taille reçue durant la
 * dernière  fenêtre révolue et sa durée
 */
double probe_throughput(struct probe_t * p);

/**
 * @brief Reading the nth sample in an exhaustive probe
 * @param probe The exhaustive probe to read from
 * @param n the number of sample to read
 * @result the value of sample n
 */
double probe_exhaustiveGetSampleN(struct probe_t * probe, int n);

/**
 * @brief Reading the nth sample's date in an exhaustive probe
 * @param probe The exhaustive probe to read from
 * @param n the number of sample to read
 * @result the date of sample n
 */
double probe_exhaustiveGetDateN(struct probe_t * probe, int n);

/*
 * Conversion d'une sonde exhaustive en une graphBar
 */
void probe_exhaustiveToGraphBar(struct probe_t * ep, struct probe_t * gbp);

/*
 * Réduction du nombre d'échantillons d'une sonde exhaustive en
 * remplaçant blockSize échantillons consécutifs par leur moyenne
 */
void probe_exhaustiveToBlockMean(struct probe_t * ep, struct probe_t * bmp, unsigned long blockSize);

#define dumpGnuplotFormat 1

/*
 * Dump d'une sonde dans un fichier
 */
void probe_dumpFd(struct probe_t * probe, int fd, int format);
/*
 * Dump d'un graphBar dans un fichier
 */
void probe_graphBarDumpFd(struct probe_t * probe, int fd, int format);

int probe_graphBarGetMinValue(struct probe_t * probe);
int probe_graphBarGetMaxValue(struct probe_t * probe);
int probe_graphBarGetValue(struct probe_t * probe, int n);

/*
 * Chaîner la nouvelle probe p2 dans une liste débutant par p1 qui
 * peut être nul 
 * 
 *   p1 <- p2 suivi de p1
 */
#define addProbe(p1, p2) {assert(p2 != NULL) ; p2->nextProbe = p1;p1 = p2;}

/*
 * Les méta sondes !!
 * 
 * La sonde p2 observe une valeur de la sonde 1. p2 sera typiquement
 * une sonde périodique et p1 une sonde non exhaustive !
 */
// Une sonde systématique. Ceci peut être utile lorsque p2 collecte
// les échantillons de plusieurs sondes
void probe_addSampleProbe(struct probe_t * p1, struct probe_t * p2);

// Une sonde sur la moyenne
void probe_addMeanProbe(struct probe_t * p1, struct probe_t * p2);

// Une sonde sur le débit
void probe_addThroughputProbe(struct probe_t * p1, struct probe_t * p2);

/*
 * Nombre maximal d'echantillons dans un set. Ca n'a pas lieu d'être
 * public a priori, mais c'est pratique pour certains tests de debogage
 */
#define PROBE_NB_SAMPLES_MAX 32768


#endif

