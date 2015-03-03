#ifndef __DEF_EVENT_LIST
#define __DEF_EVENT_LIST

#include <event.h>

#ifndef EVENTS_ARE_NDES_OBJECTS

struct eventList_t;

/**
 * @brief Create an empty event list
 */
struct eventList_t * eventList_create();

/**
 * @brief Insert an event in an ordered list
 * @param list the list in which the event must be inserted
 * @param event a (non NULL) pointer to the event to insert
 * @param date the date at which the event is supposed to run
 *
 * The event is inserted after the last event in the list with an
 * earlier date.  
 */
void eventList_insert(struct eventList_t * list,
                      struct event_t * event,
                      motSimDate_t date);

/**
 * @brief Add an event in an event list
 *
 * The event is prepended to the list
 */
void eventList_add(struct eventList_t * file, struct event_t * event);

/**
 * @brief Append an event in an event list
 * @param list a list of event
 * @param event an event
 *
 * The event is appended to the list
 */
void eventList_append(struct eventList_t * list, struct event_t * event);

/**
 * @brief Extract the first event from a list
 * @param list A pointer to an event list
 * @result a pointer to the first event in the list, if any. NULL  if
 * the file is empty
 */
struct event_t * eventList_extractFirst(struct eventList_t * list);

/*
 * Consultation (sans extraction) du prochain. NULL si liste vide
 */
struct event_t * eventList_nextEvent(struct eventList_t * list);

/**
 * @brief Run all the events in the list
 *
 * Run all the events in a list, in the ordrer of the list (dates are
 * not taken into account).
 * Remember that en event is destroy after execution, so the list is
 * empty at the end of runList.
 */
void eventList_runList(struct eventList_t * list);

/**
 * @brief Number of events in the list
 * @param file a (non NULL) pointer to an event list
 * @return Nomber of events in the list
 */
int eventList_getLength(struct eventList_t * list);

#else // EVENTS_ARE_NDES_OBJECTS

#include <ndesObjectList.h>

// ndesObjectList are used as eventList
#define eventList_t ndesObjectList_t

// Some functions are simply macros
#define eventList_getLength ndesObjectList_length
#define eventList_extractFirst ndesObjectList_extractFirst
#define eventList_nextEvent ndesObjectList_getFirst
#define eventList_add ndesObjectList_prepend

/**
 * @brief Create an empty event list
 */
struct eventList_t * eventList_create();

/**
 * @brief Insert an event in an ordered list
 * @param list the list in which the event must be inserted
 * @param event a (non NULL) pointer to the event to insert
 * @param date the date at which the event is supposed to run
 *
 * The event is inserted after the last event in the list with an
 * earlier date.  
 */
void eventList_insert(struct eventList_t * file,
                      struct event_t * event,
                      motSimDate_t date);
/**
 * @brief Run all the events in the list
 *
 * Run all the events in a list, in the ordrer of the list (dates are
 * not taken into account).
 * Remember that en event is destroy after execution, so the list is
 * empty at the end of runList.
 */
void eventList_runList(struct eventList_t * list);

#endif
#endif
