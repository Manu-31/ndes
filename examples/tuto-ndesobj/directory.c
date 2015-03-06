#include <motsim.h>
#include <ndesObjectList.h>
#include <directory.h>

struct directory_t {
   declareAsNdesObject;
   char name[STRING_MAX_LENGTH];
   struct ndesObjectList_t * contacts;
};


/**
 * @brief Les entrées de log sont des ndesObject
 */
struct ndesObjectType_t directoryType = {
  ndesObjectTypeDefaultValues(directory)
};


/**
 * @brief Définition des fonctions spécifiques liées au ndesObject
 */
defineObjectFunctions(directory);

/**
 * @brief Create a directory
 */
struct directory_t * directory_create()
{
   struct directory_t * result;

   printf_debug(DEBUG_ALWAYS, "IN\n");

   result = directory_createWithObject();

   result->contacts = ndesObjectList_create(&contactType);

   printf_debug(DEBUG_ALWAYS, "OUT (%p created)\n", result);

   return result;
} 
