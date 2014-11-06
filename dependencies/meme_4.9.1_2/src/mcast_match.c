#include <assert.h>
#include "mcast_match.h"
#include "object-list.h"
#include "utils.h"
#include "cisml.h"

struct motif_hit_t {
  char *motif_id;
  char *seq;
  int start;
  int stop;
  char strand;
  double pvalue;
};

struct mcast_match_t {
  int cluster_id;
  OBJECT_LIST_T *motif_hits;
  char *seq_name;
  char *sequence;
  double score;
  double evalue;
  double pvalue;
  double qvalue;
  int start;
  int stop;
};

/***********************************************************************
 * allocate_mcast_match
 *
 * This function generates a new mcast match object.
 ***********************************************************************/
MCAST_MATCH_T *allocate_mcast_match() {

  static int cluster_id = 0;
  MCAST_MATCH_T *mcast_match = mm_malloc(sizeof(MCAST_MATCH_T));
  ++cluster_id;
  mcast_match->cluster_id = cluster_id;

  mcast_match->motif_hits
    = new_object_list(NULL, NULL, NULL, free_motif_hit);
  mcast_match->seq_name = NULL;
  mcast_match->sequence = NULL;
  mcast_match->score = -1.0;
  mcast_match->evalue = NaN();
  mcast_match->pvalue = NaN();
  mcast_match->qvalue = NaN();
  mcast_match->start = -1;
  mcast_match->stop = -1;

  return mcast_match;
}

/***********************************************************************
 * copy_mcast_match
 *
 * This function deep copies an mcast match object.
 *
 ***********************************************************************/
void *copy_mcast_match(void *m) {

  MCAST_MATCH_T *mcast_match = m;
  MCAST_MATCH_T *new_mcast_match = mm_malloc(sizeof(MCAST_MATCH_T));

  new_mcast_match->motif_hits
    = new_object_list(NULL, NULL, NULL, free_motif_hit);
  MOTIF_HIT_T *motif_hit = retrieve_next_object(mcast_match->motif_hits);
  while (motif_hit != NULL) {
    // FIXME actually do the copying
    motif_hit = retrieve_next_object(mcast_match->motif_hits);
  }

  new_mcast_match->seq_name = strdup(mcast_match->seq_name);
  new_mcast_match->sequence = strdup(mcast_match->sequence);
  new_mcast_match->score = mcast_match->score;
  new_mcast_match->evalue = mcast_match->evalue;
  new_mcast_match->pvalue = mcast_match->pvalue;
  new_mcast_match->qvalue = mcast_match->qvalue;
  new_mcast_match->start = mcast_match->start;
  new_mcast_match->stop = mcast_match->stop;

  return new_mcast_match;
}

/***********************************************************************
 * free_mcast_match
 *
 * This function frees the memory associated with a mcast_matc object.
 ***********************************************************************/
void free_mcast_match(void* m) {
  MCAST_MATCH_T *mcast_match = m;
  free_object_list(mcast_match->motif_hits);
  myfree(mcast_match->seq_name);
  myfree(mcast_match->sequence);
  myfree(mcast_match);
}

/***********************************************************************
 * add_mcast_match_motif_hit
 *
 * This function adds a motif_hit to an mcast_match.
 * Freeing the mcast_match will free the motif_hits that have been added.
 ***********************************************************************/
void add_mcast_match_motif_hit(
    MOTIF_HIT_T *motif_hit, 
    MCAST_MATCH_T* mcast_match
) {
  store_object(motif_hit, NULL, 0.0, mcast_match->motif_hits);
}

/***********************************************************************
 * print_mcast_match
 *
 * This function prints the data from mcast_match object
 *
 ***********************************************************************/
