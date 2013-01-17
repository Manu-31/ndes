/*
 * Forme g�n�rale d'un ordonnanceur pour un lien ACM
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
 * Les �l�ments de gestion de la QoS d'une file
 */
typedef struct {
   int typeQoS;              // Quelle QoS ?
   double beta;              // Un param�tre li� au type de QoS
   double rmin;              // Un param�tre li� au type de QoS (d�bit "min")
   double debit;             // Une mesure du d�bit
   struct probe_t * bwProbe; // Une sonde sur le d�bit �valu� par et pour l'algo
} t_qosMgt;

/*
 * Un mode de remplissage d'une BBFRAME. La fonction d'ordonnancement
 * doit renseigner les premiers champs.
 */
typedef struct {
   int       modcod;         // Le numero de MODCOD choisi
   int    ** nbrePaquets;    // Nombre de paquets de chaque file � transmettre
   int       volumeTotal;    // Le nombre total d'octets � transmettre

   /* Les champs suivants sont � la dispo de l'ordonnanceur. Il
      faudrait surement faire plus propre, avec un  pointeur sur prive
      ou une union, ... */
   double    interet;
   int       nbChoix;        // Nombre de choix menant � cet interet
   int       casTraite;      // Pour �viter de retraiter un cas
} t_remplissage ;

/*
 * D�finition des fonctions que doit implanter un ordonnanceur sur
 * ACM.
 *
 * Les fonctions getPDU/processPDU peuvent �tre null si rien de
 * sp�cifique n'est n�cessaire. Des fonctions g�n�riques font le
 * travail.
 *
 * La fonction buildBBFRAME est invoqu�e par les fonctions
 * getPDU/processPDU pour construire une BBFRAME lorsque le support
 * est disponible. Elle peut �tre null, m�me si les deux fonctions
 * pr�c�dentes ne le sont pas. Une fonction g�n�rique se chargera
 * alors d'invoquer l'ordonnanceur.
 *
 * la fonction schedule est invoqu�e par la fonction buildBBFRAME
 * g�n�rique. Si cette derni�re est utilis�e, la fonction
 * d'ordonnancement ne peut donc pas �tre null. Elle doit construire
 * le champ "solutionChoisie" de la structure schedACM. C'est ce
 * champ, de type t_remplissage, 
 * qui est utilis� par la fonction buildBBFRAME g�n�rique pour
 * construire la BBFRAME en fonction du choix d'ordonnancement.
 */
struct schedACM_func_t {
   struct PDU_t * (*getPDU)(void * private);
   void  (*processPDU)(void * private,
	               getPDU_t getPDU, void * source);

   struct PDU_t * (* buildBBFRAME)(void * private);

   void (*schedule)(void * private);
};

struct schedACM_t;

/*
 * Cr�ation d'un scheduler avec sa "destination". Cette derni�re doit
 * �tre de type struct DVBS2ll_t  et avoir d�j� �t� compl�tement
 * construite (tous les MODCODS cr��s).
 * Le nombre de files de QoS diff�rentes par MODCOD est �galement
 * pass� en param�tre.
 */
struct schedACM_t * schedACM_create(struct DVBS2ll_t * dvbs2ll, int nbQoS, int declOK,
				    struct schedACM_func_t * func);

/*
 * Attribution des files d'attente d'entr�e pour un MODCOD donn� dans
 * le param�tre mc. Le param�tre files est un tableau de pointeurs sur
 * des files de PDU. Il doit en contenir au moins nbQoS. Les nbQoS
 * premi�res seront utilis�es ici.
 */
void schedACM_setInputQueues(struct schedACM_t * sched, int mc, struct filePDU_t * files[]);

/*
 * Attribution du type de QoS d'une file. La file est identifi�e par
 * (mc, qos), le type de QoS voulue est pass�e en param�tre, ainsi
 * qu'un �ventuel param�tre de pond�ration. rmin est le d�bit minimal
 * (un autre param�tre de certains types de QoS)
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
 * Calcul de la valeur en x de la derivee d'une fonction d'utilit�
 * Le param�tre dvbs2ll est ici n�cessaire pour certaines fonctions
 * Il faudra envisager de mettre ces info (le d�bit du lien en gros
 * pour le moment) dans la structure t_qosMgt, ou pas !)
 */
