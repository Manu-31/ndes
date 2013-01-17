/*
 * Gestion des files de PDUs. Capacite non limitee par d�faut.
 */
#ifndef __DEF_LISTE_PDU
#define __DEF_LISTE_PDU

#include <pdu.h>
#include <probe.h>

struct filePDU_t;

/*
 * Type de la strat�gie de perte en cas d'insersion dans une file
 * pleine. Attention, ins�rer une PDU de taille t dans une file de
 * capacit� max < t n'est pas une erreur, mais engendre simplement un
 * �v�nement d'overflow.
 */
enum filePDU_dropStrategy {
  filePDU_dropHead,
  filePDU_dropTail // Strat�gie par d�faut
};

/*
 * Une file doit �tre associ�e � une destination, � laquelle elle
 * transmet chaque paquet re�u par la fonction send().
 */
struct filePDU_t * filePDU_create(void * destination,
			    processPDU_t destProcessPDU);

/*
 * D�finition d'une capacit� maximale en octets. Une valeur nulle
 * signifie pas de limite.
 */
void filePDU_setMaxSize(struct filePDU_t * file, unsigned long maxSize);
unsigned long filePDU_getMaxSize(struct filePDU_t * file);

void filePDU_setMaxLength(struct filePDU_t * file, unsigned long maxLength);


/*
 * Choix de la strat�gie de perte en cas d'insersion dans une file
 * pleine. Attention, ins�rer une PDU de taille t dans une file de
 * capacit� max < t n'est pas une erreur, mais engendre simplement un
 * �v�nement d'overflow.
 */
void filePDU_setDropStrategy(struct filePDU_t * file, enum filePDU_dropStrategy dropStrategy);

/*
 * Insertion d'une PDU dans la file
 */
void filePDU_insert(struct filePDU_t * file, struct PDU_t * PDU);

/*
 * Une fonction permettant la conformit� au mod�le d'�change
 */
void filePDU_processPDU(struct filePDU_t * file,
                        getPDU_t getPDU,
                        void * source);

/*
 * Extraction d'une PDU depuis la file. Ici la signature est
 * directement compatible avec le mod�le.
 */
struct PDU_t * filePDU_extract(struct filePDU_t * file);

/*
 * Nombre de PDU dans la file
 */
int filePDU_length(struct filePDU_t * file);

int filePDU_size(struct filePDU_t * file);

/*
 * Taille cumul�e des n premi�res PDUs
 */

int filePDU_size_n_PDU(struct filePDU_t * file, int n);

/*
 * Taille du enieme paquet de la file (n>=1)
 */
int filePDU_size_PDU_n(struct filePDU_t * file, int n);
int filePDU_id_PDU_n(struct filePDU_t * file, int n);

/*
 * Affectation d'une sonde sur la taille des PDUs ins�r�es.
 */
void filePDU_addInsertSizeProbe(struct filePDU_t * file, struct probe_t * insertProbe);

/*
 * Ajo�t d'une sonde sur la taille des PDU sortantes
 */
void filePDU_addExtractSizeProbe(struct filePDU_t * file, struct probe_t * extractProbe);

/*
 * Ajo�t d'une sonde sur la taille des PDU jet�es
 */
void filePDU_addDropSizeProbe(struct filePDU_t * file, struct probe_t * dropProbe);

/*
 * Affectation d'une sonde sur le temps de s�jour
 */
void filePDU_addSejournProbe(struct filePDU_t * file, struct probe_t * sejournProbe);

/*
 * Mesure du d�bit d'entr�e sur les n-1 derni�res PDUs, o� n est le
 * nombre de PDUs pr�sentes. Le d�bit est alors obtenu en divisant la
 * somme des tailles des n-1 derni�res PDUs par la dur�e entre les
 * dates d'arriv�e de la premi�re et la derni�re.
 * S'il n'y a pas assez de PDUs, le r�sultat est nul
 *
 * WARNING, a virer, non ?
 */
double filePDU_getInputThroughput(struct filePDU_t * file);
/*
 *
 */
void filePDU_setThroughputInMode();



/*
 * Un affichage un peu moche de la file. Peut �tre utile dans des
 * phases de d�bogage.
 */
void filePDU_dump(struct filePDU_t * file);

/*
 * R�initialisation dans un �tat permettant de lancer une nouvelle
 * simulation. Ici il suffit de vider la file de tous ses �l�ments.
 */
void filePDU_reset(struct filePDU_t * file);

#endif
