/*----------------------------------------------------------------------
   Un premier exemple de base.                                        

   Nous allons créer une source qui est rythmée par un générateur de 
   dates aléatoires et qui transmet des PDU vers un puits.

  ----------------------------------------------------------------------*/

#include <stdlib.h>    // Malloc, NULL, exit, ...
#include <assert.h>
#include <strings.h>   // bzero, bcopy, ...
#include <stdio.h>     // printf, ...

#include <file_pdu.h>
#include <pdu-source.h>
#include <pdu-sink.h>
#include <date-generator.h>

int main() {
   struct PDUSource_t     * sourcePDU;
   struct dateGenerator_t * dateGenExp;
   struct PDUSink_t      * sink;

   /* Creation du simulateur */
   motSim_create();

   /* Création d'un générateur de date */
   dateGenExp = dateGenerator_createExp(10.0);

   /* Le puits */
   sink = PDUSink_create();

   /* La source */
   sourcePDU = PDUSource_create(dateGenExp, sink, PDUSink_processPDU);

   /* On active la source */
   PDUSource_start(sourcePDU);

   motSim_runUntil(50.0);
   motSim_printStatus();

    
   return 1;
}
