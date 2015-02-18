/**
 * @file random-generator.h
 * @brief Les générateurs de nombres aléatoires.
 *
 * Un gÃ©nÃ©rateur est caractÃ©risÃ© par plusieurs propriÃ©tÃ©s
 *
 * - Le type des valeurs gÃ©nÃ©rÃ©es.
 * - La distribution des probabilitÃ©s
 *    . Les paramÃ¨tres de cette distribution : min, max, moyenne, ...
 * - La source d'alÃ©a. Certaines permettent un rejeu (rand unix)
 *   d'autres sont "vraiment alÃ©atoires" (/dev/random)
 *    . Les paramÃ¨tres de cette source (seed, ...)
 */

#ifndef __DEF_RANDOM_GENERATOR
#define __DEF_RANDOM_GENERATOR

#include <probe.h>

struct randomGenerator_t;

/*
 * Available types for the random values
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

/*
 * Available distributions
 */
#define rGDistNoDist       0
#define rGDistUniform      1
#define rGDistExponential  2
#define rGDistDiscrete     3
#define rGDistITS          4

#define rGDistDefault rGDistUniform
/*
 * Entropy sources
 */
#define rGSourceErand48 1
#define rGSourceReplay  2
#define rgSourceUrandom 3

#define rgSourceDefault rGSourceErand48

/*==========================================================================*/
/*  Creators                                                                */
/*==========================================================================*/

/*
 *  Creators without distribution
 */
// Des entiers non signÃ©s quelconques
struct randomGenerator_t * randomGenerator_createUInt();

// Des entiers non signÃ©s entre min et max  (inclus)
struct randomGenerator_t * randomGenerator_createUIntRange(unsigned int min,
						      unsigned int max);

// Des entiers non signÃ©s tous égaux !
struct randomGenerator_t * randomGenerator_createUIntConstant(unsigned int v);

// Des entiers non signÃ©s listÃ©s
struct randomGenerator_t * randomGenerator_createUIntDiscrete(int nbValues,
							      unsigned int * values);
// Des réels double précision
struct randomGenerator_t * randomGenerator_createDouble();

// Des réels double précision tous égaux ...
struct randomGenerator_t * randomGenerator_createDoubleConstant(double v);


/*
 * Creators with a pre defined distribution
 */

/**
 * @brief Création d'une ditribution d'après un fichier
 */
struct randomGenerator_t * randomGenerator_createUIntDiscreteFromFile(char * fileName);

/*
 * Le nombre de valeurs possibles est passÃ© en paramÃ¨tre ainsi que la
 * liste de ces valeurs puis la liste de leurs probabilitÃ©.
 */
struct randomGenerator_t * randomGenerator_createUIntDiscreteProba(int nbValues,
                                     unsigned int * values, double * proba);
 
// Des entiers longs non signÃ©s
struct randomGenerator_t * randomGenerator_createULong(int distribution,
						       unsigned long min,
						       unsigned long max);

// Des rÃ©els double prÃ©cision, avec une distribution exp de paramÃ¨tre lambda
struct randomGenerator_t * randomGenerator_createDoubleExp(double lambda);

/* 
 * A double range [min .. max]
 */
struct randomGenerator_t * randomGenerator_createDoubleRange(double min,
							     double max);

struct randomGenerator_t * randomGenerator_createDoubleDiscrete(
                                     int nbValues,
                                     double * values);
 
struct randomGenerator_t * randomGenerator_createDoubleDiscreteProba(
                                     int nbValues,
                                     double * values,
                                     double * proba);
 
/*==========================================================================*/

// Use a (previously built) probe to re-run a sequence
struct randomGenerator_t * randomGenerator_createFromProbe(struct probe_t * p);

void randomGenerator_reset(struct randomGenerator_t * rg);

/*
 * Destructor
 */
void randomGenerator_delete(struct randomGenerator_t * rg);

/*==========================================================================*/
/*   Select distribution                                                    */
/*==========================================================================*/
/*
 * Choix de la distribution
 */

/*
 * Un nombre discret de probabilitÃ©s
 */
void randomGenerator_setDistributionDiscrete(struct randomGenerator_t * rg,
					     int nb,
                                             double * proba);

/**
 * @brief Création d'une distribution uniforme construite depuis un fichier
 *
 * Le fichier doit être un fichier texte orienté ligne où chaque ligne
 * contient un entier (la valeur) suivi d'un réel (la probabilité associée).
 */ 
void randomGenerator_setDistributionDiscreteFromFile(struct randomGenerator_t * rg,
						     char * fileName);


// Choix d'une loi uniforme
void randomGenerator_setDistributionUniform(struct randomGenerator_t * rg);

// Choix d'une loi exponentielle
void randomGenerator_setDistributionExp(struct randomGenerator_t * rg, double lambda);

/**
 * @brief Set a pareto distribution
 * @param rg The random generator to be modified
 * @param alpha The shape of the pareto distribution
 * @param xmin The scale of the pareto distribution
 */
#define randomGenerator_setDistributionPareto(rg, alpha, xmin) \
   randomGenerator_setQuantile2Param(rg, randomGenerator_paretoDistQ, alpha, xmin)

/**
 * @brief Define a distribution by its quantile function for inverse
 * transform sampling
 * @param rg The random generator
 * @param q The inverse cumulative density (quantile) function
 * @param p1 The single parameter of the quantile function
 */
void randomGenerator_setQuantile1Param(struct randomGenerator_t * rg,
				       double (*q)(double x, double p),
				       double p);

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
				       double p1, double p2);
/**
 * @brief Inverse of CDF for exponential distribution
 */
double randomGenerator_expDistQ(double x, double lambda);

/**
 * @brief Inverse of CDF for pareto distribution
 */
double randomGenerator_paretoDistQ(double x, double alpha, double xmin);

/*
 * Change lambda
 */
void randomGenerator_setLambda(struct randomGenerator_t * rg, double lambda);


/*
 * Prepare for record values in order to replay on each reset
 */
void randomGenerator_recordThenReplay(struct randomGenerator_t * rg);

/*
 * Value generation
 */
unsigned int randomGenerator_getNextUInt(struct randomGenerator_t * rg);
double randomGenerator_getNextDouble(struct randomGenerator_t * rg);

/*
 * Obtention de certains paramÃ¨tres. Il s'agit ici de valeurs
 * thÃ©oriques, pour obtenir leurs Ã©quivalents sur une sÃ©rie
 * d'expÃ©riences, on utilisera des sondes.
 */
double randomGenerator_getExpectation(struct randomGenerator_t * rg);

/**
 * @brief Tell if a random generator is constant
 * @param rg a random generator to test
 * @result non null if rg is constant 
 *
 * A random generator is constant if it has been defined as a constant
 * or if has been defined as a discrete distribution with a single
 * value.
 */
int randomGenerator_isConstant(struct randomGenerator_t * rg);


/*==========================================================================*/
/*   Probes                                                                 */ 
/*==========================================================================*/
/**
 * @brief Ajout d'une sonde sur les valeurs générées
 */

void randomGenerator_addValueProbe(struct randomGenerator_t * rg,
				   struct probe_t * p);

#endif
