/**
 * @file schedACM.h
 * @brief Forme générale d'un ordonnanceur pour un lien ACM
 *
 * Ce fichier définit un cadre général pour l'implantation d'un
 * ordonnanceur sur un lien ACM.
 * 
 * Comme tout outil de NDES, un ordonnanceur est caractérisé par une
 * fontion d'entrée schedACM_processPDU et une fonction de sortie
 * schedACM_getPDU. Ces deux fonctions invoquent une fonction
 * générique (schedACM_processPDUGeneric et schedACM_getPDUGeneric
 * respectivement), mais chaque ordonnanceur peut redéfinir sa propre
 * fonction (a déclaer dans le champ func).
 *
 * Fonction d'entrée
 * 
 * Le comportement de la fonction schedACM_processPDUGeneric est assez
 * simple et ne nécessite a priori pas de redéfinition. Cette fonction
 * ne fait que prendre le paquet, le mettre dans une file d'attente,
 * et invoquer si necessaire l'ordonnancement via la fonction
 * schedACM_buildBBFRAME.
 *
 * Fonction de sortie
 *
 * La fonction schedACM_getPDUGeneric est également très simple,
 * puisqu'elle se contente d'invoquer la fonction
 * schedACM_buildBBFRAME. On ne la remplacera donc pas par une
 * fonction spécifique dans la mesure du possible.
 *
 * La fonction schedACM_buildBBFRAMEGeneric quant à elle invoque la fonction
 * d'ordonnancement et utilise le champ sched->solutionChoisie calculé
 * par cette dernière pour construire effectivement la BBFRAME.
 *
 * Tout repose donc évidemment sur la fonction d'odonnancement, dont
 * le rôle est donc de déterminer un remplissage de BBFRAME. Ce
 * remplissage ne sera pas mis en oeuvre (les files ne sont pas
 * touchées) mais décrit dans la structure solutionChoisie. C'est la
 * fonction schedACM_buildBBFRAMEGeneric qui se chargera de construire
 * la BBFRAME et de vider les files conformément à cette solution.
 *
 * Si cette structure convient, l'implantation de l'ordonnanceur
 * réside donc dans l'écriture d'une fonction d'ordonnancement qui
 * construit une solution. C'est la seule fonction à définir. 
 * Si la structure ne suffit pas, il faut redéfinir éventuellement la
 * fonction de création d'une BBFRAME, voire la fonction getPDU.
 */
#ifndef __SCHED_ACM
#define __SCHED_ACM

#include <motsim.h>
#include <file_pdu.h>
#include <dvb-s2-ll.h>

#define alpha 0.99
#define calculeEMA(ema, sample, al)     \
   (al) * (ema) + (1.0 - (al))*(sample) 


/*
 * Les éléments de gestion de la QoS d'une file
 */
typedef struct {
   int typeQoS;              // Quelle QoS ?
   double beta;              // Un paramètre lié au type de QoS
   double rmin;              // Un paramètre lié au type de QoS (débit "min")
   double debit;             // Une mesure du débit
   struct probe_t * bwProbe; // Une sonde sur le débit évalué par et pour l'algo
} t_qosMgt;

/**
 * @brief Un mode de remplissage d'une BBFRAME.
 *
 * Cette structure permet de définir le remplissage d'une BBFRAME,
 * sans réellement le mettre en oeuvre. Cela permet à l'algorithme
 * d'évaluer les conséquences d'une décision d'ordonnancement sans
 * aller chercher les paquets dans les files, par exemple. C'est donc
 * avec cet outil que l'ordonnanceur peut envisager diverses
 * solutions.
 *
 */
typedef struct {
   int       modcod;         //!< Le numero de MODCOD choisi
   int    ** nbrePaquets;    //!< Nombre de paquets de chaque file à transmettre
   int       volumeTotal;    //!< Le nombre total d'octets à transmettre

   /* Les champs suivants sont à la dispo de l'ordonnanceur. Il
      faudrait surement faire plus propre, avec un  pointeur sur prive
      ou une union, ... */
   double    interet;
   int       nbChoix;        // Nombre de choix menant à cet interet
   int       casTraite;      // Pour éviter de retraiter un cas
} t_remplissage ;

/**
 * @brief Une séquence de remplissages
 *
 * Cette structure permer de ne plus simplement chercher une trame
 * avec l'ordonnanceur, mais une séquence de trames de longueur
 * quelconque.
 */
