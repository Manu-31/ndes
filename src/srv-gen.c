/**
 * @file srv-gen.c
 * @brief Un serveur générique
 *
 * Un serveur generique. Il prend des PDU en entree, les sert
 * en un temps defini par une loi (cte, exp, ...) et les envoie
 * en sortie.
 */
#include <assert.h>

#include <motsim.h>
#include <event.h>
#include <date-generator.h>
#include <srv-gen.h>
#include <ndesObject.h>
#include <log.h>

#include <stdlib.h>    // Malloc, NULL, exit...

enum srvState_t {
  srvStateIdle,
  srvStateBusy
};

/** 
 * @brief Définition d'un serveur générique
 */
struct srvGen_t {
   declareAsNdesObject; //< C'est un ndesObject 

   enum srvState_t          srvState;
   struct PDU_t           * currentPDU;   // The PDU being served
   float                    serviceStartTime;

   // Dealing with service time
   // C'est un peu de la merde parceque mes dates ne savent pas 
   // gérer un débit, ...
   struct dateGenerator_t * dateGenerator;
   enum serviceTime_t       serviceTime;
   double                   serviceTimeParameter;

   // Gestion du destinataire
   void * destination;          // L'objet auquel sont destinées les PDUs
   processPDU_t destProcessPDU; // La fonction permettant d'envoyer la PDU   

   // WARNING : les attributs suivants ne nous permettent pas de
   // préparer la réception de plusieurs PDU !!
   void * source ;  // L'objet susceptible de nous fournir une PDU
   getPDU_t getPDU; // La méthode d'obtention  de cette PDU

   // Les sondes
   struct probe_t * serviceProbe;
};

/**
 * @brief Définition des fonctions spécifiques liées au ndesObject
 */
defineObjectFunctions(srvGen);

/**
 * @brief Les entrées de log sont des ndesObject
 */
struct ndesObjectType_t srvGenType = {
   ndesObjectTypeDefaultValues(srvGen)
};

/*
 * Creation et initialisation d'un serveur
 */
struct srvGen_t * srvGen_create(void * destination,
                                processPDU_t destProcessPDU)
{
   struct srvGen_t * result = sim_malloc(sizeof(struct srvGen_t));
   ndesObjectInit(result, srvGen);

   result->srvState = srvStateIdle;
   result->currentPDU = NULL;

   // La destination des PDU servies
   result->destination = destination;
   result->destProcessPDU = destProcessPDU;

   // Le mode de calcul du temps de traitement (par défaut débit unitaire)
   result->dateGenerator = NULL;
   result->serviceTime = serviceTimeProp;
   result->serviceTimeParameter = 1.0;

   // WARNING A modifier (cf struct)
   result->source = NULL;

   result->serviceProbe = NULL;

   return result;
}


void srvGen_terminateProcess(struct srvGen_t * srv);
/*
 * Début de traitement d'une PDU
 */
void srvGen_startService(struct srvGen_t * srv, struct PDU_t * pdu)
{
   struct event_t * event;
   double date ; 

   assert(pdu != NULL);

   // Si le destinataire n'a pas récupéré la précédente, trop tard !
   if (srv->currentPDU != NULL) {
      PDU_free(srv->currentPDU);
   }

   srv->srvState = srvStateBusy;
   srv->serviceStartTime = motSim_getCurrentTime();
   srv->currentPDU = pdu;

   //Déterminer une date de fin en fonction du temps de traitement
   if (srv->serviceTime == serviceTimeProp){
      date = motSim_getCurrentTime() + PDU_size(pdu) * srv->serviceTimeParameter;
   } else {
      assert(srv->dateGenerator);
      date = dateGenerator_nextDate(srv->dateGenerator);
   }

   printf_debug(DEBUG_SRV, " PDU %d from %6.3f to %6.3f\n", PDU_id(pdu), motSim_getCurrentTime(), date);

   // On crée un événement pour cette date
   event = event_create((eventAction_t)srvGen_terminateProcess, srv, date);

   // On ajoute cet événement au simulateur
   motSim_addEvent(event);
}