void print_mcast_match(FILE *output, void *m) {

  MCAST_MATCH_T *mcast_match = m;
  fprintf(
    output,
    "%s\t%5.1g\t%3.1g\t%3.1g\t%d\t%d\n",
    mcast_match->seq_name,
    mcast_match->score,
    mcast_match->pvalue,
    mcast_match->evalue,
    mcast_match->start,
    mcast_match->stop
  );
  MOTIF_HIT_T *motif_hit = retrieve_next_object(mcast_match->motif_hits);
  while(motif_hit != NULL) {
    fprintf(
      output,
      "\t%s\t%3.1g\t%c\t%d\t%d\n",
      motif_hit->motif_id,
      motif_hit->pvalue,
      motif_hit->strand,
      motif_hit->start,
      motif_hit->stop
    );
    motif_hit = retrieve_next_object(mcast_match->motif_hits);
  }

}

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
int compare_mcast_matches(void *m1, void *m2) {

  MCAST_MATCH_T *match1 = m1;
  MCAST_MATCH_T *match2 = m2;

  if (match1->score > match2->score) {
    return 1;
  }
  else if (match1->score == match2->score) {
    return 0;
  }
  else {
    return -1;
  }
}

/***********************************************************************
 * get_mcast_match_cluster_id
 *
 * This function returns the cluster id from an MCAST match object.
 *
 ***********************************************************************/
int get_mcast_match_cluster_id(MCAST_MATCH_T *match) {
  return match->cluster_id;
}

/***********************************************************************
 * set_mcast_match_seq_name
 *
 * This function sets the seq_name for an MCAST match object.
 *
 * The string will be duplicated and freed when the mcatch object 
 * is destroyed.
 *
 ***********************************************************************/
void set_mcast_match_seq_name(char *name, MCAST_MATCH_T *match) {
  match->seq_name = strdup(name);
}

/***********************************************************************
 * get_mcast_match_seq_name
 *
 * This function returns the seq_name from an MCAST match object.
 *
 * The caller should NOT free the returned string. It will be freed
 * when the match object is destroyed.
 *
 ***********************************************************************/
char *get_mcast_match_seq_name(MCAST_MATCH_T *match) {
  return match->seq_name;
}

/***********************************************************************
 * set_mcast_match_sequence
 *
 * This function sets the sequence for an MCAST match object.
 *
 * The string will be duplicated and freed when the mcatch object 
 * is destroyed.
 *
 ***********************************************************************/
void set_mcast_match_sequence(char *sequence, MCAST_MATCH_T *match) {
  match->sequence = strdup(sequence);
}

/***********************************************************************
 * get_mcast_match_sequence
 *
 * This function returns the sequence from an MCAST match object.
 *
 * The caller should NOT free the returned string. It will be freed
 * when the match object is destroyed.
 *
 ***********************************************************************/
char *get_mcast_match_sequence(MCAST_MATCH_T *match) {
  return match->sequence;
}

/***********************************************************************
 * set_mcast_match_score
 *
 * This function sets the score for an MCAST match object.
 *
 ***********************************************************************/
void set_mcast_match_score(double score, MCAST_MATCH_T *match) {
  match->score = score;
}

/***********************************************************************
 * get_mcast_match_score
 *
 * This function returns the score from an MCAST match object.
 *
 ***********************************************************************/
double get_mcast_match_score(MCAST_MATCH_T *match) {
  return match->score;
}

/***********************************************************************
 * set_mcast_match_evalue
 *
 * This function sets the evalue for an MCAST match object.
 *
 ***********************************************************************/
void set_mcast_match_evalue(double evalue, MCAST_MATCH_T *match) {
  match->evalue = evalue;
}

/***********************************************************************
 * get_mcast_match_evalue
 *
 * This function returns the evalue from an MCAST match object.
 *
 ***********************************************************************/
double get_mcast_match_evalue(MCAST_MATCH_T *match) {
  return match->evalue;
}

/***********************************************************************
 * set_mcast_match_pvalue
 *
 * This function sets the pvalue for an MCAST match object.
 *
 ***********************************************************************/
void set_mcast_match_pvalue(double pvalue, MCAST_MATCH_T *match) {
  match->pvalue = pvalue;
}

/***********************************************************************
 * get_mcast_match_pvalue
 *
 * This function returns the pvalue from an MCAST match object.
 *
 ***********************************************************************/
double get_mcast_match_pvalue(MCAST_MATCH_T *match) {
  return match->pvalue;
}

/***********************************************************************
 * set_mcast_match_qvalue
 *
 * This function sets the qvalue for an MCAST match object.
 *
 ***********************************************************************/
void set_mcast_match_qvalue(double qvalue, MCAST_MATCH_T *match) {
  match->qvalue = qvalue;
}

