/*----------------------------------------------------------------------
   Un premier exemple de base.                                        

   Nous allons cr�er une source qui est rythm�e par un g�n�rateur de 
   dates al�atoires et qui transmet des PDU vers un puits.

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
   struct PDU_sink_t      * sink;

   /* Creation du simulateur */
   motSim_create();

   /* Cr�ation d'un g�n�rateur de date */
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
