/**
 *   @file ipv4.h
 *   @brief trying to implement a basic IP entity
 */
#ifndef __DEF_IPv4
#define __DEF_IPv4

#include <stdint.h>

#include <ndesObject.h>

#include <random-generator.h>
#include <date-generator.h>
#include <pdu.h>
#include <motsim.h>

struct IPv4_t; //!< Le type d'une entité IPv4

/**
 * @brief Declare the object relative functions
 */
declareObjectFunctions(IPv4);

/**
 * @brief Démarrage d'une transmission de paquet ICMP ER
 *
 */
void IPv4_startPing(struct IPv4_t * ipv4);

void IPv4_setDestination(struct IPv4_t * ipv4,
			 void * destination,
			 processPDU_t destProcessPDU);
  
/**
 * @brief création d'une entité IPv4
 *
 * Pour le moment, ce sera une source de paquets IPV4 ICMP echo
 * request puisque mon but est de l'injecté dans le système
 */
struct IPv4_t * IPv4_create(struct dateGenerator_t * dateGen,
			    uint32_t adresse,
	                    void * destination,
		            processPDU_t destProcessPDU);

/**
 * @brief Démarrage d'une transmission de paquet ICMP ER
 *
 */
void IPv4_startPing(struct IPv4_t * ipv4);

/**
 * @brief The function used by the destination to actually get the next PDU
 */
struct PDU_t * IPv4_getNextPDU(void * src);


int IPv4_processPDU(void * s, getPDU_t getPDU, void * source);

#endif
