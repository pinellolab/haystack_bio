/**********************************************************************
 * FILE: ramen_scan.c
 * AUTHOR: Fabian Buske / Robert McLeay for refactoring
 * PROJECT: MEME
 * COPYRIGHT: 2007-2008, UQ
 * VERSION: $Revision: 1.0$
 * DESCRIPTION: Routines to perform average motif affinity scans
 *
 **********************************************************************/

#include "assert.h"
#include "ramen_scan.h"

/*************************************************************************
 * Converts the motif frequency matrix into a odds matrix: taken from old ama-scan.c
 *************************************************************************/
void convert_to_odds_matrix(MOTIF_T* motif, ARRAY_T* bg_freqs){
  const int asize = alph_size(get_motif_alph(motif), ALPH_SIZE);
  int motif_position_index,alph_index;
  MATRIX_T *freqs;
  freqs = get_motif_freqs(motif);

  const int num_motif_positions = get_num_rows(freqs);
  for (alph_index=0;alph_index<asize;++alph_index){
    double bg_likelihood = get_array_item(alph_index, bg_freqs);
    for (motif_position_index=0;motif_position_index<num_motif_positions;++motif_position_index){
      freqs->rows[motif_position_index]->items[alph_index] /= bg_likelihood;
    }
  }
}
/*************************************************************************
 * Copies the motif frequency matrix and converts it into a odds matrix
 *************************************************************************/
MATRIX_T* create_odds_matrix(MOTIF_T *motif, ARRAY_T* bg_freqs){
  const int asize = alph_size(get_motif_alph(motif), ALPH_SIZE);
  int pos, aidx;
  MATRIX_T *odds;
  
  odds = duplicate_matrix(get_motif_freqs(motif));
  const int num_pos = get_num_rows(odds);
  for (aidx = 0; aidx < asize; ++aidx) {
    double bg_likelihood = get_array_item(aidx, bg_freqs);
    for (pos = 0; pos < num_pos; ++pos) {
      odds->rows[pos]->items[aidx] /= bg_likelihood;
    }
  }
  return odds;
}


/*************************************************************************
 * Calculate the odds score for each motif-sized window at each
 * site in the sequence using the given nucleotide frequencies.
 *
 * This function is a lightweight version based on the one contained in
 * motiph-scoring. Several calculations that are unnecessary for gomo
 * have been removed in order to speed up the process
 *************************************************************************/
