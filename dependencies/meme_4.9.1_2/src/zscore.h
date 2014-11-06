/**
 * @file zscore.h
 *
 * This module computes the z-score by supplying the population
 * from which the sample mean and deviation is determined
 *
 * author: Fabian Buske
 * version: 1.0 (26.06.2008)
 *
*/
#ifndef ZSCORE_H_
#define ZSCORE_H_

typedef struct zscore ZSCORE_T;

/**********************************************************************
  allocate_zscore()

  Constructor for the zscore data structure
**********************************************************************/
ZSCORE_T *allocate_zscore();

/**********************************************************************
  get_population_details()

  Returns the population details for the given zscore
**********************************************************************/
void get_population_details(
		ZSCORE_T* zscore,	// population of interest (IN)
		double* mean, 		// mean of population (OUT)
		double* sd			// standard deviation of population (OUT)
);

/**********************************************************************
  get_zscore()

  Compute the z-score from the mean and standard deviation.
  Returns the z-score for x.
**********************************************************************/
double get_zscore(
		double x,	 	/* the value for which a z-score is requested */
		double mean, 	/* the mean */
		double sd		/* the standard deviation */
);

/**********************************************************************
  get_zscore_z()

  Compute the z-score for x given a zscore struct.
**********************************************************************/
double get_zscore_z(
		ZSCORE_T* zscore, 	/* zscore struct holding mean and sd */
		double x			/* the value for which a z-score is requested */
);

/**********************************************************************
  destroy_zscore()

  Frees the memory of a ZSCORE_T* data structure
**********************************************************************/
void destroy_zscore(ZSCORE_T *zscore);

#endif /*ZSCORE_H_*/