double utiliteDerivee(t_qosMgt * qos, double x, struct DVBS2ll_t * dvbs2ll);

/*
 * Fonction � invoquer lorsque le support est libre afin de solliciter
 * la construction d'une nouvelle trame
 */
struct PDU_t * schedACM_getPDU(struct schedACM_t * sched);
void schedACM_processPDU(struct schedACM_t * sched,
                         getPDU_t getPDU, void * source);


/*
 * Ajout d'une sonde pour compter les paquets d'une file (m, q) �mis par un
 * MODCOD mc (mc peut �tre < m en cas de reclassement).
 * Attention, c'est goret ! Faut-il vraiment le mettre l� dans la
 * mesure o� c'est pas  ce module qui le g�re ?
 */
void schedACM_setPqFromMQinMC(struct schedACM_t * sched, int m, int q, int mc, struct probe_t * pr);

/*
 * Attribution des files d'attente d'entr�e pour un MODCOD donn� dans
 * le param�tre mc. Le param�tre files est un tableau de pointeurs sur
 * des files de PDU. Il doit en contenir au moins nbQoS. Les nbQoS
 * premi�res seront utilis�es ici.
 *
void schedACM_setInputQueues(struct schedACM_t * sched, int mc, struct filePDU_t * files[]);
*/

/*
 * Affectation d'une sonde permettant de suivre le d�bit estim� par
 * l'algorithme pour chaque file
 */
void schedACM_setThoughputProbe(struct schedACM_t * sched, int m, int q, struct probe_t * bwProbe);


/*
 * Attribution du type de QoS d'une file. La file est identifif�e par
 * (mc, qos), le type de QoS voulue est pass�e en param�tre, ainsi
 * qu'un �ventuel param�tre de pond�ration.
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
 * Peut-on faire du "d�classement" ?
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
 * Modification des donn�es priv�es.
 */
void schedACM_setPrivate(struct schedACM_t * sched, void * private);

/*
 * Obtention des donn�es priv�es
 */
void * schedACM_getPrivate(struct schedACM_t * sched);

/*
 * Y a-t-il des paquets en attente ? Le r�sultat est bool�en
 */
int schedACM_getPacketsWaiting(struct schedACM_t * sched);

/*
 * Si les fonctions getPDU et processPDU sont red�finies, la pr�sence
 * de paquets en attente n'est plus mise � jour. Il faut donc
 * l'assurer par des appels � la fonction suivante.
 */
void schedACM_setPacketsWaiting(struct schedACM_t * sched, int b);

void schedACM_afficherFiles(struct schedACM_t * sched, int mc);

/*
 * Obtention d'un pointeur sur la solution choisie
 */
t_remplissage * schedACM_getSolution(struct schedACM_t * sched);



/*
 *   Fonction � invoquer par l'ordonnanceur pour d�compter les solutions
 */
void schedACM_tryingNewSolution(struct schedACM_t * sched);

/*
 * Ajout d'une sonde permettant de mesurer le nombre de solutions test�es
 */
void schedACM_addNbSolProbe(struct schedACM_t * sched, struct probe_t * probe);

/*
 * Combien de solutions test�es ?
 */
int schedACM_getNbSolutions(struct schedACM_t * sched);

/**********************************************************************************/
/*   Gestion des remplissages                                                     */
/**********************************************************************************/

/*
 * Initialisation (cr�ation) d'une solution de remplissage
 */
void remplissage_init(t_remplissage * tr, int nbModCod, int nbQoS);

/*
 * Remise � z�ro.
 */
void remplissage_raz(t_remplissage * tr, int nbModCod, int nbQoS);

void remplissage_free(t_remplissage * tr, int nbModCod);

void tabRemplissage_init(t_remplissage * tr, int nbR, int nbModCod, int nbQoS);
void tabRemplissage_raz(t_remplissage * tr, int nbR, int nbModCod, int nbQoS);
void remplissage_copy(t_remplissage * src, t_remplissage * dst, int nbModCod, int nbQoS);

#endif
