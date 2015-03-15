/**
 * @file pdu-sink.h
 * @brief Définition des puits de PDU
 */
#ifndef __DEF_PDU_SINK
#define __DEF_PDU_SINK

#include <pdu.h>
#include <ndesObject.h>

struct PDUSink_t;

struct PDUSink_t * PDUSink_create();

/*
 * La fonction de consommation d'une PDU
 */
int PDUSink_processPDU(void * pduSink, getPDU_t getPDU, void * source);

// WARNING faudrait voir à pouvoir faire ça :
//processPDU_t PDUSink_processPDU;

/**
 * @brief Affectation d'une sonde sur les evenements d'insertion
 */
void PDUSink_addInputProbe(struct PDUSink_t * sink,
			   struct probe_t * insertProbe);

/**
 * @brief PDUSink can also be seen as ndesObjects
 */
declareObjectFunctions(PDUSink);

#endif
