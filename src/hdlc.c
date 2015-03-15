/**
 * @file hdlc.c
 * @brief A very basic HDLC implementation (unusable for now)
 *
 * Quelques notes
 *
 *    Transmission :
 *      L'émission est faite par hdlc_getFrame qui transmet une trame
 *      U ou S s'il y en a, une trame de donnée s'il y en a, rien
 *      sinon.
 *      . Les trames de données sont placées dans la fenêtre puis
 *      transmises dès que possible.
 *      . La prochaine trame U/S a émettre sera placée dans nextFrame
 *      (on suppose donc qu'il n'y en a qu'une) WARNING en fait, on
 *      doit pouvoir déterminer quelle trame transmettre dans la
 *      fonction hdlc_getFrame en fonction de l'état actuel. Re
 *      WARNING en fait non ! Ca marche pas pour les UAs en
 *      particulier ...
 *      . Une trame RR ne sera pas envoyée s'il y a une trame de
 *      données en attente
 */
#include <hdlc.h>

/**
 * @brief Definition of an HDLC entity
 */
struct hdlc_t {
   void * outLink; //!< Output lowlevel
   processPDU_t sendPDU;

   struct PDU_t * nextFrame;   //!< Should be removed
   int state;

   // Sender-side window
   unsigned short senderWindowCapacity;
   unsigned short senderWindowSize;
   unsigned short senderReceivedNr;
   struct hdlcFrame_t * senderWindow[HDLC_MAX_WINDOW_SIZE];

   // Notification function/object for an incoming connection
   int (*connectionNotification)(struct hdlc_t * , void * ) ;
   void * connectionNotificationData;
};

/**
 * @brief HDLC frame format
 * 
 * It is not (yet) a "real" implementation of a frame, but it is
 * suitable for simulations
 */
struct hdlcFrame_t {
   union hdlcFrameHeader_t {
      struct {
	 unsigned int type;
	 unsigned int pf;
	 unsigned int padding;
      } u;
      struct {
         unsigned int nr;
	 unsigned int type;
	 unsigned int pf;
      } s;
      struct {
         unsigned int nr;
	 unsigned int ns;
	 unsigned int pf;
      } i;
   } header;
   unsigned int type;
   void * data;
};

/**
 *
 */
#define HDLC_U_FRAME 0
#define HDLC_I_FRAME 1
#define HDLC_S_FRAME 2

#define HDLC_U_FRAME_SIZE 2

#define HDLC_SNRM 0
#define HDLC_UA 1


/**
 * @brief HDLC entity current state
 */
#define HDLC_STATE_DISCONNECTED   0
#define HDLC_STATE_CONNECTING     1
#define HDLC_STATE_CONNECTED      2          

void hdlc_printFrame(struct hdlcFrame_t * frame)
{
   if (frame->type == HDLC_I_FRAME) {
      printf("I %d, %d %c\n", frame->header.i.nr, frame->header.i.ns, frame->header.i.pf?'P':'-');
   } else if  (frame->type == HDLC_U_FRAME) {
     if (frame->header.u.type == HDLC_SNRM) {
        printf("SNRM\n");
     } else if (frame->header.u.type == HDLC_UA) {
        printf("UA\n");
     } else {
        printf("U?\n");
     }
   } else if  (frame->type == HDLC_S_FRAME) {
        printf("S?\n");
   } else {
      printf("UNKNOWN (%d)!!!\n", frame->type);
   }
}

/**
 * @brief Send an un-numbered frame
 */
struct hdlcFrame_t  *  hdlc_createUFrame(struct hdlc_t * h,
                 unsigned int uFrameType,
                 unsigned int pf)
{
   struct hdlcFrame_t  * frame;
   printf_debug(DEBUG_ALWAYS, "(%p) IN\n", h);

   // creation and initialisation
   frame = sim_malloc(sizeof(struct hdlcFrame_t));

   frame->type = (unsigned int)HDLC_U_FRAME;
   frame->header.u.type = uFrameType;
   frame->header.u.pf = pf;

   hdlc_printFrame(frame);

   // send the frame
   printf_debug(DEBUG_ALWAYS, "(%p) OUT\n", h);

   return frame;
}

