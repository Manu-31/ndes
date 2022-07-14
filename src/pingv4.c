/**
 * @file pingv4.c
 * @brief Application ping v4
 *
 * A faire
 *   . un dateGen par défaut qui envoie un ping chaque seconde
 *   . tout !
 */
#include <pingv4.h>
#include <event.h>

/**
 * @brief Définition d'un processus ping
 */
struct pingv4_t {
   declareAsNdesObject;  //!< C'est un ndesObject

   struct dateGenerator_t * dateGen; //!< Le générateur de date de départ
   processPDU_t destProcessPDU; //!< La fonction permettant de signaler la présence de la PDU

   struct ICMPv4_t * icmpv4;  //!< L'entité ICMPv4 utilisée
   uint32_t  dstAddr;
};

/**
 * @brief On utilise les fonctions par défaut
 */
struct ndesObjectType_t pingv4Type = {
  ndesObjectTypeDefaultValues(pingv4)
};

/**
 * @brief Définition des fonctions spécifiques liées au ndesObject
 */
defineObjectFunctions(pingv4);

/**
 * @brief création d'un processus pingv4
 *
 */
struct pingv4_t * pingv4_create(struct ICMPv4_t * icmpv4,
				struct dateGenerator_t * dateGen,
                                uint32_t adresseDestination)
{
   struct pingv4_t * result = (struct pingv4_t *)
              sim_malloc(sizeof(struct pingv4_t));

   ndesObjectInit(result, pingv4);

   result->icmpv4 = icmpv4;
   result->dstAddr = adresseDestination;
   pingv4_setDateGenerator(result, dateGenerator_createPeriodic(PING_PERIOD));

   return result;
}

/**
 * @brief Change the date generator
 * @param src The PDUSource to modify
 * @param gen The new date generator
 * The previos date generator should be freed by the caller
 */
void pingv4_setDateGenerator(struct pingv4_t * src,
                             struct dateGenerator_t * dateGen)
{
   src->dateGen = dateGen;
}

/**
 * @brief émission d'un message ICMP Echo Request
 * 
 */
void pingv4_sendNewPing(struct pingv4_t * pv4)
{
   motSimDate_t     date;
   struct event_t * event;
   struct PDU_t   * pdu;
   char           * data;
   
   printf_debug(DEBUG_IPV4, "in\n");

   data = (char *) malloc(ICMP_MSG_SIZE);

   for (int i = 0; i<ICMP_MSG_SIZE; i++){
      data[i] = 0x42;
   }
   
   // Création d'un message
   pdu = PDU_create(ICMP_MSG_SIZE, data);
   
   // Transmission via ICMP
   ICMPv4_sendMessage(pv4->icmpv4, ICMP_TYPE_ECHO_REQUEST, 0, pv4->dstAddr, pdu);

   // On détermine la date de prochaine transmission
   date = dateGenerator_nextDate(pv4->dateGen);
   
   // On crée un événement
   event = event_create((eventAction_t)pingv4_sendNewPing, pv4);

   // On ajoute cet événement au simulateur pour cette date
   motSim_scheduleEvent(event, date);
   
   printf_debug(DEBUG_IPV4, "out\n");
}

/**
 * @brief Démarrage d'une transmission de paquet ICMP ER
 *
 */
void pingv4_startPing(struct pingv4_t * pv4)
{
   printf_debug(DEBUG_IPV4, "in\n");

   // On lance la machine
   pingv4_sendNewPing(pv4);

   printf_debug(DEBUG_IPV4, "out\n");
}
