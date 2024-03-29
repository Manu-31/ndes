/**
 * @file tun-dev.c
 * @brief Implantation d'un TUN device
 *
 */

#include <unistd.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <net/if.h>
#include <linux/if_tun.h>

#include <tun-dev.h>
#include <event.h>

/*
 * Pointeur, taille, message
 */
#define DUMP_PACKET(p, t, m)                   \
   printf_debug(DEBUG_TUN, "<< DUMP %d bytes (%s) >>\n", t, m);   \
   for (int i = 0; i < t; i++){                \
      printf_debug(DEBUG_TUN, "%2x ", ((unsigned char *)p)[i]); \
      if (!((i+1)%8)) printf_debug(DEBUG_TUN, "\n");            \
   }                                           \


/**
 * @brief Structure d'un TUN device
 */
struct TUNDevice_t {
   declareAsNdesObject;  //!< C'est un ndesObject

   void * destination; //!< L'objet auquel sont destinées les PDUs
   processPDU_t destProcessPDU; //!< La fonction permettant de signaler la présence de la PDU

   struct PDU_t *pdu;  //!< La dernière PDU entrante
   struct dateGenerator_t * dateGen; //!< Le générateur de date de départ
  
   char deviceName[IFNAMSIZ]; 
   int  fd;
};

/**
 * @brief Les TUN devices sont des ndesObject
 */
struct ndesObjectType_t TUNDeviceType = {
  ndesObjectTypeDefaultValues(TUNDevice)
};

/**
 * @brief Définition des fonctions spécifiques liées au ndesObject
 */
defineObjectFunctions(TUNDevice);

/**
 * @brief Création d'un TUNDevice
 * @param period période de scrutation des paquets
 *
 * Il peut y avoir une destination à laquelle sont transmis les
 * paquets. Ce sera probablement une entité IPv4.
 */
struct TUNDevice_t * TUNDevice_create(double period,
				      void * destination,
				      processPDU_t destProcessPDU)
{
   struct TUNDevice_t * result = (struct TUNDevice_t *)sim_malloc(sizeof(struct TUNDevice_t));

   struct ifreq ifr;
   int err;

   // Gestion des objets
   ndesObjectInit(result, TUNDevice);

   // On a besoin d'aller scruter régulièrement
   TUNDevice_setDateGenerator(result, dateGenerator_createPeriodic(period));

   // On se branche sur la destination
   result->destProcessPDU = destProcessPDU;
   result->destination = destination;

   // Ajout à la liste des choses à réinitialiser avant une prochaine simu
   motsim_addToResetList(result, (void (*)(void *))TUNDevice_start);
   
   // On essaye d'ouvrir le fichier
   if ((result->fd = open("/dev/net/tun", O_RDWR)) < 0) {
     printf("Foirage !\n");
     exit(1);
   }

   memset(&ifr, 0, sizeof(ifr));
   ifr.ifr_flags = IFF_TUN | IFF_NO_PI;

   if ((err = ioctl(result->fd, TUNSETIFF, (void *)&ifr)) < 0) {
      close(result->fd);
      printf("Plantage !\n");
      exit(1);
   }
   strcpy(result->deviceName, ifr.ifr_name);      

   printf(" device : [%s]\n", result->deviceName);
   return result;
}

/**
 * @brief consommation du prochain paquet entrant
 * 
 */
struct PDU_t * TUNDevice_getNextPacket(struct TUNDevice_t * td)
{
   printf_debug(DEBUG_TUN, "in\n");
   struct PDU_t * result = td->pdu;

   td->pdu = NULL;

   printf_debug(DEBUG_TUN, "out\n");
   return result;
}

/**
 * @brief consommation du prochain paquet entrant
 * 
 */
void TUNDevice_poll(struct TUNDevice_t * td)
{
#define BUFFLEN 4096
  
   motSimDate_t     date;
   struct event_t * event;
   ssize_t          bytesRead ;
   void           * buffer;
   
   printf_debug(DEBUG_IPV4, "in\n");

   // On va chercher un paquet s'il y en a un
   buffer = malloc(BUFFLEN); // WARNING utiliser les primitives d'alloc
   bytesRead = read(td->fd, buffer, BUFFLEN);

   if (bytesRead > 0) {
      printf_debug(DEBUG_TUN, "Paquet de %ld octets ...\n", bytesRead);
      DUMP_PACKET(buffer, (int)bytesRead, __FUNCTION__);
   }
   
   // On construit une PDU qui contient le paquet
   if (bytesRead > 0) {
      td->pdu = PDU_create(bytesRead, buffer);
   } else {
      free(buffer);  // WARNING utiliser les primitices d'alloc
   }
   
   // On prévient la destination s'il y a un paquet
   // WARNING : il faudrait choisir le destinataire en fonction
   // du protocole (IPv4, IPv6, ...)
   if ((td->pdu) && (td->destProcessPDU) && (td->destination)) {
      printf_debug(DEBUG_TUN, "On transmet !\n");
      (void)td->destProcessPDU(td->destination,
			       (getPDU_t)TUNDevice_getNextPacket,
			       td);
   }
   
   // On détermine la date de prochaine scrutation
   date = dateGenerator_nextDate(td->dateGen);
   
   // On crée un événement
   event = event_create((eventAction_t)TUNDevice_poll, td);

   // On ajoute cet événement au simulateur pour cette date
   motSim_scheduleEvent(event, date);
   
   printf_debug(DEBUG_IPV4, "out\n");
}


/**
 * @brief Traitement d'une PDU
 */
int TUNDevice_processPDU(void * s, getPDU_t getPDU, void * source)
{
   printf_debug(DEBUG_IPV4, "in\n");
   
   struct TUNDevice_t * TUNDev = (struct TUNDevice_t * )s;
   struct PDU_t * pdu = getPDU(source);

   // Si c'est juste pour tester si je suis pret
   if ((getPDU == NULL) || (source == NULL)) { 
      printf_debug(DEBUG_TUN, "getPDU and source should now be non NULL\n");
      return 1;
   }

   DUMP_PACKET(PDU_private(pdu), PDU_size(pdu), "Paquet IP");

   // WARNING : gros bricolage !
   write(TUNDev->fd, PDU_private(pdu), PDU_size(pdu));

   printf_debug(DEBUG_IPV4, "out\n");

   return 0; // WARNING à vérifier
}

/**
 * @brief Change the date generator
 * @param src The tun device to modify
 * @param gen The new date generator
 * The previos date generator should be freed by the caller
 */
void TUNDevice_setDateGenerator(struct TUNDevice_t * src,
                                struct dateGenerator_t * dateGen)
{
   src->dateGen = dateGen;
}

/**
 * @brief Démarrage d'une interface
 *
 * Le mode de fonctionnement actuel : on va régulièrement voir s'il y
 * a des paquets en attente. C'est pas top, il faudrait de
 * l'événementiel, soit guidé par le monde réel, soit par la simu.
 */
void TUNDevice_start(struct TUNDevice_t * tunDev)
{
   // On ne sait jamais (cette fonction sert de reset)
   if (tunDev->pdu) {
      PDU_free(tunDev->pdu);
      tunDev->pdu = NULL;
   }
   
   // On lance la machine
   TUNDevice_poll(tunDev);
}

