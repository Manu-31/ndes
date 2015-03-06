#include <stdlib.h>    // Malloc, NULL, ...
#include <assert.h>

#include <pdu.h>
#include <ndesObject.h>

/* WARNING
 * Le type est visible car utilisé par différents modules vu que c'est
 * un peu le couteau suisse de l'outil de simulation. Idéalement, il
 * faut utiliser autant que possible les méthodes de manipulation
 * fournies plus bas.
 */
struct PDU_t {
   declareAsNdesObject;

   int      id;    // Un identifiant général
   motSimDate_t  creationDate;

   void   * data;  // Des donnees privées
   int      taille ;

   // Les pointeurs suivants sont à la discrétion du propriétaire de la PDU
   // WARNING c'est une horreur à virer
   struct PDU_t * prev;
   struct PDU_t * next;
};

/**
 * @brief Déclaration des fonctions spécifiques liées au ndesObject
 */
declareObjectFunctions(PDU);

/**
 * @brief Les entrées de log sont des ndesObject
 */
struct ndesObjectType_t PDUType = {
  ndesObjectTypeDefaultValues(PDU)
};

/**
 * @brief Définition des fonctions spécifiques liées au ndesObject
 */
defineObjectFunctions(PDU);

static int pduNB = 0;

// Pour suivre un peu l'origine des PDU
struct probe_t * PDU_createProbe;
struct probe_t * PDU_reuseProbe;
struct probe_t * PDU_mallocProbe;
struct probe_t * PDU_releaseProbe;

// Pointeur sur une PDU libre (pour accélerer alloc/free)
struct PDU_t * firstFreePDU = NULL;


int PDU_size(struct PDU_t * PDU){
   return PDU->taille;
}

int PDU_id(struct PDU_t * PDU){
   return PDU->id;
}

void * PDU_private(struct PDU_t * PDU){
   return PDU->data;
}

motSimDate_t PDU_getCreationDate(struct PDU_t * PDU){
   return PDU->creationDate;
}

struct PDU_t * PDU_create(int size, void * private) 
{
   struct PDU_t * PDU ;

   if (firstFreePDU){
      PDU = firstFreePDU;
      firstFreePDU = PDU->next;
      assert(PDU);
      probe_sample(PDU_reuseProbe, (double)PDU->id);
   } else {
      PDU = (struct PDU_t *)sim_malloc(sizeof(struct PDU_t));
      assert(PDU);
      probe_sample(PDU_mallocProbe, (double)PDU->id);
   }

   ndesObjectInit(PDU, PDU);
   PDU->taille = size;
   PDU->id = pduNB ++;
   PDU->data = private;
   PDU->creationDate = motSim_getCurrentTime();
   PDU->next = NULL;
   PDU->prev = NULL;

   probe_sample(PDU_createProbe, (double)PDU->id);

   printf_debug(DEBUG_FILE, "PDU %d created (size %d)\n", PDU->id, PDU->taille);

   return PDU;
}

/*
 * Destruction d'une PDU. Les donnees privees doivent avoir
 * ete detruites par l'appelant.
 */
void PDU_free(struct PDU_t * pdu)
{
   if (pdu != NULL) {
      probe_sample(PDU_releaseProbe, (double)pdu->id);

      pdu->next = firstFreePDU;
      firstFreePDU = pdu;
   }
}

/**
 * @brief Get next PDU
 * @param pdu non NULL
 */
struct PDU_t * PDU_getNext(struct PDU_t * pdu)
{
   return  pdu->next;
}

/**
 * @brief Get next PDU
 * @param pdu non NULL
 */
struct PDU_t * PDU_getPrev(struct PDU_t * pdu)
{
   return  pdu->prev;
}

/**
 * @brief Get next PDU
 * @param pdu non NULL
 * @param next can be null
 */
void PDU_setNext(struct PDU_t * pdu, struct PDU_t * next)
{
   pdu->next = next;
}

/**
 * @brief Get next PDU
 * @param pdu non NULL
 * @param prev can be null
 */
void PDU_setPrev(struct PDU_t * pdu, struct PDU_t * prev)
{
   pdu->prev = prev;
}
