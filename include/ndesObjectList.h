/**
 * @file ndesObjectList.h
 * @brief Définition de la gestion des files de ndesObject
 *
 */
#ifndef __DEF_LISTE_NDES_OBJECT
#define __DEF_LISTE_NDES_OBJECT

#include <ndesObject.h>

//struct ndesObjectList_t;

/**
 *  @brief Cr�ation d'une list.
 * 
 *  @param type permet de d�finir le type d'objets de la liste
 *  @return Une struct ndesObjectList_t * allou�e et initialis�e
 *
 */
struct ndesObjectList_t * ndesObjectList_create(struct ndesObjectType_t * ndesObjectType);

/**
 * @brief Insertion d'un objet dans la list
 * @param list la list dans laquelle on ins�re l'objet
 * @param object � ins�rer � la fin de la list
 *
 */
void ndesObjectList_insert(struct ndesObjectList_t * list,
                           void * object);

/**
 * @brief Insertion d'un objet en t�te de liste
 * @param list la list dans laquelle on ins�re l'objet
 * @param object � ins�rer au d�but de la liste
 *
 */
void ndesObjectList_prependObject(struct ndesObjectList_t * list,
                                  struct ndesObject_t * object);
void ndesObjectList_prepend(struct ndesObjectList_t * list,
                            void * object);

/**
 * @brief Insertion d'un objet dans la list
 * @param list la list dans laquelle on ins�re l'objet
 * @param object ndesObject_t � ins�rer � la fin de la list
 *
 */
void ndesObjectList_insertObject(struct ndesObjectList_t * list,
                                 struct ndesObject_t * object);

/**
 * @brief Insert an object in a list after a given position
 * @param list The list in which the object must be inserted
 * @param position The object after which the object must be inserted 
 * @param object The ndesObject to insert
 * It is the responsibility of the sender to ensure that the position
 * is present in the list
 */
void ndesObjectList_insertObjectAfterObject(struct ndesObjectList_t * list,
					    struct ndesObject_t * position,
					    struct ndesObject_t * object);
/**
 * @brief Insert an object in a list after a given position
 * @param list The list in which the object must be inserted
 * @param position The object after which the object must be inserted 
 * @param object The object to insert
 * It is the responsibility of the sender to ensure that the position
 * is present in the list
 */
void ndesObjectList_insertAfterObject(struct ndesObjectList_t * list,
				      struct ndesObject_t * position,
				      void * object);
void ndesObjectList_insertAfter(struct ndesObjectList_t * list,
				void * position,
				void * object);
void ndesObjectList_insertObjectAfter(struct ndesObjectList_t * list,
				      void * position,
				      struct ndesObject_t * object);

/**
 * @brief insert an object in a sorted list
 * 
 * sorted(a, b) must be no nul if b is to be placed after a 
 */
void ndesObjectList_insertSortedObject(struct ndesObjectList_t * file,
                                       struct ndesObject_t * object,
                                       int (*sorted)(void * a, void * b));
/**
 * @brief insert an object in a sorted list
 * 
 * sorted(a, b) must be no nul if b is to be placed after a 
 */
void ndesObjectList_insertSorted(struct ndesObjectList_t * file,
                                 void * object,
                                 int (*sorted)(void * a, void * b));

/**
 * @brief Extraction d'un objet depuis la liste
 * @param list la list depuis laquelle on souhaite extraire
 * @return le premier objet de la liste ou NULL si la liste est vide
 */
struct ndesObject_t * ndesObjectList_extractFirstObject(struct ndesObjectList_t * list);
void * ndesObjectList_extractFirst(struct ndesObjectList_t * list);

/**
 * @brief Consultation d'un objet de la liste
 * @param list la list depuis laquelle on souhaite extraire
 * @return le premier objet de la liste ou NULL si la liste est vide
 *
 * L'objet n'est pas supprim� de la liste
 */
struct ndesObject_t * ndesObjectList_getFirstObject(struct ndesObjectList_t * list);
void * ndesObjectList_getFirst(struct ndesObjectList_t * list);

/**
 * @brief Extraction d'une PDU depuis la list
 * @param list la list depuis laquelle on souhaite extraire la
 * premi�re PDU
 * @return la premi�re PDU ou NULL si la list est vide
 * 
 * Ici la signature est directement compatible avec le modèle
 * d'entr�e-sortie de NDES.
 */
struct PDU_t * ndesObjectList_getPDU(void * list);

/*
 * Nombre de PDU dans la list
 */
int ndesObjectList_length(struct ndesObjectList_t * list);

/*
 * Un affichage un peu moche de la list. Peut être utile dans des
 * phases de débogage.
 */
void ndesObjectList_dump(struct ndesObjectList_t * list);

/**
 * @brief Outil d'it�ration sur une liste
 */
struct ndesObjectListIterator_t;

/**
 * @brief Initialisation d'un it�rateur
 */
struct ndesObjectListIterator_t * ndesObjectList_createIterator(struct ndesObjectList_t * of);

/**
 * @brief Obtention du prochain �l�ment
 * @param ofi Pointeur sur un it�rateur
 * @result pointeur sur le prochain objet de la list, NULL si on
 * atteint la fin de la list
 */
void * ndesObjectList_iteratorGetNext(struct ndesObjectListIterator_t * ofi);

/**
 * @brief Obtention du prochain objet 
 * @param ofi Pointeur sur un it�rateur
 * @result pointeur sur le prochain objet de la list, NULL si on
 * atteint la fin de la list
 */
struct ndesObject_t * ndesObjectList_iteratorGetNextObject(struct ndesObjectListIterator_t * ofi);

/**
 * @brief Terminaison de l'it�rateur
 */
void ndesObjectList_deleteIterator(struct ndesObjectListIterator_t * ofi);

#endif
