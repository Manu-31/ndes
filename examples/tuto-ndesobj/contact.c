#include <contact.h>

struct contact_t {
   declareAsNdesObject;
   char name[STRING_MAX_LENGTH];
   char telNum[STRING_MAX_LENGTH];
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
