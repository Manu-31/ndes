/** @file sched_drr.c
 *  @brief Un ordonnanceur Deficit Round Robin élémentaire
 *
 * Pas encore pret � �tre utilis�
 * 
 * Cette implantation se fonde aussi largement que possible sur le
 * papier qui a introduit cette technique [1].
 * 
 *   [1] M. Shreedhar, G. Varghese - "Efficient Fair Queueing using
 *   Deficit Round Robin". SIGCOMM '95.
 */
#include <limits.h>

#include <sched_drr.h>
#include <file_pdu.h>

#include <ndesObject.h>
#include <log.h>

/**
 * Chaque entr�e d'un Deficit Round Robin est caract�ris�e par un
 * certain nombre de param�tres.
 */
struct schedDRRInput_t {
   void * source ;    //!< La source elle-m�me
   getPDU_t getPDU;   //!< Sa fonction fournissant une PDU

   struct filePDU_t * file;  //!< La file dans laquelle sont plac�es
			    //les PDU de la source
   unsigned long quantum;         //!< Le quantum attribu� � chaque tour (cf [1])
   unsigned long deficitCounter ; //!< Le deficit (cf [1])
   int           nbToursSupp;     //!< Combien de tours lui faut-il
				  //!avant de pouvoir �mettre ?
   struct schedDRRInput_t * next ; //!< On chaine les sources
   struct schedDRRInput_t * prev ; //!< On chaine les sources
};

/**
 * Structure définissant notre ordonanceur
 */
struct schedDRR_t {
   declareAsNdesObject;
   void         * destination;    //!< La destination (typiquement un lien)
   processPDU_t   destProcessPDU;   //!< Fonction de r�ception de la destination

   struct schedDRRInput_t  * unactiveSourceList; //!< La liste des
						 //sources inactives
   struct schedDRRInput_t  * activeSourceList;   //!< La liste des
						 //sources actives
   struct schedDRRInput_t  * nextInput;   //!< Prochaine source � servir
};

/**
 * @brief D�finition des fonctions sp�cifiques li�es au ndesObject
 */
defineObjectFunctions(schedDRR);
struct ndesObjectType_t schedDRRType = {
  ndesObjectTypeDefaultValues(schedDRR)
};

/**
 * Création d'une instance de l'ordonnanceur avec la destination en
 * paramètre 
 * @param destination l'entité aval (un lien)
 * @param destProcessPDU la fonction de réception de la destination
 * @result la structure allouée et initialisée
 */
struct schedDRR_t * schedDRR_create(void * destination,
				  processPDU_t destProcessPDU)
{
   struct schedDRR_t * result = (struct schedDRR_t * )sim_malloc(sizeof(struct schedDRR_t));

   printf_debug(DEBUG_SCHED, "in\n");
   ndesObjectInit(result, schedDRR);

   // Gestion de la destination
   result->destination = destination; // Coucou !
   result->destProcessPDU = destProcessPDU;

   // Pas de source définie
   result->unactiveSourceList = NULL;
   result->activeSourceList = NULL;
   result->nextInput = NULL;

   printf_debug(DEBUG_SCHED, "out\n");

   return result;
}

/*
 * Ajout d'une source
 */
void schedDRR_addSource(struct schedDRR_t * sched,
			unsigned long quantum,
			void * source,
			getPDU_t getPDU)
{
   struct schedDRRInput_t  *  input = 
     (struct schedDRRInput_t  *)sim_malloc(sizeof(struct schedDRRInput_t));

   printf_debug(DEBUG_SCHED, "in\n");

   // On cr�e une structure d�finissant cette source
   input->source = source;
   input->getPDU = getPDU;

   // On cr�e une file qui stoquera ses paquets
   input->file = filePDU_create(NULL, NULL);

   input->quantum = quantum;
   input->deficitCounter = 0;
   input->nbToursSupp = 0;

   // On l'ins�re � la fin de la liste des sources inactives
   input->next = sched->unactiveSourceList;
   input->prev = NULL;
   if (input->next != NULL) {
      input->next->prev = input;
   }
   sched->unactiveSourceList = input;

   printf_debug(DEBUG_SCHED, "input %p : source %p, file %p (length %d)\n",
		input, input->source, input->file, filePDU_length(input->file));
   printf_debug(DEBUG_SCHED, "out\n");

}

/*
 * La fonction permettant de demander une PDU à notre scheduler
 * C'est ici qu'est implanté l'algorithme
 */
struct PDU_t * schedDRR_getPDU(void * s)
{
   struct schedDRR_t      * sched = (struct schedDRR_t * )s;
   struct PDU_t           * result = NULL;
   struct schedDRRInput_t * currentInput, *cINext;
   unsigned long nbToursSup = ULONG_MAX;

