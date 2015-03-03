#include <stdlib.h>    // Malloc, NULL, ...
#include <assert.h>

#include <stdio.h>     // printf, ...

#include <event-list.h>
#include <pdu.h>

#ifndef EVENTS_ARE_NDES_OBJECTS

struct eventList_t {
   int nombre;
   struct event_t * premier;
   struct event_t * dernier;
};

struct eventList_t * eventList_create()
{
   struct eventList_t * result = (struct eventList_t *) sim_malloc(sizeof(struct eventList_t));

   result->nombre = 0;
   result->premier = NULL;
   result->dernier = NULL;

   return result;
}

/**
 * @brief Add an event in an event list
 *
 * The event is prepended to the list
 */
void eventList_add(struct eventList_t * file, struct event_t * event)
{
   event->prev = NULL;
   event->next = file->premier;
   // Si ce n'est pas le seul
   if (file->premier) {
      file->premier->prev = event;
   } else { // S'il est seul, il est aussi dernier
      file->dernier = event;
   }
   file->premier = event;

   file->nombre++;
}

/**
 * @brief Append an event in an event list
 * @param list a list of event
 * @param event an event
 *
 * The event is appended to the list
 */
void eventList_append(struct eventList_t * list, struct event_t * event)
{
   event->next = NULL;
   event->prev = list->dernier;

   // Si ce n'est pas le seul
   if (list->dernier) {
      list->dernier->next = event;
   } else { // S'il est seul, il est aussi premier
      list->premier = event;
   }
   list->dernier = event;

   list->nombre++;
}


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
                      motSimDate_t date)
{
   struct event_t * precedent = file->dernier;

   printf_debug(DEBUG_EVENT, "IN\n");

   // On attribue la date à l'événement
   event_setDate(event, date);

   // On cherche sa place
   while ((precedent) && (event_getDate(precedent) > event_getDate(event))){
      precedent = precedent->prev;
   }

   // precedent est null ou represente le dernier evenement AVANT event

   // Si precedent == NULL, event est le premier
   if (precedent == NULL) {
      event->prev = NULL;
      event->next = file->premier;
      // Si ce n'est pas le seul
      if (file->premier) {
         file->premier->prev = event;
      } else { // S'il est seul, il est aussi dernier
         file->dernier = event;
      }
      file->premier = event;
   } else {
      event->next = precedent->next;
      precedent->next = event;
      event->prev = precedent;
      if (event->next) {
         event->next->prev = event;
      } else { // C'est le dernier
         file->dernier = event;
      }
   }

   file->nombre++;
}

/**
 * @brief Extract the first event from a list
 * @param list A pointer to an event list
 * @result a pointer to the first event in the list, if any. NULL  if
 * the file is empty
 */
struct event_t * eventList_extractFirst(struct eventList_t * file)
{
  struct event_t * premier = NULL;

   if (file->premier) {
      premier = file->premier;
      file->premier = premier->next;
      premier->next = NULL;
      premier->prev = NULL;
      // Si c'était le seul
      if (file->dernier == premier) {
         assert(premier->next == NULL);
	 assert(file->nombre == 1);
         file->dernier = NULL;
      } else { // Il en reste un
	 assert(file->premier != NULL);
         file->premier->prev = NULL;
      }
      file->nombre --;
   }
   return premier;
}

/*
 * Consultation (sans extraction) du prochain
 */
struct event_t * eventList_nextEvent(struct eventList_t * file)
{
   if (file->premier) {
      return file->premier;
   }

   return NULL;

}

void eventList_dump(struct eventList_t * file)
{
   struct event_t * el;

   for (el = file->premier; el != NULL; el = el->next) {
     //   for (el = file->premier; el != file->dernier; el = el->suivant) {
      printf("(%p : %6.3f) ", el, event_getDate(el));
   }
   printf("\n");
}

/**
 * @brief Run all the events in the list
 *
 * Run all the events in a list, in the ordrer of the list (dates are
 * not taken into account)
 */
void eventList_runList(struct eventList_t * file)
{
   struct event_t * el;

   printf_debug(DEBUG_EVENT, "running a list (%p)  of %d events\n", file, file->nombre);
   while ((el = eventList_extractFirst(file)) != NULL){
      event_run(el);
   }  
   printf_debug(DEBUG_EVENT, "end of list (%p)  of %d events\n", file, file->nombre);
}

/**
 * @brief Number of events in the list
 * @param file a (non NULL) pointer to an event list
 * @return Number of events in the list
 */
int eventList_getLength(struct eventList_t * file)
{
   return file->nombre;
}

#else // EVENTS_ARE_NDES_OBJECTS

/**
 * @brief Create an empty event list
 */
inline struct eventList_t * eventList_create()
{
   return ndesObjectList_create(&eventType);
}

int eventSorted(void * a, void * b)
{
   struct event_t * ea = (struct event_t *)a;
   struct event_t * eb = (struct event_t *)b;

   return (event_getDate(ea) < event_getDate(eb));
}

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
                      motSimDate_t date)
{
   printf_debug(DEBUG_EVENT, "IN\n");

   // On attribue la date à l'événement
   event_setDate(event, date);

   // On insère
   ndesObjectList_insertSortedObject(file, event_getObject(event), eventSorted);
   printf_debug(DEBUG_EVENT, "OUT\n");

}

/**
 * @brief Run all the events in the list
 *
 * Run all the events in a list, in the ordrer of the list (dates are
 * not taken into account).
 * Remember that en event is destroy after execution, so the list is
 * empty at the end of runList.
 */
void eventList_runList(struct eventList_t * list)
{
   struct event_t * event;

   printf_debug(DEBUG_EVENT, "IN\n");

   while ((event = ndesObjectList_extractFirst(list)) != NULL) {
      printf_debug(DEBUG_EVENT, "Next event ...\n");
      event_run(event);
      printf_debug(DEBUG_EVENT, " ... done\n");
   }

   printf_debug(DEBUG_EVENT, "OUT\n");
}

#endif

