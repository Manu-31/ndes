/**
 * @file muxdemux.h
 * @brief A trivial multiplexing/demultiplexing protocol
 *
 */
#ifndef __DEF_MUXDEMUX__
#define __DEF_MUXDEMUX__

#include <pdu.h>
#include <ndesObject.h>
#include <ndesObjectList.h>
#include <pdu-filter.h>

/**
 * @brief A multiplexing sender
 *
 */
struct muxDemuxSender_t;

/**
 * @brief A multiplexer service access point
 */
struct muxDemuxSenderSAP_t;

/**
 * @brief A demultiplexing sender
 *
 */
struct muxDemuxReceiver_t;

/**
 * @brief A demultiplexer service access point
 */
struct muxDemuxReceiverSAP_t;

/**
 * @brief The sender input fonction
 */
processPDU_t muxDemuxSenderProcessPDU;

/**
 * @brief Sender (multiplexer) creator
 */
struct muxDemuxSender_t * muxDemuxSender_create(void * destination,
						processPDU_t destProcessPDU);

/**
 * @brief Creation of a new Service Access Point on a sender
 * @param sender Pointer to the associated sender
 * @param newSAPI SAP identifier (0 if unspecified)
 */
struct muxDemuxSenderSAP_t * muxDemuxSender_createNewSAP(struct muxDemuxSender_t * sender,
							 unsigned int newSAPI);

struct muxDemuxSenderSAP_t * muxDemuxSender_getSAP(struct muxDemuxSender_t *);

 
/**
 * @brief Input function for a sender
 */
int muxDemuxSender_processPDU(void * muxSenderSAP,
				 getPDU_t getPDU,
				 void * source);

/**
 * @brief Create a filter based on a SAP
 */
struct PDUFilter_t * muxDemuxSender_createFilterFromSAP(struct muxDemuxSenderSAP_t * sap);


/**
 * @brief Receiver (demultiplexer) creator
 */
struct muxDemuxReceiver_t * muxDemuxReceiver_create();

/**
 * @brief Creation of a new Service Access Point on a receiver
 * @param receiver Pointer to the associated receiver
 * @param newSAPI SAP identifier (0 if unspecified)
 */
struct muxDemuxReceiverSAP_t * muxDemuxReceiver_createNewSAP(struct muxDemuxReceiver_t * receiver,
						             unsigned int newSAPI,
							     void * destination,
							     processPDU_t destProcessPDU);
/**
 * @brief The receiver input fonction
 */
int muxDemuxReceiver_processPDU(void * rcv,
   			        getPDU_t getPDU,
			        void * source);

#endif
