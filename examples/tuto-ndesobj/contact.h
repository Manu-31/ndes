/**
 * @file contact.h
 * @brief Definition of a contact in a directory service
 *
 * This is not supposed to be useful, it is just a toy example for
 * ndesObject usage.
 */

#include <ndesObject.h>
#include <ndesObjectList.h>

struct contact_t;

/**
 * @brief Declare contact as a ndesObject type
 */
declareObjectFunctions(contact);

/**
 * @brief Create a contact with a name and number
 * @param name the name of the contact
 * @param number the phone number for the contact
 * @result a new contact, allocated and initialized*
 * 
 * The strings are copied and thus can be freed after the contact
 * creation
 */
struct contact_t * contact_create(char * name, char * num);

/**
 * @brief get a contact name
 * @param contact a pointer to the contact
 * @result a pointer to the name
 *
 * The result should not be written/freed
 */
char * contact_getName(struct contact_t * contact);
#define contact_getName contact_getObjectName

/**
 * @brief set a contact name
 * @param contact a pointer to the contact
 * @param name the name of the contact
 *
 * The strings are copied and thus can be freed after the contact
 * creation
 */
void contact_setName(struct contact_t * contact, char * name);
#define contact_setName contact_setObjectName

/**
 * @brief get a contact phone number
 * @param contact a pointer to the contact
 * @result a pointer to the number
 *
 * The result should not be written/freed
 */
char * contact_getNumber(struct contact_t * contact);

/**
 * @brief set a contact phone number
 * @param contact a pointer to the contact
 * @param number the number of the contact
 *
 * The strings are copied and thus can be freed after the contact
 * creation
 */
void contact_setNumber(struct contact_t * contact, char * number);
