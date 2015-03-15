#include <contact.h>

struct contact_t {
   declareAsNdesObject;
   char * telNum;
};


/**
 * @brief Les entrées de log sont des ndesObject
 */
struct ndesObjectType_t contactType = {
  ndesObjectTypeDefaultValues(contact)
};


/**
 * @brief Définition des fonctions spécifiques liées au ndesObject
 */
defineObjectFunctions(contact);

/**
 * @brief Create a contact with a name and number
 * @param name the name of the contact
 * @param number the phone number for the contact
 * @result a new contact, allocated and initialized*
 * 
 * The strings are copied and thus can be freed after the contact
 * creation
 */
struct contact_t * contact_create(char * name, char * num)
{
   struct contact_t * result = contact_createWithObject(contactType);

   printf_debug(DEBUG_OBJECT, "IN (%s, %s)\n", name, num);

   contact_setName(result, name);
   contact_setNumber(result, num);
   printf_debug(DEBUG_OBJECT, "created %p (obj %p)\n", result, contact_getObject(result));

   return result;
}

/**
 * @brief get a contact phone number
 * @param contact a pointer to the contact
 * @result a pointer to the number
 *
 * The result should not be written/freed
 */
char * contact_getNumber(struct contact_t * contact)
{
   return contact->telNum;
}

/**
 * @brief set a contact phone number
 * @param contact a pointer to the contact
 * @param number the number of the contact
 *
 * The strings are copied and thus can be freed after the contact
 * creation
 */
void contact_setNumber(struct contact_t * contact, char * number)
{
   if (contact->telNum) {
      free(contact->telNum);
   }
   contact->telNum = strdup(number);
}
