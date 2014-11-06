/********************************************************************
 * FILE: fisher_exact.h
 * AUTHOR: Robert McLeay
 * CREATE DATE: 9/10/2008
 * PROJECT: MEME suite
 * COPYRIGHT: 2008, Robert McLeay
 *
 * This is a log space version of Fisher's exact test.
 * It does not use a gamma table.
 * Algorithmic order is O( max(b-a ; d-c)*n + n)
 ********************************************************************/

#ifndef FISHER_EXACT_H_
#define FISHER_EXACT_H_
double* log_factorial;
void fisher_exact_init(int len);
double fet(int a, int b, int c, int d);
void fisher_exact_destruct();
void fisher_exact(int a, //x[0,0]
				  int b, //x[0,1]
				  int c, //x[1,0]
				  int d, //x[1,1]
				  double* two, //two-tailed p-value (out)
				  double* left, //one-tailed left p-value (out)
				  double* right, //one-tailed right p-value (out)
				  double* p //exact p-value of this matrix (out)
				  );

double* fisher_exact_get_log_factorials();
void init_FET();
// Faster Fisher's Exact Test
double getLogFETPvalue(
  double p,     // positive successes
  double P,     // positives
  double n,     // negative successes
  double N,     // negatives
  int fast 	// if TRUE and p-value > 0.5, return log(1)
);

#endif /* FISHER_EXACT_H_ */
