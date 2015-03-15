/**
 * @file pdu-filter.h
 * @brief A PDU filter, ...
 *
 * A quick hack that could turn useless !
 */

#ifndef __DEF_NDES_PDU_FILTER
#define __DEF_NDES_PDU_FILTER

#include <ndesObject.h>
#include <ndesObjectList.h>
#include <pdu.h>

/**
 * @brief Filters type
 */
struct PDUFilter_t;


/**
 * @brief Creation of a filter
 * @result An allocated/initialized filter with default behavior
 */
struct PDUFilter_t * PDUFilter_create();

/**
 * @brief Set the private data of a filter
 * @param filter The filter do modify
 * @param private The private data (can be NULL);
 */
void PDUFilter_setPrivate(struct PDUFilter_t * filter, void * private);

/**
 * @brief Get the private data of a filter
 * @param filter The filter
 */
void * PDUFilter_getPrivate(struct PDUFilter_t * filter);

/**
 * @brief Set the test function of a filter
 * @param filter The filter do modify
 * @param testFunc The test function (can be NULL);
 */
void PDUFilter_setTestFunction(struct PDUFilter_t * filter, 
			       int (*test)(void *, struct PDU_t *));

/**
 * @brief Filtering a PDU
 * @param f The filter to apply
 * @param pdu The PDU to filter
 * @result Yes (1) or no (0)
 */
int PDUFilter_filterPDU(struct PDUFilter_t * f, struct PDU_t * pdu);

/**
 * @brief filters can also be seen as ndesObjects
 */
declareObjectFunctions(PDUFilter);

#endif
