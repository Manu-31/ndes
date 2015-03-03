/**
 *  @file log.c
 *  @brief Gestion des log de NDES
 *
 *  Pas encore pret à être utilisé ! L'objectif des logs est
 *  d'enregistrer des événements significatifs dans une simulation. La
 *  définition de ce qui est significatif est à la discrétion de
 *  l'utilisateur ! L'idée n'est pas de fournir un outil de débogage,
 *  il n'a pas vocation à être utilisé pour des gros volumes de
 *  données. Il s'agit plutôt de permettre de transcrire des petites
 *  simulations, par exemple pour tracer des chronogrammes à vocation
 *  pédagogique ou illustrative.
 */
#include <string.h>

#ifdef NDES_USES_LOG
#   define NDES_USES_LOG_IS_SET
#else
#   undef NDES_USES_LOG_IS_SET
#endif

#define NDES_USES_LOG // Nécessaire localement
#include <log.h>

#ifdef NDES_USES_LOG_IS_SET
#   define NDES_USES_LOG
#else
#   undef NDES_USES_LOG
#endif


#include <ndesObjectList.h>

/**
 * @brief définition d'une entrée dans les log
 */
struct  ndesLogEntry_t {
   declareAsNdesObject; //< C'est un ndesObject 

   motSimDate_t date ; //< Date d'occurence de l'événement
   char * msg;         //< Représentation textuelle
   struct ndesObject_t * object;  //< L'objet sur lequel se produit l'événement
};

/**
 * @brief Définition des fonctions spécifiques liées au ndesObject
 */
defineObjectFunctions(ndesLogEntry);

/**
 * @brief Les entrées de log sont des ndesObject
 */
struct ndesObjectType_t ndesLogEntryType = {
  ndesObjectTypeDefaultValues(ndesLogEntry)
};


/**
 * @brief La fonction d'allocation
 */
struct ndesLogEntry_t * ndesLogEntry_alloc()
{
   struct ndesLogEntry_t * result;

   result = sim_malloc(sizeof(struct ndesLogEntry_t *));

   return result;
}

/**
 * @brief Un créateur avec paramètres
 */
struct ndesLogEntry_t * ndesLogEntry_create(struct ndesObject_t * object,
					    char * msg)
{
   struct ndesLogEntry_t * result;

   result = ndesLogEntry_alloc();

   ndesObjectInit(result, ndesLogEntry);

   result->date = motSim_getCurrentTime();
   result->object = object;
   result->msg = strdup(msg);

   return result;
}

/**
 * @brief Structure du log
 * 
 * On peut en avoir plusieurs si besoin est.
 */
struct ndesLog_t {
   struct ndesObjectList_t * journal; //< Liste des événements
} ndesLog_t;

/**
 * Le log général
 */
struct ndesLog_t * ndesLog;

/**
 * @brief Activation du log
 */
void ndesLog_enable()
{
}

/**
 * @brief Désactivation du log
 */
void ndesLog_disable()
{
}

/**
 * @brief Création d'un log
 */
struct ndesLog_t * ndesLog_create()
{
   struct ndesLog_t * result;

   // On crée l'objet
   result = (struct ndesLog_t *)sim_malloc(sizeof(struct ndesLog_t));
 
   // Le journal est une liste de logEntry
   result->journal = ndesObjectList_create(&ndesLogEntryType);

   return result;
};

/**
 * @brief Initialisation du log
 */
void ndesLog_init()
{
   ndesLog = ndesLog_create();
}

/**
 * @brief Insertion d'une ligne de log
 */
void ndesLog_logLine(struct ndesObject_t * ndesObject, char * line)
{
   struct ndesLogEntry_t * le;

   le = ndesLogEntry_create(ndesObject, line);
   ndesObjectList_insert(ndesLog->journal, le);
}

/**
 * @brief Insertion d'une ligne de log avec des ndesObject
 */
void ndesLog_logLineF(struct ndesObject_t * ndesObject, char * fmt, ...)           
{
   char msg[1024];
   va_list args;
 
   printf_debug(DEBUG_ALWAYS, "Coucou IN\n");

   va_start(args, fmt);
   printf(fmt, args);
   //   vsprintf(msg, fmt, args);
   va_end(args);

   ndesLog_logLine(ndesObject, msg);

   printf_debug(DEBUG_ALWAYS, "OUT (msg = \"%s\")\n", msg);
}


/*
 * Dump du log dans un fichier (l'éventuel contenu précédent est détruit)
 */
int ndesLog_dump(char * fileName)
{
   struct ndesObjectListIterator_t * ofi;
   struct ndesObject_t             * obj;
   struct ndesLogEntry_t           * le;

   ofi = ndesObjectList_createIterator(ndesLog->journal);
   while ((obj = ndesObjectList_iteratorGetNext(ofi))) {
      assert(ndesObject_getType(obj) == &ndesLogEntryType);
      le = ndesObject_getPrivate(obj);
      printf("[LOG] %f %d \"%s\" !\n", le->date, ndesObject_getId(le->object), le->msg);
   }
   ndesObjectList_deleteIterator(ofi);
   
   return -1;
}

/*
 * @brief Définition permettant d'utiliser ndesObject pour le Log
 *
 */
struct ndesObjectType_t ndesLog_type = {
   .name = "log",
   .malloc = (void * (*)())ndesLog_create
};
