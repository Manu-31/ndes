/**
 * @file file_pdu.h
 * @brief DÃ©finition de la gestion des files de PDUs
 *
 * Par dÃ©faut, une liste Ã  une capacitÃ© non limitÃ©e et fonctionne
 * selon une politique FIFO.
 */
#ifndef __DEF_LISTE_PDU
#define __DEF_LISTE_PDU

#include <ndesObject.h>
#include <pdu.h>
#include <probe.h>

struct filePDU_t;

/**
 * Type de la stratÃ©gie de perte en cas d'insersion dans une file
 * pleine. Attention, insÃ©rer une PDU de taille t dans une file de
 * capacitÃ© max < t n'est pas une erreur, mais engendre simplement un
 * Ã©vÃ©nement d'overflow.
 */
enum filePDU_dropStrategy {
  filePDU_dropHead,
  filePDU_dropTail // StratÃ©gie par dÃ©faut
};

/** @brief Création d'une file.
 * 
 *  @param destination l'entité aval (ou NULL ai aucune)
 *  @param destProcessPDU la fonction de traitement de l'entité aval
 *  (ou NULL si aucune entité)
 *  @return Une strut filePDU_t * allouée et initialisée
 *
 *  Il est possible de ne pas fournir d'entité aval en paramètre, car
 *  une file peut être utilisée également comme un simple outil de
 *  gestion mémoire, sans entrer dans un modèle de réseau. On
 *  utilisera alors simplement les fonctions d'insertion et d'extraction
 */
struct filePDU_t * filePDU_create(void * destination,
			    processPDU_t destProcessPDU);

/*
 * DÃ©finition d'une capacitÃ© maximale en octets. Une valeur nulle
 * signifie pas de limite.
 */
void filePDU_setMaxSize(struct filePDU_t * file, unsigned long maxSize);
unsigned long filePDU_getMaxSize(struct filePDU_t * file);

void filePDU_setMaxLength(struct filePDU_t * file, unsigned long maxLength);
unsigned long filePDU_getMaxLength(struct filePDU_t * file);


/*
 * Choix de la stratÃ©gie de perte en cas d'insersion dans une file
 * pleine. Attention, insÃ©rer une PDU de taille t dans une file de
 * capacitÃ© max < t n'est pas une erreur, mais engendre simplement un
 * Ã©vÃ©nement d'overflow.
 */
void filePDU_setDropStrategy(struct filePDU_t * file, enum filePDU_dropStrategy dropStrategy);

/**
 * @brief Insertion d'une PDU dans la file
 * @param file la file dans laquelle on insère la PDU
 * @param PDU la PDU à insérer à la fin de la file
 *
 * Si une destination a été affectée à la file, alors la fonction de
 * traitement de cette destination est invoquée.
 */
void filePDU_insert(struct filePDU_t * file,
		    struct PDU_t * PDU);

/*
 * Une fonction permettant la conformitÃ© au modÃ¨le d'Ã©change
 */
int filePDU_processPDU(void * file,
		       getPDU_t getPDU,
		       void * source);

/**
 * @brief Extraction d'une PDU depuis la file
 * @param file la file depuis laquelle on souhaite extraire la
 * première PDU
 * @return la première PDU ou NULL si la file est vide
 */
struct PDU_t * filePDU_extract(struct filePDU_t * file);

/**
 * @brief Extraction d'une PDU depuis la file
 * @param file la file depuis laquelle on souhaite extraire la
 * première PDU
 * @return la première PDU ou NULL si la file est vide
 * 
 * Ici la signature est directement compatible avec le modÃ¨le
 * d'entrèe-sortie de NDES.
 */
struct PDU_t * filePDU_getPDU(void * file);

/**
 * @brief Nombre de PDU dans la file
 */
int filePDU_length(struct filePDU_t * file);

/**
 * @brief Taille cumulée des PDU d'une file
 */
int filePDU_size(struct filePDU_t * file);

/**
 * @brief Taille cumulée des n premières PDUs
 * @param file la file 
 * @param n le nombre (positif ou nul) de PDUs
 * @return le cumul des tailles des n premières PDUs de la file
 */
int filePDU_size_n_PDU(struct filePDU_t * file, int n);

/*
 * Taille du enieme paquet de la file (n>=1)
 */
int filePDU_size_PDU_n(struct filePDU_t * file, int n);
int filePDU_id_PDU_n(struct filePDU_t * file, int n);

/****************************************************************************
    File probes
 ***************************************************************************/

/*
 * Affectation d'une sonde sur la taille des PDUs insÃ©rÃ©es.
 */
void filePDU_addInsertSizeProbe(struct filePDU_t * file, struct probe_t * insertProbe);

/*
 * AjoÃ»t d'une sonde sur la taille des PDU sortantes
 */
void filePDU_addExtractSizeProbe(struct filePDU_t * file, struct probe_t * extractProbe);

/*
 * AjoÃ»t d'une sonde sur la taille des PDU jetÃ©es
 */
void filePDU_addDropSizeProbe(struct filePDU_t * file, struct probe_t * dropProbe);

/*
 * Affectation d'une sonde sur le temps de sÃ©jour
 */
void filePDU_addSejournProbe(struct filePDU_t * file, struct probe_t * sejournProbe);

/**
 * @brief Add a probe on queue length (nb of PDU) on insert events
 */
void filePDU_addFileLengthProbe(struct filePDU_t * file, struct probe_t * length);

/*
 * Mesure du dÃ©bit d'entrÃ©e sur les n-1 derniÃ¨res PDUs, oÃ¹ n est le
 * nombre de PDUs prÃ©sentes. Le dÃ©bit est alors obtenu en divisant la
 * somme des tailles des n-1 derniÃ¨res PDUs par la durÃ©e entre les
 * dates d'arrivÃ©e de la premiÃ¨re et la derniÃ¨re.
 * S'il n'y a pas assez de PDUs, le rÃ©sultat est nul
 *
 * WARNING, a virer, non ?
 */
double filePDU_getInputThroughput(struct filePDU_t * file);
/*
 *
 */
void filePDU_setThroughputInMode();



/*
 * Un affichage un peu moche de la file. Peut Ãªtre utile dans des
 * phases de dÃ©bogage.
 */
void filePDU_dump(struct filePDU_t * file);

/*
 * RÃ©initialisation dans un Ã©tat permettant de lancer une nouvelle
 * simulation. Ici il suffit de vider la file de tous ses Ã©lÃ©ments.
 */
void filePDU_reset(struct filePDU_t * file);

/**
 * @brief filePDU can also be seen as ndesObjects
 */
declareObjectFunctions(filePDU);

#endif
