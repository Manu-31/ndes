/*
 * Implantation des gÃ©nÃ©rateurs alÃ©atoires. C'est encore un beau
 * fouilli ! Il faut arriver Ã  distinguer proprement les divers
 * Ã©lÃ©ments. Il y a schÃ©matiquement trois niveaux dans ces objets : 
 *
 *  - Le type des donnÃ©es renvoyÃ©es (entier, rÃ©el, ...)
 *  - La distribution
 *  - La source d'alÃ©a (un PRNG, /dev/random, un fichier, ...)
 */

#include <stdio.h>     // printf
#include <stdlib.h>    // Malloc, NULL, exit...
#include <strings.h>   // bzero
#include <math.h>      // log
#include <values.h>    // *_MAX

#include <sys/time.h>  // gettimeofday

#include <assert.h>

#include <motsim.h>
#include <file_pdu.h>
#include <random-generator.h>

/**
 * @brief Available distributions
 */
#define rGDistNoDist               0
#define rGDistUniform              1
#define rGDistExponential          2
#define rGDistDiscrete             3
#define rGDistITS                  4
#define rGDistTruncatedLogNormal   5

#define rGDistDefault rGDistUniform

/**
 * @brief Available types for the random values
 */
#define rGTypeUInt         1
#define rGTypeULong        2
#define rGTypeFloat        3
#define rGTypeDouble       4
#define rGTypeDoubleRange  5
#define rGTypeUIntEnum     6
#define rGTypeDoubleEnum   7
#define rGTypeUIntRange    8
#define rGTypeUIntConstant 9
#define rGTypeDoubleConstant 10

/**
 * @brief Entropy sources
 */
#define rGSourceErand48 1
#define rGSourceReplay  2
#define rGSourceUrandom 3

#define rGSourceDefault rGSourceErand48

/*
 * Structure gÃ©nÃ©rale d'un gÃ©nÃ©rateur alÃ©atoire
 */
struct randomGenerator_t {
   int valueType;
   int distribution;
   int source;
   struct probe_t * values; // Liste des valeurs gÃ©nÃ©rÃ©es durant la
  // phase de record, paramÃ¨tre de la
  // source durant la phase de replay
  // c'est une sonde exhaustive


   // Paramêtres liés au type des données produites
   union {
      struct uLparameter_t {
         unsigned long min;
         unsigned long max;
      } ul;
      struct uIRparameter_t { // A range of unsigned integers
         unsigned int min;
         unsigned int max;
      } uir;
      // Pour un ensemble discret de valeurs entiÃ¨res 
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

   //!< Parameters describing the distribution 
   struct {
      double min, max; //!< Distribution support
      union {
         double lambda; //!< Exponential distribution
         struct {
            double mu;
            double sigma;
         } logNormal;
         struct {       //!< Discrete distribution
            int nbProba;
            double * proba;
         } discrete;
         struct {           //!< ITS distribution
            int nbParam;    //!< Quantile function number of parameters 
            double p1, p2, p3;  //!< Parameters values
	    union {
               double(*q0par)(double x);
               double(*q1par)(double x, double p1);
               double(*q2par)(double x, double p1, double p2);
               double(*q3par)(double x, double p1, double p2, double p3);
            } q;    //!< Quantile function
         } its;
      } d;
   } distParam; 

   // Prochaine valeur alÃ©atoire conformÃ©ment Ã  la distribution
   double (*distGetNext)(struct randomGenerator_t * rg); 

   // ParamÃ¨tres de la source d'alÃ©a utilisÃ©e
   union {
      int nextIdx; // Index for next (recorded) value to be generated
      unsigned short xsubi[3]; // for xrand48
   } aleaSrc;

   // La fonction donnant la prochaine valeur alÃ©atoire entre 0 et 1
   double (*aleaGetNext)(struct randomGenerator_t * rg); 

