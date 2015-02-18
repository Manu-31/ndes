/*----------------------------------------------------------------------*/
/*   Un troisième exemple simple.                                       */
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

int main() {
   struct PDUSource_t     * sourcePDU;
   struct dateGenerator_t * dateGenExp;
   struct filePDU_t       * filePDU;
   struct srvGen_t        * serveur;
   struct PDUSink_t      * sink;

   /* Creation du simulateur */
   motSim_create();

   /* Création d'un générateur de date */
   dateGenExp = dateGenerator_createExp(10.0);

   /* Le puits */
   sink = PDUSink_create();

   /* Le serveur */
   serveur = srvGen_create(sink, PDUSink_processPDU);

   /* La file */
   filePDU = filePDU_create(serveur, srvGen_processPDU);

   /* La source */
   sourcePDU = PDUSource_create(dateGenExp, filePDU, filePDU_processPDU);

   /* On active la source */
   PDUSource_start(sourcePDU);

   motSim_runUntil(50.0);
   motSim_printStatus();

   return 1;
}
