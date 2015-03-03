/**
 * @file ndesObjectList.c
 * @brief Implantation de la gestion des files d'objets
 *
 */
#include <stdlib.h>    // Malloc, NULL, ...
#include <assert.h>

#include <ndesObjectList.h>
#include <motsim.h>

/**
 * @brief Les maillons de la files sont repr�sent�s par cette structure
 */
struct ndesObjectListElt_t {
   struct ndesObject_t        * object;   //< Pointeur sur l'�l�ment

   struct ndesObjectListElt_t * prev;
   struct ndesObjectListElt_t * next;
};

struct ndesObjectListElt_t * ndesObjectListElt_create(struct ndesObject_t * object)
{
   struct ndesObjectListElt_t * result ;

   result = (struct ndesObjectListElt_t *)sim_malloc(sizeof(struct ndesObjectListElt_t));

   result->object = object;

   return result;
};

void ndesObjectListElt_free(struct ndesObjectListElt_t * elt)
{
   free(elt);
};

/*
 * @brief Structure d�finissant une file d'objets
 */
struct ndesObjectList_t {
   struct ndesObjectType_t * type; //< Type des �l�ments
   int           nombre;   //< Nombre d'�l�ments dans la file

   /* Gestion de la file */
   struct ndesObjectListElt_t * premier;
   struct ndesObjectListElt_t * dernier;
};


/**
 * @brief Cr�ation d'une file.
 *
 * @param type permet de d�finir le type d'objets de la liste
 * @return Une struct ndesObjectList_t * allou�e et initialis�e
 *
 */
struct ndesObjectList_t * ndesObjectList_create(struct ndesObjectType_t * type)
{
   printf_debug(DEBUG_FILE, "in\n");

   struct ndesObjectList_t * result = (struct ndesObjectList_t *) sim_malloc(sizeof(struct ndesObjectList_t));
   assert(result);

   result->nombre = 0;
   result->type = type;

   result->premier = NULL;
   result->dernier = NULL;

   printf_debug(DEBUG_FILE, "out\n");

   return result;
}

/*
 * Extraction du premier élément de la file.
 *
 * Retour
 *   pointeur sur l'object qui vient d'être extrait
 *   NULL si la file était vide
 */
struct ndesObject_t * ndesObjectList_extractFirstObject(struct ndesObjectList_t * file)
{
   struct ndesObject_t * object = NULL;
   struct ndesObjectListElt_t * premier;

   printf_debug(DEBUG_FILE, " file %p extracting object (out of %d)\n", file, file->nombre);
   //  ndesObjectList_dump(file);

   if (file->premier) {
      object = file->premier->object;
      premier = file->premier;
      file->premier = premier->next;
      // Si c'était le seul
      if (file->dernier == premier) {
         assert(premier->next == NULL);
	 assert(file->nombre == 1);
         file->dernier = NULL;
      } else {
	 assert(file->premier != NULL);
         file->premier->prev = NULL;
      }
      file->nombre --;

      ndesObjectListElt_free(premier);
   }
   printf_debug(DEBUG_FILE, "out (object id %d), file length %d\n", object?ndesObject_getId(object):-1, file->nombre);
   //   ndesObjectList_dump(file);

   return object;
}

void * ndesObjectList_extractFirst(struct ndesObjectList_t * file)
{
   struct ndesObject_t * object = ndesObjectList_extractFirstObject(file);
   void * result;

   if (object) {
      result =  object->data;
   } else {
      result = NULL;
   }

   return result;
}

struct ndesObject_t * ndesObjectList_getFirstObject(struct ndesObjectList_t * file)
{
   return file->premier->object;
}

void * ndesObjectList_getFirst(struct ndesObjectList_t * file)
{
   if (file->premier) {
      return file->premier->object->data;
   } else {
      return NULL;
   }
}

/**
 * @brief Insertion d'un objet dans la file
 * @param file la file dans laquelle on ins�re l'objet
 * @param object � ins�rer � la fin de la file
 *
 */
void ndesObjectList_insertObject(struct ndesObjectList_t * file,
                                 struct ndesObject_t * object)
{
   struct ndesObjectListElt_t * pq;
 
   printf_debug(DEBUG_FILE, " file %p insert object %d (Length = %d)\n",
                file, ndesObject_getId(object),
                file->nombre);

   assert(object->type == file->type);

   //   ndesObjectList_dump(file);

