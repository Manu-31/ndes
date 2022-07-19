/**
 * @file Programme de test du multiplexeur FCFS
 * 
 * @brief Ce programme crée NBSRC sources qui sont miltiplexées puis
 * traitées par un ordonnanceur FCFS
 */
#include <stdio.h>     // printf, ...

#include <motsim.h>
#include <muxfcfs.h>
#include <pdu-sink.h>
#include <pdu-source.h>

// Le nombre de sources
#define NBSRC 10

// L'interarrivée pour chaque source
#define LAMBDA 5.0

int main()
{
   struct muxfcfs_t    * mux;
   struct PDUSink_t    * sink;
   struct PDUSource_t  * source[NBSRC];
   struct probe_t      * sinkInputProbe;

   int n;   

   /* Creation du simulateur */
   motSim_create();

   /* Le puits */
   sink = PDUSink_create();

   /* On place une sonde sur le puits */
   sinkInputProbe = probe_createExhaustive();
   PDUSink_addInputProbe(sink, sinkInputProbe);

   /* Le multiplexeur */
   mux = muxfcfs_create(sink, PDUSink_processPDU);

   /* Les sources */
   for (n = 0; n < NBSRC; n++) {
      source[n] = PDUSource_create(dateGenerator_createExp(LAMBDA),
				   mux, muxfcfs_processPDU);
   }

   /* On active les sources */
   for (n = 0; n < NBSRC; n++) {
      PDUSource_start(source[n]);
   };

   motSim_runUntil(100000.0);

   motSim_printStatus();

   printf("IA = %f\n", probe_IAMean(sinkInputProbe));
   return 0;
}

