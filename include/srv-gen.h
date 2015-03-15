/**
 * @file srv-gen.h
 * @brief Un serveur générique
 *
 * Permet de modéliser des serveurs simples. Utile pour modéliser par
 * exemple un lien.
 */
#ifndef __GENERIC_SERVER__
#define __GENERIC_SERVER__

#include <pdu.h>
#include <motsim.h>
#include <ndesObject.h>

struct srvGen_t;

/**
 * @brief Création d'un serveur générique
 */
struct srvGen_t * srvGen_create(void * destination,
                                processPDU_t destProcessPDU);

/*
 * La fonction utilisée par le destinataire pour prendre une PDU servie
 */
struct PDU_t * srvGen_getPDU(void * srv);

/*
 * La fonction de consommation d'une PDU
 */
int srvGen_processPDU(void * srv,
		      getPDU_t getPDU, void * source);

/*
 * Obtention de la dernière PDU servie (éventuellement NULL si trop tard !)
struct PDU_t * srvGen_getPDU(struct srvGen_t * srv);
 */

/*
 * Fonction de calcul du temps de service
 */
enum serviceTime_t {
   serviceTimeCst,
   serviceTimeExp,
   serviceTimeProp
} ;

/**
 * @brief Choix du temps de service
 * @param srv le serveur à modifier
 * @param st le type de temps service
 * @param parameter signification dépendant du type de temps de
 * service
 *
 * Si st == serviceTimeCst, alors parameter est la période
 * Si st == serviceTimeExp, alors parameter est le "mu" de la loi
 * exponentielle
 * Si st == serviceTimeProp, alors parameter est le facteur
 * multiplicatif sur la taille de la PDU (temps de traitement par octet)
 */
void srvGen_setServiceTime(struct srvGen_t * srv,
			   enum serviceTime_t st,
			   double parameter);

/**
 * @brief Ajout d'une sonde sur le temps de service
 */
void srvGen_addServiceProbe(struct srvGen_t * srv, struct probe_t * serviceProbe);

/**
 * @brief srvGen can also be seen as ndesObjects
 */
declareObjectFunctions(srvGen);

#endif
