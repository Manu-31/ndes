/**
 * @file ndesObjectFile.h
 * @brief Définition de la gestion des files de ndesObject
 *
 */
#ifndef __DEF_LISTE_NDES_OBJECT
#define __DEF_LISTE_NDES_OBJECT

#include <ndesObject.h>
#include <probe.h>

//struct ndesObjectFile_t;

/**
 *  @brief Cr�ation d'une file.
 * 
 *  @param type permet de d�finir le type d'objets de la liste
 *  @return Une struct ndesObjectFile_t * allou�e et initialis�e
 *
 */
struct ndesObjectFile_t * ndesObjectFile_create(struct ndesObjectType_t * ndesObjectType);

/**
 * @brief Insertion d'un objet dans la file
 * @param file la file dans laquelle on ins�re l'objet
 * @param object � ins�rer � la fin de la file
 *
 */
void ndesObjectFile_insert(struct ndesObjectFile_t * file,
                           void * object);
/**
 * @brief Insertion d'un objet dans la file
 * @param file la file dans laquelle on ins�re l'objet
 * @param object ndesObject_t � ins�rer � la fin de la file
 *
 */
void ndesObjectFile_insertObject(struct ndesObjectFile_t * file,
                                 struct ndesObject_t * object);

/**
 * @brief Insert an object in a file after a given position
 * @param file The file in which the object must be inserted
 * @param position The object after which the object must be inserted 
 * @param object The ndesObject to insert
 * It is the responsibility of the sender to ensure that the position
 * is present in the file
 */
void ndesObjectFile_insertObjectAfterObject(struct ndesObjectFile_t * file,
					    struct ndesObject_t * position,
					    struct ndesObject_t * object);
/**
 * @brief Insert an object in a file after a given position
 * @param file The file in which the object must be inserted
 * @param position The object after which the object must be inserted 
 * @param object The object to insert
 * It is the responsibility of the sender to ensure that the position
 * is present in the file
 */
void ndesObjectFile_insertAfterObject(struct ndesObjectFile_t * file,
				      struct ndesObject_t * position,
				      void * object);
void ndesObjectFile_insertAfter(struct ndesObjectFile_t * file,
				void * position,
				void * object);
void ndesObjectFile_insertObjectAfter(struct ndesObjectFile_t * file,
				      void * position,
				      struct ndesObject_t * object);

/**
 * @brief Extraction d'un objet depuis la file
 * @param file la file depuis laquelle on souhaite extraire
 * @return le premier object de la file ou NULL si la file est vide
 */
struct ndesObject_t * ndesObjectFile_extract(struct ndesObjectFile_t * file);

/**
 * @brief Extraction d'une PDU depuis la file
 * @param file la file depuis laquelle on souhaite extraire la
 * premi�re PDU
 * @return la premi�re PDU ou NULL si la file est vide
 * 
 * Ici la signature est directement compatible avec le modèle
 * d'entr�e-sortie de NDES.
 */
struct PDU_t * ndesObjectFile_getPDU(void * file);

/*
 * Nombre de PDU dans la file
 */
int ndesObjectFile_length(struct ndesObjectFile_t * file);

/*
 * Un affichage un peu moche de la file. Peut être utile dans des
 * phases de débogage.
 */
void ndesObjectFile_dump(struct ndesObjectFile_t * file);

/**
 * @brief Outil d'it�ration sur une liste
 */
struct ndesObjectFileIterator_t;

/**
 * @brief Initialisation d'un it�rateur
 */
struct ndesObjectFileIterator_t * ndesObjectFile_createIterator(struct ndesObjectFile_t * of);

/**
 * @brief Obtention du prochain �l�ment
 * @param ofi Pointeur sur un it�rateur
 * @result pointeur sur le prochain objet de la file, NULL si on
 * atteint la fin de la file
 */
void * ndesObjectFile_iteratorGetNext(struct ndesObjectFileIterator_t * ofi);

/**
 * @brief Obtention du prochain objet 
 * @param ofi Pointeur sur un it�rateur
 * @result pointeur sur le prochain objet de la file, NULL si on
 * atteint la fin de la file
 */
struct ndesObject_t * ndesObjectFile_iteratorGetNextObject(struct ndesObjectFileIterator_t * ofi);

/**
 * @brief Terminaison de l'it�rateur
 */
void ndesObjectFile_deleteIterator(struct ndesObjectFileIterator_t * ofi);

#endif
