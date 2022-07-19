/**
 *   @file ipv4.h
 *   @brief trying to implement a basic IP entity
 * 
 *   Il s'agit d'une première version très simple, avec une seule
 *   "interface" d'entrée et de sortie. Du coup, pas de masque, de
 *   table de routage, ...
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

#include <icmpv4.h>

#define IP_HEADER_SIZE 20

#define IP_PROTO_ICMP 0x01
#define IP_PROTO_IGMP 0x02
#define IP_PROTO_TCP  0x06
#define IP_PROTO_UDP  0x11

/**
 * @brief Declare the object relative functions
 */
declareObjectFunctions(IPv4);

/**
 * @brief création d'une entité IPv4
 *
 */
struct IPv4_t * IPv4_create(uint32_t adresse,
			    void * destination,
		            processPDU_t destProcessPDU);

/**
 * @brief The function used by the destination to actually get the next PDU
 */
struct PDU_t * IPv4_getNextPDU(void * src);

/**
 * @brief Définition de l'entité ICMP
 */
void IPv4_setICMP(struct IPv4_t * ipv4, struct ICMPv4_t * icmpv4);

int IPv4_processPDU(void * s, getPDU_t getPDU, void * source);

/**
 * @brief Construction et émission d'un paquet
 */
void IPv4_sendPacket(struct IPv4_t * ipv4,
		     uint32_t destAddr,
		     uint8_t proto,
		     struct PDU_t * payload);

/**
 * @brief 
 */
void IPv4_setDestination(struct IPv4_t * ipv4,
			 void * destination,
			 processPDU_t destProcessPDU);

/**
 * @brief Traitement d'une PDU
 */
unsigned short IPv4_buildChecksum(unsigned short * addr, short count);

#endif
