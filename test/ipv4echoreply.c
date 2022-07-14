/**
 * @brief Demonstration de la communication des outils IPv4 avec le
 * "monde réel"
 */

#include <ipv4.h>
#include <icmpv4.h>

#include <tun-dev.h>
#include <date-generator.h>

int main() {
   struct IPv4_t            * ipv4;
   struct ICMPv4_t          * icmpv4;
   struct TUNDevice_t       * TUNDev;     // Pour injecter sur le système

   uint32_t                 add = 0x0a000001; // 10.0.0.1
   
   // Creation du simulateur 
   motSim_create();

   // Création de l'entité IPv4 (on n'a pas encore créé sa dest)
   ipv4 = IPv4_create(add,
		      NULL, NULL);

   // Création de l'entité ICMPv4
   icmpv4 = icmpv4_create(ipv4);
   
   // Création du TUNDev 
   TUNDev = TUNDevice_create(1.0, ipv4, (processPDU_t)IPv4_processPDU);

   IPv4_setDestination(ipv4, TUNDev, TUNDevice_processPDU);
   
   // On active le polling
   TUNDevice_start(TUNDev);

   /* C'est parti pour 100 000 millisecondes de temps simulé */
   //   motSim_runUntil(10000000.0);
   motSim_runUntilTheEnd();

   motSim_printStatus();
}
   