typedef struct {
   int longueur; //!< Nombre maximal de trames consécutives

   /* Les champs suivants sont à la dispo de l'ordonnanceur. Il
   faudrait surement faire plus propre, avec un  pointeur sur prive
   ou une union, ... */

   int       positionActuelle; //!< Identification de la BBFRAME en
                               //!cours d'analyse
   double    interet;
   int       casTraite;        //!< Pour éviter de retraiter un cas
} t_sequence ;

/**
 * @brief Définition des fonctions que doit implanter un ordonnanceur
 * sur un lien ACM.
 *
 * Les fonctions getPDU/processPDU peuvent être null si rien de
 * spécifique n'est nécessaire. Des fonctions génériques font le
 * travail.
 *
 * La fonction buildBBFRAME est invoquée par les fonctions
 * getPDU/processPDU pour construire une BBFRAME lorsque le support
 * est disponible. Elle peut être null, même si les deux fonctions
 * précédentes ne le sont pas. Une fonction générique se chargera
 * alors d'invoquer l'ordonnanceur.
 *
 * la fonction schedule est invoquée par la fonction buildBBFRAME
 * générique. Si cette dernière est utilisée, la fonction
 * d'ordonnancement ne peut donc pas être null. Elle doit construire
 * le champ "solutionChoisie" de la structure schedACM. C'est ce
 * champ, de type t_remplissage, 
 * qui est utilisé par la fonction buildBBFRAME générique pour
 * construire la BBFRAME en fonction du choix d'ordonnancement.
 */
struct schedACM_func_t {
   struct PDU_t * (*getPDU)(void * private);
   int  (*processPDU)(void * private,
	               getPDU_t getPDU, void * source);

   struct PDU_t * (* buildBBFRAME)(void * private);

   void (*schedule)(void * private);
};

struct schedACM_t;

/*
 * Création d'un scheduler avec sa "destination". Cette dernière doit
 * être de type struct DVBS2ll_t  et avoir déjà été complêtement
 * construite (tous les MODCODS créés).
 * Le nombre de files de QoS différentes par MODCOD est également
 * passé en paramètre.
 */
struct schedACM_t * schedACM_create(struct DVBS2ll_t * dvbs2ll, int nbQoS, int declOK,
				    struct schedACM_func_t * func);

/*
 * Attribution des files d'attente d'entrée pour un MODCOD donné dans
 * le paramètre mc. Le paramètre files est un tableau de pointeurs sur
 * des files de PDU. Il doit en contenir au moins nbQoS. Les nbQoS
 * premières seront utilisées ici.
 */
void schedACM_setInputQueues(struct schedACM_t * sched, int mc, struct filePDU_t * files[]);

/*
 * Attribution du type de QoS d'une file. La file est identifiée par
 * (mc, qos), le type de QoS voulue est passée en paramètre, ainsi
 * qu'un éventuel paramètre de pondération. rmin est le débit minimal
 * (un autre paramètre de certains types de QoS)
 */
void schedACM_setFileQoSType(struct schedACM_t * sched, int mc, int qos, int qosType, double beta, double rmin);

#define kseQoS_log 1
#define kseQoS_lin 2
#define kseQoS_exp 3
#define kseQoS_exn 4
#define kseQoS_PQ  5
#define kseQoS_BT  6
#define kseQoS_BB  7

/*
 * Calcul de la valeur en x de la derivee d'une fonction d'utilité
 * Le paramètre dvbs2ll est ici nécessaire pour certaines fonctions
 * Il faudra envisager de mettre ces info (le débit du lien en gros
 * pour le moment) dans la structure t_qosMgt, ou pas !)
 */
double utiliteDerivee(t_qosMgt * qos, double x, struct DVBS2ll_t * dvbs2ll);

/*
 * Fonction à invoquer lorsque le support est libre afin de solliciter
 * la construction d'une nouvelle trame
 */
struct PDU_t * schedACM_getPDU(struct schedACM_t * sched);
int schedACM_processPDU(struct schedACM_t * sched,
                         getPDU_t getPDU, void * source);


/*
 * Ajout d'une sonde pour compter les paquets d'une file (m, q) émis par un
 * MODCOD mc (mc peut être < m en cas de reclassement).
 * Attention, c'est goret ! Faut-il vraiment le mettre là dans la
 * mesure où c'est pas  ce module qui le gère ?
 */
