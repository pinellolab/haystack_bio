/**************************************************************************
 * FILE: mcast_match.h
 * AUTHOR: William Stafford Noble, Timothy L. Bailey, James Johnson 
 *         and Charles Grant
 * CREATE DATE: 2011-12-02
 * PROJECT: MEME Suite
 * DESCRIPTION: Data structures for MCAST sequence-motif cluster match scoring.
 **************************************************************************/
#ifndef MCAST_MATCH_H
#define MCAST_MATCH_H

#include <stdio.h>
#include "heap.h"
#include "mhmmscan.h"
#include "utils.h"

typedef struct mcast_match_t MCAST_MATCH_T;
typedef struct motif_hit_t MOTIF_HIT_T;

/***********************************************************************
 * allocate_mcast_match
 *
 * This function generates a new mcast match object.
 ***********************************************************************/
MCAST_MATCH_T *allocate_mcast_match();

/***********************************************************************
 * copy_mcast_match
 *
 * This function deep copies an mcast match object.
 *
 ***********************************************************************/
void *copy_mcast_match(void *m);

/***********************************************************************
 * free_mcast_match
 *
 * This function frees the memory associated with a mcast_matc object.
 ***********************************************************************/
void free_mcast_match(void* m);

/***********************************************************************
 * add_mcast_match_motif_hit
 *
 * This function adds a motif_hit to an mcast_match.
 * Freeing the mcast_match will free the motif_hits that have been added.
 ***********************************************************************/
void add_mcast_match_motif_hit(
    MOTIF_HIT_T *motif_hit, 
    MCAST_MATCH_T* mcast_match
);

/***********************************************************************
 * print_mcast_match
 *
 * This function prints the data from mcast_match object
 *
 ***********************************************************************/
void print_mcast_match(FILE *output, void *m);

/**********************************************************************
  print_cisml_from_mcast_matches

  Print a heap of mcast_matches asCisML XML
**********************************************************************/
void print_mcast_matches_as_cisml(
  FILE* out,
  BOOLEAN_T stats_available,
  int num_matches,
  MCAST_MATCH_T **mcast_matches,
  MHMMSCAN_OPTIONS_T *options
);

/***********************************************************************
 * compare_mcast_matches
 *
 * This function compares two mcast match objects.
 * mcast match objects are ordered by score from lower to higher.
 * This function returns 
 *   1 if match1->pvalue > match2.pvalue,
 *   0 if match1->pvalue = match2.pvalue,
 *  -1 if match1->pvalue < match2.pvalue,
 *
 ***********************************************************************/
int compare_mcast_matches(void *m1, void *m2);

/***********************************************************************
 * set_mcast_match_seq_name
 *
 * This function sets the seq_name for an MCAST match object.
 *
 ***********************************************************************/
void set_mcast_match_seq_name(char *name, MCAST_MATCH_T *match);

/***********************************************************************
 * get_mcast_match_seq_name
 *
 * This function returns the seq_name from an MCAST match object.
 *
 ***********************************************************************/
char *get_mcast_match_seq_name(MCAST_MATCH_T *match);

/***********************************************************************
 * set_mcast_match_sequence
 *
 * This function sets the sequence for an MCAST match object.
 *
 * The string will be duplicated and freed when the mcatch object 
 * is destroyed.
 *
 ***********************************************************************/
void set_mcast_match_sequence(char *sequence, MCAST_MATCH_T *match);

/***********************************************************************
 * get_mcast_match_sequence
 *
 * This function returns the sequence from an MCAST match object.
 *
 * The caller should NOT free the returned string. It will be freed
 * when the match object is destroyed.
 *
 ***********************************************************************/
char *get_mcast_match_sequence(MCAST_MATCH_T *match);

/***********************************************************************
 * set_mcast_match_score
 *
 * This function sets the score for an MCAST match object.
 *
 ***********************************************************************/
void set_mcast_match_score(double score, MCAST_MATCH_T *match);

/***********************************************************************
 * get_mcast_match_score
 *
 * This function returns the score from an MCAST match object.
 *
 ***********************************************************************/
double get_mcast_match_score(MCAST_MATCH_T *match);

/***********************************************************************
 * set_mcast_match_evalue
 *
 * This function sets the evalue for an MCAST match object.
 *
 ***********************************************************************/
