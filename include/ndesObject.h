/**
 * @file ndesObject.h
 * @brief Définition du type "ancètre" de tous les objets du simulateur
 *
 * L'idée est d'avoir un type général qui permette de regrouper toutes
 * les propriétés communes des objets utilisés dans le simulateur afin
 * d'en simplifier l'usage.
 *
 * Un certain nombre de fonctions manipulant des ndesObject est alors
 * founi. Ces fonctions permettent par exemple une gestion mémoire
 * évitant des appels à malloc, une recherche d'objet par identifiant,
 * ou encore une gestion de liste d'objets.
 *
 * Des fonctions spécifiques à chaque type permettent en particulier
 * des manipulations de conversion entre le type et le ndesObject.
 *
 * Enfin des fonctions génériques permettent de manipuler un pointeur
 * indépendemment de son type. Mais ce dernier doit avoir été déclaré
 * comme décrit ci-dessous.
 */
#ifndef __DEF_ndesObject
#define __DEF_ndesObject

#include <stddef.h>   // offsetof
#include <string.h>   // strdup

#include <motsim.h>
#include <probe.h>

/**
 * @brief Le type général de tous les objets
 *
 * Le type est visible car utilisé par différents modules vu que c'est
 * un peu le couteau suisse de l'outil de simulation. Idéalement, il
 * faut utiliser autant que possible les méthodes de manipulation
 * fournies plus bas.
 */
struct ndesObject_t {
   int             id;           //!< Un identifiant général
   char          * name;
   motSimDate_t    creationDate;
   
   void        * data;  //!< Des donnees privées
   struct ndesObjectType_t * type; //!< Les fonctions spécifiques de manipulation
};

/**
 *  @brief Comment manipuler un type d'objet particulier
 *
 * C'est une instance de cette structure qui définit un type
 * particulier d'objets 
 */
struct ndesObjectType_t {
   char * name;                 //!< Le nom du type 
   struct ndesObject_t * (*getObject)(void*);
   void   (*setObject)(void *, struct ndesObject_t *);
   void * (*malloc)() ;         //!< Allocation d'une instance
   void   (*init)(void *);      //!< Initialisation
   void   (*free)(void *) ;     //!< Destruction d'une instance
   int    size;                 //!< La taille de la structure privée
   int    objectOffset; 
   void * firstFree;             //!!< A pointer to the first free entry
};

/*
 * Les macros suivantes permettent de simplifier la déclaration d'un
 * type qui est aussi un objet.
 *
 * Un type les utilisant devra être décrit de la façon suivante
 *
 * struct exemple_t {
 *    declareAsNdesObject;
 *    ...
 * };
 *
 * defineObjectFunctions(exemple);
 *
 * struct ndesObjectType_t exempleType = {
 *    ndesObjectTypeDefaultValues(exemple),
 *    ...
 *}
 */

/**
 * @brief Définition d'un champ ndesObject dans la structure
 *
 * Cette déclaration doit être la première de toute structure qui doit
 * profiter des ndesObject.
 * Le champs nextFree est une superbe horreur pour chainer des object
 * libérés afin de les ré-utiliser
 */
#define declareAsNdesObject \
   struct ndesObject_t * ndesObject; \
   void                * nextFree

/**
 * @brief Définition de la fonction d'obtention de l'objet
 */
#define defineObjectFunctions(ndesObjectType) \
struct ndesObject_t * ndesObjectType##_getObject(struct ndesObjectType##_t * o) \
{    \
   printf_debug(DEBUG_OBJECT, "IN\n");\
   return o->ndesObject; \
}  \
  \