/***********************************************************************
 * get_mcast_match_qvalue
 *
 * This function returns the qvalue from an MCAST match object.
 *
 ***********************************************************************/
double get_mcast_match_qvalue(MCAST_MATCH_T *match) {
  return match->qvalue;
}

/***********************************************************************
 * set_mcast_match_start
 *
 * This function sets the match start position for an MCAST match object.
 *
 ***********************************************************************/
void set_mcast_match_start(int start, MCAST_MATCH_T *match) {
  match->start = start;
}

/***********************************************************************
 * get_mcast_match_start
 *
 * This function returns the match start position from an MCAST match object.
 *
 ***********************************************************************/
int get_mcast_match_start(MCAST_MATCH_T *match) {
  return match->start;
}

/***********************************************************************
 * set_mcast_match_stop
 *
 * This function sets the match stop position for an MCAST match object.
 *
 ***********************************************************************/
void set_mcast_match_stop(int stop, MCAST_MATCH_T *match) {
  match->stop = stop;
}

/***********************************************************************
 * get_mcast_match_stop
 *
 * This function returns the match start position from an MCAST match object.
 *
 ***********************************************************************/
int get_mcast_match_stop(MCAST_MATCH_T *match) {
  return match->stop;
}

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
) {

  assert(motif_id != NULL);

  MOTIF_HIT_T *motif_hit = mm_malloc(sizeof(MOTIF_HIT_T));

  motif_hit->motif_id = strdup(motif_id);
  motif_hit->seq = strdup(seq);
  motif_hit->strand = strand;
  motif_hit->start = start;
  motif_hit->stop = stop;
  motif_hit->pvalue = pvalue;

  return motif_hit;
}

/***********************************************************************
 * free_motif_hit
 *
 * This function frees the memory associated with a motif hit object.
 ***********************************************************************/
void free_motif_hit(MOTIF_HIT_T* motif_hit) {
  myfree(motif_hit->motif_id);
  myfree(motif_hit->seq);
  myfree(motif_hit);
}

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
const char* get_motif_hit_motif_id(MOTIF_HIT_T* motif_hit) {
  return motif_hit->motif_id;
}

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
const char* get_motif_hit_seq(MOTIF_HIT_T* motif_hit) {
  return motif_hit->seq;
}

/***********************************************************************
 * get_motif_hit_strand
 *
 * This function returns the strand assocaited with a motif hit object.
 *
 ***********************************************************************/
char get_motif_hit_strand(MOTIF_HIT_T* motif_hit) {
  return motif_hit->strand;
}

/***********************************************************************
 * get_motif_hit_start
 *
 * This function returns the starting position assocaited with 
 * a motif hit object.
 *
 ***********************************************************************/
int get_motif_hit_start(MOTIF_HIT_T* motif_hit) {
  return motif_hit->start;
}

/***********************************************************************
 * get_motif_hit_stop
 *
 * This function returns the stop position assocaited with 
 * a motif hit object.
 *
 ***********************************************************************/
int get_motif_hit_stop(MOTIF_HIT_T* motif_hit) {
  return motif_hit->stop;
}

/***********************************************************************
 * get_motif_hit_pvalue
 *
 * This function returns the pvalue assocaited with a motif hit object.
 *
 ***********************************************************************/
double get_motif_hit_pvalue(MOTIF_HIT_T* motif_hit) {
  return motif_hit->pvalue;
}