   pq = ndesObjectListElt_create(object);

   pq->next = NULL;
   pq->prev = file->dernier;

   if (file->dernier)
      file->dernier->next = pq;

   file->dernier = pq;

   if (!file->premier)
      file->premier = pq;

   file->nombre++;

   printf_debug(DEBUG_FILE, " END inserting object (Length = %d)\n",
		file->nombre);
}

/**
 * @brief insert an object in a sorted list
 * 
 * sorted(a, b) must be no nul if b is to be placed after a 
 */
void ndesObjectList_insertSortedObject(struct ndesObjectList_t * file,
                                       struct ndesObject_t * object,
                                       int (*sorted)(void * a, void * b))
{
   struct ndesObjectListElt_t * precedent;
   struct ndesObjectListElt_t * pq;

   printf_debug(DEBUG_FILE, "IN  list length %d\n", file->nombre);

   // Let's start from the end of the list
   precedent = file->dernier;

   // Searching for the previous element
   while ((precedent) && (sorted(object->data, precedent->object->data))){
      precedent = precedent->prev;
   }

   pq = ndesObjectListElt_create(object);

   // Now either precedent == NULL or object must be inserted after
   // precedent
   if (precedent == NULL) { // We must insert object as first in the
			    // list
      pq->next = file->premier;
      pq->prev = NULL;

      // Si ce n'est pas le seul
      if (file->premier) {
         file->premier->prev = pq;
      } else { // S'il est seul, il est aussi dernier
         file->dernier = pq;
      }
      file->premier = pq;
   } else { // We must insert object after precedent

      // pq is after precedent
      if (precedent->next != NULL) {
         precedent->next->prev = pq;
      }
      pq->next = precedent->next;
      pq->prev = precedent;
      precedent->next = pq;
     
      // if precedent was the last
      if (file->dernier == precedent) {
         file->dernier = pq;
      }
   }
   file->nombre++;
   printf_debug(DEBUG_FILE, "OUT list length %d\n", file->nombre);

}

void ndesObjectList_insert(struct ndesObjectList_t * file,
			   void * object)
{
   printf_debug(DEBUG_FILE, "IN\n");
   assert(file->type->getObject(object)->type == file->type);

   ndesObjectList_insertObject(file, file->type->getObject(object));
   printf_debug(DEBUG_FILE, "OUT\n");

}

/**
 * @brief Insertion d'un objet en t�te de liste
 * @param list la list dans laquelle on ins�re l'objet
 * @param object � ins�rer au d�but de la liste
 *
 */
void ndesObjectList_prependObject(struct ndesObjectList_t * list,
                                  struct ndesObject_t * object)
{
   struct ndesObjectListElt_t * pq;

   printf_debug(DEBUG_FILE, "IN\n");

   pq = ndesObjectListElt_create(object);

   pq->next = list->premier;
   pq->prev = NULL;

   if (list->premier) {
      list->premier->prev = pq;
   } else { // If premier==NULL list was empty
      list->dernier = pq; 
   }
   list->premier = pq;
   list->nombre++;
   printf_debug(DEBUG_FILE, "OUT\n");
}
void ndesObjectList_prepend(struct ndesObjectList_t * list,
                            void * object)
{
   printf_debug(DEBUG_FILE, "IN\n");
   assert(list->type->getObject(object)->type == list->type);

   ndesObjectList_prependObject(list, list->type->getObject(object));
   printf_debug(DEBUG_FILE, "OUT\n");

}


/**
 * @brief Insert an object in a file after a given position
 * @param file The file in which the object must be inserted
 * @param position The object after which the object must be inserted 
 * @param object The ndesObject to insert
 * It is the responsibility of the sender to ensure that the position
 * is present in the file
 */
void ndesObjectList_insertObjectAfterObject (struct ndesObjectList_t * file,
					     struct ndesObject_t * position,
					     struct ndesObject_t * object)
{
   struct ndesObjectListElt_t * pq;
   struct ndesObjectListElt_t * pos;

   printf_debug(DEBUG_FILE, " file %p insert object %d after object %d (Length = %d)\n",
                file, ndesObject_getId(object), ndesObject_getId(position),
                file->nombre);

   assert(object != NULL);
   assert(position != NULL);
   assert(object->type == file->type);
   assert(position->type == file->type);

   //   ndesObjectList_dump(file);

