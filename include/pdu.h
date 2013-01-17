/*
 * Le type PDU, initialement cens� repr�senter les PDU, sert
 * maintenant � manipuler un peu tout ce qui est dynamique dans le
 * syst�me. Il faudra s�rement le renommer lorsque je trouverai un
 * nom pertinent et que je cr�erai des PDU !
 */
#ifndef __DEF_PDU
#define __DEF_PDU

#include <motsim.h>
#include <probe.h>

/*
 * Le type est visible car utilis� par diff�rents modules vu que c'est
 * un peu le couteau suisse de l'outil de simulation. Id�alement, il
 * faut utiliser autant que possible les m�thodes de manipulation
 * fournies plus bas.
 */
struct PDU_t {
   int      id;    // Un identifiant g�n�ral
   motSimDate_t  creationDate;

   void   * data;  // Des donnees priv�es
   int      taille ;

   // Les pointeurs suivants sont � la discr�tion du propri�taire de la PDU
   struct PDU_t * prev;
   struct PDU_t * next;
};

/*
 * Cr�ation d'une PDU de taille fournie. Elle peut contenir
 * un pointeur vers des donnees privees.
 */
extern struct PDU_t * PDU_create(int size, void * private);

/*
 * Consultation de la taille d'une PDU
 */
extern int PDU_size(struct PDU_t * PDU);

extern int PDU_id(struct PDU_t * PDU);

/*
 * Obtention de la date de cr�ation
 */
extern motSimDate_t PDU_getCreationDate(struct PDU_t * PDU);

void * PDU_private(struct PDU_t * PDU);

/*
 * Destruction d'une PDU. Les donnees privees doivent avoir
 * ete detruites par l'appelant.
 */
void PDU_free(struct PDU_t * pdu);

/*
 * Le type des fonctions utilis�es entre les producteurs
 * et consommateurs de PDU.
 */
typedef struct PDU_t * (*getPDU_t)(void * source);

typedef void (*processPDU_t)(void * receiver,
                             getPDU_t getPDU,
                             void * source);

/*
 * Les sondes systeme
 */
extern struct probe_t * PDU_createProbe;
extern struct probe_t * PDU_reuseProbe;
extern struct probe_t * PDU_mallocProbe;
extern struct probe_t * PDU_freeProbe;

#endif
