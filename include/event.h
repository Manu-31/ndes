#ifndef __DEF_EVENT
#define __DEF_EVENT

#include <motsim.h>

struct event_t {
   int    type;
   double period;  // Pour les �v�nements p�riodiques
   double date;
   void * data;
   void (*run)(void * data);

   // Pour le chainage
   struct event_t * prev;
   struct event_t * next;
};

#define EVENT_PERIODIC 0x00000001

/*
 * Les mesures suivantes pourraient �tre faites par des sondes
 */
extern unsigned long event_nbCreate;
extern unsigned long event_nbMalloc;
extern unsigned long event_nbReuse;
extern unsigned long event_nbFree;

typedef void (*eventAction_t)(void *);

/*
 * Cr�ation d'un �v�nement qui devra �tre ex�cut� � la date pass�e en
 * param�tre. A cette date, la fonction 'run' sera invoqu�e avec le
 * param�tre 'data' en param�tre.
 * ATTENTION, il faut l'ins�rer dans la liste du simulateur
 */
struct event_t * event_create(void (*run)(void *data), void * data, double date);

/*
 * La m�me, avec insersion dans le simulateur
 */
void event_add(void (*run)(void *data), void * data, double date);

/*
 * Cr�ation d'un �v�nement p�riodique
 */
struct event_t * event_periodicCreate(void (*run)(void *data), void * data, double date, double period);

/*
 * La m�me, avec insersion dans le simulateur
 */
void event_periodicAdd(void (*run)(void *data), void * data, double date, double period);

double event_getDate(struct event_t * event);

void event_run(struct event_t * event);

#endif
