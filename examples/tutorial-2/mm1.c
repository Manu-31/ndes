/*----------------------------------------------------------------------*/
/*   Test de NDES : simulation d'un syst�me M/M/1                       */
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
#include <random-generator.h>
#include <probe.h>
#include <gnuplot.h>


/*
 * Affichage (via gnuplot) de la probre pr
 * elle sera affich�e comme un graphbar de nbBar barres
 * avec le nom name
 */
void tracer(struct probe_t * pr, char * name, int nbBar)
{
   struct probe_t   * gb;
   struct gnuplot_t * gp;

   /* On cr�e une sonde de type GraphBar */
   gb = probe_createGraphBar(probe_min(pr), probe_max(pr), nbBar);

   /* On convertit la sonde pass�e en param�tre en GraphBar */
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

int main() {
   struct PDUSource_t       * sourcePDU;  // Une source
   struct dateGenerator_t   * dateGenExp; // Un g�n�rateur de dates
   struct randomGenerator_t * sizeGen; // Un g�n�rateur de tailles
   struct filePDU_t         * filePDU; // D�claration de notre file
   struct srvGen_t          * serveur; // D�claration d'un serveur g�n�rique
   struct PDUSink_t         * sink;    // D�claration d'un puits

   struct probe_t           * sejProbe, * iaProbe, * srvProbe, *szProbe; // Les sondes

   float frequencePaquets = 5.0;      // Nombre moyen de pq/s
   float tailleMoyenne    = 1000.0;   // Taille moyenne des pq
   float debit            = 10000.0;  // En bit par seconde

   /* Creation du simulateur */
   motSim_create();

   /* Cr�tion du puits */
   sink = PDUSink_create();

   /* Cr�ation du serveur */
   serveur = srvGen_create(sink, (processPDU_t)PDUSink_processPDU);

   /* Param�trage du serveur */
   srvGen_setServiceTime(serveur, serviceTimeProp, 1.0/debit);

   /* Cr�ation de la file */
   filePDU = filePDU_create(serveur, (processPDU_t)srvGen_processPDU);

   /* Cr�ation d'un g�n�rateur de date */
   dateGenExp = dateGenerator_createExp(frequencePaquets);

   /* Cr�ation de la source */
   sourcePDU = PDUSource_create(dateGenExp, 
				filePDU,
				(processPDU_t)filePDU_processPDU);

   /* Cr�ation d'un g�n�rateur de taille (tailles non born�es) */
   sizeGen = randomGenerator_createUInt();
   randomGenerator_setDistributionExp(sizeGen, 1.0/tailleMoyenne);

   /* Une sonde sur les tailles */
   szProbe = probe_createExhaustive();
   randomGenerator_setValueProbe(sizeGen, szProbe);

   /* Affectation � la source */
   PDUSource_setPDUSizeGenerator(sourcePDU, sizeGen);

   /* Une sonde sur les interarriv�es */
   iaProbe = probe_createExhaustive();
   dateGenerator_setInterArrivalProbe(dateGenExp, iaProbe);

   /* Une sonde sur les temps de s�jour */
   sejProbe = probe_createExhaustive();
   filePDU_addSejournProbe(filePDU, sejProbe);

   /* Une sonde sur les temps de service */
   srvProbe = probe_createExhaustive();
   srvGen_setServiceProbe(serveur, srvProbe);

   /* On active la source */
   PDUSource_start(sourcePDU);

   /* C'est parti pour 100 000 millisecondes de temps simul� */
   motSim_runUntil(100000.0);

   motSim_printStatus();

   /* Affichage de quelques r�sultats scalaires */
   printf("%d paquets restant dans  la file\n",
	  filePDU_length(filePDU));
   printf("Temps moyen de sejour dans la file = %f\n",
	  probe_mean(sejProbe));
   printf("Interarive moyenne     = %f (1/lambda = %f)\n",
	  probe_mean(iaProbe), 1.0/frequencePaquets);
   printf("Temps de service moyen = %f (1/mu     = %f)\n",
	  probe_mean(srvProbe), tailleMoyenne/debit);

   tracer(iaProbe, "Interarrivee", 100);
   tracer(sejProbe, "Temps de s�jour", 100);
   tracer(szProbe, "Taille des paquets", 100);

   printf("*** ^C pour finir ;-)\n");
   while (1) {};

   return 1;
}
