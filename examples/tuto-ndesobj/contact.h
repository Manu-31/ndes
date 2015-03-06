/**
 * @file contact.h
 * @brief Definition of a contact in a directory service
 *
 * This is not supposed to be useful, it is just a toy example for
 * ndesObject usage.
 */

#include <ndesObject.h>
#include <ndesObjectList.h>

#define STRING_MAX_LENGTH 32

struct contact_t;

/**
 * @brief Déclaration des fonctions spécifiques liées au ndesObject
 */
declareObjectFunctions(contact);
