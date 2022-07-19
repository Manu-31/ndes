/**
 * @file muxfcfs.h
 * @brief Définition d'un multiplexeur FCFS
 * 
 * Il permet donc de transformer un nombre quelconque de sources en
 * une source unique.  
 */
#ifndef __DEF_MUX_FCFS
#define __DEF_MUX_FCFS

#include <pdu.h>
#include <probe.h>

struct muxfcfs_t ;

/*
 * Création d'un multiplexeur
 */
struct muxfcfs_t * muxfcfs_create(void * destination,
				  processPDU_t destProcessPDU);

/*
 * Demande d'une PDU par la destination
 */
struct PDU_t * muxfcfs_getPDU(void * vm);

/*
 * Soumission d'une PDU par une source
 */
int muxfcfs_processPDU(void * vm,
		       getPDU_t getPDU,
		       void * source);

#endif

