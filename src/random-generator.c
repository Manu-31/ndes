/*
 * Implantation des g�n�rateurs al�atoires. C'est encore un beau
 * fouilli ! Il faut arriver � distinguer proprement les divers
 * �l�ments. Il y a sch�matiquement trois niveaux dans ces objets : 
 *
 *  - Le type des donn�es renvoy�es (entier, r�el, ...)
 *  - La distribution
 *  - La source d'al�a (un PRNG, /dev/random, un fichier, ...)
 */

#include <stdio.h>     // printf
#include <stdlib.h>    // Malloc, NULL, exit...
#include <strings.h>   // bzero
#include <math.h>      // log
#include <values.h>    // *_MAX

#include <assert.h>

#include <motsim.h>
#include <file_pdu.h>
#include <random-generator.h>


/*
 * Structure g�n�rale d'un g�n�rateur al�atoire
 */
struct randomGenerator_t {
   int valueType;
   int distribution;
   int source;
   struct probe_t * values; // Liste des valeurs g�n�r�es durant la
  // phase de record, param�tre de la
  // source durant la phase de replay
  // c'est une sonde exhaustive


   // Param�tres li�s au type des donn�es produites
   union {
      struct uLparameter_t {
         unsigned long min;
         unsigned long max;
      } ul;
      struct uIRparameter_t { // A range of unsigned integers
         unsigned int min;
         unsigned int max;
      } uir;
      // Pour un ensemble discret de valeurs enti�res 
      struct uIDiscreteParameter_t{
         int nbValues;
         unsigned int * value;
      } uid;
      // Pour un ensemble discret de valeurs double
      struct dDiscreteParameter_t{
         int nbValues;
         double * value;
      } dd;
      struct doubleParameter_t {
         double min;
         double max;
      } d;
   } param;

   // Param�tres li�s � la distribution
   struct {
      double min, max; // Extreme values
      union {
         double lambda; // Pour expon
         struct {
            int nbProba;
            double * proba;
         } discrete;
         struct probe_t * pdf; // Ce n'est probablement pas le meilleur outil
      } d;
   } distParam; 
   // Prochaine valeur al�atoire conform�ment � la distribution
   double (*distGetNext)(struct randomGenerator_t * rg); 


   // Param�tres de la source d'al�a utilis�e
   union {
      int nextIdx; // Index for next (recorded) value to be generated
      unsigned short xsubi[3]; // for xrand48
   } aleaSrc;

   // La fonction donnant la prochaine valeur al�atoire entre 0 et 1
   double (*aleaGetNext)(struct randomGenerator_t * rg); 

   // Une sonde sur les valeurs g�n�r�es
   struct probe_t * valueProbe;

};

/*==========================================================================*/
/*       Les fonctions li�es aux sources.                                   */
/*==========================================================================*/
/*
 * Next value with erand48
 */
inline double randomGenerator_erand48GetNext(struct randomGenerator_t * rg)
{
   double result = erand48(rg->aleaSrc.xsubi);

   if (rg->values)
      probe_sample(rg->values, result);

   return result;
}

/*
 * Initialisation of erand48
 */
void randomGenerator_erand48Init(struct randomGenerator_t * rg)
{
   assert(rg->source == rGSourceErand48);

   bzero(rg->aleaSrc.xsubi, 3);
   rg->aleaGetNext = randomGenerator_erand48GetNext;

}

/*
 * Next value with replay
 */
inline double randomGenerator_replayGetNext(struct randomGenerator_t * rg)
{
  return probe_exhaustiveGetSampleN(rg->values, rg->aleaSrc.nextIdx++);
}

/*
 * Initialisation of replay
 */
void randomGenerator_replayInit(struct randomGenerator_t * rg)
{
   assert(rg->source == rGSourceReplay);

   rg->aleaSrc.nextIdx = 0;
   rg->aleaGetNext = randomGenerator_replayGetNext;
}


/*==========================================================================*/
/*       Les fonctions li�es aux distributions.                             */
/*==========================================================================*/
inline double randomGenerator_noDistGetNext(struct randomGenerator_t * rg)
{
  motSim_error(MS_FATAL, "No default random distribution");
}

/*
 * Next value with uniform distribution  
 */
inline double randomGenerator_uniformGetNext(struct randomGenerator_t * rg)
{
  return rg->aleaGetNext(rg); //Les sources sont cens�es �tre uniformes entre 0 et 1 ...
}

/*
 * Initialisation of uniform distribution  
 */
void randomGenerator_uniformInit(struct randomGenerator_t * rg)
{
   assert(rg->distribution == rGDistUniform); 

   //Les sources sont cens�es �tre uniformes entre 0 et 1 ...
   rg->distParam.min = 0.0;
   rg->distParam.max = 1.0;

   rg->distGetNext = randomGenerator_uniformGetNext;
}