/**
 * @brief Default behaviour is accept and ack anything coherent
 */
int hdlcDefaultConnectionNotification(struct hdlc_t * h,
				      void * data)
{
   printf_debug(DEBUG_ALWAYS, "IN\n");

   printf_debug(DEBUG_ALWAYS, "OUT\n");

   return 0;
}

/**
 * @brief Creation/initialisation of a HDLC entity
 */
struct hdlc_t * hdlc_create()
{
   struct hdlc_t * result = sim_malloc(sizeof(struct hdlc_t));

   printf_debug(DEBUG_ALWAYS, "IN\n");

   result->outLink = NULL;
   result->sendPDU = NULL;
   result->state = HDLC_STATE_DISCONNECTED;
   result->senderWindowCapacity = 3;
   result->senderWindowSize = 0;
   result->senderReceivedNr = 0;

   hdlc_setConnectionNotification(result, hdlcDefaultConnectionNotification, NULL);

   printf_debug(DEBUG_ALWAYS, "OUT\n");

   return result;
}

/**
 * @brief Association with an output link
 */
void hdlc_setOutLink(struct hdlc_t * h,
		     void * destination,
		     processPDU_t destProcessPDU)
{
   printf_debug(DEBUG_ALWAYS, "IN\n");

   h->outLink = destination;
   h->sendPDU = destProcessPDU;

   printf_debug(DEBUG_ALWAYS, "OUT\n");
}

/**
 * @brief Initialization of the function to be called on an incomming
 * connection request
 */
void hdlc_setConnectionNotification(struct hdlc_t * h,
				      int (*notifFunc)(struct hdlc_t * , void * ),
				    void * data)
{
   printf_debug(DEBUG_ALWAYS, "IN\n");
   h->connectionNotification = notifFunc;
   h->connectionNotificationData = data;
   printf_debug(DEBUG_ALWAYS, "OUT\n");
}

/**
 * @bried Actual transmission of a frame to output link
 */
struct PDU_t * hdlc_getFrame(void * hv)
{
   struct hdlcFrame_t * frame = NULL;
   struct hdlc_t * h = (struct hdlc_t *)hv;

   printf_debug(DEBUG_ALWAYS, "IN\n");

   switch (h->state) {
      case HDLC_STATE_CONNECTING :
         // We can only send S..M frame in this state
	frame = hdlc_createUFrame(h, HDLC_SNRM, 1);
      break ;
      case HDLC_STATE_CONNECTED :
      break ;
      default :
         printf_debug(DEBUG_ALWAYS, "Unknown state %d\n", h->state);
      break ;
   }

   // Prepare a timer

   // Actually send the frame
   printf_debug(DEBUG_ALWAYS, "OUT\n");
   if (frame) {
      return PDU_create(0, frame);
   } else {
      return NULL;
   }
}

/**
 *
 */
void hdlc_sendUFrame(struct hdlc_t *  h, int uFrameType, int usePF)
{
   // The actual frame is built in getFrame
   h->sendPDU(h->outLink, hdlc_getFrame, h);
}

/**
 *
 */
void hdlc_processUFrame(struct hdlc_t *  h, struct hdlcFrame_t * frame)
{
   printf_debug(DEBUG_ALWAYS, "IN\n");
   hdlc_printFrame(frame);
   switch(frame->header.u.type) {
      case HDLC_SNRM :
         if (h->state == HDLC_STATE_DISCONNECTED) {
            h->connectionNotification(h, h->connectionNotificationData);
         }
         printf_debug(DEBUG_ALWAYS, "Acking SNRM\n");
         // We ack, even if already connected (may be a dup)
         hdlc_sendUFrame(h, HDLC_UA, frame->header.u.pf);
      break;
      case HDLC_UA :
         if (h->state == HDLC_STATE_CONNECTING) {
            printf_debug(DEBUG_ALWAYS, "CONNECTED !");
            h->state = HDLC_STATE_CONNECTED;
         }
      break;
   }
   printf_debug(DEBUG_ALWAYS, "OUT\n");
}

