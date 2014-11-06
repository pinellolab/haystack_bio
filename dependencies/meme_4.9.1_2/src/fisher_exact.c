/********************************************************************
 * FILE: fisher_exact.c
 * AUTHOR: Robert McLeay
 * CREATE DATE: 9/10/2008
 * PROJECT: MEME suite
 * COPYRIGHT: 2008, Robert McLeay
 *
 * This is a log space version of the fisher exact test.
 * It does not use a gamma table.
 * Algorithmic order is O( max(b-a ; d-c)*n + n)
 ********************************************************************/

#include "fisher_exact.h"
#include <math.h>
#include <stdlib.h>
#include "utils.h"

//double _mm_nats;
double _mm_nats = 0;
double _log10;
double _log0_99999999;
double _log1_00000001;

// Global constants for hypergeometric computation
#define _log_zero (-1e10)		// Zero on the log scale.
#define _log_small (-0.5e10)		// Threshold below which everything is zero
#define _log_half (-0.69314718)

// Routines for computing the logarithm of a sum in log space.
#define my_exp(x) ( \
  ((x) < _log_small) ? 0.0 : exp(x) \
)
#define log_sum1(logx, logy) ( \
  ((logx) - (logy) > _mm_nats) ? (logx) : (logx) + log(1 + my_exp((logy) - (logx))) \
)
#define log_sum(logx, logy) ( \
  ((logx) > (logy)) ? log_sum1((logx), (logy)) : log_sum1((logy), (logx)) \
)
#define lnfact(n) ( \
  ((n) <= 1) ? 0 : lngamm((n) + 1.0)\
)
#define lnbico(n, k) ( \
  lnfact(n) - lnfact(k) - lnfact((n) - (k)) \
)
#define log_hyper_323(n11, n1_, n_1, n) ( \
  lnbico((n1_), (n11)) + lnbico((n) - (n1_), (n_1) - (n11)) - lnbico((n), (n_1)) \
)
#define log_hyper(n11, f_vals) ( \
  log_hyper0((n11), 0, 0, 0, (f_vals)) \
)

typedef struct fisher_val {
  double _sn11;
  double _sn1_;
  double _sn_1;
  double _sn;
  double _log_sprob;
} FISHER_VAL_T;

// Prototypes.
double lngamm(int z);

void fisher_exact_init(int len) {
  double* p;
  int i;

  p = malloc(sizeof(double) * (len + 1));
  p[0] = 0;
  for (i = 1; i <= len; i++) {
    p[i] = log(i) + p[i - 1];
    //fprintf(stderr, "%i: %g\n", i, p[i]);
  }
  log_factorial = p;
}

double* fisher_exact_get_log_factorials() {
  return log_factorial;
}

double fet(int a, int b, int c, int d) {
  return exp(log_factorial[a + b] + log_factorial[c + d] + log_factorial[a + c] + log_factorial[b + d] - (log_factorial[a + b + c + d] + log_factorial[a] + log_factorial[b] + log_factorial[c] + log_factorial[d]));
}

void fisher_exact(
    int a, //x[0,0]
    int b, //x[0,1]
    int c, //x[1,0]
    int d, //x[1,1]
    double* two, //two-tailed p-value (out)
    double* left, //one-tailed left p-value (out)
    double* right, //one-tailed right p-value (out)
    double* p //exact p-value of this matrix (out)
    ) {

  int tmpa, tmpb, tmpc, tmpd; //used for modifying the tables to be more 'extreme'.

  double tmpp;

  *two = *left = *right = *p = tmpp = 0;

  *p = fet(a, b, c, d);

  tmpa = a;
  tmpb = b;
  tmpc = c;
  tmpd = d;
  *left = *p; //for the current state
  while (tmpa != 0 && tmpd != 0) {
    tmpa--;
    tmpb++;
    tmpc++;
    tmpd--;
    tmpp = fet(tmpa, tmpb, tmpc, tmpd);
    // FIXED tlb: want prob pos succ >= observed
    //if (tmpp <= *p)
    if (tmpp < *p)
      *left += tmpp;
  }

  //reset to go the other way (right)
  tmpa = a;
  tmpb = b;
  tmpc = c;
  tmpd = d;
  *two = *left;
  while (tmpb != 0 && tmpc != 0) {
    tmpa++;
    tmpb--;
    tmpc--;
    tmpd++;
    tmpp = fet(tmpa, tmpb, tmpc, tmpd);
    // FIXED tlb: want prob pos succ <= observed
    //if (tmpp <= *p)
    if (tmpp < *p)
      *two += tmpp;
  }

  // FIXED tlb: Now right is 1 - left - Pr(pos succ == observed)
  *right = 1 - *left + *p;
}