/*
 * Next value with exponential distribution  
 */
double randomGenerator_exponentialGetNext(struct randomGenerator_t * rg)
{
   double alea;
   double result ;

   //  Les sources sont cens�es �tre uniformes ...
   alea = rg->aleaGetNext(rg);

   result =  - log(alea) /rg->distParam.d.lambda;

/*
   printf_debug(DEBUG_GENE, " alea = %6.3f, lambda = %6.3f, result = %6.3f\n",
		alea, rg->distParam.d.lambda, result);
*/
   return result;
}

/*
 * Initialisation of exponential distribution  
 */
void randomGenerator_exponentialInit(struct randomGenerator_t * rg, double lambda)
{
   assert(rg->distribution == rGDistExponential); 
   rg->distParam.min = 0.0;
   rg->distParam.max = DBL_MAX;

   rg->distParam.d.lambda = lambda;
   rg->distGetNext = randomGenerator_exponentialGetNext;
}

/*
 * Change lambda
 */
void randomGenerator_setLambda(struct randomGenerator_t * rg, double lambda)
{
   assert(rg->distribution == rGDistExponential); 

   rg->distParam.d.lambda = lambda;
}

/*
 * Next value with discrete distribution  
 *
 * On g�n�re une valeur situ�e au milieu de l'intervalle
 */
double randomGenerator_discreteGetNext(struct randomGenerator_t * rg)
{
   unsigned int n = 0;
 
   double alea = rg->aleaGetNext(rg);

   do  {
      alea -= rg->distParam.d.discrete.proba[n];
   } while ((alea > 0 ) && (++n < rg->distParam.d.discrete.nbProba));
   return ((double)n)/ rg->distParam.d.discrete.nbProba;
}

/*
 * Initialisation of discrete distribution
 */
void randomGenerator_discreteInit(struct randomGenerator_t * rg,
				  int nb, double * proba)
{

   int v;
 
   assert(rg->distribution == rGDistDiscrete); 

   rg->distParam.d.discrete.nbProba = nb;
   rg->distParam.d.discrete.proba = (double *) sim_malloc(nb*sizeof(double));

   //  bcopy ferait l'affaire
   for (v = 0 ; v < nb ; v++){
      rg->distParam.d.discrete.proba[v] = proba[v];
   }

   rg->distGetNext = randomGenerator_discreteGetNext;
}


/*==========================================================================*/
/*       Les fonctions de plus haut niveau.                                 */
/*==========================================================================*/
/*
 * Get the next unsigned int value
 */
unsigned int randomGenerator_getNextUInt(struct randomGenerator_t * rg)
{
   unsigned int result = 0; // Init contre warning

   // Etape 1 : le g�n�rateur donne une nouvelle valeur
   //   (elle n'apparait pas explicitement ici, elle est r�alis�e dans
   //    le distGetNext())
   // Etape 2 : La distribution est appliqu�e (invocation du
   //    distGetNext())
   double alea = rg->distGetNext(rg);

   // Etape 3 : On adapte au type des donn�es !
   switch (rg->valueType) {
      // Pour un unsigned int quelconque, on prend juste la partie enti�re
      // Attention, �a peut donner des trucs �trange si la distribution
      // ne donne pas des r�sultats sur R+ !
      case rGTypeUInt :
	result = (unsigned int)alea;
      break;

      // Pour un intervalle, si on a une distribution qui donne un
      // intervalle (qu'on supposera �tre [0, 1]), on fait une
      // correspondance lin�aire canonique. Si elle est non born�e
      // (genre [0, +inf[), on �cr�te
      case rGTypeUIntRange :
         result = rg->param.uir.min + (unsigned int)(alea*(rg->param.uir.max - rg->param.uir.min));
         result = min(result, rg->param.uir.max);
      break;

      // Pour un sous ensmble, on fait, en gros, comme pour un
      // intervalle : on se ram�ne � l'intervalle [0, nbValues - 1]
      case rGTypeUIntEnum :
	result = rg->param.uid.value[min((int)(rg->param.uid.nbValues*alea), rg->param.uid.nbValues -1)];
      break;
      default :
	 motSim_error(MS_FATAL, "not implemented\n");
      break;
   }

   // On probe �ventuellement
   if (rg->valueProbe) {
     probe_sample(rg->valueProbe, (double)result);
   }

   return result;
}

/*
 * ATTENTION ici gros soucis de normalisation !!!
 * Certaines distributions ont des valeurs discretes, d'autres
 * born�es, d'autres encore illimit�es. Comment ramener tout �a �
 * l'ensemble de valeurs du type ???
 */
