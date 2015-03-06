/**
 * @file file_pdu.c
 * @brief Implantation de la gestion des files de PDUs
 *
 */
#include <stdlib.h>    // Malloc, NULL, ...
#include <assert.h>

#include <file_pdu.h>
#include <motsim.h>
#include <ndesObject.h>
#include <log.h>

/*
struct filePDU_t_elt {
   struct PDU_t         * PDU;
   motSimDate_t           date_in;
   struct filePDU_t_elt * suivant;
   struct filePDU_t_elt * precedent;
};
*/
/**
 * @brief Structure d'une file
 */
struct filePDU_t {
   declareAsNdesObject;  //!< C'est un ndesObject

   int           nombre;
   unsigned long size;       //!< Le volume rÃ©el
   int           nbOverflow; //!< Nombre de pertes par dÃ©passement

   enum filePDU_dropStrategy dropStrategy;

   int           maxLength;  //!< Nombre maximal d'Ã©lÃ©ments
   int           maxSize ;   //!< Le volume maximal

   /* Gestion de la file */
   struct PDU_t * premier;
   struct PDU_t * dernier;

   /* Mesure des dÃ©bits d'entrÃ©e et sortie */
   struct probe_t * throuhputIn;
   struct probe_t * throuhputOut;

   /* Gestion de la sortie */
   void * destination; // L'objet auquel sont destinÃ©es les PDUs
   processPDU_t destProcessPDU; // La fonction permettant d'envoyer la PDU

   /* Les sondes */
   struct probe_t * insertProbe;
   struct probe_t * extractProbe;
   struct probe_t * dropProbe;
   struct probe_t * sejournProbe;
   struct probe_t * lengthProbe;
};

/**
 * @brief Définition des fonctions spécifiques liées au ndesObject
 */
struct ndesObjectType_t filePDUType = {
   ndesObjectTypeDefaultValues(filePDU)
};
defineObjectFunctions(filePDU);

/*
 * Un affichage un peu moche de la file. Peut Ãªtre utile dans des
 * phases de dÃ©bogage.
 */
void filePDU_dump(struct filePDU_t * file)
{
  struct PDU_t * pq;

  printf("DUMP %2d elts (id:size/data) : ", file->nombre);
  for (pq=file->premier; pq != NULL; pq=PDU_getNext(pq)){
    printf("[%d:%d/%p] ", PDU_id(PDU_private(pq)), PDU_size(PDU_private(pq)),
	   PDU_private((struct PDU_t *)PDU_private(pq)));
  }
  printf("\n");
}

/*
 * Extraction du premier Ã©lÃ©ment de la file.
 *
 * Retour
 *   pointeur sur la PDU qui vient d'Ãªtre extraite
 *   NULL si la file Ã©tait vide
 */
struct PDU_t * filePDU_extract(struct filePDU_t * file)
{
   void * PDU = NULL;
   struct PDU_t * premier;

   printf_debug(DEBUG_FILE, " file %p extracting PDU (out of %d) at %6.3f\n", file, file->nombre, motSim_getCurrentTime());
   //  filePDU_dump(file);

   if (file->premier) {
      PDU = PDU_private(file->premier);
      premier = file->premier;
      file->premier = PDU_getNext(premier);
      // Si c'Ã©tait le seul
      if (file->dernier == premier) {
         assert(PDU_getNext(premier) == NULL);
	 assert(file->nombre == 1);
         file->dernier = NULL;
      } else {
	 assert(file->premier != NULL);
         PDU_setPrev(file->premier, NULL);
      }
      file->nombre --;
      file->size -= PDU_size(PDU_private(premier));

      /* Gestion des sondes */
      if (file->extractProbe) {
         probe_sampleValuePDUFilter(file->extractProbe, PDU_size(PDU_private(premier)), PDU_private(premier));
         //probe_sample(file->extractProbe, PDU_size(premier->data));
      }
      if (file->sejournProbe) {
         if(motSim_getCurrentTime() <  PDU_getCreationDate(premier)){
	    printf_debug(DEBUG_WARN, "Attention, quand on purge, il ne faut pas mettre dans les sondes\n");
         } else {
	    probe_sampleValuePDUFilter(file->sejournProbe, motSim_getCurrentTime() - PDU_getCreationDate(premier), PDU);
            //probe_sample(file->sejournProbe, motSim_getCurrentTime() - premier->creationDate);
	 }
      }
      PDU_free(premier);
   }
   printf_debug(DEBUG_FILE, "out (pdu id %d)\n", PDU?PDU_id(PDU):-1);
   //   filePDU_dump(file);
   ndesLog_logLineF(PDU_getObject(PDU), "OUT %d", filePDU_getObjectId(file));
   
