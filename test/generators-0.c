/*
 * Programme d'un générateur exponentiel
 */
#include <stdlib.h>    // Malloc, NULL, exit, ...
#include <assert.h>
#include <strings.h>   // bzero, bcopy, ...
#include <stdio.h>     // printf, ...
#include <math.h>      // fabs

#include <probe.h>
#include <date-generator.h>
#include <motsim.h>

/*----------------------------------------------------------------------*/
/*                                                                      */
/*----------------------------------------------------------------------*/
int main() {
   struct dateGenerator_t * dateGenExp;
   unsigned long n;
   double l = 3.0;
   double m;

   /* Les sondes */
   struct probe_t     * iap; // Interarrivee de la source

   motSim_create(); // Pour la date courante !

   iap = probe_createExhaustive();

   dateGenExp = dateGenerator_createExp(l);
   dateGenerator_addInterArrivalProbe(dateGenExp, iap);

   for (n = 0 ; n < 10000000;n++){
      dateGenerator_nextDate(dateGenExp);
   }

   m = probe_mean(iap);
   printf("Lambda = %f : InterArrival  %f (%ld samples)\n", l, m, probe_nbSamples(iap));

   if (fabs(m-1.0/l)*l < 0.05) {
      return 0;
   } else {
      return 1;
   }
}
