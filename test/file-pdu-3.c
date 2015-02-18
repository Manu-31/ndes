/*----------------------------------------------------------------------*/
/*   Test de NDES : tests des files de PDU à longueur limitée.          */
/*   On insère 2*NBMAX PDU dans une file de capacité NBMAX. D'abord avec*/
/* une politique drop tail, puis avec une politique drop head.          */
/*----------------------------------------------------------------------*/

#include <stdlib.h>    // Malloc, NULL, exit, ...
#include <assert.h>
#include <strings.h>   // bzero, bcopy, ...
#include <stdio.h>     // printf, ...

#include <file_pdu.h>
#include <pdu-source.h>
#include <srv-gen.h>
#include <date-generator.h>

#define NBMAX 10

int main() {
   struct filePDU_t       * filePDU;
   int n;
   int result = 0;

   /* Creation du simulateur */
   motSim_create();

   /* La file */
   filePDU = filePDU_create(NULL, NULL);
   filePDU_setMaxLength(filePDU, NBMAX);

   for (n = 0 ; n < 2 * NBMAX; n++) {
     filePDU_insert(filePDU, PDU_create(0, (void *)(long)n));
      filePDU_dump(filePDU);
   }

   printf("%d PDU dans la file\n", filePDU_length(filePDU));

   result = result || (filePDU_length(filePDU) != NBMAX);

   filePDU_setDropStrategy(filePDU, filePDU_dropHead);
   filePDU_reset(filePDU);
   for (n = 0 ; n < 2 * NBMAX; n++) {
     filePDU_insert(filePDU, PDU_create(0, (void *)(long)n));
      filePDU_dump(filePDU);
   }

   printf("%d PDU dans la file\n", filePDU_length(filePDU));

   result = result || (filePDU_length(filePDU) != NBMAX);

   return result;
}
