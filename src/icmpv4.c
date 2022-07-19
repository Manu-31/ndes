/**
 * @file icmpv4.c
 * @brief Entité ICMPv4
 *
 * A faire
 *   . tout !
 */
#include <arpa/inet.h>  // ntoh
#include <stdint.h>     // uint8_t and friends
#include <string.h>     // memcpy

#include <ndesObject.h>
#include <pdu.h>
#include <icmpv4.h>

/**
 * @brief ICMPv4 header
 */
struct ICMPv4Header {
   uint8_t  type;
   uint8_t  code;
   uint16_t checksum;
   union  {
      struct {
        uint16_t id;
        uint16_t sequence;
      } echo;
      uint32_t gateway;
      struct {
        uint16_t unused;
        uint16_t mtu;
      } frag;
   } un;
};

/**
 * @brief Format d'un paquet ICMPv4
 * WARNING, à revoir, ce n'est qu'une version rapide pour structurer
 * les choses  
 */
struct ICMPv4Packet {
   struct ICMPv4Header header;
   unsigned char data[0];
};
  
/**
 * @brief Définition d'une entité ICMPv4
 */
struct ICMPv4_t {
   declareAsNdesObject;  //!< C'est un ndesObject

   processPDU_t destProcessPDU; //!< La fonction permettant de signaler la présence de la PDU

   struct dateGenerator_t * dateGenerator; //!< Pour générer des ping
					   //!(à mettre ailleurs)

   struct IPv4_t * ipv4;
};

/**
 * @brief On utilise les fonctions par défaut
 */
struct ndesObjectType_t ICMPv4Type = {
  ndesObjectTypeDefaultValues(ICMPv4)
};

/**
 * @brief Définition des fonctions spécifiques liées au ndesObject
 */
defineObjectFunctions(ICMPv4);

/**
 * @brief création d'une entité icmpv4
 * @param ipv4 l'entité à laquelle on se raccroche
 */
struct ICMPv4_t * icmpv4_create(struct IPv4_t * ipv4)
{
   struct ICMPv4_t * result = (struct ICMPv4_t *)
              sim_malloc(sizeof(struct ICMPv4_t));

   ndesObjectInit(result, ICMPv4);

   // On se rattache à une entité IP
   IPv4_setICMP(ipv4, result);
   result->ipv4 = ipv4;
   
   return result;
}

/**
 * @brief Émission d'un message
 * @param type
 * @param code
 * @param pdu contient les éventuelles données
 */
void ICMPv4_sendMessage(struct ICMPv4_t * icmpv4,
			int type, int code,
			uint32_t dstAddr,
			struct PDU_t * pduData)
{
   struct PDU_t * pdu;
   struct ICMPv4Packet * p;

   printf_debug(DEBUG_IPV4, "in\n");

   // On crée un paquet
   p = (struct ICMPv4Packet *)malloc(sizeof(struct ICMPv4Header) + PDU_size(pduData));
 
   // On initialise les champs
   p->header.type = type;
   p->header.code = code;
   
   // On copie les données s'il y en a
   memcpy(p->data, PDU_private(pduData), PDU_size(pduData));

   printf("**Message ICMP\n");
   for (int i = 0; i < sizeof(struct ICMPv4Header) + PDU_size(pduData); i++){
     printf("%2x ", ((unsigned char *)p)[i]);
     if (!((i+1)%8)) printf("\n");
   }

   // On met tout ça dans une PDU
   pdu = PDU_create(sizeof(struct ICMPv4Header) + PDU_size(pduData), p);

   // On la transmet à IP
   IPv4_sendPacket(icmpv4->ipv4, dstAddr, 1 /*WARNING */, pdu);

   printf_debug(DEBUG_IPV4, "out\n");
}

/**
 * @brief Traitement d'une PDU passée en paramètre
 *  WARNING à généraliser : on ne sait faire que répondre à un ping
 */
int ICMPv4_processThisPDU(struct ICMPv4_t * icmpv4,
			  uint32_t          srcAddr,
			  struct PDU_t    * pdu)
{
   struct ICMPv4Packet * msgRecu;    // Le message ICMP reçu
   struct ICMPv4Packet * msgReponse; // La réponse 

   struct PDU_t * pduReponse;
   
   // On récupère le message ICMPv4 qui est dans le champ private
   msgRecu = PDU_private(pdu);

   // On va pour le moment se contenter de répondre à un ping
   assert(msgRecu->header.type == ICMP_TYPE_ECHO_REQUEST);
   assert(msgRecu->header.code == 0);

   // Ici, dans l'esprit, on crée une nouvelle PDU avec comme charge
   // utile une réponse et on détruit l'ancienne. Du coup on utilise
   // plutôt les structures de données reçues en changeant ce qui doit
   // l'être (type/code echo reply)
   msgReponse = msgRecu;
   
   // C'est une réponse ICMPv4
   msgReponse->header.type = ICMP_TYPE_ECHO_REPLY;
   msgReponse->header.code = 0;

   // Calcul du checksum
   msgReponse->header.checksum = 0;
   msgReponse->header.checksum =
     IPv4_buildChecksum((unsigned short *)&(msgReponse->header),
			PDU_size(pdu));

   // On met la réponse dans une PDU
   pduReponse = PDU_create(PDU_size(pdu), msgReponse);

   // On la transmet à IP avec comme adresse destination celle de
   // l'émetteur du message reçu
   IPv4_sendPacket(icmpv4->ipv4, srcAddr, IP_PROTO_ICMP, pduReponse);

   return 0;
}

