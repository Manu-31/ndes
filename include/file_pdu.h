/**
 * @file file_pdu.h
 * @brief Définition de la gestion des files de PDUs
 *
 * Par défaut, une liste à une capacité non limitée et fonctionne
 * selon une politique FIFO.
 */
#ifndef __DEF_LISTE_PDU
#define __DEF_LISTE_PDU

#include <pdu.h>
#include <probe.h>

struct filePDU_t;

/**
 * Type de la stratégie de perte en cas d'insersion dans une file
 * pleine. Attention, insérer une PDU de taille t dans une file de
 * capacité max < t n'est pas une erreur, mais engendre simplement un
 * événement d'overflow.
 */
enum filePDU_dropStrategy {
  filePDU_dropHead,
  filePDU_dropTail // Stratégie par défaut
};

/** @brief Cr�ation d'une file.
 * 
 *  @param destination l'entit� aval (ou NULL ai aucune)
 *  @param destProcessPDU la fonction de traitement de l'entit� aval
 *  (ou NULL si aucune entit�)
 *  @return Une strut filePDU_t * allou�e et initialis�e
 *
 *  Il est possible de ne pas fournir d'entit� aval en param�tre, car
 *  une file peut �tre utilis�e �galement comme un simple outil de
 *  gestion m�moire, sans entrer dans un mod�le de r�seau. On
 *  utilisera alors simplement les fonctions d'insertion et d'extraction
 */
struct filePDU_t * filePDU_create(void * destination,
			    processPDU_t destProcessPDU);

/*
 * Définition d'une capacité maximale en octets. Une valeur nulle
 * signifie pas de limite.
 */
void filePDU_setMaxSize(struct filePDU_t * file, unsigned long maxSize);
unsigned long filePDU_getMaxSize(struct filePDU_t * file);

void filePDU_setMaxLength(struct filePDU_t * file, unsigned long maxLength);
unsigned long filePDU_getMaxLength(struct filePDU_t * file);


/*
 * Choix de la stratégie de perte en cas d'insersion dans une file
 * pleine. Attention, insérer une PDU de taille t dans une file de
 * capacité max < t n'est pas une erreur, mais engendre simplement un
 * événement d'overflow.
 */
void filePDU_setDropStrategy(struct filePDU_t * file, enum filePDU_dropStrategy dropStrategy);

/**
 * @brief Insertion d'une PDU dans la file
 * @param file la file dans laquelle on ins�re la PDU
 * @param PDU la PDU � ins�rer � la fin de la file
 *
 * Si une destination a �t� affect�e � la file, alors la fonction de
 * traitement de cette destination est invoqu�e.
 */
void filePDU_insert(struct filePDU_t * file,
		    struct PDU_t * PDU);

/*
 * Une fonction permettant la conformité au modèle d'échange
 */
int filePDU_processPDU(void * file,
		       getPDU_t getPDU,
		       void * source);

/**
 * @brief Extraction d'une PDU depuis la file
 * @param file la file depuis laquelle on souhaite extraire la
 * premi�re PDU
 * @return la premi�re PDU ou NULL si la file est vide
 */
struct PDU_t * filePDU_extract(struct filePDU_t * file);

/**
 * @brief Extraction d'une PDU depuis la file
 * @param file la file depuis laquelle on souhaite extraire la
 * premi�re PDU
 * @return la premi�re PDU ou NULL si la file est vide
 * 
 * Ici la signature est directement compatible avec le modèle
 * d'entr�e-sortie de NDES.
 */
struct PDU_t * filePDU_getPDU(void * file);

/*
 * Nombre de PDU dans la file
 */
int filePDU_length(struct filePDU_t * file);

int filePDU_size(struct filePDU_t * file);

/**
 * @brief Taille cumul�e des n premi�res PDUs
 * @param file la file 
 * @param n le nombre (positif ou nul) de PDUs
 * @return le cumul des tailles des n premi�res PDUs de la file
 */
int filePDU_size_n_PDU(struct filePDU_t * file, int n);

/*
 * Taille du enieme paquet de la file (n>=1)
 */
int filePDU_size_PDU_n(struct filePDU_t * file, int n);
int filePDU_id_PDU_n(struct filePDU_t * file, int n);

/*
 * Affectation d'une sonde sur la taille des PDUs insérées.
 */
void filePDU_addInsertSizeProbe(struct filePDU_t * file, struct probe_t * insertProbe);

/*
 * Ajoût d'une sonde sur la taille des PDU sortantes
 */
void filePDU_addExtractSizeProbe(struct filePDU_t * file, struct probe_t * extractProbe);

/*
 * Ajoût d'une sonde sur la taille des PDU jetées
 */
void filePDU_addDropSizeProbe(struct filePDU_t * file, struct probe_t * dropProbe);

/*
 * Affectation d'une sonde sur le temps de séjour
 */
void filePDU_addSejournProbe(struct filePDU_t * file, struct probe_t * sejournProbe);

/*
 * Mesure du débit d'entrée sur les n-1 dernières PDUs, où n est le
 * nombre de PDUs présentes. Le débit est alors obtenu en divisant la
 * somme des tailles des n-1 dernières PDUs par la durée entre les
 * dates d'arrivée de la première et la dernière.
 * S'il n'y a pas assez de PDUs, le résultat est nul
 *
 * WARNING, a virer, non ?
 */
double filePDU_getInputThroughput(struct filePDU_t * file);
/*
 *
 */
void filePDU_setThroughputInMode();



/*
 * Un affichage un peu moche de la file. Peut être utile dans des
 * phases de débogage.
 */
void filePDU_dump(struct filePDU_t * file);

/*
 * Réinitialisation dans un état permettant de lancer une nouvelle
 * simulation. Ici il suffit de vider la file de tous ses éléments.
 */
void filePDU_reset(struct filePDU_t * file);

#endif
