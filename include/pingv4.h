/**
 *   @file pingv4.h
 *   @brief 
 */
#ifndef __DEF_PINGv4
#define __DEF_PINGv4

#include <icmpv4.h>

#include <ndesObject.h>

#define PING_PERIOD 1.0
#define ICMP_MSG_SIZE 26

struct pingv4_t;

/**
 * @brief Declare the object relative functions
 */
declareObjectFunctions(pingv4);

/**
 * @brief création d'un processus pingv4
 *
 */
struct pingv4_t * pingv4_create(struct ICMPv4_t * icmpv4,
				struct dateGenerator_t * dateGen,
                                uint32_t adresseDestination);

/**
 * @brief Change the date generator
 * @param src The PDUSource to modify
 * @param gen The new date generator
 * The previos date generator should be freed by the caller
 */
void pingv4_setDateGenerator(struct pingv4_t * src,
                             struct dateGenerator_t * dateGen);

/**
 * @brief 
 */
void pingv4_setICMPv4();

/**
 * @brief Démarrage d'une transmission de paquet ICMP ER
 *
 */
void pingv4_startPing(struct pingv4_t * pv4);

#endif
