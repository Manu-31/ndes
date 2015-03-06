/**
 * @file pdu-filter.c
 * @brief A PDU filter, ...
 *
 * A quick hack that could turn useless !
 */

#include <pdu-filter.h>
#include <ndesObject.h>

/**
 * @brief Filters type
 */
struct PDUFilter_t {
   declareAsNdesObject;

  int (*test)(void *, struct PDU_t *);
  void * private;
};

/**
 * @brief ndesObject declaration
 */
struct ndesObjectType_t PDUFilterType = {
   ndesObjectTypeDefaultValues(PDUFilter)
};

/**
 * @brief ndesObject basic definitions
 */
defineObjectFunctions(PDUFilter);

/**
 * @brief Creation of a filter
 * @result An allocated/initialized filter with default behavior
 */
struct PDUFilter_t * PDUFilter_create()
{
   struct PDUFilter_t * result = (struct PDUFilter_t * ) sim_malloc(sizeof(struct PDUFilter_t));

   if (result) {
      ndesObjectInit(result, PDUFilter);
      result->test = NULL;
      result->private = NULL;
   }

   return result;
}

/**
 * @brief Set the private data of a filter
 * @param filter The filter do modify
 * @param private The private data (can be NULL);
 */
void PDUFilter_setPrivate(struct PDUFilter_t * filter, void * private)

{
   filter->private = private;
}

/**
 * @brief Get the private data of a filter
 * @param filter The filter
 */
void * PDUFilter_getPrivate(struct PDUFilter_t * filter)
{
   return filter->private;
}

/**
 * @brief Set the test fucntion of a filter
 * @param filter The filter do modify
 * @param testFunc The test function (can be NULL);
 */
void PDUFilter_setTestFunction(struct PDUFilter_t * filter, 
			       int (*test)(void *, struct PDU_t *))
{
   filter->test = test;

}

/**
 * @brief Filtering a PDU
 * @param f The filter to apply
 * @param pdu The PDU to filter
 * @result Yes (1) or no (0)
 */
int PDUFilter_filterPDU(struct PDUFilter_t * f, struct PDU_t * pdu)
{
   int result = 0;

   //   printf_debug(DEBUG_ALWAYS, "IN\n");

   if (f->test) {
      result = f->test(f->private, pdu); 
   } else {  //!< Default behavior (yes for non NULL pdu)
      if (pdu != NULL) {
         result = 1;
      }
   }

   //   printf_debug(DEBUG_ALWAYS, "OUT %s\n", result?"Yes":"No");

   return result;
}