   printf_debug(DEBUG_SCHED, "in\n");

   // Si on n'a pas de nextInput, c'est qu'il faut d�marrer un
   // tour. La premi�re phase consiste � avancer autant que n�cessaire
   // (en faisant �ventuellement des tours � vide) pour qu'une source
   // active puisse �mettre.
   do {
      if (sched->nextInput == NULL) {
	 printf_debug(DEBUG_SCHED, "Starting a new round\n");
         // On va chercher la premi�re source active
         if (sched->activeSourceList == NULL) {
            printf_debug(DEBUG_SCHED, "No active source, aborting\n");
            return NULL; // Si pas de source active, pas de PDU � fournir !
         }
         // On parcourt la liste des sources actives jusqu'� en trouver une
         // qui puisse envoyer au moins un paquet. Le probl�me, c'est qu'il
         // sera peut-�tre n�cessaire de faire plusieurs tours afin de
         // cumuler un d�ficit suffisant pour envoyer ce paquet. Du coup on
         // compte pour chaque file active le nombre de tours dont elle a
         // besoin avant d'�mettre.
         currentInput = sched->activeSourceList;
         do {
	/* Si le d�ficit est suffisant, on peut �mettre de suite, sinon
	 * le nombre de tours � faire d�pend de la taille du prochain
	 * paquet et du quantum
	 */
	    currentInput->nbToursSupp = (currentInput->deficitCounter 
				      >= filePDU_size_n_PDU(currentInput->file, 1)) ?
	      0:((filePDU_size_n_PDU(currentInput->file, 1)-currentInput->deficitCounter-1)/currentInput->quantum+1);
            nbToursSup = min(nbToursSup, currentInput->nbToursSupp);
 	    printf_debug(DEBUG_SCHED, "current input %p (source %p, file %p) : %d PDU (first size %d), deficit %d, quantum %d, needs %d rounds\n",
			 currentInput,
			 currentInput->source, currentInput->file,
			 filePDU_length(currentInput->file),
			 filePDU_size_n_PDU(currentInput->file, 1),
			 currentInput->deficitCounter,
			 currentInput->quantum,
			currentInput->nbToursSupp );

            currentInput = currentInput->next;
         } while (currentInput);
         
        /* On connait maintenant le nombre minimal de tours � faire pour
         * que quelqu'un puisse �mettre. Le deuxi�me phase consiste donc
         * � appliquer la cons�quence de ces tours sur les d�ficits
         */
	 printf_debug(DEBUG_SCHED, "Fast forwarding %d rounds\n",
		      nbToursSup);
         currentInput = sched->activeSourceList;
         do {
            currentInput->deficitCounter += nbToursSup*currentInput->quantum;
  	    printf_debug(DEBUG_SCHED, "current input %p (source %p, file %p) : %d PDU (first size %d), deficit %d, quantum %d\n",
			 currentInput,
			 currentInput->source, currentInput->file,
			 filePDU_length(currentInput->file),
			 filePDU_size_n_PDU(currentInput->file, 1),
			 currentInput->deficitCounter,
 			 currentInput->quantum);
           currentInput = currentInput->next;
         } while (currentInput);
      
         /* Maintenant que tout le monde a obtenu le d�ficit du tour, on
          * commence effectivement le tour. C'est la troisi�me phase.
          */
         sched->nextInput = sched->activeSourceList;
      }
      printf_debug(DEBUG_SCHED, "Proceeding current round ...\n");

      // Tant qu'on n'a pas fini le tour actuel (que l'on d�bute
      // �ventuellement, si le code pr�c�dent a �t� ex�cut�), on sert la
      // prochaine source � servir
      currentInput = sched->nextInput;
      do {
         // Si la source en cours n'a pas �t� int�gralement servie, on
         // envoie le prochain de ses paquets. On ne change pas
         // n�cessairement nextInput car elle pourra �ventuellement
         // encore �tre servie au prochain tour 
         if (filePDU_size_n_PDU(currentInput->file, 1) <= currentInput->deficitCounter) {
            result = filePDU_extract(currentInput->file);
	    assert(result != NULL);
            currentInput->deficitCounter -= PDU_size(result);
            // Si c'est le dernier paquet de la source, elle n'est
            // plus active, il faut donc la sortir (avec un d�ficit
            // nul) et passer � l'�ventuelle prochaine
            if (filePDU_length(currentInput->file) == 0) {
               cINext = currentInput->next;
               currentInput->deficitCounter = 0;

	       // On met � jour la liste active (� laquelle elle appartient)
               if (currentInput->prev) {
                  currentInput->prev->next = currentInput->next;
	       } else { // Le cas de la premi�re
                  sched->activeSourceList = currentInput->next;
	       }
               if (currentInput->next) {
                  currentInput->next->prev = currentInput->prev;
	       }
               // On met � jour la liste inactive (dans laquelle elle
               // va)
	       currentInput->next = sched->unactiveSourceList;
	       sched->unactiveSourceList = currentInput;
	       currentInput->prev = NULL;
               if (currentInput->next) {
                  currentInput->next->prev = currentInput;
	       }
               // Puisque cette source n'a plus rien � donner, on
               // passe � la suivante
	       currentInput = cINext;
               printf_debug(DEBUG_SCHED, "Next input now %p ...\n", currentInput);
	    }
         } else {
            // Si la source en cours ne peut pas �mettre (pas assez cumul�
            // de d�ficit), alors on passe � la suivante
            printf_debug(DEBUG_SCHED, "Not enough deficit for input %p ...\n", currentInput);
            currentInput =  currentInput->next;
         }
      } while ((result == NULL) && (currentInput != NULL));

      // Si on a trouv� quelquechose, on est content ! On arr�te l� apr�s
      // avoir not� o� on en est dans le tour actuel
      // Si on n'a rien trouv�, ici, c'est qu'on vient de finir un tour,
      // il faut donc en entamer un nouveau
      sched->nextInput = currentInput;
   } while (result == NULL);