double randomGenerator_getNextDouble(struct randomGenerator_t * rg)
{
   double result = 0.0;

   switch (rg->valueType) {
      case rGTypeDouble :
         result = rg->distGetNext(rg);
      break;
      case rGTypeDoubleRange :
   // ATTENTION, il faudrait normaliser entre distParam.min et max 
	result = rg->param.d.min + rg->distGetNext(rg) * (rg->param.d.max - rg->param.d.min);
      break;
      case rGTypeDoubleEnum :
        result = rg->param.dd.value[(int)(rg->param.dd.nbValues*rg->distGetNext(rg))];
      break;
      default :
	motSim_error(MS_FATAL, "not implemented\n");
      break;
   }

   // On probe �ventuellement
   if (rg->valueProbe) {
      probe_sample(rg->valueProbe, result);
   }
   return result;

}

double randomGenerator_UIntDiscreteGetExpectation(struct randomGenerator_t * rg)
{
   double result = 0.0;
   int n;

   for (n = 0 ; n < rg->param.uid.nbValues; n++) {
     result += (double)rg->param.uid.value[n] * rg->distParam.d.discrete.proba[n];
   }

   return result;
}

/*==========================================================================*/

double randomGenerator_getExpectation(struct randomGenerator_t * rg)
{
   double result = 0.0;

   switch (rg->distribution) {
      case rGDistDiscrete :
         switch (rg->valueType) {
            case rGTypeUIntEnum :
	      result = randomGenerator_UIntDiscreteGetExpectation(rg);  // WARNING, et les Double !?
            break;
            default :
	      motSim_error(MS_FATAL, "Distribution %d non implantee !\n", rg->distribution);
            break;
	 };
      break;
      default :
         motSim_error(MS_FATAL, "Distribution %d non implantee !\n", rg->distribution);
      break;
   }

   return result;
}

void randomGenerator_reset(struct randomGenerator_t * rg)
{
   printf_debug(DEBUG_GENE, "IN\n");
   // Gestion du "record then replay"
   if (rg->values) {
      rg->source = rGSourceReplay;
      randomGenerator_replayInit(rg);
   }

   // Rien pour les autres pour le moment !
   printf_debug(DEBUG_GENE, "OUT\n");
}

/*==========================================================================*/
/*      CREATORS.                                                           */
/*==========================================================================*/
struct randomGenerator_t * randomGenerator_createRaw()
{
   struct randomGenerator_t * result
          = sim_malloc(sizeof(struct randomGenerator_t ));

   result->values = NULL;

   result->distribution = rGDistNoDist; //WARNING on doit pouvoir en
					//mettre une ...
   result->distGetNext = randomGenerator_noDistGetNext;

   // Ajout � la liste des choses � r�initialiser avant une prochaine simu
   motsim_addToResetList(result, (void (*)(void * data)) randomGenerator_reset);

   // Source
   result->source = rGSourceErand48; // WARNING use rgSourceDefault
   randomGenerator_erand48Init(result); // ... ?

   return result;
}

/*--------------------------------------------------------------------------*/
/*      CREATORS : ULONG                                                    */
/*--------------------------------------------------------------------------*/
struct randomGenerator_t * randomGenerator_createULong(int distribution,
						       unsigned long min,
						       unsigned long max)
{
   struct randomGenerator_t * result = randomGenerator_createRaw();

   motSim_error(MS_FATAL, "Non implantee !\n");

   return result;
}

/*--------------------------------------------------------------------------*/
/*      CREATORS : DOUBLE                                                   */
/*--------------------------------------------------------------------------*/
struct randomGenerator_t * randomGenerator_createDouble()
{
   struct randomGenerator_t * result = randomGenerator_createRaw();

   // Data type
   result->valueType = rGTypeDouble; 
 
   return result;
}

struct randomGenerator_t * randomGenerator_createDoubleExp(double lambda)
{
   struct randomGenerator_t * result = randomGenerator_createDouble();

   randomGenerator_setDistributionExp(result, lambda);

   return result;
}
/* 
 * A double range [min .. max}, default distribution : uniform
 */
struct randomGenerator_t * randomGenerator_createDoubleRange(double min,
							     double max)
{
   struct randomGenerator_t * result = randomGenerator_createRaw();

   // Data type
   result->valueType = rGTypeDoubleRange; 
   result->param.d.min = min;
   result->param.d.max = max;

   // Distribution-specific
   result->distribution = rGDistUniform;

   randomGenerator_uniformInit(result);

   return result;
}

/*--------------------------------------------------------------------------*/
/*      CREATORS : UINT                                                     */
/*--------------------------------------------------------------------------*/
/*
 * Cr�ation d'un g�n�rateur d'entiers
 */
struct randomGenerator_t * randomGenerator_createUInt()
{
  struct randomGenerator_t * result = randomGenerator_createRaw();

   // Data type
   result->valueType = rGTypeUInt;

