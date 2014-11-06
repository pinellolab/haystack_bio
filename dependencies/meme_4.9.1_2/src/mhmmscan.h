/**************************************************************************
 * FILE: mhmmscan.h
 * AUTHOR: William Stafford Noble, Timothy L. Bailey, and Charles Grant
 * CREATE DATE: 2011-06-20
 * PROJECT: MEME Suite
 * DESCRIPTION: Search a database of sequences using a motif-based HMM.
 **************************************************************************/
#ifndef MHMMSCAN_H
#define MHMMSCAN_H

#include "matrix.h"
#include "alphabet.h"
#include "array-list.h"
#include "match.h"

// Total size of DP matrix.
#define MAX_MATRIX_SIZE 10000000

// Number of columns of overlap between shifted versions of the matrix.
#define OVERLAP_SIZE 1000

#ifndef SHIFT_DEBUG
#define SHIFT_DEBUG 0
#endif

// Foward declartions of global variables
extern int dp_rows; // Size of the DP matrix.
extern int dp_cols; 
extern MATRIX_T* dp_matrix;      // Dynamic programming matrix.
extern MATRIX_T* trace_matrix;   // Traceback for Viterbi.
extern MATCH_T*  complete_match; // All repeated matches.
extern MATCH_T*  partial_match;  // One partial match.

