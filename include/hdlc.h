/**
 *
 */
#ifndef __DEF_HDLC
#define __DEF_HDLC

#include <pdu.h>

struct hdlc_t ;

struct hdlc_t * hdlc_create();

void hdlc_setOutLink(struct hdlc_t * h,
		     void * destination,
		     processPDU_t destProcessPDU);
/**
 * @brief Traitement d'une PDU envoyée par l'entité homologue
 */
void hdlc_processPDU(struct hdlc_t * h,
                     getPDU_t getPDU,
                     void * source);

/**
 * @brief Traitement d'une SDU soumise par la couche supérieure
 */
void hdlc_processSDU(struct hdlc_t * h,
                     getPDU_t getPDU,
                     void * source);

/**
 * @brief Initialization of an entity waiting for an incomming
 * connection 
 */
void hdlc_init(struct hdlc_t * h);

/**
 * @brief Initialization of a connection
 */
int hdlc_connect(struct hdlc_t * h, int mode);

/**
 * @brief Emission d'un message
 */
int hdlc_send(struct hdlc_t * h, struct PDU_t * data);

/**
 * @brief Fonction d'entrée/sortie
 */
struct PDU_t * hdlc_getPDU(struct hdlc_t * h);


#endif
