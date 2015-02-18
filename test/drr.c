/**
 * @file Test de NDES : démonstration du DRR
 *
 * Ce petit programme définit le scénario décrit dans le papier qui
 * défini le Deficit Round Robin. Le résultat est bien le même que
 * celui obtenu "à la main".
 */

#include <pdu-source.h>
#include <sched_drr.h>
#include <pdu-sink.h>
#include <srv-gen.h>
#include <log.h>

int main() {
  struct dateSize sequence1[] = {
     {0.0, 200},
     {0.0, 750},
     {0.0,  20},
     {-1.0, 0}
   };
  struct dateSize sequence2[] = {
     {0.0, 500},
     {0.0, 500},
     {-1.0, 0}
   };
  struct dateSize sequence3[] = {
     {0.0, 100},
     {0.0, 600},
     {0.0, 200},
     {-1.0, 0}
   };
  struct dateSize sequence4[] = {
     {0.0, 180},
     {0.0, 700},
     {0.0,  50},
     {-1.0, 0}
   };

   struct PDUSource_t * sourcePDU1, *sourcePDU2, * sourcePDU3, *sourcePDU4;
   struct PDUSink_t   * sink;
   struct srvGen_t    * link;
   struct schedDRR_t  * schedDRR;

   // Initialisation du simulateur
   motSim_create();

   // Le puits
   sink = PDUSink_create();

   // Le lien
   link = srvGen_create(sink, PDUSink_processPDU);

   // Le scheduler
   schedDRR = schedDRR_create(link, srvGen_processPDU);

   // Initialisation des sources
   sourcePDU1 = PDUSource_createDeterministic(sequence1,
					      schedDRR,
					      schedDRR_processPDU);
   sourcePDU2 = PDUSource_createDeterministic(sequence2,
					      schedDRR,
					      schedDRR_processPDU);
   sourcePDU3 = PDUSource_createDeterministic(sequence3,
					      schedDRR,
					      schedDRR_processPDU);
   sourcePDU4 = PDUSource_createDeterministic(sequence4,
					      schedDRR,
					      schedDRR_processPDU);
   schedDRR_addSource(schedDRR, 500, sourcePDU1, PDUSource_getPDU);
   schedDRR_addSource(schedDRR, 500, sourcePDU2, PDUSource_getPDU);
   schedDRR_addSource(schedDRR, 500, sourcePDU3, PDUSource_getPDU);
   schedDRR_addSource(schedDRR, 500, sourcePDU4, PDUSource_getPDU);

   // On active les sources pour que l'ordonnanceur les traite dans le
   // bon ordre (sur le premier round)
   PDUSource_start(sourcePDU1);
   PDUSource_start(sourcePDU4);
   PDUSource_start(sourcePDU3);
   PDUSource_start(sourcePDU2);

   motSim_runUntilTheEnd();

   motSim_printStatus();
   printf("Fini !\n");
   ndesLog_dump("fichier");

   return 1;
}