   return PDU;
}

struct PDU_t * filePDU_getPDU(void * file)
{
   struct PDU_t * pdu = filePDU_extract((struct filePDU_t *) file);

   return pdu;
}

/*
 * DÃ©finition d'une capacitÃ© maximale en octets. Une valeur nulle
 * signifie pas de limite.
 */
void filePDU_setMaxSize(struct filePDU_t * file, unsigned long maxSize)
{
   printf_debug(DEBUG_ALWAYS, "Taille max %ld bytes\n", maxSize);
   file->maxSize = maxSize; 
}

unsigned long filePDU_getMaxSize(struct filePDU_t * file)
{
  return file->maxSize;
}

void filePDU_setMaxLength(struct filePDU_t * file, unsigned long maxLength)
{
   file->maxLength = maxLength; 
}

unsigned long filePDU_getMaxLength(struct filePDU_t * file)
{
  return file->maxLength;
}

/*
 * Choix de la stratÃ©gie de perte en cas d'insersion dans une file
 * pleine. Attention, insÃ©rer une PDU de taille t dans une file de
 * capacitÃ© max < t n'est pas une erreur, mais engendre simplement un
 * Ã©vÃ©nement d'overflow.
 */
void filePDU_setDropStrategy(struct filePDU_t * file, enum filePDU_dropStrategy dropStrategy)
{
   file->dropStrategy = dropStrategy; 
}

/*
 * RÃ©initialisation dans un Ã©tat permettant de lancer une nouvelle
 * simulation. Ici il suffit de vider la file de tous ses Ã©lÃ©ments.
 */
void filePDU_reset(struct filePDU_t * file)
{
   struct PDU_t * PDU;

   PDU = filePDU_extract(file);

   while (PDU) {
      PDU_free(PDU);
      PDU = filePDU_extract(file);
   }

   file->nbOverflow = 0;

   assert(file->premier == NULL);
   assert(file->dernier == NULL);
   assert(file->nombre == 0);
   assert(file->size == 0);
}

/** @brief Création d'une file.
 * 
 *  @param destination l'entité aval (ou NULL ai aucune)
 *  @param destProcessPDU la fonction de traitement de l'entité aval
 *  (ou NULL si aucune entité)
 *  @return Une strut filePDU_t * allouée et initialisée
 *
 *  Il est possible de ne pas fournir d'entité aval en paramètre, car
 *  une file peut être utilisée également comme un simple outil de
 *  gestion mémoire, sans entrer dans un modèle de réseau. On
 *  utilisera alors simplement les fonctions d'insertion et d'extraction
 */
struct filePDU_t * filePDU_create(void * destination,
                                  processPDU_t destProcessPDU)
{
   printf_debug(DEBUG_FILE, "in\n");

   struct filePDU_t * result = (struct filePDU_t *) sim_malloc(sizeof(struct filePDU_t));
   assert(result);

   ndesObjectInit(result, filePDU);

   result->nombre = 0;
   result->size = 0;
   result->maxSize = 0;
   result->maxLength = 0;
   result->nbOverflow = 0;
   result->dropStrategy = filePDU_dropTail;

   //   result->throughputIn = ;
   //   result->throughputOut = ;

   result->premier = NULL;
   result->dernier = NULL;

   result->destProcessPDU = destProcessPDU;
   result->destination = destination;

   // A priori, pas de sonde
   result->insertProbe = NULL;
   result->extractProbe = NULL;
   result->sejournProbe = NULL;
   result->lengthProbe = NULL;

   // Ajout Ã  la liste des choses Ã  rÃ©initialiser avant une prochaine simu
   motsim_addToResetList(result, (void (*)(void *))filePDU_reset);

   printf_debug(DEBUG_FILE, "out\n");

   return result;
}

