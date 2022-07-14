/**
 *   @file icmpv4.h
 *   @brief 
 */
#ifndef __DEF_ICMPv4
#define __DEF_ICMPv4

#include <ndesObject.h>

struct ICMPv4_t;


#define ICMP_TYPE_ECHO_REPLY   0
#define ICMP_TYPE_ECHO_REQUEST 8

#include <ipv4.h>

/**
 * @brief Declare the object relative functions
 */
declareObjectFunctions(ICMPv4);

/**
 * @brief création d'une entité icmpv4
 * @param ipv4 l'entité à laquelle on se raccroche
 */
struct ICMPv4_t * icmpv4_create(struct IPv4_t * ipv4);

void ICMPv4_sendMessage(struct ICMPv4_t * icmpv4,
		        int type, int code,
			uint32_t dstAddr,
			struct PDU_t * pdu);

/**
 * @brief Traitement d'une PDU passée en paramètre
 * @param srcAddr l'adresse de l'émetteur
 */
int ICMPv4_processThisPDU(struct ICMPv4_t * icmpv4,
                          uint32_t          srcAddr,
			  struct PDU_t    * pdu);

#endif
