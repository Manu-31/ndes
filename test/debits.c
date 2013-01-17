/*----------------------------------------------------------------------*/
/*   Test de NDES : diff�rentes m�thodes d'�valuation d'un d�bit.       */
/*----------------------------------------------------------------------*/

#include <stdlib.h>    // Malloc, NULL, exit, ...
#include <assert.h>
#include <strings.h>   // bzero, bcopy, ...
#include <stdio.h>     // printf, ...

#include <file_pdu.h>
#include <pdu-source.h>
#include <pdu-sink.h>
#include <srv-gen.h>
#include <date-generator.h>
#include <gnuplot.h>

#define NBMAX 1000
#define NBBOUCLE 100

int main() {
   struct PDUSource_t       * sourcePDU;
   struct dateGenerator_t   * dateGenExp;
   struct randomGenerator_t * sizeGen;
   struct filePDU_t         * filePDU;
   struct probe_t           * srcOutputProbe,  // Sortie de la source
                            * sopBw;           // D�bit correspondant
   struct probe_t           * fileInputProbe;  // Entr�e de la file
   struct gnuplot_t         * gp;

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

   /* La file */
   filePDU = filePDU_create(NULL, NULL);
   filePDU_setMaxLength(filePDU, NBMAX);
   filePDU_setDropStrategy(filePDU, filePDU_dropHead);

   /* Cr�ation d'un g�n�rateur de dates */
   dateGenExp = dateGenerator_createExp(1.0);

   /* Cr�ation d'un g�n�rateur de tailles */
   sizeGen = randomGenerator_createUIntDiscrete(nbTailles, tailles, probas);

   /* La source */
   sourcePDU = PDUSource_create(dateGenExp, filePDU, filePDU_processPDU);
   PDUSource_setPDUSizeGenerator(sourcePDU, sizeGen);

   /* On va utiliser une sonde de type fen�tre glissante pour mesurer
      le d�bit de sortie  de la source
   */
   srcOutputProbe = probe_slidingWindowCreate(NBMAX);
   PDUSource_addPDUGenerationSizeProbe(sourcePDU, srcOutputProbe);
   sopBw = probe_createExhaustive();

   /* Idem en entr�e de file */
   fileInputProbe = probe_slidingWindowCreate(NBMAX);
   filePDU_addInsertSizeProbe(filePDU, fileInputProbe);

   /* On active la source */
   PDUSource_start(sourcePDU);

   /* On mesure r�guli�rement le d�bit d'entr�e de la file sur NBMAX PDUs */
   for (n = 1; n <= NBBOUCLE; n++) {
      motSim_runUntil(n*500.0);
      probe_sample(sopBw, filePDU_getInputThroughput(filePDU));
      if (probe_throughput(srcOutputProbe) != probe_throughput(fileInputProbe)) {
	printf("ERREUR : Sortie de src  : %f =/= Entree de file : %f\n", probe_throughput(srcOutputProbe), probe_throughput(fileInputProbe));
      }
   };

   /* Trac� gnuplot */
   gp = gnuplot_create();
   gnuplot_displayProbe(gp, WITH_BOXES, sopBw);
   printf("*** ^C pour finir ;-)\n");
   pause();

   motSim_printStatus();

   return 1;
}
