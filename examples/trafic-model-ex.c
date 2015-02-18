/**
 * @file : trafic-model-ex.c
 * @brief : A basic example for trafic source model
 *
 */
#include <motsim.h>
#include <pdu-sink.h>
#include <pdu-source.h>
#include <random-generator.h>
#include <date-generator.h>
#include <probe.h>

int main(){
   struct PDUSource_t       * sourcePDU; //!< The trafic source
   struct dateGenerator_t   * dateGen;   //!< The date generator for the source
   struct randomGenerator_t * IARandGen; //!< The random generator for the source date generator
   struct randomGenerator_t * PSRandGen; //!< Packet size generator for the source
   struct PDUSink_t         * sink;      //!< The trafic is directed to a sink
   struct probe_t           * tpProbe;   //!< To evaluate the throughput

#define nbSizes 4
   unsigned int sizes[nbSizes] = {
     128, 256, 512, 1024
   };
   double probas[nbSizes] = {
      0.25, 0.25, 0.25, 0.25
   };

   double lambda = 10.0;  //!< Source rate

   // Let's create the simulator
   motSim_create();

   // Creation/initialisation of the sink
   sink = PDUSink_create();

   // 
   dateGen = dateGenerator_createExp(lambda);

   // We create the source and connect it to the sink
   sourcePDU = PDUSource_create(NULL,
				sink,
			        (processPDU_t)PDUSink_processPDU);

   // Now, we want to define the packet creation process
   dateGen =  dateGenerator_create();

   // Initialisation of the interarrival random generator
   IARandGen = randomGenerator_createDoubleExp(lambda);

   // The date generator is characterised by the interarrival random
   // process
   dateGenerator_setRandomGenerator(dateGen, IARandGen);

   // Creation of the packet size generator
   PSRandGen = randomGenerator_createUIntDiscreteProba(nbSizes, sizes, probas);

   // Set the packet date generator
   PDUSource_setDateGenerator(sourcePDU, dateGen);

   // Set the packet size generator
   PDUSource_setPDUSizeGenerator(sourcePDU, PSRandGen);

   // Just for fun, we will measure the throughput
   tpProbe = probe_createMean();
   PDUSource_addPDUGenerationSizeProbe(sourcePDU, tpProbe);

   // Starting the source
   PDUSource_start(sourcePDU);

   // Run 100 seconds of simulation
   motSim_runUntil(100000.0);

   printf("Throughput = %f\n", probe_throughput(tpProbe));
   return 0;
}
