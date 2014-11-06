/**************************************************************************
 * FILE: binomial.c
 * AUTHOR: James Johnson 
 * CREATE DATE: 06-December-2011 
 * PROJECT: shared
 * COPYRIGHT: UQ, 2011
 * VERSION: $Revision: 1.0 $
 * DESCRIPTION: Calculates binomial distributions
 **************************************************************************/
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
/**************************************************************************
 * binomial exact
 **************************************************************************/
double binomial_exact(int succ, int trials, double prob) {
  double out;
  //note: 
  //gamma(x + 1) = x!
  //lgamma(x) = log(gamma(x)) = log((x-1)!)
  out = lgamma(trials + 1) - lgamma(trials - succ + 1) - lgamma(succ + 1) + 
    log(prob) * succ + log(1.0-prob) * (trials - succ) ;

  return exp(out);
}

/**************************************************************************
 * compute a pvalue by summing the tail of a binomial distribution
 **************************************************************************/
double one_minus_binomial_cdf(double probability, int successful_trials, int total_trials) {
  double sum;
  int x;
  for (x = successful_trials, sum = 0; x <= total_trials; ++x) {
    sum += binomial_exact(x, total_trials, probability);
  }
  return sum;
}

/**************************************************************************
 * Evaluates the continued fraction for the regularized incomplete beta
 * function by modified Lentz's method.
 *
 * See "Numerical Recipes in C" section 6.4 page 226.
 **************************************************************************/
static const int MAXIT = 1000; // upped from 100: needed in rare low p-val cases
static const double EPS = 3.0e-7;
static const double FPMIN = 1.0e-300; // note original used 1.0e-30
static double betacf(double a, double b, double x) {
  int m, m2;
  double aa, c, d, del, h, qab, qam, qap;

  // These q's will be used in factors that occur in the coefficients
  qab = a + b;
  qap = a + 1.0;
  qam = a - 1.0;
  // First step of Lentz's method
  c = 1.0;
  d = 1.0 - qab*x/qap;
  if (fabs(d) < FPMIN) d = FPMIN;
  d = 1.0/d;
  h = d;
  for (m = 1; m <= MAXIT; m++) {
    m2 = 2*m;
    // One step (the even one) of the recurrence.
    aa = m*(b - m)*x / ((qam + m2) * (a + m2));
    d = 1.0 + aa * d;
    if (fabs(d) < FPMIN) d = FPMIN;
    c = 1.0 + aa/c;
    if (fabs(c) < FPMIN) c = FPMIN;
    d = 1.0 / d;
    h *= d*c;
    // Next step of the recurrence (the odd one).
    aa = -(a+m)*(qab + m)*x/((a + m2) * (qap + m2));
    d = 1.0 + aa*d;
    if (fabs(d) < FPMIN) d = FPMIN;
    c = 1.0 + aa/c;
    if (fabs(c) < FPMIN) c = FPMIN;
    d = 1.0 / d;
    del = d*c;
    h *= del;
    if (fabs(del - 1.0) < EPS) break; // Are we done?
  }
  if (m > MAXIT) {
    fprintf(stderr, "a (%g) or b (%g) too big, or MAXIT (%d) too small in"
        " betacf. Input x is %g. Returning %g.\n",
        a, b, MAXIT, x, h);
  }
  return h;
}

/**************************************************************************
 * Compute the regularized incomplete beta function.
 * This makes use of the continued fraction representation.
 *
 * See "Numerical Recipes in C" section 6.4 page 226.
 **************************************************************************/
double betai(double a, double b, double x) {
  double bt;
  
  if (x < 0.0 || x > 1.0) {
    fprintf(stderr, "Bad x in routine betai");
    exit(EXIT_FAILURE);
  }
  if (x == 0.0 || x == 1.0) {
    bt = 0.0;
  } else {
    bt = exp(lgamma(a + b) - lgamma(a) - lgamma(b)+ a*log(x)+ b*log(1.0 - x));
  }
  // to ensure the quick convergance of the 
  // continued fraction use the symmetry relation:
  // I_x(a, b) = 1 - I_(1-x)(b, a)
  if (x < ((a + 1.0) / (a + b + 2.0))) {
    return bt * betacf(a, b, x) / a;
  } else {
    return 1.0 - bt * betacf(b, a, 1.0 - x) / b;
  }
}

/**************************************************************************
 * Compute the regularized incomplete beta function in log space.
 * This is a modification of the above betai.
 **************************************************************************/
double log_betai(double a, double b, double x) {
  double log_bt;

  if (x < 0.0 || x > 1.0)  {
    fprintf(stderr, "Bad x in routine betai");
    exit(EXIT_FAILURE);
  }
  if (x == 0.0 || x == 1.0) {
    log_bt = 1.0;
  } else {
    log_bt = lgamma(a + b) - lgamma(a) - lgamma(b)+ a*log(x)+ b*log(1.0 - x);
  }
  // to ensure the quick convergance of the 
  // continued fraction use the symmetry relation:
  // I_x(a, b) = 1 - I_(1-x)(b, a)
  if (x < ((a + 1.0) / (a + b + 2.0))) {
    return log_bt + log(betacf(a, b, x)/a);
  } else {
    // we believe that (exp(log_bt) * betacf(b, a, 1.0 - x) / b) 
    // will not be very close to 1 or 0 in this case
    // as the original author would not have subtracted it from 1
    // because that would have resulted in restricting the number
    // to the limits of precision of floating point or 1e-16 and
    // we don't think he's stupid.
    return log(1.0 - exp(log_bt) * betacf(b, a, 1.0 - x) / b);
  }
}