   // Une sonde sur les valeurs gÃ©nÃ©rÃ©es
   struct probe_t * valueProbe;

};

/*==========================================================================*/
/*       Les fonctions liÃ©es aux sources.                                   */
/*==========================================================================*/
/*
 * Next value with erand48
 */
inline double randomGenerator_erand48GetNext(struct randomGenerator_t * rg)
{
   //   double result = drand48();
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
   struct timeval now;
   assert(rg->source == rGSourceErand48);

   gettimeofday(&now, NULL);
   bcopy(&now + sizeof(now) - sizeof(rg->aleaSrc.xsubi), rg->aleaSrc.xsubi, sizeof(rg->aleaSrc.xsubi));
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
/*       Les fonctions liÃ©es aux distributions.                             */
/*==========================================================================*/
inline double randomGenerator_noDistGetNext(struct randomGenerator_t * rg)
{
   motSim_error(MS_FATAL, "No default random distribution");
   return 0.0;
}

/*
 * Next value with uniform distribution  
 */
inline double randomGenerator_uniformGetNext(struct randomGenerator_t * rg)
{
  return rg->aleaGetNext(rg); //Les sources sont censÃ©es Ãªtre uniformes entre 0 et 1 ...
}

/*
 * Initialisation of uniform distribution  
 */
void randomGenerator_uniformInit(struct randomGenerator_t * rg)
{
   assert(rg->distribution == rGDistUniform); 

   //Les sources sont censÃ©es Ãªtre uniformes entre 0 et 1 ...
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

   //  Les sources sont censÃ©es Ãªtre uniformes ...
   alea = rg->aleaGetNext(rg);

   result =  - log( alea) /rg->distParam.d.lambda;

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
 * On gÃ©nÃ¨re une valeur situÃ©e au milieu de l'intervalle
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

   if (rg->valueType == rGTypeUIntConstant) {
      result = rg->param.uir.min;
   } else {
   // Etape 1 : le gÃ©nÃ©rateur donne une nouvelle valeur
   //   (elle n'apparait pas explicitement ici, elle est rÃ©alisÃ©e dans
   //    le distGetNext())
   // Etape 2 : La distribution est appliquÃ©e (invocation du
   //    distGetNext())
   double alea = rg->distGetNext(rg);

   // Etape 3 : On adapte au type des donnÃ©es !
   switch (rg->valueType) {
      // Pour un unsigned int quelconque, on prend juste la partie entiÃ¨re
      // Attention, Ã§a peut donner des trucs Ã©trange si la distribution
      // ne donne pas des rÃ©sultats sur R+ !
      case rGTypeUInt :
	result = (unsigned int)alea;
      break;

      // Pour un intervalle, si on a une distribution qui donne un
      // intervalle (qu'on supposera Ãªtre [0, 1]), on fait une
      // correspondance linÃ©aire canonique. Si elle est non bornÃ©e
      // (genre [0, +inf[), on Ã©crÃ¨te
      case rGTypeUIntRange :
         result = rg->param.uir.min + (unsigned int)(alea*(rg->param.uir.max - rg->param.uir.min));
         result = min(result, rg->param.uir.max);
      break;

      case rGTypeUIntConstant :
	result = rg->param.uir.min;
      break;

      // Pour un sous ensemble, on fait, en gros, comme pour un
      // intervalle : on se ramÃ¨ne Ã  l'intervalle [0, nbValues - 1]
      case rGTypeUIntEnum :
	result = rg->param.uid.value[min((int)(rg->param.uid.nbValues*alea), rg->param.uid.nbValues -1)];
      break;
      default :
	 motSim_error(MS_FATAL, "not implemented\n");
      break;
   }
   }

   // On probe Ã©ventuellement
   if (rg->valueProbe) {
     probe_sample(rg->valueProbe, (double)result);
   }

   return result;
}

/*
 * ATTENTION ici gros soucis de normalisation !!!
 * Certaines distributions ont des valeurs discretes, d'autres
 * bornÃ©es, d'autres encore illimitÃ©es. Comment ramener tout Ã§a Ã 
 * l'ensemble de valeurs du type ???
 */
double randomGenerator_getNextDouble(struct randomGenerator_t * rg)
{
   double result = 0.0;

   switch (rg->valueType) {
      case rGTypeDoubleConstant :
         result = rg->param.d.min;
      break;
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

   // On probe Ã©ventuellement
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
   printf_debug(DEBUG_GENE, "IN\n");

   struct randomGenerator_t * result
          = sim_malloc(sizeof(struct randomGenerator_t ));

   result->values = NULL;

   result->distribution = rGDistNoDist; //WARNING on doit pouvoir en
					//mettre une ...
   result->distGetNext = randomGenerator_noDistGetNext;

   // Ajout Ã  la liste des choses Ã  rÃ©initialiser avant une prochaine simu
   motsim_addToResetList(result, (void (*)(void * data)) randomGenerator_reset);

   // Source
   result->source = rGSourceErand48; // WARNING use rgSourceDefault
   randomGenerator_erand48Init(result); // ... ?

   result->valueProbe = NULL;

   printf_debug(DEBUG_GENE, "OUT\n");

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
   printf_debug(DEBUG_GENE, "IN\n");
   struct randomGenerator_t * result = randomGenerator_createRaw();

   // Data type
   result->valueType = rGTypeDouble; 

   printf_debug(DEBUG_GENE, "OUT (created %p)\n", result);

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
 * CrÃ©ation d'un gÃ©nÃ©rateur d'entiers
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

// Des entiers non signÃ©s tous égaux !
struct randomGenerator_t * randomGenerator_createUIntConstant(unsigned int v)
{
  struct randomGenerator_t * result = randomGenerator_createRaw();

   // Data type
   result->valueType = rGTypeUIntConstant;
   result->param.uir.min = v;
   result->param.uir.max = v;

   return result;
}

struct randomGenerator_t * randomGenerator_createDoubleConstant(double v)
{
  struct randomGenerator_t * result = randomGenerator_createRaw();

   // Data type
   result->valueType = rGTypeDoubleConstant;
   result->param.d.min = v;
   result->param.d.max = v;

   return result;
}

/*
 * CrÃ©ation d'un gÃ©nÃ©rateur alÃ©atoire de nombres entiers.
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
 * Le nombre de valeurs possibles est passÃ© en paramÃ¨tre ainsi que la
 * liste de ces valeurs puis la liste de leurs probabilitÃ©.
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

void readUIntDiscreteProbaFromFile(char * fileName,
				int * nbValues,
				unsigned int ** values,
				   double ** proba);
/**
 * @brief Création d'une ditribution d'après un fichier
 */
struct randomGenerator_t * randomGenerator_createUIntDiscreteFromFile(char * fileName)
{
   struct randomGenerator_t * result ;
   int nbValues;
   unsigned int * values;
   double * proba;

   printf("On y va\n");
   readUIntDiscreteProbaFromFile(fileName, &nbValues, &values, &proba);

   result = randomGenerator_createUIntDiscreteProba(nbValues, values, proba);

   free(values);
   free(proba);

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
   printf_debug(DEBUG_TBD, "Pas encore implantÃ© !!!\n");
}



/*==========================================================================*/
/*   Select distribution                                                    */
/*==========================================================================*/
void randomGenerator_setDistributionUniform(struct randomGenerator_t * rg)
{
   rg->distribution = rGDistUniform;
   randomGenerator_uniformInit(rg);
}

// Un nombre discret de probabilitÃ©s
void randomGenerator_setDistributionDiscrete(struct randomGenerator_t * rg,
					     int nb,
                                             double * proba)
{
   // SpÃ©cification de la dist
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

/*-------------------------------------------------------------------------*/
/*  Truncated Log Normal                                                   */
/*-------------------------------------------------------------------------*/

/**
 * @brief Generating next value from a truncated log normal
 * @param rg a pointer to the random generator
 * @result a random value produced by the generator
 *
 * WARNING : this is an awfull implementation.
 */
double randomGenerator_truncatedLogNormalGetNext(struct randomGenerator_t * rg)
{
   double R,theta;
   double result ;

   //  Les sources sont censées être uniformes. La boucle do..while garantit un résultat < max.
   do{
      R = sqrt(-2*log(rg->aleaGetNext(rg)));
      theta = 2*M_PI*rg->aleaGetNext(rg); //R*cos(theta) suit une loi normale (0,1)
      result = exp(rg->distParam.d.logNormal.mu + rg->distParam.d.logNormal.sigma * R*cos(theta));
   } while (result > rg->distParam.max || result < rg->distParam.min);

   return result;
}

/**
 * @brief Initalization of a truncated log normal distribution
 */
void randomGenerator_truncatedLogNormalInit(struct randomGenerator_t * rg,
                                                       double mu, double sigma,
                                                       double min, double max)
{
   assert(rg->distribution == rGDistTruncatedLogNormal);

   // Define the support
   rg->distParam.min = min ;
   rg->distParam.max = max;

   rg->distParam.d.logNormal.mu = mu;
   rg->distParam.d.logNormal.sigma = sigma;
   rg->distGetNext = randomGenerator_truncatedLogNormalGetNext;   
}

/**
 * @brief Set a truncated log normal distribution
 */
void randomGenerator_setDistributionTruncatedLogNormal(struct randomGenerator_t * rg,
                                                       double mu, double sigma,
                                                       double min, double max)
{
   rg->distribution = rGDistTruncatedLogNormal;
   randomGenerator_truncatedLogNormalInit(rg, mu, sigma, min, max);
}
/*-------------------------------------------------------------------------*/
/*   ITS functions                                                         */ 
/*-------------------------------------------------------------------------*/

/**
 * @brief Next value with ITS
 *
 */
double randomGenerator_ITSGetNext(struct randomGenerator_t * rg)
{
   double alea;
   double result ;

   printf_debug(DEBUG_GENE, "IN\n");

   alea = rg->aleaGetNext(rg); //!< Uniform ]0, 1] sources
   printf_debug(DEBUG_GENE, "alea %f\n", alea);

   switch (rg->distParam.d.its.nbParam) {
      case 0 :
         printf_debug(DEBUG_GENE, "no param\n");
         result = rg->distParam.d.its.q.q0par(alea);
      break;
      case 1 :
         printf_debug(DEBUG_GENE, "one param\n");
         result = rg->distParam.d.its.q.q1par(alea, rg->distParam.d.its.p1);
      break;
      case 2 :
         printf_debug(DEBUG_GENE, "two param : %f %f\n", rg->distParam.d.its.p1, rg->distParam.d.its.p2);
         result = rg->distParam.d.its.q.q2par(alea, rg->distParam.d.its.p1, rg->distParam.d.its.p2);
      break;
      case 3 :
         printf_debug(DEBUG_GENE, "three param : %f %f %f\n", rg->distParam.d.its.p1, rg->distParam.d.its.p2, rg->distParam.d.its.p3);
         result = rg->distParam.d.its.q.q3par(alea, rg->distParam.d.its.p1, rg->distParam.d.its.p2, rg->distParam.d.its.p3);
      break;
      default:
          motSim_error(MS_FATAL, "too many parameters");
   }

   printf_debug(DEBUG_GENE, "OUT : %f\n", result);

   return result;
}

/**
 * @brief Define a distribution by its quantile function for inverse
 * transform sampling
 * @param rg The random generator
 * @param q The inverse cumulative density (quantile) function
 * @param p1 The single parameter of the quantile function
 */
void randomGenerator_setQuantile1Param(struct randomGenerator_t * rg,
				       double (*q)(double x, double p),
				       double p)
{
   printf_debug(DEBUG_GENE, "IN\n");

   rg->distribution = rGDistITS;
   rg->distParam.d.its.nbParam = 1;
   rg->distParam.d.its.p1 = p;
   rg->distParam.d.its.q.q1par = q;
   rg->distGetNext = randomGenerator_ITSGetNext;

   printf_debug(DEBUG_GENE, "OUT\n");
}


/**
 * @brief Define a distribution by its quantile function for inverse
 * transform sampling
 * @param rg The random generator
 * @param q The inverse cumulative density (quantile) function
 * @param p1 The first parameter of the quantile function
 * @param p2 The second parameter of the quantile function
 */
void randomGenerator_setQuantile2Param(struct randomGenerator_t * rg,
				       double (*q)(double x, double p1, double p2),
				       double p1, double p2)
{
   printf_debug(DEBUG_GENE, "IN\n");

   rg->distribution = rGDistITS;
   rg->distParam.d.its.nbParam = 2;
   rg->distParam.d.its.p1 = p1;
   rg->distParam.d.its.p2 = p2;
   rg->distParam.d.its.q.q2par = q;
   rg->distGetNext = randomGenerator_ITSGetNext;

   printf_debug(DEBUG_GENE, "OUT\n");
}

/**
 * @brief Define a distribution by its quantile function for inverse
 * transform sampling
 * @param rg The random generator
 * @param q The inverse cumulative density (quantile) function
 * @param p1 The first parameter of the quantile function
 * @param p2 The second parameter of the quantile function
 * @param p3 The third parameter of the quantile function
 */
void randomGenerator_setQuantile3Param(struct randomGenerator_t * rg,
				       double (*q)(double x, double p1, double p2, double p3),
				       double p1, double p2, double p3)
{
   printf_debug(DEBUG_GENE, "IN\n");

   rg->distribution = rGDistITS;
   rg->distParam.d.its.nbParam = 3;
   rg->distParam.d.its.p1 = p1;
   rg->distParam.d.its.p2 = p2;
   rg->distParam.d.its.p3 = p3;
   rg->distParam.d.its.q.q3par = q;
   rg->distGetNext = randomGenerator_ITSGetNext;

   printf_debug(DEBUG_GENE, "OUT\n");
}

/**
 * @brief Inverse of CDF for exponential distribution
 */
double randomGenerator_expDistQ(double x, double lambda)
{
   return - log(1.0 - x) / lambda;
}

/**
 * @brief Inverse of CDF for pareto distribution
 */
double randomGenerator_paretoDistQ(double x, double alpha, double xmin)
{
   return  xmin/(pow(x, 1.0/alpha));
}

/**
 * @brief Inverse of CDF for truncated pareto distribution
 */
double randomGenerator_truncatedParetoDistQ(double x, double alpha, double xmin, double xmax)
{
   return  xmin*xmax / pow(pow(xmax, alpha) + x*pow(xmin, alpha) - x*pow(xmax, alpha), 1.0/alpha);
}

/*==========================================================================*/

/*
 * Prepare for record values in order to replay on each reset
 */
void randomGenerator_recordThenReplay(struct randomGenerator_t * rg)
{
   // On crÃ©e une sonde pour enregistrer les valeurs
   rg->values = probe_createExhaustive();

   // Cette probe, par dÃ©finition, ne doit pas Ãªtre resetÃ©e
   probe_setPersistent(rg->values);
}

/*==========================================================================*/
/*   Probes                                                                 */ 
/*==========================================================================*/
void randomGenerator_addValueProbe(struct randomGenerator_t * rg,
				   struct probe_t * p)
{
   rg->valueProbe = probe_chain(p, rg->valueProbe);
}

/*==========================================================================*/
/*   Miscelleanous helper functions                                         */
/*==========================================================================*/

/** 
 * @brief Build values and proba arrays from file
 */
#define ALLOC_STEP 10
void readUIntDiscreteProbaFromFile(char * fileName,
				int * nbValues,
				unsigned int ** values,
				double ** proba)
{
   *nbValues = 0;
   *values = NULL;
   *proba = NULL;

   FILE * f;

   f = fopen(fileName, "r");
   printf("Fichier ouvert\n");
   if (f == NULL) {
      return;
   }
 
   while (!feof(f)) {
      if ((*nbValues % ALLOC_STEP) == 0) {
	*values = realloc(*values, (*nbValues  + ALLOC_STEP)*sizeof(unsigned int));
	*proba  = realloc(*proba, (*nbValues  + ALLOC_STEP)*sizeof(double));
      }

      if (fscanf(f, "%d %lf\n", &(*values)[*nbValues], &(*proba)[*nbValues]) == 2) {
	(*nbValues)++;
      }

   } 
   fclose(f);
}

/**
 * @brief Tell if a random generator is constant
 * @param rg a random generator to test
 * @result non null if rg is constant
 *
 * A random generator is constant if it has been defined as a constant
 * or if has been defined as a discrete distribution with a single
 * value.
 */
int randomGenerator_isConstant(struct randomGenerator_t * rg)
{
  printf_debug(DEBUG_ALWAYS, "%p Value type %d, distribution %d\n", rg, rg->valueType, rg->distribution);

   // Constant types
   if ((rg->valueType == rGTypeDoubleConstant)
     ||(rg->valueType == rGTypeUIntConstant)){
      return 1;
   // Discrete types with single value
   } else if ((rg->distribution == rGDistDiscrete) && ( rg->distParam.d.discrete.nbProba == 1)){
      return 1;
   } else {
      return 0;
   }
}
