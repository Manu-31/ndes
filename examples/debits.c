/*----------------------------------------------------------------------*/
/*    NDES : différentes méthodes d'évaluation d'un débit.              */
/*                                                                      */
/*   Nous allons rejouer plusieurs fois le même scénario (trivial) avec */
/* différentes techniques de mesure de débit puis les afficher ensemble */
/*----------------------------------------------------------------------*/

#include <stdlib.h>    // Malloc, NULL, exit, ...
#include <assert.h>
#include <strings.h>   // bzero, bcopy, ...
#include <stdio.h>     // printf, ...
#include <unistd.h>    // pause

#include <file_pdu.h>
#include <pdu-source.h>
#include <pdu-sink.h>
#include <srv-gen.h>
#include <date-generator.h>
#include <gnuplot.h>

// Taille de la fenêtre glissante en echantillons
#define NBMAX 2500

// Nombre d'écantillons sur la durée de la simu
#define NBECH 200   

int main() {
   struct PDUSource_t       * sourcePDU;
   struct dateGenerator_t   * dateGenExp;
   struct randomGenerator_t * sizeGen;
   struct probe_t           * swP, * perSwP,
                            * emaP, * perEmaP,
                            * taP;
   struct gnuplot_t         * swGp, * emaGp, * taGp;

   double                    c;
   double                    duree  = 10000.0; // Durée de la simu en secondes
   double                    alpha  = 0.99;
   double                    lambda = 50.0; // Nombre moyen de paquets/seconde
   int n;

#define nbTailles 4
   unsigned int tailles[nbTailles] = {
     128, 256, 512, 1024
   };
   double probas[nbTailles] = {
      0.25, 0.25, 0.25, 0.25
   };

   /* Creation du simulateur */
   motSim_create();

   /* Création d'un générateur de dates */
     dateGenExp = dateGenerator_createExp(lambda);

   /* Création d'un générateur de tailles */
   sizeGen = randomGenerator_createUIntDiscreteProba(nbTailles, tailles, probas);

   /* La source */
   sourcePDU = PDUSource_create(dateGenExp, NULL, NULL);
   PDUSource_setPDUSizeGenerator(sourcePDU, sizeGen);

/*------------------*/
/* Première méthode */
/*------------------*/

   /* On va utiliser une sonde de type fenêtre glissante pour mesurer
      le débit de sortie  de la source
   */
   swP = probe_slidingWindowCreate(NBMAX);
   probe_setName(swP, "Sliding Window");
   PDUSource_addPDUGenerationSizeProbe(sourcePDU, swP);

   /*
    * On va prélever NBECH echantillons de cette sonde grâce à une sonde
    * périodique.
    */
   perSwP = probe_periodicCreate(duree/NBECH);
   probe_setName(perSwP, "(periodic) Sliding Window");
   probe_addThroughputProbe(swP, perSwP);

/*------------------*/
/* Deuxième méthode */
/*------------------*/
   /* On va utiliser une moyenne mobile */
   emaP = probe_EMACreate(alpha);
   probe_setName(emaP, "EMA");
   PDUSource_addPDUGenerationSizeProbe(sourcePDU, emaP);

   /*
    * On va prélever NBECH echantillons de cette sonde grâce à une sonde
    * périodique.
    */
   perEmaP = probe_periodicCreate(duree/NBECH);
   probe_setName(perEmaP, "(periodic) EMA");
   probe_addThroughputProbe(emaP, perEmaP);

/*-------------------*/
/* Troisième méthode */
/*-------------------*/
   /* On va utiliser une moyenne temporelle */
   taP = probe_createTimeSliceThroughput(duree/NBECH);
   probe_setName(taP, "Temporal Average");
   PDUSource_addPDUGenerationSizeProbe(sourcePDU, taP);

/*---------------------*/
/* On active la source */
/*---------------------*/
   PDUSource_start(sourcePDU);

   motSim_runUntil(duree);

/*---------------*/
/* Tracé gnuplot */
/*---------------*/

   swGp = gnuplot_create();
   gnuplot_displayProbe(swGp, WITH_POINTS, perSwP);

   emaGp = gnuplot_create();
   gnuplot_displayProbe(emaGp, WITH_POINTS, perEmaP);

   taGp = gnuplot_create();
   gnuplot_displayProbe(taGp, WITH_POINTS, taP);

   gnuplot_displayProbes(taGp, WITH_POINTS, taP, perSwP, perEmaP, NULL);

   c = tailles[0]*probas[0];
   for (n = 1; n < nbTailles; n++) {
     c+= tailles[n]*probas[n];
   };

   // Les tailles sont en octets et les durées en secondes
   c = c*8.0*lambda;
   printf("Débit moyen théorique : %lf bits/s\n", c);

   printf("*** ^C pour finir ;-)\n");
   pause();

   motSim_printStatus();

   return 1;
}
