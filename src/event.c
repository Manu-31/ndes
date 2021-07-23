#include <stdio.h>     // printf
#include <stdlib.h>    // Malloc, NULL, exit...
#include <assert.h>

#include <event.h>
#include <motsim.h>

#define EVENT_REUSE

#ifdef EVENTS_ARE_NDES_OBJECTS

struct event_t {
   declareAsNdesObject;
   int    type;
   motSimDate_t period;  // Pour les événements périodiques
   motSimDate_t date;
   void * data;
   void (*run)(void * data);
};

/**
 * @brief Définition des fonctions spécifiques liées au ndesObject
 */
defineObjectFunctions(event);

/**
 * @brief Les événements sont des ndesObject
 */
struct ndesObjectType_t eventType = {
   ndesObjectTypeDefaultValues(event)
};

#endif

/*
 * Une file d'événements libres
 */
#ifdef EVENT_REUSE
struct event_t * freeEvent = NULL;
#endif

unsigned long event_nbCreate = 0;
unsigned long event_nbMalloc = 0;
unsigned long event_nbReuse = 0;
unsigned long event_nbFree = 0;


struct event_t * event_create(void (*run)(void *data), void * data)
{
   struct event_t * result;

#ifndef EVENTS_ARE_NDES_OBJECTS
#   ifdef EVENT_REUSE
   if (freeEvent)  {
      result = freeEvent;
      freeEvent = result->next;
      event_nbReuse++;
   } else
#   endif
#endif
   {
      result = (struct event_t *)sim_malloc(sizeof(struct event_t));
      event_nbMalloc ++;
   }
   assert(result);
   event_nbCreate ++;

   result->type = 0;
   result->period = 0.0;

   result->run = run;
   result->data = data;
   result->date = 0.0;

#ifdef EVENTS_ARE_NDES_OBJECTS
   ndesObjectInit(result, event);
#else
   result->prev = NULL;
   result->next = NULL;
#endif

   return result;
}

struct event_t * event_periodicCreate(void (*run)(void *data), void * data, motSimDate_t date, motSimDate_t period)
{
   struct event_t * result;

   result = event_create(run, data);

   result->type = EVENT_PERIODIC;
   result->period = period;

   return result;
}

/*
 * La même, avec insersion dans le simulateur
 */
void event_periodicAdd(void (*run)(void *data), void * data, motSimDate_t date, motSimDate_t period)
{
  motSim_scheduleEvent(event_periodicCreate(run, data, date, period), date);
}

void free_event(struct event_t * ev)
{
   event_nbFree++;
#ifdef EVENTS_ARE_NDES_OBJECTS
   ndesObject_free(event_getObject(ev));
#else
#   ifdef EVENT_REUSE
   ev->next = freeEvent;
   freeEvent = ev;
#   else
   free(ev);
#   endif
#endif
}

/**
 * @brief Run the destroy an event, whatever the date
 * @param event an event to run
 *
 * If the event is periodic, it is rescheduled an not destroyed.
 */
void event_run(struct event_t * event)
{
   printf_debug(DEBUG_EVENT, " running ev %p at %f\n", event, event->date);
 
   event->run(event->data);

   if (event->type &EVENT_PERIODIC) {
      event->date += event->period;
      motSim_scheduleEvent(event, event->date);
   } else {
      free_event(event);
   }

   printf_debug(DEBUG_EVENT, " end of ev %p at %f\n", event, event->date);
}

motSimDate_t event_getDate(struct event_t * event)
{
   return event->date;
}

/**
 * @brief change the date for an event
 * @param event a pointer to the event to update
 * @param date the new date of the event
 *
 * No control is made on the date, it can be either in the past or the
 * future.
 */
void event_setDate(struct event_t * event, motSimDate_t date)
{
   event->date = date;
}

