/**
 * @file ipv4.c
 * @brief Quelques essais d'implantation de IP
 *
 * A faire
 *   . séparer IP de ICMP
 *   . un dateGen par défaut qui envoie un ping chaque seconde
 *   . tout !
 */
#include <event.h>
#include <ipv4.h>

/*
 * @brief IPv4 header (borrowed from 
 * https://www.saminiir.com/lets-code-tcp-ip-stack-2-ipv4-icmpv4
 *
 */
struct iphdr {
   uint8_t version : 4;
   uint8_t ihl : 4;
   uint8_t tos;
   uint16_t len;
   uint16_t id;
   uint16_t flags : 3;
   uint16_t frag_offset : 13;
   uint8_t ttl;
   uint8_t proto;
   uint16_t csum;
   uint32_t saddr;
   uint32_t daddr;
} __attribute__((packed));

/*
 * @brief Définition d'une entité IPv4
 */
struct IPv4_t {
   declareAsNdesObject;  //!< C'est un ndesObject

   struct dateGenerator_t * dateGen; //!< Le générateur de date de départ
   void * destination; //!< L'objet auquel sont destinées les PDUs
   processPDU_t destProcessPDU; //!< La fonction permettant de signaler la présence de la PDU

  uint32_t address;
  struct PDU_t *pdu;  // ???
};

/*
 * @brief On utilise les fonctions par défaut
 */
struct ndesObjectType_t IPv4Type = {
  ndesObjectTypeDefaultValues(IPv4)
};

/**
 * @brief Définition des fonctions spécifiques liées au ndesObject
 */
defineObjectFunctions(IPv4);

/**
 * @brief Change the date generator
 * @param src The IPV4 to modify
 * @param gen The new date generator
 * The previos date generator should be freed by the caller
 */
void IPv4_setDateGenerator(struct IPv4_t * ipv4,
                           struct dateGenerator_t * dateGen)
{
   ipv4->dateGen = dateGen;
}

void IPv4_setDestination(struct IPv4_t * ipv4,
			 void * destination,
			 processPDU_t destProcessPDU)
{
   ipv4->destination = destination;
   ipv4->destProcessPDU = destProcessPDU;
}

/**
 * @brief création d'une entité IPv4
 *
 * Pour le moment, ce sera une source de paquets IPV4 ICMP echo
 * request puisque mon but est de l'injecté dans le système
 */
struct IPv4_t * IPv4_create(struct dateGenerator_t * dateGen,
			    uint32_t adresse,
	                    void * destination,
		            processPDU_t destProcessPDU)
{
   struct IPv4_t * result = (struct IPv4_t *)
              sim_malloc(sizeof(struct IPv4_t));

   ndesObjectInit(result, IPv4);

   // Caratéristiques IPv4
   result->address = adresse;

   // Le générateur de dates de ping. WARNING pas là !
   IPv4_setDateGenerator(result, dateGen);   

   IPv4_setDestination(result, destination, destProcessPDU);
   
   // Ajout à la liste des choses à réinitialiser avant une prochaine simu
   motsim_addToResetList(result, (void (*)(void *))IPv4_startPing);

   return result;
}


/**
 * @brief émission d'un message ICMP Echo Request
 * 
 */
void IPv4_sendNewPing(struct IPv4_t * ipv4)
{
   motSimDate_t     date;
   struct event_t * event;
   
   printf_debug(DEBUG_IPV4, "in\n");

   // Création d'un paquet IPv4

   // On prévient la destination
   if ((ipv4->destProcessPDU) && (ipv4->destination)) {
      (void)ipv4->destProcessPDU(ipv4->destination,
                                 (getPDU_t)IPv4_getNextPDU,
                                 ipv4);
   }
   
   // On détermine la date de prochaine transmission
   date = dateGenerator_nextDate(ipv4->dateGen);
   
   // On crée un événement
   event = event_create((eventAction_t)IPv4_sendNewPing, ipv4);

   // On ajoute cet événement au simulateur  pour cette date
   motSim_scheduleEvent(event, date);
   
   printf_debug(DEBUG_IPV4, "out\n");
}

/**
 * @brief Démarrage d'une transmission de paquet ICMP ER
 *
 */
void IPv4_startPing(struct IPv4_t * ipv4)
{
   // On ne sait jamais (cette fonction sert de reset)
   if (ipv4->pdu) {
      PDU_free(ipv4->pdu);
      ipv4->pdu = NULL;
   }

   // On lance la machine
   IPv4_sendNewPing(ipv4);
}

/**
 * @brief The function used by the destination to actually get the next PDU
 */
struct PDU_t * IPv4_getNextPDU(void * src)
{
   printf_debug(DEBUG_IPV4, "in\n");
   
   struct IPv4_t * source = (struct IPv4_t *)src;
   struct PDU_t  * pdu = source->pdu;

   source->pdu = NULL;

   printf_debug(DEBUG_IPV4, "out\n");
   return pdu;
}

/**
 * @brief Traitement d'une PDU
 */
int IPv4_processPDU(void * s, getPDU_t getPDU, void * source)
{
   printf_debug(DEBUG_ALWAYS, "in\n");
  
   struct IPv4_t * ipv4 = (struct IPv4_t * )s;
   struct PDU_t * pduIn;
   struct PDU_t * pduOut;
   uint32_t a;
   
   // Si c'est juste pour tester si je suis pret
   if ((getPDU == NULL) || (source == NULL)) { 
      printf_debug(DEBUG_ALWAYS, "getPDU and source should now be non NULL\n");
      return 1;
   }

   pduIn = getPDU(source);
   
   // On crée une PDU de réponse
   pduOut = PDU_create(0, PDU_private(pduIn));

   // On détruit la PDU entrante
   PDU_free(pduIn);
   
   // On va permuter les adresses source et dest
   struct iphdr * h = (struct iphdr *)PDU_private(pduOut);
   a = h->saddr;
   h->saddr = h->daddr;
   h->daddr = a;

   ipv4->pdu = pduOut;
   
   // On envoie
   (void)ipv4->destProcessPDU(ipv4->destination,
                              (getPDU_t)IPv4_getNextPDU,
                              ipv4);  
   return 0;
}
