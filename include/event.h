/**
 * @file event.h
 * @brief Définition des événements de NDES
 *
 * Un événement est décrit par une fonction, un pointeur et une
 * date. A la date choisie, la fonction est invoquée avec le pointeur
 * en guise de paramêtre.
 */
#ifndef __DEF_EVENT
#define __DEF_EVENT

#include <motsim.h>

#ifdef EVENTS_ARE_NDES_OBJECTS
#include <ndesObject.h>

struct event_t;

declareObjectFunctions(event);

struct ndesObjectType_t eventType;

#else

struct event_t {
   int    type;
   motSimDate_t period;  // Pour les événements périodiques
   motSimDate_t date;
   void * data;
   void (*run)(void * data);

   // Pour le chainage
   struct event_t * prev;
   struct event_t * next;
};

#endif

#define EVENT_PERIODIC 0x00000001

/*
 * Les mesures suivantes pourraient être faites par des sondes
 */
extern unsigned long event_nbCreate;
extern unsigned long event_nbMalloc;
extern unsigned long event_nbReuse;
extern unsigned long event_nbFree;

typedef void (*eventAction_t)(void *);

/**
 * @brief  Création d'un événement 
 * @param run La fonction à invoquer lors de l'occurence de l'événement
 * @param data Un pointeur (ou NULL) passé en paramètre à run
 * @return L'événement créé
 *
 * ATTENTION, il faut l'insérer dans la liste du simulateur, sinon
 * l'événement ne sera jamais exécuté. Pour cela, on utilisera la
 * fonction motSim_scheduleEvent.
 *
 */
struct event_t * event_create(void (*run)(void *data),
			      void * data);

/**
 * @brief read the date for an event
 * @param event a (non NULL) pointer to the event to read
 * @return the date of the event
 */
motSimDate_t event_getDate(struct event_t * event);

/**
 * @brief change the date for an event
 * @param event a pointer to the event to update
 * @param date the new date of the event
 *
 * No control is made on the date, it can be either in the past or the
 * future.
 */
void event_setDate(struct event_t * event, motSimDate_t date);

/**
 * @brief Run an event, whatever the date
 * @param event an event to run
 *
 * If the event is periodic, it is rescheduled an not destroyed.
 */
void event_run(struct event_t * event);

/**
 * @brief  Création d'un événement périodique
 * Cet événement devra être exécuté à partir de la date passée en
 * paramètre de façon périodique.
 * A chaquee date, la fonction 'run' sera invoquée avec le
 * paramêtre 'data' en paramètre.
 * ATTENTION, il faut l'insérer dans la liste du simulateur, sinon
 * l'événement ne sera jamais exécuté. Pour cela, on utilisera la
 * fonction motSim_scheduleEvent.
 *
 * @param run La fonction à invoquer lors de l'occurence de l'événement
 * @param data Un pointeur (ou NULL) passé en paramètre à run
 * @param date Date de la première occurence de l'événement
 * @param period Période d'exécution
 * @return L'événement créé
 */
struct event_t * event_periodicCreate(void (*run)(void *data),
				      void * data,
				      motSimDate_t date,
				      motSimDate_t period);

/**
 * @brief  Création et insertion d'un événement périodique
 * Cet événement devra être exécuté à partir de la date passée en
 * paramètre de façon périodique.
 * A chaquee date, la fonction 'run' sera invoquée avec le
 * paramêtre 'data' en paramètre.
 * @param run La fonction à invoquer lors de l'occurence de l'événement
 * @param data Un pointeur (ou NULL) passé en paramètre à run
 * @param date Date de la première occurence de l'événement
 * @param period Période d'exécution
 */
void event_periodicAdd(void (*run)(void *data),
		       void * data,
		       motSimDate_t date,
		       motSimDate_t period);

#endif