/**********************************************************************
  print_mcast_match_as_cisml

  Print a heap of mcast_matches asCisML XML
**********************************************************************/
void print_mcast_match_as_cisml(
  FILE* out, 
  MCAST_MATCH_T *mcast_match
) {

  assert(out != NULL);
  assert(mcast_match != NULL);

  int cluster_id = get_mcast_match_cluster_id(mcast_match);
  char *seq_name = get_mcast_match_seq_name(mcast_match);
  char *match_seq = get_mcast_match_sequence(mcast_match);
  double match_score = get_mcast_match_score(mcast_match);
  double match_evalue = get_mcast_match_evalue(mcast_match);
  double match_pvalue = get_mcast_match_pvalue(mcast_match);
  double match_qvalue = get_mcast_match_qvalue(mcast_match);
  int match_start = get_mcast_match_start(mcast_match);
  int match_stop = get_mcast_match_stop(mcast_match);

  fprintf(out, "<multi-pattern-scan");
  fprintf(out, " score=\"%g\"", match_score);
  fprintf(out, " pvalue=\"%3.1g\"", match_pvalue);
  fprintf(out, ">\n");
  MOTIF_HIT_T *motif_hit = retrieve_next_object(mcast_match->motif_hits);
  while(motif_hit != NULL) {
    const char *motif_id = get_motif_hit_motif_id(motif_hit);
    const char *hit_seq = get_motif_hit_seq(motif_hit);
    char strand = get_motif_hit_strand(motif_hit);
    double hit_pvalue = get_motif_hit_pvalue(motif_hit);
    int hit_start = get_motif_hit_start(motif_hit);
    int hit_stop = get_motif_hit_stop(motif_hit);
    if (strand == '-') {
      // Reverse strand, swap start and stop
      int tmp = hit_stop;
      hit_stop = hit_start;
      hit_start = tmp;
    }
    fprintf(
      out,
      "<pattern accession=\"%s\" name=\"%s\">\n",
      motif_id,
      motif_id
    );
    fprintf(
      out,
      "<scanned-sequence accession=\"%s\" name=\"%s\">\n",
      seq_name,
      seq_name
    );
    fprintf(
      out,
      "<matched-element start=\"%d\" stop=\"%d\" pvalue=\"%.3g\">\n",
      hit_start,
      hit_stop,
      hit_pvalue
    );
    fprintf(out, "<sequence>%s</sequence>\n", hit_seq);
    fputs("</matched-element>\n", out);
    fputs("</scanned-sequence>\n", out);
    fputs("</pattern>\n", out);
    motif_hit = retrieve_next_object(mcast_match->motif_hits);
  }
  fprintf(
    out, 
    "<mem:match cluster-id=\"cluster-%d\" "
    "seq-name=\"%s\" start=\"%d\" stop=\"%d\" "
    "evalue=\"%3.1g\" qvalue=\"%3.1g\""
    ">",
    cluster_id,
    seq_name,
    match_start,
    match_stop,
    match_evalue,
    match_qvalue
  );
  fprintf(out, "%s\n", match_seq);
  fputs("</mem:match>\n", out);
  fputs("</multi-pattern-scan>\n", out);
}

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
) {

  assert(mcast_matches != NULL);

	const char *stylesheet = options->HTML_STYLESHEET; 
  print_cisml_start(out, options->program, TRUE, stylesheet, TRUE);

  fprintf(out, "<parameters>\n");
  fprintf(
    out,
    "<pattern-file>%s</pattern-file>\n",
    options->motif_filename 
  );
  fprintf(
    out,
    "<sequence-file>%s</sequence-file>\n",
    options->seq_filename
  );
  if (options->bg_filename != NULL) {
    fprintf(
      out,
      "<background-seq-file>%s</background-seq-file>\n",
      options->bg_filename
    );
  }
  fprintf(
    out,
    "<pattern-pvalue-cutoff>%g</pattern-pvalue-cutoff>\n",
    options->motif_pthresh
  );
  fprintf(
    out,
    "<sequence-pvalue-cutoff>%g</sequence-pvalue-cutoff>\n",
    options->p_thresh
  );
  /*
   char *filter = get_cisml_sequence_filter(cisml);
  if (filter != NULL) {
    fprintf(
      out,
      "<sequence-filtering on-off=\"on\" type=\"%s\" />\n",
      filter
    );
  }
  else {
    fprintf(
      out,
      "<sequence-filtering on-off=\"off\"/>\n"
    );
  }
  */


  fprintf(out, "</parameters>\n" );
  int i = 0;
  for (i = 0; i < num_matches; ++i) {
    double evalue = get_mcast_match_evalue(mcast_matches[i]);
    double pvalue = get_mcast_match_pvalue(mcast_matches[i]);
    double qvalue = get_mcast_match_qvalue(mcast_matches[i]);
    if (!stats_available
       || (
         evalue <= options->e_thresh 
         && pvalue <= options->p_thresh 
         && qvalue <= options->q_thresh)
     ) {
      print_mcast_match_as_cisml(out, mcast_matches[i]);
    }
  }

  print_cisml_end(out);

}