void ndesObjectType##_setObject(struct ndesObjectType##_t * o,  struct ndesObject_t *ndesObject)	\
{    \
   o->ndesObject = ndesObject; \
}  \
int ndesObjectType##_getObjectId(struct ndesObjectType##_t * o) \
{    \
   return o->ndesObject->id; \
} \
void ndesObjectType##_setName(struct ndesObjectType##_t * o, const char * n)	\
{    \
  o->ndesObject->name = strdup(n);		\
} \
char * ndesObjectType##_getName(struct ndesObjectType##_t * o)	\
{    \
  return o->ndesObject->name;		\
} \
struct ndesObjectType##_t * ndesObjectType##_createWithObject() \
{ \
   struct ndesObjectType##_t * result; \
   struct ndesObject_t       * obj; \
   printf_debug(DEBUG_OBJECT, "allocating data\n");	\
   result = ndesObjectType##Type.malloc();		\
   printf_debug(DEBUG_OBJECT, "allocating object\n");	\
   obj = ndesObject_create(result, &ndesObjectType##Type);	\
   ndesObjectType##_setObject(result, obj); \
     printf_debug(DEBUG_OBJECT, "OUT (%p created)\n", result);	\
   return result;\
} \

   
/**
 * @brief Déclaration pour le .h
 */
#define declareObjectFunctions(ndesObjectType) \
struct ndesObject_t * ndesObjectType##_getObject(struct ndesObjectType##_t * o); \
void ndesObjectType##_setObject(struct ndesObjectType##_t * o,  struct ndesObject_t *ndesObject); \
int ndesObjectType##_getObjectId(struct ndesObjectType##_t * o); \
void ndesObjectType##_setName(struct ndesObjectType##_t * o, const char * n); \
char * ndesObjectType##_getName(struct ndesObjectType##_t * o);	\
struct ndesObjectType##_t * ndesObjectType##_create_(); \
 struct ndesObjectType_t ndesObjectType##Type;

   
/**
 * @brief Valeurs par défaut des champs de définition du type
 */
#define ndesObjectTypeDefaultValues(myType)	\
   .name         = #myType,       \
   .malloc       = ndesObject_defaultMalloc,	\
   .init         = ndesObject_defaultInit,  \
   .free         = ndesObject_defaultFree,   \
   .firstFree    = NULL,    \
   .getObject    = ndesObject_defaultGetObject, \
   .setObject    = ndesObject_defaultSetObject, \
   .size         = sizeof(struct myType##_t),    \
   .objectOffset = offsetof(struct myType##_t, ndesObject)

/**
 * @brief L'initialisation de la partie ndesObject d'un objet
 */
#define ndesObjectInit(private, myType)		\
  private->ndesObject = ndesObject_create(private, &myType##Type);	\
  private->nextFree = NULL;

/*-----------------------------------------------------------------------
 * Les fonctions de manipulation des ndesObject
 *-----------------------------------------------------------------------
 */

/**
 * @brief Création d'un ndesObject.
 * @param private pointeur sur les données privées
 * @param objectType pointeur sur la structure définissant le type
 * @return un ndesObject alloué et initialisé
 */
extern struct ndesObject_t * ndesObject_create(void * private,
			  struct ndesObjectType_t * objectType);

/**
 * @brief Obtention de l'identifiant d'un ndesObject
 */
extern int ndesObject_getId(struct ndesObject_t * ndesObject);

/**
 * @brief Obtention de la date de création
 */
extern motSimDate_t ndesObject_getCreationDate(struct ndesObject_t * ndesObject);

/**
 * @brief Obtention des données associées à l'objet
 * @param ndesObject a non NULL ndesObject pointer
 * @return The private data associated with this object
 */
void * ndesObject_getPrivate(struct ndesObject_t * ndesObject);

/**
 * @brief Obtention du type de l'objet
 */
struct ndesObjectType_t * ndesObject_getType(struct ndesObject_t * ndesObject);

/**
 * @brief Destruction d'un ndesObject.
 *
 * Les donnees privees doivent avoir ete detruites par l'appelant.
 */
void ndesObject_free(struct ndesObject_t * pdu);

/*-----------------------------------------------------------------------
 * Les fonctions par défaut
 *-----------------------------------------------------------------------
 */

/**
 * @brief L'allocation de la mémoire pour un objet
 */
void * ndesObject_defaultMalloc(struct ndesObjectType_t * ndesObjectType);

/**
 * @brief Initialisation générique d'un objet
 */
void ndesObject_defaultInit(void * ob);

/**
 * @brief Libération générique d'un objet
 */
void ndesObject_defaultFree(void * ob);

/**
 * @brief Obtention du pointeur sur le ndesObject
 * @param ob Pointeur vers un objet "quelconque"
 * @result Un pointeur sur le ndesObject associé
 */
struct ndesObject_t * ndesObject_defaultGetObject(void * ob);

/**
 * @brief Affectation du pointeur sur le ndesObject
 * @param ob Pointeur vers un objet "quelconque"
 * @param ndesObject Un pointeur sur le ndesObject associé
 */
void ndesObject_defaultSetObject(void * ob, struct ndesObject_t * ndesObject);

/*-----------------------------------------------------------------------
 */

/**
 * @brief Création d'un objet du type voulu
 * @param ndesObjectType Le type de l'objet à créer
 * @return Pointeur sur une instance initialisée de l'objet
 *
 * Cette fonction permet de créer un objet dont le type est passé en
 * paramètre. Elle fera pour cela appel aux fonctions d'allocation,
 * d'initialisation définies par le type en question. 
 */
void * ndesObject_createObject(struct ndesObjectType_t * ndesObjectType);

/*-----------------------------------------------------------------------
 * Gestion des types d'objets
 */
void ndesObject_addType(char * name, struct ndesObjectType_t * helper);


#endif