void filePDU_insert(struct filePDU_t * file, struct PDU_t * PDU)
{
   struct PDU_t * pq;
   struct PDU_t * pduDel;
 
   printf_debug(DEBUG_FILE, " file %p insert PDU %d size %d (Length = %d/%d, size = %lu/%d, strat %d)\n",
                file, PDU_id(PDU), PDU_size(PDU),
                file->nombre, file->maxLength, file->size, file->maxSize, file->dropStrategy );

   //   filePDU_dump(file);

   // S'il s'agit d'une "drop head", on fait la place si besoin est !
   if (file->dropStrategy == filePDU_dropHead) {
      while ((((file->maxLength) && (file->maxLength < file->nombre + 1))    // Trop de PDUs
             ||            
	    ((file->maxSize) && (file->maxSize < file->size + PDU_size(PDU))) // Trop de volume
	    ) && (file->nombre)) {
 	 printf_debug(DEBUG_FILE, "need some room, head droping ...\n");

         /* Gestion des sondes */
         if (file->dropProbe) {
            probe_sample(file->dropProbe, PDU_size(PDU));
         }

         pduDel = filePDU_extract(file);
         ndesLog_logLineF(PDU_getObject(pduDel), "DELETED BY %d", filePDU_getObjectId(file));

         PDU_free(pduDel);

         file->nbOverflow++;
      }
   }

   // S'il s'agit d'une drop tail 
   // WARNING a mieux expliquer, voire re écrire
   if (((file->maxSize == 0)||(file->size + PDU_size(PDU) <= file->maxSize))
       && ((file->maxLength == 0)||(file->nombre + 1 <= file->maxLength))) {
      pq = PDU_create(0, PDU);// Oui, les PDUs servent de chainons de liste !

      PDU_setNext(pq, NULL);
      PDU_setPrev(pq, file->dernier);

      if (file->dernier)
         PDU_setNext(file->dernier, pq);

      file->dernier = pq;

      if (!file->premier)
         file->premier = pq;

      file->nombre++;
      file->size += PDU_size(PDU);

      ndesLog_logLineF(PDU_getObject(PDU), "IN %d", filePDU_getObjectId(file));

      /* Gestion des sondes */
      if (file->insertProbe) {
         probe_sample(file->insertProbe, PDU_size(PDU));
      }
      if (file->lengthProbe) {
         probe_sampleValuePDUFilter(file->lengthProbe, filePDU_length(file)-1, PDU);
      }

      /* WARNING : on le fait toujours ou pour le premier ? */
      if (file->destProcessPDU && file->destination) {
 	 printf_debug(DEBUG_FILE, " on passe la PDU (Length = %d/%d, size = %lu/%d, strat %d)\n",
		      file->nombre, file->maxLength, file->size, file->maxSize, file->dropStrategy );


	 (void)file->destProcessPDU(file->destination, (getPDU_t)filePDU_extract, file);
      }
      //printf_debug(DEBUG_FILE, "Apres :\n");
      //   filePDU_dump(file);
   } else {
     printf_debug(DEBUG_FILE, "need some room, tail droping ...\n");

      /* Gestion des sondes */
      if (file->dropProbe) {
         probe_sample(file->dropProbe, PDU_size(PDU));
      }
      ndesLog_logLineF(PDU_getObject(PDU), "DELETED BY %d", filePDU_getObjectId(file));

      PDU_free(PDU); 
      file->nbOverflow++;
   }

   printf_debug(DEBUG_FILE, " END insterting PDU (Length = %d/%d, size = %lu/%d, strat %d)\n",
		file->nombre, file->maxLength, file->size, file->maxSize, file->dropStrategy );

}

/*
 * Une fonction permettant la conformitÃ© au modÃ¨le d'Ã©change
 */
int filePDU_processPDU(void * f,
		       getPDU_t getPDU,
		       void * source)
{
   struct filePDU_t * file = (struct filePDU_t *)f;
   struct PDU_t * pdu;

   // Si c'est juste pour tester si je suis pret
   if ((getPDU == NULL) || (source == NULL)) {
      printf_debug(DEBUG_ALWAYS, "getPDU and source should now be non NULL\n");
      return 1; // WARNING Une file est toujours prete quitte à perdre !
   }


   assert(getPDU != NULL);
   assert(source != NULL);

   pdu = getPDU(source);

   printf_debug(DEBUG_FILE, "got PDU %d (size %d)\n",
                PDU_id(pdu), PDU_size(pdu));

   if (pdu) {
      filePDU_insert(file, pdu);
      return 1;
   }

   return 0;
}

int filePDU_length(struct filePDU_t * file)
{
   return file->nombre;
}

/**
 * @brief Taille cumulée des n premières PDUs
 * @param file la file 
 * @param n le nombre (positif ou nul) de PDUs
 * @return le cumul des tailles des n premières PDUs de la file
 */
