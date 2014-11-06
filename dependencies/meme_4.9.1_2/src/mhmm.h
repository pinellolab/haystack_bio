/**************************************************************************
 * FILE: mhmm.h
 * AUTHOR: William Stafford Noble, Timothy L. Bailey, and Charles E. Grant
 * CREATE DATE: 07-27-2007
 * PROJECT: MHMM
 * DESCRIPTION: Create a HMM from MEME motif and motif occurrence information.
 **************************************************************************/
#ifndef MHMM_H
#define MHMM_H

#include "motif.h"
#include "order.h"
#include "string-list.h"

#define MOTIF_PLUS 1
#define MOTIF_MINUS 2
/*************************************************************************
 * store a motif number in a passed set with the strands of it that
 * have been requested.
 * Returns true on success.
 *************************************************************************/
BOOLEAN_T store_requested_num(RBTREE_T *request_nums, const char *value);

/*************************************************************************
 * store a motif identifier in a passed set with the strands of it that
 * have been requested.
 * Returns true on success.
 *************************************************************************/
BOOLEAN_T store_requested_id(RBTREE_T *request_ids, char *value);

/***********************************************************************
 * Read the motifs, filter them, create an order and spacing if
 * possible and create the transition frequency matrix and the
 * spacer average matrix.
 ***********************************************************************/
void load_filter_process_motifs_for_hmm
(
  int request_n, // IN select the top n positive strand motifs
  RBTREE_T *request_nums, // IN select the requested motif numbers
  RBTREE_T *request_ids, // IN select the requested motif ids
  double request_ev, // IN select motifs with a <= evalue
  double request_complexity, // IN select motifs with a >= complexity
  const char *request_order, // IN specifiy motifs and their ordering and spacing
  double hit_pv, // IN specify the p value for keeping motif occurences
  const char *motif_filename, // IN file containing motifs to load
  const char *bg_source, // IN source of background
  double motif_pseudo, // IN pseudocount applied to motifs
  double trans_pseudo,  // IN pseudocount applied to transitions
  double spacer_pseudo, // IN pseudocount applied to spacers
  BOOLEAN_T keep_unused, // IN keep the unused motifs
  ALPH_T *alphabet, // OUT alphabet of the motifs
  RBTREE_T **motifs, // OUT motifs with file position as key
  ARRAY_T **background, // OUT background from motif file
  ORDER_T **order_spacing, // OUT order and spacing (uses motif key as identifiers)
  MATRIX_T **transp_freq, // OUT transition frequencies
  MATRIX_T **spacer_ave // OUT spacer averages
);

#endif