   // Searching position
   for (pos = file->premier ; ((pos!=NULL)&&(pos->object != position));pos = pos->next);

   // If position not found, append
   if (pos == NULL) {
      ndesObjectList_insertObject(file, object);
   } else {
      pq = ndesObjectListElt_create(object);

      // pq is after position
      if (pos->next != NULL) {
         pos->next->prev = pq;
      }
      pq->next = pos->next;
      pq->prev = pos;
      pos->next = pq;
     
      // if position was the last
      if (file->dernier == pos)
         file->dernier = pq;
  
      file->nombre++;
   }
   printf_debug(DEBUG_FILE, " END inserting object (Length = %d)\n",
		file->nombre);
}

/**
 * @brief Insert an object in a file after a given position
 * @param file The file in which the object must be inserted
 * @param position The object after which the object must be inserted 
 * @param object The object to insert
 * It is the responsibility of the sender to ensure that the position
 * is present in the file
 */
void ndesObjectList_insertAfterObject(struct ndesObjectList_t * file,
				struct ndesObject_t * position,
				void * object)
{
   assert(file->type->getObject(object)->type == file->type);

   ndesObjectList_insertObjectAfterObject(file, position, file->type->getObject(object));
}

void ndesObjectList_insertAfter(struct ndesObjectList_t * file,
				void * position,
				void * object)
{
   assert(file->type->getObject(object)->type == file->type);
   assert(file->type->getObject(position)->type == file->type);

   ndesObjectList_insertObjectAfterObject(file,
					  file->type->getObject(position),
					  file->type->getObject(object));
}

void ndesObjectList_insertObjectAfter(struct ndesObjectList_t * file,
				      void * position,
				      struct ndesObject_t * object)
{
   assert(file->type->getObject(position)->type == file->type);

   ndesObjectList_insertObjectAfterObject(file,
					  file->type->getObject(position),
					  object);
}

int ndesObjectList_length(struct ndesObjectList_t * file)
{
   return file->nombre;
}

/*
 * Un affichage un peu moche de la file. Peut être utile dans des
 * phases de débogage.
 */
void ndesObjectList_dump(struct ndesObjectList_t * file)
{
  struct ndesObjectListElt_t * pq;

  printf("DUMP %2d elts (id:size/data) : ", file->nombre);
  for (pq=file->premier; pq != NULL; pq=pq->next){
    printf("[%d] ", ndesObject_getId(pq->object));
  }
  printf("\n");
}

/*
 * @brief Outil d'it�ration sur une liste
 */
struct ndesObjectListIterator_t {
   struct ndesObjectList_t    * ndesObjectList;
   struct ndesObjectListElt_t * position;   
};

/**
 * @brief Initialisation d'un it�rateur
 */
struct ndesObjectListIterator_t * ndesObjectList_createIterator(struct ndesObjectList_t * of)
{
   struct ndesObjectListIterator_t * ofi;

   ofi = (struct ndesObjectListIterator_t *)sim_malloc(sizeof(struct ndesObjectListIterator_t));
   ofi->ndesObjectList = of;
   ofi->position = of->premier;

   return ofi;
}

/**
 * @brief Obtention du prochain objet
 * @param ofi Pointeur sur un it�rateur
 * @result pointeur sur le prochain objet de la file, NULL si on
 * atteint la fin de la file
 */
struct ndesObject_t * ndesObjectList_iteratorGetNextObject(struct ndesObjectListIterator_t * ofi)
{
   struct ndesObject_t * result ;

   if (ofi->position) {
      result = ofi->position->object;
      ofi->position = ofi->position->next;
   } else {
      result = NULL;
   }
   return result;
}

/**
 * @brief Obtention du prochain �l�ment
 * @param ofi Pointeur sur un it�rateur
 * @result pointeur sur le prochain objet de la file, NULL si on
 * atteint la fin de la file
 */
void * ndesObjectList_iteratorGetNext(struct ndesObjectListIterator_t * ofi)
{
   struct ndesObject_t * resultObj =  ndesObjectList_iteratorGetNextObject(ofi);

   if (resultObj) {
      return ndesObject_getPrivate(resultObj);
   } else {
     return NULL;
   }
}

/*
 * @brief Terminaison de l'it�rateur
 */
void ndesObjectList_deleteIterator(struct ndesObjectListIterator_t * ofi)
{
   free(ofi);
}