/**
 * @brief Ajout d'une sonde sur le temps de service
 */
void srvGen_addServiceProbe(struct srvGen_t * srv, struct probe_t * serviceProbe)
{
   srv->serviceProbe = probe_chain(serviceProbe, srv->serviceProbe);
}

/*
 * Fin de traitement d'une PDU
 */
void srvGen_terminateProcess(struct srvGen_t * srv)
{
   struct PDU_t * pdu;

   srv->srvState = srvStateIdle;

   printf_debug(DEBUG_SRV, " end of service\n");

   if (srv->serviceProbe) {
      probe_sample(srv->serviceProbe, motSim_getCurrentTime() - srv->serviceStartTime);
   }

   // On propose la PDU à la destination
   (void)srv->destProcessPDU(srv->destination, srvGen_getPDU, srv);

   // On va chercher la prochaine s'il y en a une
   // WARNING : et s'il y en a deux ?
   if (srv->source) {
      printf_debug(DEBUG_SRV, " looking for a new PDU\n");
      pdu = srv->getPDU(srv->source);
      // Est-elle encore prete ?
      if (pdu) {
         ndesLog_logLineF(PDU_getObject(pdu), "IN %d", srvGen_getObjectId(srv));
         srvGen_startService(srv, pdu);
      } else { // Si elle ne l'est plus, inutile d'y revenir pour le moment
         srv->source = NULL; 
      }
   }
}

/*
 * Notification de la présence d'une PDU
 */
int srvGen_processPDU(void * srv,
                       getPDU_t getPDU, void * source)
{
   struct srvGen_t * server = (struct srvGen_t * )srv;
   struct PDU_t * pdu;

   printf_debug(DEBUG_SRV, " server processing\n");

   // Si c'est juste pour tester si je suis pret
   if ((getPDU == NULL) || (source == NULL)) {
      return (server->srvState == srvStateIdle);
   }

   // Si je suis dispo, je vais la chercher et la traiter
   if (server->srvState == srvStateIdle){
      printf_debug(DEBUG_SRV, " server will serve PDU\n");

      // On va chercher une PDU puisqu'il y en a une de prête
      pdu = getPDU(source);
      ndesLog_logLineF(PDU_getObject(pdu), "IN %d", srvGen_getObjectId(server));

      srvGen_startService(server, pdu);
      return 1;

   // Sinon, je note qu'elle est dispo
   } else {
      printf_debug(DEBUG_SRV, " server unavailable\n");
      if (server->source != NULL) {
	 if ((server->source != source) || (server->getPDU != getPDU)) {
            printf_debug(DEBUG_ALWAYS, " server can not handle multiple sources.\n");
	    exit(1);
	 }
      } else {
         server->source = source;
         server->getPDU = getPDU;
      }
      return 0;
   }
}

/*
 * Obtention de la dernière PDU servie (éventuellement NULL si trop tard !)
 */
struct PDU_t * srvGen_getPDU(void * s)
{
   struct srvGen_t * srv = (struct srvGen_t * )s;
   struct PDU_t * pdu = srv->currentPDU;
   
   srv->currentPDU = NULL;

   ndesLog_logLineF(PDU_getObject(pdu), "OUT %d", srvGen_getObjectId(srv));

   return pdu;
}

void srvGen_setServiceTime(struct srvGen_t * srv,
			   enum serviceTime_t st,
			   double parameter)
{
   srv->serviceTime = st;
   srv->serviceTimeParameter = parameter;

   switch (srv->serviceTime) {
      case serviceTimeCst :
         srv->dateGenerator = dateGenerator_createPeriodic(parameter);
      break ;
      case serviceTimeExp :
         srv->dateGenerator = dateGenerator_createExp(parameter);
      break ;
      case serviceTimeProp :
         srv->dateGenerator = NULL; // WARNING pas génial de gérer ça ainsi
      break ;
      default :
         motSim_error(MS_FATAL, "Unknown service time strategy");
      break;
   }
}
