/**
 * @file ipv4.c
 * @brief Quelques essais d'implantation de IP
 *
 * A faire
 *   . Attention on ne gère pas les options IP !!!
 *   . un dateGen par défaut qui envoie un ping chaque seconde
 *   . tout !
 */
#include <arpa/inet.h>  // ntoh

#include <event.h>

#include <ipv4.h>

#include <icmpv4.h>

/**
 * @brief IPv4 header (borrowed from 
 * https://www.saminiir.com/lets-code-tcp-ip-stack-2-ipv4-icmpv4
 *
 */
struct iphdr {
#if __BYTE_ORDER == __LITTLE_ENDIAN
   uint8_t ihl : 4;
   uint8_t version : 4;
#else
   uint8_t version : 4;
   uint8_t ihl : 4;
#endif
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

/**
 * @brief Format d'un paquet ICMPv4
 * WARNING, à revoir, ce n'est qu'une version rapide pour structurer
 * les choses  
 */
struct IPv4Packet {
   struct iphdr header;
   unsigned char data[0];
};

/**
 * @brief Affichage d'un paquet IPv4
 */
inline static void IPv4Packet_print(struct IPv4Packet* p)
{
   printf_debug(DEBUG_IPV4, "--------<paquet ipv4>---------\n");
   printf_debug(DEBUG_IPV4, "v=%d, hl=%d, tos=%d, lg=%d\n",
	  p->header.version, p->header.ihl, p->header.tos, ntohs(p->header.len));
   printf_debug(DEBUG_IPV4, "ttl=%d, proto=%d\n", p->header.ttl, p->header.proto);
   printf_debug(DEBUG_IPV4, "sa=%d.%d.%d.%d  ",
	  (ntohl(p->header.saddr)>>24)&0xff,
	  (ntohl(p->header.saddr)>>16)&0xff,
	  (ntohl(p->header.saddr)>> 8)&0xff,
	  (ntohl(p->header.saddr)    )&0xff
	  );
   printf_debug(DEBUG_IPV4, "da=%d.%d.%d.%d\n",
	  (ntohl(p->header.daddr)>>24)&0xff,
	  (ntohl(p->header.daddr)>>16)&0xff,
	  (ntohl(p->header.daddr)>> 8)&0xff,
	  (ntohl(p->header.daddr)    )&0xff
	  );
   printf_debug(DEBUG_IPV4, "csum = 0x%x\n", p->header.csum);
   printf_debug(DEBUG_IPV4, "------------------------------\n");
}

/**
 * @brief Définition d'une entité IPv4
 */
struct IPv4_t {
   declareAsNdesObject;  //!< C'est un ndesObject

   void * destination; //!< L'objet auquel sont destinées les PDUs
   processPDU_t destProcessPDU; //!< La fonction permettant de signaler la présence de la P     
   struct ICMPv4_t * icmpv4; //!< L'entité ICMP