void schedACM_setPqFromMQinMC(struct schedACM_t * sched, int m, int q, int mc, struct probe_t * pr);

/*
 * Attribution des files d'attente d'entrée pour un MODCOD donné dans
 * le paramètre mc. Le paramètre files est un tableau de pointeurs sur
 * des files de PDU. Il doit en contenir au moins nbQoS. Les nbQoS
 * premières seront utilisées ici.
 *
void schedACM_setInputQueues(struct schedACM_t * sched, int mc, struct filePDU_t * files[]);
*/

/*
 * Affectation d'une sonde permettant de suivre le débit estimé par
 * l'algorithme pour chaque file
 */
void schedACM_setThoughputProbe(struct schedACM_t * sched, int m, int q, struct probe_t * bwProbe);


/*
 * Attribution du type de QoS d'une file. La file est identififée par
 * (mc, qos), le type de QoS voulue est passée en paramètre, ainsi
 * qu'un éventuel paramètre de pondération.
 *
void schedACM_setFileQoSType(struct schedACM_t * sched, int mc, int qos, int qosType, double beta, double rmin);
*/

/*
 * Consultation du nombre de ModCod
 */
int nbModCod(struct schedACM_t * sched);


/*
 * Consultation du nombre de QoS par MODCOD
 */
int nbQoS(struct schedACM_t * sched);

/*
 * Obtention d'un pointeur sur une des files
 */
struct filePDU_t * schedACM_getInputQueue(struct schedACM_t * sched, int mc, int qos);

/*
 * Peut-on faire du "déclassement" ?
 */
inline int schedACM_getReclassification(struct schedACM_t * sched);

/*
 * Obtention d'un pointeur sur une des QoS
 */
inline t_qosMgt * schedACM_getQoS(struct schedACM_t * sched, int mc, int qos);

/*
 * Obtention d'un pointeur sur le lien
 */
inline struct DVBS2ll_t * schedACM_getACMLink(struct schedACM_t * sched);

/*
 * Obtention d'un pointeur vers une sonde
 */
struct probe_t *  schedACM_getPqFromMQinMC(struct schedACM_t * sched, int m, int q, int mc);

/*
 * Modification des données privées.
 */
void schedACM_setPrivate(struct schedACM_t * sched, void * private);

/*
 * Obtention des données privées
 */
void * schedACM_getPrivate(struct schedACM_t * sched);

/*
 * Y a-t-il des paquets en attente ? Le résultat est booléen
 */
int schedACM_getPacketsWaiting(struct schedACM_t * sched);

/*
 * Si les fonctions getPDU et processPDU sont redéfinies, la présence
 * de paquets en attente n'est plus mise à jour. Il faut donc
 * l'assurer par des appels à la fonction suivante.
 */
void schedACM_setPacketsWaiting(struct schedACM_t * sched, int b);

void schedACM_afficherFiles(struct schedACM_t * sched, int mc);

/*
 * Obtention d'un pointeur sur la solution choisie
 */
t_remplissage * schedACM_getSolution(struct schedACM_t * sched);



/*
 *   Fonction à invoquer par l'ordonnanceur pour décompter les solutions
 */
void schedACM_tryingNewSolution(struct schedACM_t * sched);

/*
 * Ajout d'une sonde permettant de mesurer le nombre de solutions testées
 */
void schedACM_addNbSolProbe(struct schedACM_t * sched, struct probe_t * probe);

/*
 * Combien de solutions testées ?
 */
int schedACM_getNbSolutions(struct schedACM_t * sched);

/**********************************************************************************/
/*   Gestion des remplissages                                                     */
/**********************************************************************************/

/*
 * Initialisation (création) d'une solution de remplissage
 */
void remplissage_init(t_remplissage * tr, int nbModCod, int nbQoS);

/*
 * Remise à zéro.
 */
void remplissage_raz(t_remplissage * tr, int nbModCod, int nbQoS);

void remplissage_free(t_remplissage * tr, int nbModCod);

void tabRemplissage_init(t_remplissage * tr, int nbR, int nbModCod, int nbQoS);
void tabRemplissage_raz(t_remplissage * tr, int nbR, int nbModCod, int nbQoS);
void remplissage_copy(t_remplissage * src, t_remplissage * dst, int nbModCod, int nbQoS);

#endif
