/**
 * @file truncated-pareto.c
 * @brief Some basic tests for truncated pareto
 */
#include <stdlib.h>    // Malloc, NULL, exit, ...
#include <assert.h>
#include <strings.h>   // bzero, bcopy, ...
#include <stdio.h>     // printf, ...
#include <math.h>      // fabs

#include <probe.h>
#include <gnuplot.h>
#include <random-generator.h>
#include <motsim.h>

/*
 * Affichage (via gnuplot) de la probre pr
 * elle sera affichée comme un graphbar de nbBar barres
 * avec le nom name
 */
void tracer(struct probe_t * pr, char * name, int nbBar)
{
   struct probe_t   * gb;
   struct gnuplot_t * gp;

   /* On crée une sonde de type GraphBar */
   gb = probe_createGraphBar(probe_min(pr), probe_max(pr), nbBar);

   /* On convertit la sonde passée en paramètre en GraphBar */
   probe_exhaustiveToGraphBar(pr, gb);

   /* On la baptise */
   probe_setName(gb, name);

   /* On initialise une section gnuplot */
   gp = gnuplot_create();

   /* On recadre les choses */
   gnuplot_setXRange(gp, probe_min(gb), probe_max(gb)/2.0);

   /* On affiche */
   gnuplot_displayProbe(gp, WITH_BOXES, gb);
}

/*----------------------------------------------------------------------*/
/*                                                                      */
/*----------------------------------------------------------------------*/
int main() {
   struct randomGenerator_t * rg;
   struct probe_t  * pr;

   unsigned long n;
   double min = 1.0;
   double max = 10.0;
   double alpha = 2.0;

   motSim_create();

   rg = randomGenerator_createDouble();
   randomGenerator_setDistributionTruncatedPareto(rg, alpha, min, max);

   pr = probe_createExhaustive();
   randomGenerator_addValueProbe(rg, pr);

   for (n = 0 ; n < 100000000;n++){
      (void)randomGenerator_getNextDouble(rg);
   }

   printf("mean %f (%ld samples)\n", probe_mean(pr), probe_nbSamples(pr));

   tracer(pr, "TruncPareto", 100);

   printf("*** ^C pour finir ;-)\n");
   while (1) {};

   return 1;
}