   printf_debug(DEBUG_SCHED, "scheduling PDU %d (size %d)\n", 
                PDU_id(result),
                PDU_size(result));

   ndesLog_logLineF(PDU_getObject(result), "OUT %d", schedDRR_getObjectId(sched));

   return result;
}

/*
 * La fonction de soumission d'un paquet à notre ordonnanceur
 */
int schedDRR_processPDU(void *s,
			getPDU_t getPDU,
			void * source)
{
   int                      result;
   struct schedDRR_t      * sched = (struct schedDRR_t *)s;
   struct schedDRRInput_t * src, *unknownSource = NULL;
   struct PDU_t           * pdu;

   printf_debug(DEBUG_SCHED, "in\n");

   // Si c'est un test de dispo, je suis pr�t !
   if ((getPDU == NULL) || (source == NULL)) {
      printf_debug(DEBUG_ALWAYS, "getPDU/source should not be NULL\n");
      printf_debug(DEBUG_SCHED, "c'etait juste un test\n");
      result = 1;
   } else {
      // On cherche la source dans la liste des sources inactives
      src = sched->unactiveSourceList;
      while (( src != NULL)  && (src->source != source) ){
         src = src->next;
      }
      assert((src == NULL)||(src->source == source));

      // Si on l'a trouv�, il faut l'extraire et la mettre dans la
      // liste des sources actives
      if ((src != NULL) && (src->source == source)) {
         // On met � jour la liste inactive (� laquelle elle appartient)
         if (src->prev) {
            src->prev->next = src->next;
         } else { // Le cas de la premi�re
            sched->unactiveSourceList = src->next;
         }
         if (src->next) {
            src->next->prev = src->prev;
         }

         // On met � jour la liste inactive (dans laquelle elle va)
         src->next = sched->activeSourceList;
         sched->activeSourceList = src;
         src->prev = NULL;
         if (src->next) {
            src->next->prev = src;
         }
      }

      // On va maintenant la chercher dans les sources actives (elle y
      // est forc�ment)
      if (src == NULL) {
         src = sched->activeSourceList;
         while (( src != NULL)  && (src->source != source) ){
            src = src->next;
         }
         assert((src == NULL)||(src->source == source));
      }

      // Si on ne l'a pas trouv�, il y a un probl�me, car elle est
      // donc inconnue !
      assert(src != unknownSource);

      // Une fois qu'on a trouv� la source (qui est n�cessairement
      // active), on prend le paquet et on le met dans la file
      // correspondante 
      pdu = src->getPDU(src->source);
      assert(pdu != NULL);
      filePDU_insert(src->file, pdu);

      // Si l'aval est dispo, on lui dit de venir chercher une PDU, ce
      // qui déclanchera l'ordonnancement
      printf_debug(DEBUG_SCHED, "on signale que la PDU %d (size %d) est dispo\n",
		PDU_id(pdu),
		PDU_size(pdu));
      result = sched->destProcessPDU(sched->destination, 
				     schedDRR_getPDU,
				     sched);
   }

   printf_debug(DEBUG_SCHED, "out %d\n", result);

   return result;
}
