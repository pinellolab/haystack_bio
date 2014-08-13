/**************************************************************************
 * FILE: binomial.h
 * AUTHOR: James Johnson 
 * CREATE DATE: 06-December-2011 
 * PROJECT: shared
 * COPYRIGHT: UQ, 2011
 * VERSION: $Revision: 1.0 $
 * DESCRIPTION: Calculates binomial distributions
 **************************************************************************/

/**************************************************************************
 * binomial exact
 **************************************************************************/
double binomial_exact(int succ, int trials, double prob);

/**************************************************************************
 * compute a pvalue by summing the tail of a binomial distribution
 **************************************************************************/
double one_minus_binomial_cdf(double probability, int successful_trials, int total_trials);

/**************************************************************************
 * Compute the regularized incomplete beta function.
 * This makes use of the continued fraction representation.
 *
 * See "Numerical Recipes in C" section 6.4 page 226.
 **************************************************************************/
double betai(double a, double b, double x);

/**************************************************************************
 * Compute the regularized incomplete beta function in log space.
 * This is a modification of the above betai.
 **************************************************************************/
double log_betai(double a, double b, double x);
