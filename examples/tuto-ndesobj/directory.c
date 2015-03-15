#include <motsim.h>
#include <ndesObjectList.h>
#include <directory.h>

struct directory_t {
   declareAsNdesObject;

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

   printf_debug(DEBUG_OBJECT, "IN\n");

   result = directory_createWithObject();
   printf_debug(DEBUG_OBJECT, "creating contacts...\n");

   result->contacts = ndesObjectList_create(&contactType);

   printf_debug(DEBUG_OBJECT, "OUT (%p created)\n", result);

   return result;
} 

/**
 * @brief Insert a name+number in a directory
 * @param dir the directory 
 * @param name the name of the new contact
 * @param num the tel number of the new contact
 *
 * The strings are copied and thus can be freed after the contact
 * insertin
 */
void directory_insertEntry(struct directory_t *dir, char * name, char * num)
{
   struct contact_t * contact;
   struct ndesObject_t * conOb ;

   contact = contact_create(name, num);
   conOb = contact_getObject(contact);

   ndesObjectList_insertSortedObject(dir->contacts, conOb, ndesObject_alphabeticallySorted);
}

void directory_print(struct directory_t *dir)
{
   struct ndesObjectListIterator_t  * iter;
   struct ndesObject_t * ob;
   struct contact_t * c;

   int n = 1;

   printf_debug(DEBUG_OBJECT, "IN\n");

   iter = ndesObjectList_createIterator(dir->contacts);

   printf_debug(DEBUG_OBJECT, "iterator created\n");

   while ((ob = ndesObjectList_iteratorGetNextObject(iter)) != NULL) {
      printf_debug(DEBUG_OBJECT, "one object : %p\n", ob);
      c= contact_getNdesObjectPrivate(ob);
      printf_debug(DEBUG_OBJECT, "got %p\n", c);
      printf("[%2d] \"%s\" -> %s\n", n++,
            contact_getName(c),
            contact_getNumber(contact_getNdesObjectPrivate(ob)));
			   
   }

   printf_debug(DEBUG_OBJECT, "OUT\n");
}