/**
 * @brief Traitement d'une PDU envoyée par l'entité homologue
 */
void hdlc_processPDU(struct hdlc_t * h,
                     getPDU_t getPDU,
                     void * source)
{
   struct PDU_t * pdu;
   struct hdlcFrame_t * frame;

   pdu = getPDU(source);

   printf_debug(DEBUG_ALWAYS, "pdu %d received\n", PDU_id(pdu));

   // WARNING : How could we know for sure it's HDLC ?
   frame = PDU_private(pdu);
   hdlc_printFrame(frame);
   switch (frame->type) {
      case HDLC_U_FRAME :
         hdlc_processUFrame(h, frame);
      break;
   }
}

/**
 * @brief
 */
int hdlc_senderWindowIsFull(struct hdlc_t * h)
{
   return (h->senderWindowCapacity == h->senderWindowSize);
}

/**
 * @brief
 */
void hdlc_insertInSenderWindow(struct hdlc_t * h,
                               struct PDU_t * sdu)
{
   int ns; //!< Sequence number of this next frame

   printf_debug(DEBUG_ALWAYS, "IN\n");

   assert(!hdlc_senderWindowIsFull(h));

   h->senderWindowSize += 1;
   ns = (h->senderReceivedNr + h->senderWindowSize)%h->senderWindowCapacity;

   // Build the frame (nr will be set later)
   h->senderWindow[ns] = (struct hdlcFrame_t *)sim_malloc(sizeof(struct hdlcFrame_t));
   h->senderWindow[ns]->header.i.ns = ns;
   h->senderWindow[ns]->data = sdu; // Should be copied

   printf_debug(DEBUG_ALWAYS, "OUT\n");
}

/**
 * @brief Traitement d'une SDU soumise par la couche supérieure
 */
void hdlc_processSDU(struct hdlc_t * h,
                     getPDU_t getPDU,
                     void * source)
{
   struct PDU_t * sdu;
   printf_debug(DEBUG_ALWAYS, "IN\n");

   // If the sender window is full, we need to delay
   if (hdlc_senderWindowIsFull(h)) {  
      printf_debug(DEBUG_ALWAYS, "Not ready\n");
   }

   // We can insert a new frame in the window
   sdu = getPDU(source);
   if (sdu == NULL) {
      printf_debug(DEBUG_ALWAYS, "Received NULL sdu !\n");
      return ;
   }
   printf_debug(DEBUG_ALWAYS, "sdu received\n");
   hdlc_insertInSenderWindow(h, sdu);

   // Can we send the frame ?
   printf_debug(DEBUG_ALWAYS, "Now we send the frame\n");
   h->sendPDU(h->outLink, hdlc_getFrame, h);

   printf_debug(DEBUG_ALWAYS, "OUT\n");
}

/**
 * @brief Initialization of a connection
 *
 * Only SNRM for now ...
 *
 */
int hdlc_connectRequest(struct hdlc_t * h)
{
   printf_debug(DEBUG_ALWAYS, "IN\n");

   // Can connect only if disconnected ...
   if (h->state != HDLC_STATE_DISCONNECTED) {
      return 1;
   }

   // Change current state ...
   h->state = HDLC_STATE_CONNECTING;

   // Then send the frame
   hdlc_sendUFrame(h, HDLC_SNRM, 1);

   // The reception of UA will raise the connectConfirm
   printf_debug(DEBUG_ALWAYS, "OUT\n");

   return 0; //!< OK
}

