/**********************************************************************
 * FILE: ama_scan.j
 * AUTHOR: Fabian Buske / Robert McLeay for refactoring
 * PROJECT: MEME
 * COPYRIGHT: 2007-2008, UQ
 * VERSION: $Revision: 1.0$
 * DESCRIPTION: Routines to perform average motif affinity scans
 *
 **********************************************************************/

#ifndef RAMEN_SCAN_H_
#define RAMEN_SCAN_H_

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <float.h>
#include <math.h>
#include "matrix.h"
#include "string-list.h"
#include "macros.h"
#include "motif.h"
#include "matrix.h"
#include "motif.h"
#include "pssm.h"
#include "array.h"
#include "seq.h"
#include "alphabet.h"
#include "cisml.h"
#include "zscore.h"
#include "assert.h"

extern char* motif_name; // Use this motif name in the output.

extern VERBOSE_T verbosity;

#define AVG_ODDS 0
#define MAX_ODDS 1
#define SUM_ODDS 2
#define TOTAL_HITS 3

#define min(a,b) (a<b)?a:b
#define max(a,b) (a>b)?a:b

/*************************************************************************
 * Converts the motif frequency matrix into a odds matrix
 *************************************************************************/
void convert_to_odds_matrix(
    MOTIF_T* motif,    // motif to be converted
    ARRAY_T* bg_freqs  // background frequencies to use
    );

/*************************************************************************
 * Copies the motif frequency matrix and converts it into a odds matrix.
 * Caller is responsible for freeing memory.
 *************************************************************************/
MATRIX_T* create_odds_matrix(
    MOTIF_T *motif,   // motif to copy the positional frequencies from
    ARRAY_T* bg_freqs // background frequencies for converting into odds
    );

/**********************************************************************
  ramen_sequence_scan()

  scan a given sequence with a specified motif using either
  average motif affinity scoring or maximum one. In addition z-scores
  may be calculated.

  The motif has to be converted to log odds in advance (in order
  to speed up the scanning). Use convert_to_odds_matrix() once for each
  motif.
 **********************************************************************/
double ramen_sequence_scan(
    SEQ_T* sequence,    // the sequence to scan INPUT
    MOTIF_T* motif,     // the motif to scan with (converted to odds matrix) INPUT
    MOTIF_T* rev_motif, // the reversed motif
    PSSM_T* pssm,       // forward pssm - only for TOTALHITS
    PSSM_T* rev_pssm,   // reversed pssm - only for TOTALHITS
    MATRIX_T* odds,
    MATRIX_T* rev_odds,
    int scoring,        // the scoring function to apply AVG_ODDS or MAX_ODDS INPUT
    int zscoring,       // the number of shuffled sequences used for z-score computation INPUT
    BOOLEAN_T scan_both_strands, //Should we scan with both motifs and combine scores
    double threshold,   // Threshold to use in TOTAL_HITS mode with a PWM
    ARRAY_T* bg_freqs   //background model
    );

#endif /* RAMEN_SCAN_H_ */
