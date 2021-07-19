/**
 * @file tun-dev.h
 * @brief Tentative de communication avec le monde réel !
 */
#ifndef __DEF_TUN_DEV
#define __DEF_TUN_DEV

#include <motsim.h>
#include <date-generator.h>
#include <pdu.h>

#include <ndesObject.h>

struct TUNDevice_t; //!< Le type d'un tel device

/**
 * @brief Declare the object relative functions
 */
declareObjectFunctions(TUNDevice);

/**
 * @brief Création d'un TUNDevice
 * @param period période de scrutation des paquets
 *
 * Il peut y avoir une destination à laquelle sont transmis les
 * paquets. Ce sera probablement une entité IPv4.
 */
struct TUNDevice_t * TUNDevice_create(double period,
				      void * destination,
				      processPDU_t destProcessPDU);

/**
 * @brief Démarrage d'une interface
 *
 * Le mode de fonctionnement actuel : on va régulièrement voir s'il y
 * a des paquets en attente. C'est pas top, il faudrait de
 * l'événementiel, soit guidé par le monde réel, soit par la simu.
 */
void TUNDevice_start(struct TUNDevice_t * tunDev);

/**
 * @brief La fonction de consommation d'une PDU
 */
int TUNDevice_processPDU(void * tunDev, getPDU_t getPDU, void * source);

/**
 * @brief Change the date generator
 * @param src The tun device to modify
 * @param gen The new date generator
 *
 * The previous date generator should be freed by the caller
 */
void TUNDevice_setDateGenerator(struct TUNDevice_t * src,
                                struct dateGenerator_t * dateGen);

#endif