static double score_sequence(
    SEQ_T *seq,         // sequence to scan (IN)
    MOTIF_T *motif,     // motif already converted to odds values (IN)
    PSSM_T *m_pssm,     // motif pssm (IN)
    MATRIX_T *m_odds,   // motif odds (IN)
    int method,         // method used for scoring (IN)
    double threshold,   // Threshold to use in TOTAL_HITS mode with a PWM
    ARRAY_T *bg_freqs   //background model
    )
{

  assert(seq != NULL);
  assert(motif != NULL);
  assert((method == TOTAL_HITS && m_pssm) || (method != TOTAL_HITS && m_odds));

  char* raw_seq = get_raw_sequence(seq);
  int seq_length = get_seq_length(seq);

  // Get the pv lookup table
  ARRAY_T* pv_lookup = NULL;
  if (NULL != m_pssm) {
    pv_lookup = m_pssm->pv;
    assert(get_array_length(pv_lookup) > 0);
  }

  // Prepare storage for the string representing the portion
  // of the reference sequence within the window.
  char* window_seq = (char *) mm_malloc(sizeof(char) * (get_motif_length(motif) + 1));
  window_seq[get_motif_length(motif)] = '\0';

  int max_index = seq_length - get_motif_length(motif);
  if (max_index < 0) max_index = 0;
  const int asize = alph_size(get_motif_alph(motif), ALPH_SIZE);
  double* odds =  (double*) mm_malloc(sizeof(double)*max_index);
  double* scaled_log_odds =  (double*) mm_malloc(sizeof(double)*max_index);

  // For each site in the sequence
  int seq_index;
  for (seq_index = 0; seq_index < max_index; seq_index++) {
    double odd = 1.0;
    scaled_log_odds[seq_index] = 0;

    // For each site in the motif window
    int motif_position;
    for (motif_position = 0; motif_position < get_motif_length(motif); motif_position++) {
      char c = raw_seq[seq_index + motif_position];
      window_seq[motif_position] = c;

      // Check for gaps at this site
      if(c == '-' || c == '.') {
        break;
      }

      // Check for ambiguity codes at this site
      //TODO: This next call is very expensive - it takes up approx. 10% of a
      //      programme's running time. It should be fixed up somehow.
      int aindex = alph_index(get_motif_alph(motif), c);
      if (aindex > asize) {
        break;
      }
      if (method == TOTAL_HITS) {
        //If we're in this mode, then we're using LOG ODDS.
        //scaled_log_odds[seq_index] += get_matrix_cell(motif_position, aindex, get_motif_freqs(motif));
        scaled_log_odds[seq_index] += get_matrix_cell(motif_position, aindex, m_pssm->matrix);
      } else {
        odd *= get_matrix_cell(motif_position, aindex, m_odds);
      }
    }
    odds[seq_index] = odd;
  }

  // return odds as requested (MAX or AVG scoring)
  double requested_odds = 0.0;
  if (method == AVG_ODDS){
    for (seq_index = 0; seq_index < max_index; seq_index++) {
      requested_odds += odds[seq_index];
    }
    requested_odds /= max_index;
  } else if (method == MAX_ODDS){
    for (seq_index = 0; seq_index < max_index; seq_index++) {
      if (odds[seq_index] > requested_odds){
        requested_odds = odds[seq_index];
      }
    }
  } else if (method == SUM_ODDS) {
    for (seq_index = 0; seq_index < max_index; seq_index++) {
      requested_odds += odds[seq_index];
    }
  } else if (method == TOTAL_HITS) {
    for (seq_index = 0; seq_index < max_index; seq_index++) {

      if (scaled_log_odds[seq_index] >= (double)get_array_length(pv_lookup)) {
        scaled_log_odds[seq_index] = (double)(get_array_length(pv_lookup) - 1);
      } 
      double pvalue = get_array_item((int) scaled_log_odds[seq_index], pv_lookup);

      //Figure out how to calculate the p-value of a hit
      //fprintf(stderr, "m: %s pv_l len: %i scaled_log_odds: %g seq index: %i pvalue: %g\n", 
      //    get_motif_id(motif), get_array_length(pv_lookup), scaled_log_odds[seq_index], seq_index, pvalue);

      if (pvalue < threshold) {
        requested_odds++; //Add another hit.
      }

      if (verbosity > HIGHER_VERBOSE) {
        fprintf(stderr, "Window Data: %s\t%s\t%i\t%g\t%g\t%g\n",
            get_seq_name(seq), get_motif_id(motif), seq_index, scaled_log_odds[seq_index], pvalue, threshold);
      }
    }
  }

  myfree(odds);
  myfree(scaled_log_odds);
  myfree(window_seq);
  return requested_odds;
}

/**********************************************************************
  ramen_sequence_scan()

  scan a given sequence with a specified motif using either
  average motif affinity scoring or maximum one. In addition z-scores
  may be calculated.

  The motif has to be converted to log odds in advance (in order
  to speed up the scanning). Either use convert_to_odds_matrix() once for each
  motif and pass in the motif freqs to the odds, or use create_odds_matrix()
  and pass in that matrix as odds.

 **********************************************************************/
double ramen_sequence_scan(
    SEQ_T* sequence,    // the sequence to scan INPUT
    MOTIF_T* motif,     // the motif to scan with INPUT
    MOTIF_T* rev_motif, // the reversed motif
    PSSM_T* pssm,
    PSSM_T* rev_pssm,
    MATRIX_T* odds,
    MATRIX_T* rev_odds,
    int scoring,        // the scoring function to apply AVG_ODDS, MAX_ODDS or TOTAL_HITS
    int zscoring,       // the number of shuffled sequences used for z-score computation INPUT
    BOOLEAN_T scan_both_strands, //Should we scan with both motifs and combine scores
    double threshold,   // Threshold to use in TOTAL_HITS mode with a PWM
    ARRAY_T* bg_freqs   //background model
    ){
  assert(zscoring >= 0);

  // Score the forward strand.
  double odds_sc = score_sequence(
      sequence,
      motif,
      pssm,
      odds,
      scoring,
      threshold,
      bg_freqs
      );

  // Score the reverse strand.
  if (scan_both_strands) {

    double rev_odds_sc = score_sequence(
        sequence,
        rev_motif,
        rev_pssm,
        rev_odds,
        scoring,
        threshold,
        bg_freqs
        );

    if (scoring == AVG_ODDS){
      odds_sc = (odds_sc+rev_odds_sc)/2.0;
    } else if (scoring == MAX_ODDS){
      odds_sc = max(odds_sc,rev_odds_sc);
    } else if (scoring == TOTAL_HITS) {
      odds_sc = odds_sc + rev_odds_sc;
    }

  }

  return odds_sc;

}
