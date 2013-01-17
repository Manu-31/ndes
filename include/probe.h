/*----------------------------------------------------------------------*/
/*   Gestion des probes.                                                */
/*----------------------------------------------------------------------*/
#ifndef __DEF_PROBE
#define __DEF_PROBE

struct probe_t;

enum probeType_t {
   exhaustiveProbeType,           // Conserve tous les �chantillons
   meanProbeType,                 // Conserve la moyenne
   timeSliceAverageProbeType,     // Conserve des moyennes temporelles
   timeSliceThroughputProbeType,  // Conserve le d�bit moyen par tranche de temps
   graphBarProbeType,             // Conserve un histogramme
   EMAProbeType,                  // Exponential Moving Average AFAIRE
   slidingWindowProbeType,        // Conserve une fen�tre de valeurs AFAIRE
   periodicProbeType              // Enregistre p�riodiquement une valeur
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
 * Pour le moment, c'est forc�ment des doubles
 */
// Conserve tous les �chantillons
struct probe_t * probe_createExhaustive();  

// Conserve des �chantillons sur une fen�tre
struct probe_t * probe_slidingWindowCreate(int windowLength);

// Ne conserve aucun �chantillon, juste la somme et le nombre
struct probe_t * probe_createMean();

// Conserve une moyenne sur chaque tranche temporelle de dur�e t
struct probe_t * probe_createTimeSliceAverage(double t);

// Conserve un d�bit moyen par tranche temporelle de dur�e t
struct probe_t * probe_createTimeSliceThroughput(double t);

// Conserve un �chantillon � la fin de chaque tranche temporelle de dur�e t
struct probe_t * probe_periodicCreate(double t);

// Conserve une moyenne mobile M <- a.M + (1-a).sample
struct probe_t * probe_EMACreate(double a);

/*
 * Une sonde qui compte le nombre d'occurences par intervalle
 */
struct probe_t * probe_createGraphBar(double min, double max, unsigned long nbInt);

/*
 * Destruction d'une probe
 */
void probe_delete(struct probe_t * p);

/*
 * Cha�nage des probes p1 et p2, dans cet ordre. Tout �chantillon sur
 * p1 sera r�percut� sur p2. C'est la seule m�thode qui soit
 * r�perecut�e en cascade. Les reset, calcul de moyenne, ... doivent
 * �tre invoqu�es sur chaque sonde si n�cessaire
 */
void probe_chain(struct probe_t * p1, struct probe_t * p2);

/*
 * R�initialisation d'une probe (pour permettre de relancer une
 * simulation dans les m�mes conditions). Tout est effac� et doit donc
 * avoir �t� sauvegard� si besoin.
 */
void probe_reset(struct probe_t * probe);

/*
 * Une sonde persistante ne sera pas r�initialis�e en cas de reset (en
 * fin de simulation)
 */
void probe_setPersistent(struct probe_t * p);

void probe_resetAllProbes();

/*
 * Modification du nom, il est copi� depuis le param�tre
 * qui peut donc �tre d�truit ensuite
 */
void probe_setName(struct probe_t * p, char * name);

/*
 * Lecture du nom. C'est un pointeur sur le nom qui
 * est retourn�, il doit donc �tre copi� avent toute
 * modification/destruction.
 */
char * probe_getName(struct probe_t * p);

/*
 * Echantillonage d'une valeur
 */
void probe_sample(struct probe_t * probe, double value);

/*
 * Echantillonage de la date d'occurence d'un evenement
 */
void probe_sampleEvent(struct probe_t * probe);

/*
 * Lecture d'un echantillon
 */

double probe_exhaustiveGetSample(struct probe_t * probe, unsigned long n);

/*
 * Nombre d'echantillons
 */
unsigned long probe_nbSamples(struct probe_t * probe);
double probe_max(struct probe_t * probe);
double probe_min(struct probe_t * probe);

/*
 * Valeur moyenne, variance, �cart type, ... empriques !
 */
double probe_mean(struct probe_t * probe);
double probe_variance(struct probe_t * probe);
double probe_stdDev(struct probe_t * probe);

/*
 * Demi largeur de l'intervalle de confiance � 5%
 */
double probe_demiIntervalleConfiance5pc(struct probe_t * p);
/*
 * Tentative de calcul de l'IC � 5% par la m�thode des coupes. C'est
 * tr�s probablement faux ! Combien de blocs de quelle taille par
 * exemple ?
 */
double probe_demiIntervalleConfiance5pcCoupes(struct probe_t * p);

/*
 * Les moments de la loi d'inter-arriv�e des �v�nements de sondage
 */
double probe_IAMean(struct probe_t * probe);
double probe_IAVariance(struct probe_t * probe);
double probe_IAStdDev(struct probe_t * probe);

/*
 * Consultation du d�bit. On consid�re ici chaque nouvelle valeur
 * comme la taille d'une nouvelle PDU. La fonction suivante permet
 * alors de connaitre le d�bit qui en  d�coule. 
 *
 * Il s'agit d'une valeur "instantan�e"
 *
 * La m�thode de calcul est �videmment d�pendante de la nature de la
 * sonde et sa pr�cision est donc variable
 *
 * - Fen�tre glissante : le d�bit est, � tout moment, le rapport entre
 * la taille cumul�e re�ue et la dur�e depuis la premi�re PDU re�ue
 * non inclue pour la taille).
 * - Fen�tre temporelle : rapport entre la taille re�ue durant la
 * derni�re  fen�tre r�volue et sa dur�e
 */
double probe_throughput(struct probe_t * p);

/*
 * Reading the nth sample in an exhaustive probe
 */
double probe_exhaustiveGetSampleN(struct probe_t * probe, int n);

/*
 * Conversion d'une sonde exhaustive en une graphBar
 */
void probe_exhaustiveToGraphBar(struct probe_t * ep, struct probe_t * gbp);

/*
 * R�duction du nombre d'�chantillons d'une sonde exhaustive en
 * rempla�ant blockSize �chantillons cons�cutifs par leur moyenne
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
 * Cha�ner la nouvelle probe p2 dans une liste d�butant par p1 qui
 * peut �tre nul 
 * 
 *   p1 <- p2 suivi de p1
 */
#define addProbe(p1, p2) {assert(p2 != NULL) ; p2->nextProbe = p1;p1 = p2;}

/*
 * Les m�ta sondes !!
 * 
 * La sonde p2 observe une valeur de la sonde 1. p2 sera typiquement
 * une sonde p�riodique et p1 une sonde non exhaustive !
 */
// Une sonde syst�matique. Ceci peut �tre utile lorsque p2 collecte
// les �chantillons de plusieurs sondes
void probe_addSampleProbe(struct probe_t * p1, struct probe_t * p2);

// Une sonde sur la moyenne
void probe_addMeanProbe(struct probe_t * p1, struct probe_t * p2);

// Une sonde sur le d�bit
void probe_addThroughputProbe(struct probe_t * p1, struct probe_t * p2);

/*
 * Nombre maximal d'echantillons dans un set. Ca n'a pas lieu d'�tre
 * public a priori, mais c'est pratique pour certains tests de debogage
 */
#define PROBE_NB_SAMPLES_MAX 32768


#endif

