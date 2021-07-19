/*
 * 
 */

#include <ipv4.h>
#include <tun-dev.h>
#include <date-generator.h>

int main() {
   struct IPv4_t          * ipv4;
   struct dateGenerator_t * dateGenExp; // Un générateur de dates
   struct TUNDevice_t     * TUNDev;     // Pour injecter sur le système

   uint32_t                 add = 0xc0a81f1f;
   float   lambda = 5.0 ; // Intensité du processus d'arrivée

   // Creation du simulateur 
   motSim_create();

   /* Création d'un générateur de date */
   dateGenExp = dateGenerator_createExp(lambda);

   /* Création de la source */
   ipv4 = IPv4_create(dateGenExp,
		      add,
		      NULL, NULL);

   // Création du TUNDev 
   TUNDev = TUNDevice_create(1000.0, ipv4, (processPDU_t)IPv4_processPDU);

   IPv4_setDestination(ipv4, TUNDev, TUNDevice_processPDU);
   
   // On active la source 
   //IPv4_startPing(ipv4);

   // On active le polling
   TUNDevice_start(TUNDev);

   /* C'est parti pour 100 000 millisecondes de temps simulé */
   //   motSim_runUntil(10000000.0);
   motSim_runUntilTheEnd();

   motSim_printStatus();
}
   