   uint32_t address;
   struct PDU_t *pdu;  // ???
};

/**
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
 * @brief création d'une entité IPv4
 *
 */
struct IPv4_t * IPv4_create(uint32_t adresse,
	                    void * destination,
		            processPDU_t destProcessPDU)
{
   struct IPv4_t * result = (struct IPv4_t *)
              sim_malloc(sizeof(struct IPv4_t));

   ndesObjectInit(result, IPv4);

   IPv4_setDestination(result, destination, destProcessPDU);

   // Caratéristiques IPv4
   result->address = adresse;
   
   // Ajout à la liste des choses à réinitialiser avant une prochaine simu
   //   motsim_addToResetList(result, (void (*)(void *))IPv4_startPing);

   return result;
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
 * Code repris de RFC1071, section 4.1
 */
unsigned short IPv4_buildChecksum(unsigned short * addr, short count)
{
   unsigned long sum = 0;

   while( count > 1 )  {
      /*  This is the inner loop */
      sum += * addr++;
      count -= 2;
   }

   /*  Add left-over byte, if any */
   if( count > 0 )
       sum += * (unsigned char *) addr;

      /*  Fold 32-bit sum to 16 bits */
   while (sum>>16)
      sum = (sum & 0xffff) + (sum >> 16);

   return (unsigned short) ~sum;     
}

/**
 * @brief Traitement d'une PDU
 */
int IPv4_processPDU(void * s, getPDU_t getPDU, void * source)
{
   printf_debug(DEBUG_IPV4, "in\n");
  
   struct IPv4_t     * ipv4 = (struct IPv4_t * )s;
   struct PDU_t      * pduIn;
   struct PDU_t      * pdu;   // Celle qu'on va fournir à la couche du dessus
   struct IPv4Packet * packet;
   void              * payload;
   //   uint32_t a;
   uint32_t            srcAddr;
   
   // Si c'est juste pour tester si je suis pret
   if ((getPDU == NULL) || (source == NULL)) { 
      printf_debug(DEBUG_IPV4, "getPDU and source should now be non NULL\n");
      return 1;
   }

   // On récupère la PDU sur la source
   printf_debug(DEBUG_IPV4, "On va chercher\n");
   pduIn = getPDU(source);

   // Puisque c'est un paquet IPv4, le champ private est un struct IPv4Packet *
   packet = (struct IPv4Packet *)PDU_private(pduIn);

   IPv4Packet_print(packet);

   // On peut déterminer l'adresse IP source
   srcAddr = ntohl(packet->header.saddr);
   
   // On crée une nouvelle PDU avec le contenu. Pas très
   // efficace, mais tellement plus simple. Un jour il faudra
   // faire comme les socket buffer de Linux.
   // WARNING la taille ne prend pas en compte les options IP !!
   payload = malloc(PDU_size(pduIn) - sizeof(struct iphdr));
   memcpy(payload, &(packet->data[0]), PDU_size(pduIn) - sizeof(struct iphdr));
   pdu = PDU_create(PDU_size(pduIn) - sizeof(struct iphdr), payload);

   // On a donc maintenant pdu dont le champ private pointe sur le contenu 
   // du paquet IPv4 d'origine. C'est cette PDU qu'on va passer à la couche
   // supérieure. On n'a donc plus besoin de la PDU d'origine
   PDU_free(pduIn);
   
   // On aiguille en fonction du protocole. Comme on a du récupérer la
   // PDU pour déterminer le protocole, on ne peut pas utiliser les
   // méthodes classiques.
   switch (packet->header.proto) {
      case IP_PROTO_ICMP :
         printf_debug(DEBUG_IPV4, "sending proto %d to icmpv4\n", packet->header.proto);
         return ICMPv4_processThisPDU(ipv4->icmpv4, srcAddr, pdu);
      break;
      default :
         printf_debug(DEBUG_IPV4, "protocole %d inconnu\n", packet->header.proto);
         PDU_free(pduIn);
	 return 1;
      break;
   }
   
   printf_debug(DEBUG_IPV4, "out\n");
   
   return 1;
}

/**
 * @brief Définition de l'entité ICMP
 */
void IPv4_setICMP(struct IPv4_t * ipv4, struct ICMPv4_t * icmpv4)
{
  ipv4->icmpv4 = icmpv4;
}

/**
 * @brief Send a packet
 * @param proto
 * 
 * C'est la fonction que doivent utiliser les entités de la couche
 * supérieure pour transmettre. 
 */
void IPv4_sendPacket(struct IPv4_t * ipv4,
		     uint32_t        destAddr,
		     uint8_t         proto,
		     struct PDU_t  * payload)
{
   struct IPv4Packet * p = (struct IPv4Packet *)malloc(sizeof(struct iphdr)+PDU_size(payload));

   printf_debug(DEBUG_IPV4, "in\n");

   // Initialisation des champs
   p->header.version = 4;
   p->header.ihl= 5;
   p->header.proto = proto;
   p->header.saddr = htonl(ipv4->address);
   p->header.daddr = htonl(destAddr);
   p->header.len = htons(sizeof(struct iphdr)+PDU_size(payload));
   p->header.ttl = htons(42);

   // Calcul du checksum
   p->header.csum = 0;
   p->header.csum = IPv4_buildChecksum((unsigned short *)&(p->header),
				       4*p->header.ihl);
   
   // Copie de la charge utile
   memcpy(&(p->data), PDU_private(payload), PDU_size(payload));

   IPv4Packet_print(p);
   
   // La PDU est prête, ...
   ipv4->pdu = PDU_create(sizeof(struct iphdr) + PDU_size(payload), p);
   
   // ... on prévient la destination
   if ((ipv4->destProcessPDU) && (ipv4->destination)) {
      (void)ipv4->destProcessPDU(ipv4->destination,
                                 (getPDU_t)IPv4_getNextPDU,
                                 ipv4);
   }
   printf_debug(DEBUG_IPV4, "out\n");
   
}

void IPv4_setDestination(struct IPv4_t * ipv4,
			 void * destination,
			 processPDU_t destProcessPDU)
{
   ipv4->destProcessPDU = destProcessPDU;
   ipv4->destination = destination;
}