void set_mcast_match_evalue(double evalue, MCAST_MATCH_T *match);

/***********************************************************************
 * get_mcast_match_evalue
 *
 * This function returns the evalue from an MCAST match object.
 *
 ***********************************************************************/
double get_mcast_match_evalue(MCAST_MATCH_T *match);

/***********************************************************************
 * set_mcast_match_pvalue
 *
 * This function sets the pvalue for an MCAST match object.
 *
 ***********************************************************************/
void set_mcast_match_pvalue(double pvalue, MCAST_MATCH_T *match);

/***********************************************************************
 * get_mcast_match_pvalue
 *
 * This function returns the pvalue from an MCAST match object.
 *
 ***********************************************************************/
double get_mcast_match_pvalue(MCAST_MATCH_T *match);

/***********************************************************************
 * set_mcast_match_qvalue
 *
 * This function sets the qvalue for an MCAST match object.
 *
 ***********************************************************************/
void set_mcast_match_qvalue(double qvalue, MCAST_MATCH_T *match);

/***********************************************************************
 * get_mcast_match_qvalue
 *
 * This function returns the qvalue from an MCAST match object.
 *
 ***********************************************************************/
double get_mcast_match_qvalue(MCAST_MATCH_T *match);

/***********************************************************************
 * set_mcast_match_start
 *
 * This function sets the match start position for an MCAST match object.
 *
 ***********************************************************************/
void set_mcast_match_start(int start, MCAST_MATCH_T *match);

/***********************************************************************
 * get_mcast_match_start
 *
 * This function returns the match start position from an MCAST match object.
 *
 ***********************************************************************/
int get_mcast_match_start(MCAST_MATCH_T *match);

/***********************************************************************
 * set_mcast_match_stop
 *
 * This function sets the match stop position for an MCAST match object.
 *
 ***********************************************************************/
void set_mcast_match_stop(int stop, MCAST_MATCH_T *match);

/***********************************************************************
 * get_mcast_match_stop
 *
 * This function returns the match start position from an MCAST match object.
 *
 ***********************************************************************/
int get_mcast_match_stop(MCAST_MATCH_T *match);

/***********************************************************************
 * allocate_motif_hit
 *
 * This function generates a new motif hit object.
 ***********************************************************************/
MOTIF_HIT_T *allocate_motif_hit(
  char *motif_id, 
  char *seq, 
  char strand, 
  int start, 
  int stop, 
  double pvalue
);

/***********************************************************************
 * free_motif_hit
 *
 * This function frees the memory associated with a motif hit object.
 ***********************************************************************/
void free_motif_hit(MOTIF_HIT_T* motif_hit);

/***********************************************************************
 * get_motif_hit_motif_id
 *
 * This function returns a pointer to a string containing the motif id
 * associated with a motif hit object. 
 *
 * The caller is NOT responsible for freeing the returnted string.
 * It will be freed when the motif hit is freed.
 *
 ***********************************************************************/
const char* get_motif_hit_motif_id(MOTIF_HIT_T* motif_hit);

/***********************************************************************
 * get_motif_hit_seq
 *
 * This function returns a pointer to a string containing the motif id
 * associated with a motif hit object. 
 *
 * The caller is NOT responsible for freeing the returnted string.
 * It will be freed when the motif hit is freed.
 *
 ***********************************************************************/
const char* get_motif_hit_seq(MOTIF_HIT_T* motif_hit);

/***********************************************************************
 * get_motif_hit_strand
 *
 * This function returns the strand assocaited with a motif hit object.
 *
 ***********************************************************************/
char get_motif_hit_strand(MOTIF_HIT_T* motif_hit);

/***********************************************************************
 * get_motif_hit_start
 *
 * This function returns the starting position assocaited with 
 * a motif hit object.
 *
 ***********************************************************************/
int get_motif_hit_start(MOTIF_HIT_T* motif_hit);

/***********************************************************************
 * get_motif_hit_stop
 *
 * This function returns the stop position assocaited with 
 * a motif hit object.
 *
 ***********************************************************************/
int get_motif_hit_stop(MOTIF_HIT_T* motif_hit);

/***********************************************************************
 * get_motif_hit_pvalue
 *
 * This function returns the pvalue assocaited with a motif hit object.
 *
 ***********************************************************************/
double get_motif_hit_pvalue(MOTIF_HIT_T* motif_hit);

#endif