   return result;
}

struct randomGenerator_t * randomGenerator_createUIntRange(unsigned int min,
						      unsigned int max)
{
  struct randomGenerator_t * result = randomGenerator_createRaw();

   // Data type
   result->valueType = rGTypeUIntRange;
   result->param.uir.min = min;
   result->param.uir.max = max;

   return result;
}

/*
 * Cr�ation d'un g�n�rateur al�atoire de nombres entiers.
 */
struct randomGenerator_t * randomGenerator_createUIntDiscrete(int nbValues,
							      unsigned int * values)
{
   struct randomGenerator_t * result = randomGenerator_createRaw();
   int v;

   // Data type
   result->valueType = rGTypeUIntEnum; 

   //    Initialisation des valeurs 
   result->param.uid.nbValues = nbValues;
   result->param.uid.value = (unsigned int *) sim_malloc(nbValues*sizeof(unsigned int));
   //    un bcopy ferait l'affaire
   for (v = 0 ; v < nbValues ; v++){
      result->param.uid.value[v] = values[v];
   }
   return result;
}


/*
 * Le nombre de valeurs possibles est pass� en param�tre ainsi que la
 * liste de ces valeurs puis la liste de leurs probabilit�.
 */
struct randomGenerator_t * randomGenerator_createUIntDiscreteProba(
				int nbValues,
				unsigned int * values,
				double * proba)
{
 
   // Create a discrete UInt generator
   struct randomGenerator_t * result 
         = randomGenerator_createUIntDiscrete(nbValues, values);

   // Specifying an explicit distribution
   randomGenerator_setDistributionDiscrete(result, nbValues, proba);

   return result;
}

struct randomGenerator_t * randomGenerator_createDoubleDiscrete(int nbValues,
                                     double * values)
{
   int v;
 
   struct randomGenerator_t * result = randomGenerator_createRaw();

   // Data type
   result->valueType = rGTypeDoubleEnum; 

   //    Initialisation des valeurs 
   result->param.dd.nbValues = nbValues;
   result->param.dd.value = (double *) sim_malloc(nbValues*sizeof(double));
   //    un bcopy ferait l'affaire
   for (v = 0 ; v < nbValues ; v++){
      result->param.dd.value[v] = values[v];
   }

   return result;
}

struct randomGenerator_t * randomGenerator_createDoubleDiscreteProba(int nbValues,
                                     double * values, double * proba)
{
   // Create a discrete double generator
   struct randomGenerator_t * result 
         = randomGenerator_createDoubleDiscrete(nbValues, values);

   // Specifying an explicit distribution
   randomGenerator_setDistributionDiscrete(result, nbValues, proba);

   return result;
}


// Use a (previously built) probe to re-run a sequence
struct randomGenerator_t * randomGenerator_createFromProbe(struct probe_t * p)
{
   struct randomGenerator_t * result
          = randomGenerator_createRaw();

   return result;
}

/*
 * Destructor
 */
void randomGenerator_delete(struct randomGenerator_t * rg)
{
   printf_debug(DEBUG_TBD, "Pas encore implant� !!!\n");
}



/*==========================================================================*/
/*   Select distribution                                                    */
/*==========================================================================*/
void randomGenerator_setDistributionUniform(struct randomGenerator_t * rg)
{
   rg->distribution = rGDistUniform;
   randomGenerator_uniformInit(rg);
}

// Un nombre discret de probabilit�s
void randomGenerator_setDistributionDiscrete(struct randomGenerator_t * rg,
					     int nb,
                                             double * proba)
{
   // Sp�cification de la dist
   rg->distribution = rGDistDiscrete;

   // Initialisation des valeurs
   randomGenerator_discreteInit(rg, nb, proba);
}

void randomGenerator_setUniformMinMaxDouble(struct randomGenerator_t * rg, double min, double max)
{
  rg->param.d.min = min;
  rg->param.d.max = max;
}

// Choix d'une loi exponentielle
void randomGenerator_setDistributionExp(struct randomGenerator_t * rg, double lambda)
{
   rg->distribution = rGDistExponential;
   randomGenerator_exponentialInit(rg, lambda);
}


/*==========================================================================*/

/*
 * Prepare for record values in order to replay on each reset
 */
void randomGenerator_recordThenReplay(struct randomGenerator_t * rg)
{
   // On cr�e une sonde pour enregistrer les valeurs
   rg->values = probe_createExhaustive();

   // Cette probe, par d�finition, ne doit pas �tre reset�e
   probe_setPersistent(rg->values);
}

/*==========================================================================*/
/*   Probes                                                                 */ 
/*==========================================================================*/
void randomGenerator_setValueProbe(struct randomGenerator_t * rg,
				   struct probe_t * p)
{
   rg->valueProbe = p;
}