void fisher_exact_destruct() {
  free(log_factorial);
}

// Log gamma function using continued fractions
inline double lngamm(int z) {
  double x = 0.0;
  x = x + 0.1659470187408462e-06 / (z + 7.0);
  x = x + 0.9934937113930748e-05 / (z + 6.0);
  x = x - 0.1385710331296526 / (z + 5.0);
  x = x + 12.50734324009056 / (z + 4.0);
  x = x - 176.6150291498386 / (z + 3.0);
  x = x + 771.3234287757674 / (z + 2.0);
  x = x - 1259.139216722289 / (z + 1.0);
  x = x + 676.5203681218835 / (z);
  x = x + 0.9999999999995183;
  return log(x) - 5.58106146679532777 - z + (z - 0.5) * log(z + 6.5);
}

double log_hyper0(int n11i, int n1_i, int n_1i, int ni, FISHER_VAL_T* f_vals) {
  if (!((n1_i | n_1i | ni) != 0)) {
    if (!(n11i % 10 == 0)) {
      if (n11i == f_vals->_sn11 + 1) {
        f_vals->_log_sprob += 
        // log ( ((_sn1_-_sn11)/float(n11i))*((_sn_1-_sn11)/float(n11i+_sn-_sn1_-_sn_1)) )
         log(((f_vals->_sn1_ - f_vals->_sn11) / n11i) * ((f_vals->_sn_1 - f_vals->_sn11) 
           / (n11i + f_vals->_sn - f_vals->_sn1_ - f_vals->_sn_1)));
        f_vals->_sn11 = n11i;
        return f_vals->_log_sprob;
      }
      if (n11i == f_vals->_sn11 - 1) {
        f_vals->_log_sprob += 
          //  log ( ((_sn11)/float(_sn1_-n11i))*((_sn11+_sn-_sn1_-_sn_1)/float(_sn_1-n11i)) )
          log(((f_vals->_sn11) / (f_vals->_sn1_ - n11i)) * ((f_vals->_sn11 + f_vals->_sn - f_vals->_sn1_ - f_vals->_sn_1) 
           / (f_vals->_sn_1 - n11i)));
        f_vals->_sn11 = n11i;
        return f_vals->_log_sprob;
      }
    }
    f_vals->_sn11 = n11i;
  }
  else {
    f_vals->_sn11 = n11i;
    f_vals->_sn1_ = n1_i;
    f_vals->_sn_1 = n_1i;
    f_vals->_sn = ni;
  }
  f_vals->_log_sprob = log_hyper_323(f_vals->_sn11, f_vals->_sn1_, f_vals->_sn_1, f_vals->_sn);
  return f_vals->_log_sprob;
}

void init_FET() {
  _log10 = log(10);
  _log0_99999999 = log(0.9999999);
  _log1_00000001 = log(1.00000001);
  _mm_nats = log(-_log_zero);
}

/*
 * Main function for hypergeomteric computation with:
 *   a1: red balls - red balls drawn
 *   a2: red balls drawn
 *   b1: green balls - green balls drawn
 *   b2: green balls drawn
 * Returns Pr(green balls drawn >= b2)
 */
