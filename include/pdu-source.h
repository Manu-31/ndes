/**
 * @file pdu-source.h
 * @brief Une source de PDU permet de produire des PDUs 
 */

#include <random-generator.h>
#include <date-generator.h>
#include <pdu.h>
#include <motsim.h>

#include <ndesObject.h>

struct PDUSource_t; //!< Le type d'une source

/**
 * @brief Declare the object relative functions
 */
declareObjectFunctions(PDUSource);

/**
 * @brief Définition de couples {date, taille}
 *
 * Pour définir explicitement une séquence de PDUs
 */
struct dateSize {
   motSimDate_t date;
   unsigned int size;
};

/**
 * @brief création d'une source de PDUs
 *
 * A chaque source est attribuée une destination et une 
 * fonction permettant de soumettre à cette destination
 * les PDU produites
 * 
 */
struct PDUSource_t * PDUSource_create(struct dateGenerator_t * dateGen,
				      void * destination,
				      processPDU_t destProcessPDU);

/** @brief Création d'un générateur déterministe
 * 
 *  @param sequence Un tableau de {date, size} définissant chaque PDU
 *  @param destination L'entité aval
 *  @param destProcessPDU La fonction de traitement de la destination
 *  @result Un pointeur sur la source créée/initialisée
 *
 *  Un tel générateur permet de définir explicitement la séquence des
 *  PDUs à générer. Cette séquence est définie par un tableau de
 *  couples {date, taille}. Le dernier élément du tableau doit être
 *  doté d'une date antérieure à la précédente (typiquement {-1.0,
 *  0}). Le tableau n'est pas copié, il ne doit donc pas être
 *  libéré tant que la source peut servir.
 */
struct PDUSource_t * PDUSource_createDeterministic(struct dateSize * sequence,
						   void * destination,
						   processPDU_t destProcessPDU);

/**
 * @brief Création d'une source périodique
 * @param period période de génération des PDU
 * @param destination l'entité aval
 * @param destProcessPDU fonction de traitement des PDU par l'aval
 * @result une source périodique initialisée
 *
 * Création d'une source de PDU périodique. La période est fournie en
 * paramètre. La taille des PDU est nulle. La première PDU est
 * générée à la date d'invocation de PDUSource_start.
 */
struct PDUSource_t * PDUSource_createPeriodic(double period,
					      void * destination,
					      processPDU_t destProcessPDU);

/**
 * @brief Création d'une source CBR
 * @param period période de génération des PDU
 * @param size taille des PDU
 * @param destination l'entité aval
 * @param destProcessPDU fonction de traitement des PDU par l'aval
 * @result une source périodique initialisée
 *
 * Création d'une source de PDU CBR. La période est fournie en
 * paramètre ainsi que la taille des PDU. La première PDU est
 * générée à la date d'invocation de PDUSource_start.
 */
struct PDUSource_t * PDUSource_createCBR(double period,
                                         unsigned int size,
					 void * destination,
					 processPDU_t destProcessPDU);

/**
 * @brief Change the date generator
 * @param src The PDUSource to modify
 * @param gen The new date generator
 * The previous date generator should be freed by the caller
 */
void PDUSource_setDateGenerator(struct PDUSource_t * src,
                                struct dateGenerator_t * dateGen);

/**
 * @brief Get access to the date generator
 * @param src The PDUSource to query
 * @result The date generator
 */
struct dateGenerator_t * PDUSource_getDateGenerator(struct PDUSource_t * src);

/**
 * @brief Spécification du générateur de taille de PDU associé
 *
 *  En l'absence d'un tel générateur, les PDUs générées sont de taille
 *  nulle.
 */
void PDUSource_setPDUSizeGenerator(struct PDUSource_t * src,
				   struct randomGenerator_t * rg);

/*
 * Positionnement d'une sonde sur la taille des PDUs produites. Toutes
 * les PDUs créées sont concernées, même si elles ne sont pas
 * récupérées par la destination.
 */
void PDUSource_addPDUGenerationSizeProbe(struct PDUSource_t * src,
					 struct probe_t *  PDUGenerationSizeProbe);

/**
 * @brief Démarrage d'une source dans le cadre d'un simulateur
 *
 * A partir de cet instant, elle peut produire des PDUs.
 */
void PDUSource_start(struct PDUSource_t * source);

/*
 * The function used by the destination to actually get the next PDU
 */
struct PDU_t * PDUSource_getPDU(void * src);
