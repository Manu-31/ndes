/**
 * @file src-tcpss.h
 * @brief A very basic TCP source model (slow start only)
 *
 * This module implements a very basic TCP source model as described
 * in [1]. This model is relevant to study the consequences of short
 * connections on the client acces link (this is the point of [1]).
 *
 * The only part of TCP that is implemented in this model is the slow
 * start (hence the name). The behaviour is the following. Each time a
 * segment has been actually sent, the transmission of two new
 * segments is scheduled at time now+(RTT-tt) where RTT is the round
 * trip time and tt is the transmission time of the access link.
 *
 * [1] "cdma2000 Evaluation Methodology" 3GPP2
 * C. R1002-0. Version1.0. December 10, 2004
 */
#ifndef __SRC_TCP_H__
#define __SRC_TCP_H__

#include <event.h>
#include <pdu.h>

struct srcTCPSS_t;

/**
 * @brief Creation/initialization of a source
 * @param MTU is the maximum transmission unit of the link
 * @param RTTmd is the Round Trip Time minus transmission time on the access link
 * @param initialWindow is the initial value of cwnd
 * @param destination is a pointer to the destination entity
 * @param destProcessPDU is the PDU processing function of the destination
 */
struct srcTCPSS_t * srcTCPss_create(int MTU,
				    double RTTmd,
                                    int initialWindow,
                                    void * destination,
				    processPDU_t destProcessPDU);

/**
 * @brief Simulate transmission of nbBytes bytes
 * @param src is a pointer to the source
 * @param nbBytes is the number of bytes to transmit
 */
void srcTCPss_sendFile(struct srcTCPSS_t * src,
		       int nbBytes);


/**
 * @brief is the source empty ?
 * @param src is a source
 * @return non null if at least one byte remains to be sent
 *
 * A source is "empty" as soon as every byte has bee sent. Be carefull
 * that an empty source will turn to non empty after a (new) call to
 * srcTCPss_sendFile.
 */
int srcTCPss_isEmpty(struct srcTCPSS_t * src);

/**
 * @brief Delete and free a source
 * @param src an empty source
 * 
 * The caller MUST ensure this source is empty.
 */
void srcTCPss_free(struct srcTCPSS_t * src);

/**
 * @brief Add an event to be run on EOT
 * @param src is a poiter to the source
 * @param ev is the event to schedule for EOT
 */
void srcTCPss_addEOTEvent(struct srcTCPSS_t * src, struct event_t * ev);

#endif