static double log_getFETprob(int a1, int a2, int b1, int b2) {

  // initialize values
  if (_mm_nats == 0) init_FET();
  FISHER_VAL_T *fisher_values = mm_malloc(sizeof(FISHER_VAL_T));
  fisher_values->_log_sprob = 0;
  fisher_values->_sn = 0;
  fisher_values->_sn11 = 0;
  fisher_values->_sn1_ = 0;
  fisher_values->_sn_1 = 0;

  double log_sless = _log_zero;
  double log_sright = _log_zero;
  double log_sleft = _log_zero;
  double log_slarg = _log_zero;
  int n = a1 + a2 + b1 + b2;
  int row1 = a1 + a2;
  int col1 = a1 + b1;
  int max = row1;

  if (a1+a2 == 0 || b1+b2 == 0) return(0); // p-value == 1 if no positive/negative samples
  if (a1+b1 == 0 || a2+b2 == 0) return(0); // p-value == 1 if no successes/failures samples

  if (col1 < max) {
    max = col1;
  }
  int min = row1 + col1 - n;
  if (min < 0) min = 0;
  if (min == max) return(_log_zero);

  double log_prob_fisher = log_hyper0(a1, row1, col1, n, fisher_values);
  log_sleft = _log_zero;
  double log_p = log_hyper(min, fisher_values);

  int i = min + 1;
  while (log_p < _log0_99999999 + log_prob_fisher) {
    log_sleft = log_sum(log_sleft, log_p);
    log_p = log_hyper(i, fisher_values);
    i = i + 1;
  }

  i = i - 1;
  if (log_p < _log1_00000001 + log_prob_fisher) {
    log_sleft = log_sum(log_sleft, log_p);
  }
  else {
    i = i - 1;
  }

  log_sright = _log_zero;
  log_p = log_hyper(max, fisher_values);

  int j = max - 1;
  while (log_p < _log0_99999999 + log_prob_fisher) {
    log_sright = log_sum(log_sright, log_p);
    log_p = log_hyper(j, fisher_values);
    j = j - 1;
  }

  j = j + 1;
  if (log_p < _log1_00000001 + log_prob_fisher) {
    log_sright = log_sum(log_sright, log_p);
  }
  else {
    j = j + 1;
  }

  if (abs(i - a1) < abs(j - a1)) {
    log_sless = log_sleft;
    log_slarg = log(1.0 - exp(log_sleft));
    log_slarg = log_sum(log_slarg, log_prob_fisher);
  }
  else {
    log_sless = log(1.0 - exp(log_sright));
    log_sless = log_sum(log_sless, log_prob_fisher);
    log_slarg = log_sright;
  }
  free(fisher_values);
  return log_slarg;
}

/*
 getLogFETPvalue

 Return log of hypergeometric pvalue of # pos successes >= p.
 */
double getLogFETPvalue(
    double p, // positive successes
    double P, // positives
    double n, // negative successes
    double N, // negatives
    int fast // if TRUE and p-value > 0.5, return log(1)
    ) {
  double log_pvalue;
  // if doing fast, check if p-value is greater than 0.5
  if (fast && (p * N < n * P)) {
    log_pvalue = 0; // pvalue = 1
  }
  else {
    //  apply Fisher Exact test (hypergeometric p-value)
    log_pvalue = log_getFETprob(N - n, n, P - p, p);
  }

  return (log_pvalue);
} // getLogFETPvalue


#ifdef FE_MAIN
#include "general.h"
int main(
  int argc,
  char** argv
) {
  int i = 1; 
  int pos_succ = 0;
  int pos = 0;
  int neg_succ = 0;
  int neg = 0;

  DO_STANDARD_COMMAND_LINE(2,
    USAGE(<pos_succ> <pos> <neg_succ> <neg> [options]);
    NON_SWITCH(1,\r,
      switch (i++) {
        case 1: pos_succ = atoi(_OPTION_); break;
        case 2: pos = atoi(_OPTION_); break;
        case 3: neg_succ = atoi(_OPTION_); break;
        case 4: neg = atoi(_OPTION_); break;
        default: COMMAND_LINE_ERROR;
      }
    );
    USAGE(\n\tCompute the Fisher Exact test p-value:);
    USAGE(\n\t\tPr(#pos_succ > pos_succ));
  );

  double log_pvalue = log_getFETprob(neg - neg_succ, neg_succ, pos - pos_succ, pos_succ);
  printf("log_p %.2e p %.2e\n", log_pvalue, exp(log_pvalue));

  //fisher_exact_init(pos+neg);
  //double p1, p2, p3, p4;
  //fisher_exact(neg - neg_succ, neg_succ, pos - pos_succ, pos_succ, &p1, &p2, &p3, &p4);
  //printf("p1 %.2e p2 %.2e p3 %.2e p4 %.2e\n", p1, p2, p3, p4);

  return(0);
}
#endif
