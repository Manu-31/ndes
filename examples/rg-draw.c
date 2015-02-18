/**
 * @file rg-draw.c
 * @brief Draws the PDF of some distributions
 *
 */

#include <stdio.h>
#include <math.h>

#include <motsim.h>
#include <random-generator.h>
#include <gnuplot.h>

/*
 * Affichage (via gnuplot) de la probre pr
 * elle sera affichée comme un graphbar de nbBar barres
 * avec le nom name
 */
void draw(struct probe_t * pr, char * fileName, char * name, int nbBar, double prop)
{
   struct probe_t   * gb;
   struct gnuplot_t * gp;

   printf("Min %f, max %f, nb %ld\n", probe_min(pr), probe_max(pr), probe_nbSamples(pr));
   /* On crée une sonde de type GraphBar */
   gb = probe_createGraphBar(probe_min(pr), probe_max(pr), nbBar/prop);
   probe_setName(gb, name);

   /* On convertit la sonde passée en paramètre en GraphBar */
   probe_exhaustiveToGraphBar(pr, gb);
   probe_graphBarNormalize(gb);

   /* On initialise une section gnuplot */
   gp = gnuplot_create();

   //! Output png to filename
   gnuplot_setTerminalType(gp, gnuplotTerminalTypePng);
   gnuplot_setOutputFileName(gp, fileName);

   /* On recadre les choses */
   gnuplot_setXRange(gp, probe_min(gb), probe_max(gb)*prop);

   /* On affiche */
   gnuplot_displayProbe(gp, WITH_BOXES, gb);

   //! Destruction
   gnuplot_delete(gp);
   probe_delete(gb);
}


/**
 * Number of samples to generate
 */
#define NB_SAMPLES 100000

int main() {
   struct randomGenerator_t * rg;
   struct probe_t * pr;

   double lambda = 10.0;
   double alpha = 40.0; //!< Must be > 1
   double xmin = 2.0;
   double average;
   int n;

   motSim_create();  //!< Always needed

   rg = randomGenerator_createDouble();
   pr = probe_createExhaustive();
   randomGenerator_addValueProbe(rg, pr);

   /**
    * Defining a random generator with exponential distribution
    */  
   randomGenerator_setDistributionExp(rg, lambda);
   average = 0.0;
   for (n = 0; n < NB_SAMPLES; n++) {
      average += randomGenerator_getNextDouble(rg);
   }
   average /= NB_SAMPLES;

   draw(pr, "DistributionExp.png", "Dist. exp.", 50, 0.4);

   motSim_reset();

   /**
    * Defining a random generator with pareto distribution
    */

   randomGenerator_setDistributionPareto(rg, alpha, xmin);
   average = 0.0;
   for (n = 0; n < NB_SAMPLES; n++) {
      average += randomGenerator_getNextDouble(rg);
   }
   average /= NB_SAMPLES;

   draw(pr, "DistributionPar.png", "Dist. par.", 50, 1.0);

   motSim_reset();

   /**
    * Defining a random generator with [0-1] uniform distribution
    */
   randomGenerator_delete(rg);
   rg = randomGenerator_createDoubleRange(0.0, 1.0); // Default dist is uniform

   randomGenerator_addValueProbe(rg, pr);
   average = 0.0;
   for (n = 0; n < NB_SAMPLES; n++) {
      average += randomGenerator_getNextDouble(rg);
   }
   average /= NB_SAMPLES;

   draw(pr, "DistributionUnif.png", "Dist. unif.", 50, 1.0);

   return 0;
}
