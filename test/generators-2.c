/*
 * Programme de test des divers générateurs
 *
 * generators-2
 *
 */
#include <stdlib.h>    // Malloc, NULL, exit, ...
#include <assert.h>
#include <strings.h>   // bzero, bcopy, ...
#include <stdio.h>     // printf, ...

#include <file_pdu.h>
#include <pdu-source.h>
#include <pdu-sink.h>
#include <date-generator.h>

#define NBECH 10000000

/*----------------------------------------------------------------------*/
/*                                                                      */
/*----------------------------------------------------------------------*/
int main() {
   struct dateGenerator_t * dateGenExp;
   int n;
   double d = 0.0;
   double l = 3.0;

   /* Les sondes */
   struct probe_t     * iap; // Interarrivee de la source

   motSim_create(); // Pour la date courante !

   iap = probe_createExhaustive();

   dateGenExp = dateGenerator_createExp(l);
   dateGenerator_addInterArrivalProbe(dateGenExp, iap);

   for (n = 0 ; n < NBECH;n++){
      d = dateGenerator_nextDate(dateGenExp);
   }
   printf("Lambda = %f : InterArrival  %f (%ld samples)\n", l, probe_mean(iap), probe_nbSamples(iap));

    
   return 1;
}
