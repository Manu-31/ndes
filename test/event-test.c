/**
 * @file event-test.c
 * @brief Test events and event-list
 */

#include <motsim.h>
#include <event-list.h>

#define NB_EVENTS 400
#define NB_LISTS  30
#define NB_ITER   10

unsigned int cpt;

/**
 * @brief Increment a value
 */
void increment(void * c)
{
   //   unsigned int * cpt = (unsigned int *) c ;
  //   printf("cpt was %d\n", cpt);
   cpt++;
}

/**
 * @brief Insert events in a list
 */
void buildEventList(void * l)
{
   int n;

   //   printf("buildEventList %p\n", l);

   struct eventList_t * el = (struct eventList_t * )l;

   for (n = 0 ; n < NB_EVENTS; n++) {
     //      printf("Add an event in %p\n", el);
      eventList_add(el, event_create(increment, &cpt));
   }
}

int main(int argc, char * argv[])
{
   int n, i;
   struct eventList_t * ef[NB_LISTS];
   cpt = 0;
   
   motSim_create();

   for (n = 0 ; n < NB_LISTS; n++) {
      ef[n] = eventList_create();
      //      printf("ef[%d] = %p\n", n, ef[n]);
      for (i = 0 ; i < NB_ITER; i++) {
         motSim_scheduleNewEvent(buildEventList, ef[n], 10.0*(double)i);
         motSim_scheduleNewEvent((void (*)(void *))eventList_runList, ef[n], 10.0*(double)i + 0.001 * (double)(n+1));
      }
   }

   motSim_runUntilTheEnd();

   motSim_printStatus();

   printf("Cpt = %d\n", cpt);

   return 0;
}
