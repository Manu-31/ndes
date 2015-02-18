/**
 * @file source-1.c
 * @brief Testing some basic sources
 *
 * This program creates a CBR source that generates 1 PDU per second
 * and checks a couple of basic results.
 
 */

#include <motsim.h>
#include <pdu-source.h>
#include <pdu-sink.h>
#include <probe.h>

#define NB_SAMPLES 100000
#define PERIOD 0.1
#define PDU_SIZE 12345

int main()
{
   struct PDUSink_t * sink;
   struct PDUSource_t * src;
   struct probe_t * pr;
 
   int result = 0;
 
   // Simulator initialization
   motSim_create();

   // The final sink
   sink = PDUSink_create();

   // The source
   src = PDUSource_createCBR(PERIOD, PDU_SIZE, sink, PDUSink_processPDU);

   // We put a probe on the source
   pr = probe_createExhaustive();
   PDUSource_addPDUGenerationSizeProbe(src, pr);

   // Let's start the source and run the simulation
   // We need to stop before an extra PDU is generated
   PDUSource_start(src);
   motSim_runUntil(((double)NB_SAMPLES - 0.5)*PERIOD); 

   printf("%ld ech size %lf\n", probe_nbSamples(pr), probe_mean(pr));

   // We check the number of PDU generated and the mean size
   result = result || ( probe_nbSamples(pr) != NB_SAMPLES) || (probe_mean(pr) != (double)PDU_SIZE);

   return result;
}