typedef struct mhmmscan_options {

  BOOLEAN_T allow_clobber;       // Allow existing output files to be overwritten
  BOOLEAN_T allow_weak_motifs;   // Allow motifs with min p-value > p-thresh?
  BOOLEAN_T beta_set;            // User provided beta
  BOOLEAN_T both_strands;        // Score both DNA strands.
  BOOLEAN_T egcost_set;          // User provided egcost value
  BOOLEAN_T gap_extend_set;      // User provided gap_extend value
  BOOLEAN_T gap_open_set;        // User provided gap_open value
  BOOLEAN_T max_gap_set;         // User provided map_gap value
  BOOLEAN_T min_score_set;       // User provided min_score value
  BOOLEAN_T motif_scoring;       // Perform motif-scoring.
  BOOLEAN_T pam_distance_set;    // User provided PAM distance
  BOOLEAN_T print_fancy;         // Print Viterbi alignments? 
  BOOLEAN_T print_header;        // Print header information? 
  BOOLEAN_T print_params;        // Print program parameters? 
  BOOLEAN_T print_time;          // Print timing info? 
  BOOLEAN_T sort_output;         // Sort output scores? 
  BOOLEAN_T text_output;         // Produce text output?
  BOOLEAN_T use_pvalues;         // Use p-value scoring?
  BOOLEAN_T use_synth;           // Use synthetic scores for distribution.
  BOOLEAN_T zero_spacer_emit_lo; // Set spacer emission log-odds = 0?

  ALPH_T    alphabet;            // Alpahbet specified by MEME file
  char*     bg_filename;         // File containing background frequencies. 
  char*     command_line;        // Full text of command line
  char*     gff_filename;        // GFF output file.
  char*     hmm_filename;        // File containing the HMM. 
  char*     motif_filename;      // File containing the motifs. 
  char*     output_dirname;      // Path of output directory
  char*     program;             // Name of controling program
  char*     sc_filename;         // File containing substitution scores. 
  char*     seq_filename;        // File containing the sequences. 

  char *html_stylesheet_path; // Path to master copy of HTML stlesheet for CisML
  char *html_stylesheet_local_path; // Path to working copy of HTML stylesheet for CisML
  char *css_stylesheet_path; // Path to master copy of CSS style sheet for CisML
  char *css_stylesheet_local_path; // Path to working copy of CSS stylesheet for CisML
  char *diagram_stylesheet_path; // Path to master copy of CSS style sheet for CisML
  char *diagram_stylesheet_local_path; // Path to working copy of CSS stylesheet for CisML
  char *text_stylesheet_path; // Path to master copy of plain-text stylesheet for CisML
  char *text_stylesheet_local_path; // Path to working copy of text stylesheet for CisML
  char *gff_stylesheet_path; // Path to master copy of GFF stylesheeet for CisML
  char *gff_stylesheet_local_path; // Path to working copy of GFF stylesheet for CisML
  char *wiggle_stylesheet_path; // Path to master copy of wiggle stylesheeet for CisML
  char *wiggle_stylesheet_local_path; // Path to working copy of GFF stylesheet for CisML
  char *html_path; // Path to MHMMSCAN HTML output file
  char *text_path; // Path to MHMMSCAN plain-text output file
  char *gff_path; // Path to MHMMSCAN GFF output file
  char *wiggle_path; // Path to MHMMSCAN wiggle output file
  char *mhmmscan_path; // Pathe to MHMMSCAN XML output file
  char *cisml_path; // Path to CisML XML output file

  const char* HTML_STYLESHEET;  // Name of HTML XSLT stylesheet.
  const char* CSS_STYLESHEET;   // Name of CSS stylesheet.
  const char* BLOCK_DIAGRAM_STYLESHEET;   // Name of CSS stylesheet.
  const char* GFF_STYLESHEET;   // Name of GFF XSLT stylesheet.
  const char* WIGGLE_STYLESHEET;   // Name of wiggle XSLT stylesheet.
  const char* TEXT_STYLESHEET;  // Name of plain-text XSLT stylesheet.
  const char* HTML_FILENAME;    // Name of HTML output file.
  const char* TEXT_FILENAME;    // Name of plain-text output file.
  const char* GFF_FILENAME;     // Name of GFF output file.
  const char* WIGGLE_FILENAME;  // Name of wiggle output file.
  const char* mhmmscan_filename;// Name of MHMMSCAN XML output file.
  const char* CISML_FILENAME;   // Name of CisML XML output file.

  int       max_chars;           // Number of chars to read at once.
  int       max_gap;             // Maximum gap length to allow in matches.
  int       max_seqs;            // Maximum number of sequences to print
  int       output_width;        // Width of output, in chars.
  int       pam_distance;        // PAM distance 
  int       progress_every;      // Show progress after every n iterations.

  double    beta;                // Weight on pseudocounts.
  double    egcost;              // Fraction of E[score] that E[gap] should cost.
  double    gap_open;            // Cost to open a gap; ignore if < 0.
  double    gap_extend;          // Cost to extend a gap; ignore if < 0.

  PROB_T    dp_thresh;           // Score threshold for DP.
  PROB_T    motif_pthresh;      // p-value threshold for motif hits. 
  PROB_T    e_thresh;            // E-value threshold for scores. 
  PROB_T    p_thresh;            // p-value threshold for scores.
  PROB_T    q_thresh;            // q-value threshold for scores.

} MHMMSCAN_OPTIONS_T;

/******************************************************************************
 * Initialize the options for mhmmscan
 ******************************************************************************/
void init_mhmmscan_options(MHMMSCAN_OPTIONS_T *options);

/***********************************************************************
  Free memory allocated in options processing
 ***********************************************************************/
void cleanup_mhmmscan_options(MHMMSCAN_OPTIONS_T *options);

/***********************************************************************
 * Print MHMMSCAN specific information to an XML file
 ***********************************************************************/
void print_mhmmscan_xml_file(
  FILE *out,
  MHMMSCAN_OPTIONS_T *options,
  ALPH_T alph,
  int num_motifs,
  MOTIF_T *motifs,
  ARRAY_T *bgfreq,
  char *stylesheet,
  int num_seqs,
  long num_residues
);

/**************************************************************************
 * Tell the user how far we are.
 **************************************************************************/
void user_update(
   int changed,
   int num_seqs,
   int num_segments,
   int num_matches,
   int num_stored,
   int progress_every
);

#endif
