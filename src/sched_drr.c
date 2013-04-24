/** @file sched_drr.c
 *  @brief Un ordonnanceur Defincit Round Robin élémentaire
 *
 * Pas encore pret � �tre utilis�
 * 
 * Cette implantation se fonde aussi largement que possible sur le
 * papier qui a introduit cette technique [1].
 * 
 *   [1] M. Shreedhar, G. Varghese - "Efficient Fair Queueing using
 *   Deficit Round Robin". SIGCOMM '95.
 */

#include <sched_drr.h>


/**
 * Chaque entr�e d'un Deficit Round Robin est caract�ris�e par un
 * certain nombre de param�tres.
 */
struct schedDRRInput_t {
   void * source ;    //!< La source elle-m�me
   getPDU_t getPDU;   //!< Sa fonction fournissant une PDU

   struct PDUFile_t * file;  //!< La file dans laquelle sont plac�es
			    //les PDU de la source
   unsigned long quantum;         //!< Le quantum attribu� � chaque tour (cf [1])
   unsigned long deficitCounter ; //!< Le deficit (cf [1])

   int dejaServi; //!< Un bool�en pour d�terminer si on l'a d�ja servi
	          //sur le cycle en cours du round robin

   struct schedDRRInput_t * next ; //!< On chaine les sources
};

/**
 * Structure définissant notre ordonanceur
 */
struct schedDRR_t {
   struct schedDRRInput_t  * unactiveSourceList; //!< La liste des
						 //sources inactives
   struct schedDRRInput_t  * activeSourceList;   //!< La liste des
						 //sources actives
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

   // Gestion de la destination
   result->destination = destination;
   result->destProcessPDU = destProcessPDU;

   // Pas de source définie
   result->unactiveSourceList = NULL;
   result->activeSourceList = NULL;

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
   struct schedDRRInput_t  *  input = (struct schedDRRInput_t  *)sim_malloc(sizeof(struct schedDRRInput_t));
   // On cr�e une structure d�finissant cette source
   input-> = ;
   input-> = ;
   input-> = ;
   input-> = ;

   // On cr�e une file qui stoquera ses paquets
   input-> = filePDU_create(NULL, NULL);

   // On l'ins�re � la fin de la liste des sources inactives

A FAIRE
}

/*
 * La fonction permettant de demander une PDU à notre scheduler
 * C'est ici qu'est implanté l'algorithme
 */
struct PDU_t * schedDRR_getPDU(void * s)
{
   struct schedDRR_t * sched = (struct schedDRR_t * )s;
   struct PDU_t * result = NULL;
   struct schedDRRInput_t * currentSource ;


   // On parcourt la liste des sources actives jusqu'� en trouver une
   // qui puisse envoyer au moins un paquet.
   do {
      // On va chercher la premi�re source active
      if (sched->activeSourceList) {
         currentSource = sched->activeSourceList->source;
      } else {
         return NULL; // Si pas de source active, pas de PDU � fournir !
      }

      // S'il d�bute son tour de round robin, on lui ajoute un quantum
      if (!currentSource->dejaServi) {
         currentSource->deficitCounter += currentSource->quantum;
         currentSource->dejaServi = 1;
         // Afin d'�viter de faire un grand nombre de tours de round robin au
         // cours desquels on incr�mente le d�ficit des sources actives d'un
         // quantum, on va d�terminer le nombre minimal.
	 nbToursSupp = (int)trunc(filePDU_size_n_PDU(currentSource->file, 1) / currentSource->quantum);
      }

      // Tant qu'il a un paquet � �mettre de taille inf�rieure � son
      // d�ficit, on l'envoie
      if (filePDU_size_n_PDU(currentSource->file, 1) <= ) {
      } else 

      // Si la source en cours n'a plus assez de d�ficit, on la met � la
      // fin de la liste des sources actives (on passe donc � la
      // suivante)
A FAIRE
   } while (result == NULL); // On continue si on n'a rien trouv�
			     // (alors qu'il y a des sources actives)
   
   return result;
}

/*
 * La fonction de soumission d'un paquet à notre ordonnanceur
 */
int schedDRR_processPDU(void *s,
			getPDU_t getPDU,
			void * source)
{
   int result;
   struct schedDRR_t * sched = (struct schedDRR_t *)s;

   A REFAIRE ! On est toujours dispo (files int�gr�es)

   printf_debug(DEBUG_SCHED, "in\n");
   // La destination est-elle prete ?
   int destDispo = sched->destProcessPDU(sched->destination, NULL, NULL);

   printf_debug(DEBUG_SCHED, "dispo du lien aval : %d\n", destDispo);

   // Si c'est un test de dispo, je dépend de l'aval
   if ((getPDU == NULL) || (source == NULL)) {
      printf_debug(DEBUG_SCHED, "c'etait juste un test\n");
      result = destDispo;
   } else {
      if (destDispo) {
         // On cherche la source dans la liste des sources inactives
         while (   (sched->unactiveSourceList != NULL) 
		  && (sched->unactiveSourceList->source != source) ){
            sched->unactiveSourceList = sched->unactiveSourceList->next;
         }
	 assert((sched->unactiveSourceList == NULL)||(sched->unactiveSourceList->source == source));

         // Si on l'a trouv�, il faut l'extraire et la mettre dans la
         // liste des sources actives (� la fin de la liste)
         if ((sched->unactiveSourceList != NULL) && (sched->unactiveSourceList->source == source)) {
	   A FAIRE (chainage � double sens ?)
	 }

         // Si l'aval est dispo, on lui dit de venir chercher une PDU, ce
         // qui déclanchera l'ordonnancement
         printf_debug(DEBUG_SCHED, "on fait suivre ...\n");
         result = sched->destProcessPDU(sched->destination, schedDRR_getPDU, sched);
      } else {
         // On ne fait rien si l'aval (un support a priori) n'est pas pret
         printf_debug(DEBUG_SCHED, "on ne fait rien (aval pas pret) ...\n");
         result = 0;
      }
   }
   printf_debug(DEBUG_SCHED, "out %d\n", result);

   return result;
}