int filePDU_size_n_PDU(struct filePDU_t * file, int n)
{
   int i = 1;
   int result;
   if (n == 0) {
      return 0;
   }

   struct PDU_t * pq = file->premier;

   assert(pq);
   result = PDU_size((struct PDU_t *)PDU_private(pq));

   while (i < n) {
      pq = PDU_getNext(pq);
      assert(pq);
      result += PDU_size((struct PDU_t *)PDU_private(pq));
      i++;
   }

   printf_debug(DEBUG_FILE, "PDU - to %d : size %d\n", n, result);

   return result;
}

/**
 * @brief Taille cumulée des PDU d'une file
 */
int filePDU_size(struct filePDU_t * file)
{
  return file->size;
  //   return filePDU_size_n_PDU(file, filePDU_length(file));
}

/* WARNING : il vaudrait mieux une fonction qui donne un
 * pointeur vers la neme PDU, non ?
 */

/*
 * Taille du enieme paquet de la file (n>=1)
 */
int filePDU_size_PDU_n(struct filePDU_t * file, int n)
{
   int i = 1;
   struct PDU_t * pq = file->premier;

   assert(pq);

   while (i < n) {
      pq = PDU_getNext(pq);
      assert(pq);
      i++;
   }

   assert(PDU_private(pq));

   //   printf_debug(DEBUG_FILE, "PDU %d : size %d\n", n, PDU_size(pq->PDU));

   return PDU_size((struct PDU_t *)PDU_private(pq));
}

/*
 * Id du enieme paquet de la file (n>=1)
 */
int filePDU_id_PDU_n(struct filePDU_t * file, int n)
{
   int i = 1;
   struct PDU_t * pq = file->premier;

   assert(pq);

   while (i < n) {
      pq = PDU_getNext(pq);
      assert(pq);
      i++;
   }

   assert(PDU_private(pq));

   //   printf_debug(DEBUG_FILE, "PDU %d : id %d\n", n, PDU_id(pq->PDU));

   return PDU_id((struct PDU_t *)PDU_private(pq));
}

/*
 * Affectation d'une sonde sur les evenements d'insertion
 */
void filePDU_addInsertSizeProbe(struct filePDU_t * file, struct probe_t * insertProbe)
{
   file->insertProbe = probe_chain(insertProbe, file->insertProbe);
}

/*
 * Affectation d'une sonde sur les evenements d'extraction
 */
void filePDU_addExtractSizeProbe(struct filePDU_t * file, struct probe_t * extractProbe)
{
   probe_chain(extractProbe, file->extractProbe);
   file->extractProbe = extractProbe;
}


/*
 * AjoÃ»t d'une sonde sur la taille des PDU jetÃ©es
 */
void filePDU_addDropSizeProbe(struct filePDU_t * file, struct probe_t * dropProbe)
{
   probe_chain(dropProbe, file->dropProbe);
   file->dropProbe = dropProbe;
}

/*
 * Affectation d'une sonde sur le temps de sÃ©jour
 */
void filePDU_addSejournProbe(struct filePDU_t * file, struct probe_t * sejournProbe)
{
   probe_chain(sejournProbe, file->sejournProbe);
   file->sejournProbe = sejournProbe;
}

/**
 * @brief Add a probe on queue length (nb of PDU) on insert events
 */
void filePDU_addFileLengthProbe(struct filePDU_t * file, struct probe_t * length)
{
   file->lengthProbe = probe_chain(length, file->lengthProbe);
}


/*
 * Mesure du dÃ©bit d'entrÃ©e sur les n-1 derniÃ¨res PDUs, oÃ¹ n est le
 * nombre de PDUs prÃ©sentes. Le dÃ©bit est alors obtenu en divisant la
 * somme des tailles des n-1 derniÃ¨res PDUs par la durÃ©e entre les
 * dates d'arrivÃ©e de la premiÃ¨re et la derniÃ¨re.
 * S'il n'y a pas assez de PDUs, le rÃ©sultat est nul
 */
double filePDU_getInputThroughput(struct filePDU_t * file)
{
   double result = 0.0;

   if (file->nombre > 1) {
      // Volume reÃ§u depuis la premiÃ¨re PDU
      result = filePDU_size_n_PDU(file, filePDU_length(file)) - PDU_size(PDU_private(file->premier));
      printf_debug(DEBUG_ALWAYS, "%f de %f a %f\n", result, PDU_getCreationDate(file->premier), PDU_getCreationDate(file->dernier));

      // On divise par le temps entre la premiÃ¨re et la derniÃ¨re
      result = result/(PDU_getCreationDate(file->dernier) - PDU_getCreationDate(file->premier));
   }

   return result;
}
