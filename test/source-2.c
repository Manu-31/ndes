/**
 * @file source-2.c
 * @brief Testing some basic sources
 *
 * This program creates a deterministic source and checks the output
 
 */

#include <motsim.h>
#include <pdu-source.h>
#include <pdu-sink.h>
#include <probe.h>
#include <file_pdu.h>

int main()
{
   struct PDUSource_t * src;
   struct probe_t * pr;
   struct filePDU_t * file;
   struct dateSize sequence[] = {
     {0.0, 000},
     {0.1, 100},
     {0.2, 200},
     {0.3, 300},
     {0.4, 400},
     {0.5, 500},
     {-1.0, 0}
   };

   int n;
   int result = 0;
 
   // Simulator initialization
   motSim_create();

   // The file
   file = filePDU_create(NULL, NULL);

   // The source
   src = PDUSource_createDeterministic(sequence, file, filePDU_processPDU);

   // We put a probe on the source
   pr = probe_createExhaustive();
   PDUSource_addPDUGenerationSizeProbe(src, pr);

   // Let's start the source and run the simulation
   PDUSource_start(src);
   motSim_runUntilTheEnd();

   printf("%ld ech size %lf\n", probe_nbSamples(pr), probe_mean(pr));

   for (n = 0; sequence[n].date >= 0; n++) {
      result = result || (sequence[n].size != filePDU_size_PDU_n(file, n+1));
   }

   return result;
}

