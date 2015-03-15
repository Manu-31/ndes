#include <string.h>    // memset
 
#include <ndesObject.h>
#include <log.h>

static int ndesObject_nb = 0;

/*-----------------------------------------------------------------------
 * Les fonctions de manipulation des ndesObject
 *-----------------------------------------------------------------------
 */
struct ndesObject_t * ndesObject_create(void * private,
					struct ndesObjectType_t * objectType)
{
   struct ndesObject_t * result;

   result = (struct ndesObject_t *)sim_malloc(sizeof(struct ndesObject_t));
   result->type = objectType;   

   result->id = ndesObject_nb++;
   result->name = NULL;

   result->creationDate = motSim_getCurrentTime();
   result->data = private;
   
   printf_debug(DEBUG_OBJECT, "ndesObject %p created, id %d type \"%s\"\n",
		result,
		result->id,
                result->type->name);

   if (result->type != &ndesLogEntryType) {
      ndesLog_logLineF(result, "TYPE %s", result->type->name);
   };

   return result;
}

/**
 * @brief free an object
 */
void ndesObject_free(struct ndesObject_t * o)
{
   free(o->name);
   free(o);
}

/**
 * @brief Obtention de l'identifiant d'un ndesObject
 */
int ndesObject_getId(struct ndesObject_t * o)
{
  return o->id;
}

/**
 * @brief Obtention du nom d'un ndesObject
 */
char * ndesObject_getName(struct ndesObject_t * o)
{
  return o->name;
}


/**
 * @brief Obtention des données associées à l'objet
 * @param ndesObject a non NULL ndesObject pointer
 * @return The private data associated with this object
 */
void * ndesObject_getPrivate(struct ndesObject_t * ndesObject)
{
   return ndesObject->data;
}

/**
 * @brief Obtention du type de l'objet
 */
struct ndesObjectType_t * ndesObject_getType(struct ndesObject_t * ndesObject)
{
   return ndesObject->type;
}

/*-----------------------------------------------------------------------
 * Les fonctions par défaut
 *-----------------------------------------------------------------------
 */

/**
 * @brief L'allocation de la mémoire pour un objet
 */
void * ndesObject_defaultMalloc(struct ndesObjectType_t * ndesObjectType)
{
   void * result = NULL;

   printf_debug(DEBUG_OBJECT, "IN\n");

   // If a free instance is available, we return it
   if (ndesObjectType->firstFree){
      printf_debug(DEBUG_OBJECT, "re-using\n");
      result = ndesObjectType->firstFree;
      ndesObjectType->firstFree = ((void **)result)[1]; // WARNING c'est pas génial, mais comment faire ?
   } else {
   // If not, we need to alloc
      printf_debug(DEBUG_OBJECT, "malloc'ing\n");
      result = sim_malloc(ndesObjectType->size);
   }

   printf_debug(DEBUG_OBJECT, "OUT\n");

   return result;
}

/**
 * @brief Initialisation générique d'un objet
 *
 * Tous les champs sont mis à 0, sauf le pointeur ndesObject bien sûr
 */
void ndesObject_defaultInit(void * ob)
{
   struct ndesObject_t * ndesObject;

   printf_debug(DEBUG_OBJECT, "IN\n");

   // On sauve le pointeur et on le repositionne ensuite
   // On aurait pu faire autrement, mais j'envisage de faire
   // sauter la contraite que ce soit le premier champ.
   ndesObject = ndesObject_defaultGetObject(ob);

   memset(ob, 0, ndesObject->type->size);

   ndesObject_defaultSetObject(ob, ndesObject);
   printf_debug(DEBUG_OBJECT, "OUT\n");
}

/**
 * @brief Libération générique d'un objet
 */
void ndesObject_defaultFree(void * ob)
{
   struct ndesObject_t *     ndesObject = ndesObject_defaultGetObject(ob);
   struct ndesObjectType_t * objectType = ndesObject->type;

   ((void **)ob)[1] = objectType->firstFree;
   objectType->firstFree = ob;   
}

/**
 * @brief Obtention du pointeur sur le ndesObject
 * @param ob Pointeur vers un objet "quelconque"
 * @result Un pointeur sur le ndesObject associé
 */
struct ndesObject_t * ndesObject_defaultGetObject(void * ob)
{
   struct ndesObject_t * result;

   printf_debug(DEBUG_OBJECT, "IN\n");
   result = ((struct ndesObject_t **)ob)[0];
   printf_debug(DEBUG_OBJECT, "OUT (result = %p)\n", result);

   return result ; 
}

/**
 * @brief Affectation du pointeur sur le ndesObject
 * @param ob Pointeur vers un objet "quelconque"
 * @param ndesObject Un pointeur sur le ndesObject associé
 */
void ndesObject_defaultSetObject(void * ob, struct ndesObject_t * ndesObject)
{
   printf_debug(DEBUG_OBJECT, "IN\n");
   ((struct ndesObject_t **)ob)[0] = ndesObject;
   printf_debug(DEBUG_OBJECT, "OUT\n");
}

/**
 * @brief Création d'un objet du type voulu
 * @param ndesObjectType Le type de l'objet à créer
 * @return Pointeur sur une instance initialisée de l'objet
 *
 * Cette fonction permet de créer un objet dont le type est passé en
 * paramètre. Elle fera pour cela appel aux fonctions d'allocation,
 * d'initialisation définies par le type en question. 
 */
void * ndesObject_createObject(struct ndesObjectType_t * ndesObjectType)
{
   void * result ;

   printf_debug(DEBUG_OBJECT, "IN\n");

   // Allocation avec la fonction spécifique
   result = ndesObjectType->malloc(ndesObjectType->size);

   // Création du ndesObject
   ndesObjectType->setObject(result,
                   ndesObject_create(result, ndesObjectType));

   // Initialisation
   ndesObjectType->init(result);

   printf_debug(DEBUG_OBJECT, "OUT\n");

   return result;
}


/*
void ndesObject_addType(char * name, struct ndesTypeHelper_t * helper)
{
}
*/

int ndesObject_alphabeticallySorted(void * a, void * b)
{
   printf_debug(DEBUG_OBJECT, "seeking object a\n");
   struct ndesObject_t * objectA = ndesObject_defaultGetObject(a);
   printf_debug(DEBUG_OBJECT, "seeking object a\n");
   struct ndesObject_t * objectB = ndesObject_defaultGetObject(b);
   printf_debug(DEBUG_OBJECT, "name of a = \"%s\"\n", ndesObject_getName(objectA));
   printf_debug(DEBUG_OBJECT, "name of b = \"%s\"\n", ndesObject_getName(objectA));

   return (strcmp(ndesObject_getName(objectA), ndesObject_getName(objectB)) < 0);
}

