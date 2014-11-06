/**************************************************************************
 * FILE: mcast.c
 * AUTHOR: William Stafford Noble, Timothy L. Bailey, Charles Grant
 * CREATE DATE: 5/21/02
 * PROJECT: MEME Suite
 * COPYRIGHT: 1998-2002, WNG, 2001, TLB
 * DESCRIPTION: Search a database of sequences using a motif-based
 * HMM with star topology.
 **************************************************************************/

#define DEFINE_GLOBALS

#include <assert.h>
#include <errno.h>
#include <float.h>
#include <stdlib.h>
#include <stdio.h>
#include "../config.h"
#include "matrix.h"
#include "array.h"
#include "array-list.h"
#include "build-hmm.h"
#include "dir.h"
#include "dp.h"
#include "fasta-io.h"
#include "fcodon.h"
#include "fitevd.h"
#include "hash_alph.h"
#include "heap.h"
#include "io.h"
#include "log-hmm.h"
#include "mcast_match.h"
#include "mhmm.h"
#include "mhmms.h"
#include "mhmm-state.h"
#include "mhmmscan.h"
#include "motif.h"
#include "motif-in.h"
#include "object-list.h"
#include "red-black-tree.h"
#include "simple-getopt.h"
#include "transfac.h"
#include "utils.h"

typedef enum { MEME_FORMAT, TRANSFAC_FORMAT } MOTIF_FORMAT_T;

VERBOSE_T verbosity = NORMAL_VERBOSE;

typedef struct mcast_options {

  BOOLEAN_T allow_clobber;
  BOOLEAN_T use_synth;

  char *bg_filename;
  char *motif_filename;
  char *output_dirname;
  char *seq_filename;
  char *pseudo_weight;
  char *eg_cost;

  int max_gap;
  int max_stored_scores;

  double motif_pthresh;
  double e_thresh;
  double p_thresh;
  double q_thresh;

  ALPH_T alphabet;
  MOTIF_FORMAT_T motif_format;

} MCAST_OPTIONS_T;

/***********************************************************************
 * process_command_line
 *
 * This functions translates the command line options into MCAST settings.
 ***********************************************************************/
static void process_command_line(
  int argc,
  char* argv[],
  MCAST_OPTIONS_T *options
) {

  // Set default values for command line arguments
  options->allow_clobber = TRUE;
  options->motif_format = MEME_FORMAT;
  options->bg_filename = NULL;
  options->motif_filename = NULL;
  options->output_dirname = "mcast_out";
  options->seq_filename = NULL;
  options->max_gap = 50;
  options->max_stored_scores = 100000;
  options->pseudo_weight = "4.0";
  options->e_thresh = 10.0;
  options->p_thresh = 1.0;
  options->q_thresh = 1.0;
  options->motif_pthresh = 0.0005;
  options->use_synth = FALSE;

  // Define command line options.
  int option_count = 14;
  int option_index = 0;
  cmdoption const cmdoptions[] = {
    {"bgfile", REQUIRED_VALUE},
    {"bgweight", REQUIRED_VALUE},
    {"max-gap", REQUIRED_VALUE},
    {"max-stored-scores", REQUIRED_VALUE},
    {"motif-pthresh", REQUIRED_VALUE},
    {"o", REQUIRED_VALUE},
    {"oc", REQUIRED_VALUE},
    {"output-ethresh", REQUIRED_VALUE},
    {"output-pthresh", REQUIRED_VALUE},
    {"output-qthresh", REQUIRED_VALUE},
    {"synth", NO_VALUE},
    {"transfac", NO_VALUE},
    {"verbosity", REQUIRED_VALUE},
    {"version", NO_VALUE}
  };
  simple_setopt(argc, argv, option_count, cmdoptions);

  // Define the usage message.
  const char *usage = 
    "USAGE: mcast [options] <query> <database>\n"
     "\n"
     "  [--bgfile <file>]             File containing n-order Markov background model\n"
     "  [--bgweight <b>]              Add b * background frequency to each count in query\n" 
     "                                (default: 4.0 )\n"
     "  [--max-gap <value>]           Maximum allowed distance between adjacent hits\n"
     "                                (default = 50)\n"
     "  [--max-stored-scores <value>] Maximum number of matches that will be stored in memory\n"
     "                                (default=100000)\n"
     "  [--motif-pthresh <value>]     p-value threshold for motif hits\n"
     "                                (default = 0.0005).\n"
     "  [--o <output dir>]            Name of output directory. Existing files will not be\n"
     "                                overwritten (default=mcast_out)\n"
     "  [--oc <output dir>]           Name of output directory. Existing files will be\n"
     "                                overwritten.\n"
     "  [--output-ethresh <value>]    Print only results with E-values less than <value>\n"
     "                                (default = 10.0).\n"
     "  [--output-pthresh <value>]    Print only results with p-values less than <value>\n"
     "                                (default: not used).\n"
     "  [--output-qthresh <value>]    Print only results with q-values less than <value>\n"
     "                                (default: not used).\n"
     "  [--synth]                     Use synthetic scores for distribution\n"
     "  [--transfac]                  Query is in TRANSFAC format (default: MEME format)\n"
     "  [--verbosity <value>]         Verbosity of error messagess, with <value> in the range 0-5.\n"
     "                                (default = 3).\n"
     "  [--version]                   Print version and exit.\n";

  // Parse the command line.
  while (1) { 
    int c = 0;
    char* option_name = NULL;
    char* option_value = NULL;
    const char* message = NULL;

    // Read the next option, and break if we're done.
    c = simple_getopt(&option_name, &option_value, &option_index);
    if (c == 0) {
      break;
    } else if (c < 0) {
      simple_getopterror(&message);
      die("Error in command line arguments.\n%s", usage);
    }

    // Assign the parsed value to the appropriate variable
    if (strcmp(option_name, "bgfile") == 0) {
      options->bg_filename = option_value;
    } else if (strcmp(option_name, "bgweight") == 0) {
      options->pseudo_weight = option_value;
    } else if (strcmp(option_name, "max-gap") == 0) {
      options->max_gap = atoi(option_value);
      if (options->max_gap < 0) {
        die("max_gap must be positve!");
      }
    } else if (strcmp(option_name, "max-stored-scores") == 0) {
      options->max_stored_scores = atoi(option_value);
      if (options->max_gap < 0) {
        die("max-stored-scores must be positve!");
      }
    } else if (strcmp(option_name, "motif-pthresh") == 0) {
      options->motif_pthresh = atof(option_value);
      if (options->motif_pthresh < 0.0 || options->motif_pthresh > 1.0) {
        die("Motif p-value threshold must be between 0.0 and 1.0!");
      }
    } else if (strcmp(option_name, "o") == 0){
      // Set output directory with no clobber
      options->output_dirname = option_value;
      options->allow_clobber = FALSE;
    } else if (strcmp(option_name, "oc") == 0){
      // Set output directory with clobber
      options->output_dirname = option_value;
      options->allow_clobber = TRUE;
    } else if (strcmp(option_name, "output-ethresh") == 0) {
      options->e_thresh = atof(option_value);
      if (options->e_thresh <= 0.0) {
        die("E-value threshold must be positve!");
      }
      options->p_thresh = 1.0;
      options->q_thresh = 1.0;
    } else if (strcmp(option_name, "output-pthresh") == 0) {
      options->e_thresh = DBL_MAX;
      options->p_thresh = atof(option_value);
      if (options->p_thresh < 0.0 || options->p_thresh > 1.0) {
        die("p-value threshold must be between 0.0 and 1.0!");
      }
      options->q_thresh = 1.0;
    } else if (strcmp(option_name, "output-qthresh") == 0) {
      options->e_thresh = 1.0;
      options->p_thresh = 1.0;
      options->q_thresh = atof(option_value);
      if (options->q_thresh < 0.0 || options->q_thresh > 1.0) {
        die("q-value threshold must be between 0.0 and 1.0!");
      }
    } else if (strcmp(option_name, "synth") == 0) {
      options->use_synth = TRUE;
    } else if (strcmp(option_name, "transfac") == 0) {
      options->motif_format = TRANSFAC_FORMAT;
    } else if (strcmp(option_name, "verbosity") == 0) {
      verbosity = (VERBOSE_T) atoi(option_value);
    } else if (strcmp(option_name, "version") == 0) {
      fprintf(stdout, VERSION "\n");
      exit(EXIT_SUCCESS);
    }
  }

  // Read and verify the two required arguments.
  if (option_index + 2 != argc) {
    fprintf(stderr, "%s", usage);
    exit(1);
  }
  options->motif_filename = argv[option_index];
  options->seq_filename = argv[option_index+1];
     
  if (options->motif_filename == NULL) {
    die("No motif file given.\n%s", usage);
  }
  if (options->seq_filename == NULL) {
    die("No sequence file given.\n%s", usage);
  }
}

/******************************************************************************
 * build_scan_options_from_mcast_options
 *
 * This function builds the mhmmscan settings from the provided MCAST settings
 *****************************************************************************/
static void build_scan_options_from_mcast_options(
  int argc,
  char *argv[],
  MCAST_OPTIONS_T *mcast_options,
  MHMMSCAN_OPTIONS_T *scan_options
) {

  init_mhmmscan_options(scan_options);
  scan_options->alphabet = DNA_ALPH;
  scan_options->bg_filename = mcast_options->bg_filename;
  scan_options->command_line = get_command_line(argc, argv);
  scan_options->motif_filename = mcast_options->motif_filename;

  // Set up output file paths

  scan_options->output_dirname = mcast_options->output_dirname;
  scan_options->program = "mcast";
  scan_options->mhmmscan_filename = "mcast.xml";
  scan_options->mhmmscan_path 
    = make_path_to_file(
        mcast_options->output_dirname, 
        scan_options->mhmmscan_filename
      );
  scan_options->cisml_path 
    = make_path_to_file(
        mcast_options->output_dirname, 
        scan_options->CISML_FILENAME
      );
  const char *etc_dir = get_meme_etc_dir();
  const char* CSS_STYLESHEET = "meme.css.xsl";
  scan_options->css_stylesheet_path 
    = make_path_to_file(etc_dir, CSS_STYLESHEET);
  scan_options->css_stylesheet_local_path
    = make_path_to_file(mcast_options->output_dirname, CSS_STYLESHEET);
  const char* BLOCK_DIAGRAM_STYLESHEET = "block-diagram.xsl";
  scan_options->diagram_stylesheet_path 
    = make_path_to_file(etc_dir, BLOCK_DIAGRAM_STYLESHEET);
  scan_options->diagram_stylesheet_local_path
    = make_path_to_file(mcast_options->output_dirname, BLOCK_DIAGRAM_STYLESHEET);
  scan_options->html_path
    = make_path_to_file(mcast_options->output_dirname, "mcast.html");
  const char* HTML_STYLESHEET = "mcast-to-html.xsl";
  scan_options->html_stylesheet_path 
    = make_path_to_file(etc_dir, HTML_STYLESHEET);
  scan_options->html_stylesheet_local_path
    = make_path_to_file(mcast_options->output_dirname, HTML_STYLESHEET);
  const char* TEXT_STYLESHEET = "mcast-to-text.xsl";
  scan_options->text_stylesheet_path 
    = make_path_to_file(etc_dir, TEXT_STYLESHEET);
  scan_options->text_stylesheet_local_path
    = make_path_to_file(mcast_options->output_dirname, TEXT_STYLESHEET);
  scan_options->text_path
    = make_path_to_file(mcast_options->output_dirname, "mcast.txt");
  const char* WIGGLE_STYLESHEET = "mcast-to-wiggle.xsl";
  scan_options->wiggle_stylesheet_path 
    = make_path_to_file(etc_dir, WIGGLE_STYLESHEET);
  scan_options->wiggle_stylesheet_local_path
    = make_path_to_file(mcast_options->output_dirname, WIGGLE_STYLESHEET);
  scan_options->wiggle_path
    = make_path_to_file(mcast_options->output_dirname, "mcast.wig");

  scan_options->seq_filename = mcast_options->seq_filename;
  scan_options->allow_weak_motifs = TRUE;
  scan_options->beta = 4.0;
  scan_options->egcost = 1;
  scan_options->max_gap = mcast_options->max_gap;
  scan_options->max_gap_set = TRUE;
  scan_options->motif_pthresh = mcast_options->motif_pthresh;
  scan_options->motif_scoring = TRUE;
  scan_options->e_thresh = mcast_options->e_thresh;
  scan_options->p_thresh = mcast_options->p_thresh;
  scan_options->q_thresh = mcast_options->q_thresh;
  scan_options->use_pvalues = TRUE;
  scan_options->use_synth = mcast_options->use_synth;

}

/***********************************************************************
 * motif_num_cmp
 *
 * This function compares motifs by motif number.
 * It's a utility function for use with red-black tree structure
 ***********************************************************************/
static int motif_num_cmp(const void *p1, const void *p2) {
  int i1, i2, abs_i1, abs_i2;
  i1 = *((int*)p1);
  i2 = *((int*)p2);
  abs_i1 = (i1 < 0 ? -i1 : i1);
  abs_i2 = (i2 < 0 ? -i2 : i2);
  if (abs_i1 < abs_i2) { // sort motifs ascending
    return -1;
  } else if (abs_i1 > abs_i2) {
    return 1;
  } else {
    if (i1 > i2) { // put reverse complements second
      return -1;
    } else if (i1 == i2) {
      return 0;
    } else {
      return 1;
    }
  }
}

/******************************************************************************
 * read_motifs
 *
 * This function reads the motifs from a MEME motif file into an array of 
 * motif data structures.
 *****************************************************************************/
void read_motifs(
  MCAST_OPTIONS_T *options, // IN
  ARRAY_T **background,     // OUT
  int *num_motifs,          // OUT
  MOTIF_T **motif_array     // OUT
) {

  BOOLEAN_T has_reverse_strand;
  ARRAYLST_T *meme_motifs;
  double pseudocount = 0.0;

  MOTIF_T *motif;
  RBTREE_T *motifs = NULL;
  RBTREE_T *ids2nums = NULL; // mapping of motif IDs to numbers

  /**********************************************
   * READING THE MOTIFS
   **********************************************/

  MREAD_T *mread = mread_create(
      options->motif_filename, 
      OPEN_MFILE | CALC_AMBIGS
    );
  mread_set_bg_source(mread, "motif-file");
  mread_set_pseudocount(mread, pseudocount);
  has_reverse_strand = (mread_get_strands(mread) == 2);
  motifs = rbtree_create(motif_num_cmp, rbtree_intcpy, free, NULL, destroy_motif);

  int motif_i, rc_motif_i, strands;
  for (motif_i = 1, rc_motif_i = -1; mread_has_motif(mread); ++motif_i, --rc_motif_i) {
    motif = mread_next_motif(mread);
    rbtree_make(motifs, &motif_i, motif);
    if (has_reverse_strand) {
      rbtree_make(motifs, &rc_motif_i, dup_rc_motif(motif));
    }
  }
  *background = mread_get_background(mread);
  mread_destroy(mread);

  // Turn tree into simple array
  motif_tree_to_array(motifs, motif_array, num_motifs);

  // Check the alphabet used by the motifs
  ALPH_T alphabet = get_motif_alph(motif_at(*motif_array, 0));
  if (alphabet != DNA_ALPH) {
    die("The provided motifs don't seem to be from DNA.");
  }

  rbtree_destroy(motifs);

}

/******************************************************************************
 * build_hmm_from_motifs
 *
 * This function builds a star topology HMM from an dynamic array of MEME
 * motif data structures.
 *****************************************************************************/
static void build_hmm_from_motifs(
  MCAST_OPTIONS_T *options, // IN
  ARRAY_T *background,      // IN
  int num_motifs,           // IN
  MOTIF_T *motif_array,     // IN
  MHMM_T **hmm              // OUT
) {

  if (verbosity >= NORMAL_VERBOSE) {
    fprintf(stderr, "Creating HMM from motif array.\n");
  }

  // Build the motif-based HMM.
  build_star_hmm(
    background,
    DEFAULT_SPACER_STATES,
    motif_array,
    num_motifs,
    FALSE, // fim
    hmm
  );
  copy_string(&((*hmm)->motif_file), options->motif_filename);

}

/**************************************************************************
 * purge_mcast_match_heap
 *
 * This function purges half of the items in the heap in
 * order of increasing score. It may purge more than half
 * of the items in order to make sure that all reamining items
 * have scores greater than the highest score removed.
 **************************************************************************/
static void purge_match_heap(HEAP *match_heap) {

  // How many matches do we need to delete?
  int num_matches_to_delete = get_num_nodes(match_heap) / 2;
  if (verbosity >= NORMAL_VERBOSE) {
	  fprintf(
      stderr,
      "%d matches stored. Need to delete at least %d matches.\n",
      get_num_nodes(match_heap),
      num_matches_to_delete
    );
  }

  // Delete the matches scoring in the bottom half
  double max_score_discarded = 0.0;
  MCAST_MATCH_T *victim = NULL;
  int deletion_count = 0;
  for (deletion_count = 0; deletion_count  < num_matches_to_delete; ++deletion_count) {
    victim = pop_heap_root(match_heap);
    max_score_discarded = get_mcast_match_score(victim);
    free_mcast_match(victim);
  }
  if (verbosity > NORMAL_VERBOSE) {
    fprintf(
      stderr,
      "%d matches deleted from storage. %d matches remaining.\n", 
      deletion_count,
      get_num_nodes(match_heap)
    );
  }
}

/**************************************************************************
 * mcast_read_and_score
 *
 * This function provides the outer wrapper for the dynamic programming 
 * algorithm at the core of MCAST algorithm. Modified from 
 * mhmmscan.c:read_and_score() for use in MCAST.
 * CONSIDER merging back into mhmmscan.c when we refactor beadstring.
 **************************************************************************/
static void mcast_read_and_score(
  HEAP *mcast_match_heap,   /// Store for mcast matches
  SCORE_SET *score_set,     // Set of scores.
  FILE*     seq_file,       // Open stream to read from.
  SEQ_T     *sequence,      // Used if seq_file is NULL.
  BOOLEAN_T negatives_only, // Stream contains only negative examples.
  MHMM_T*   log_hmm,    // The HMM, with probs converted to logs.
  MHMMSCAN_OPTIONS_T *options, // MHMMSCAN options
  BOOLEAN_T got_evd,        // Have distribution?
  EVD_SET   evd_set,        // EVD data structure.
  int       *num_seqs       // Number of sequences read.
) {

  int num_segments = 0; // Number of sequence segments processed.
  int num_matches = 0;  // Number of matches found.
  int start_pos = 0;    // Search from here.

  // Allocate memory.
  complete_match = allocate_match();
  partial_match = allocate_match();

  *num_seqs = 0;

  while (
    (seq_file 
      && read_one_fasta_segment(
        log_hmm->alph, seq_file, options->max_chars, &sequence
      )
    ) 
    || (sequence != NULL)
  ) {
    // Number of motifs x sequence length.
    MATRIX_T* motif_score_matrix = NULL;

    // Keep track of total database size for E-value calculation.
    score_set->total_length += get_seq_length(sequence);

    // Let the user know what's going on.
    if (verbosity > HIGHER_VERBOSE) {
      fprintf(
        stderr, 
        "Scoring %s (length=%d) at position %d.\n",
        get_seq_name(sequence), 
        get_seq_length(sequence),
        start_pos
      );
    }

    // Convert the sequence to alphabet-specific indices. 
    prepare_sequence(sequence, log_hmm->alph);

    /* Allocate the dynamic programming matrix. Rows correspond to
       states in the model, columns to positions in the sequence. */
    if ( (dp_rows < log_hmm->num_states) 
      || (dp_cols < get_seq_length(sequence))
    ) {
      free_matrix(dp_matrix);
      free_matrix(trace_matrix);
      if (dp_rows < log_hmm->num_states) {
        dp_rows = log_hmm->num_states;
      }
      if (dp_cols < get_seq_length(sequence)) {
        dp_cols = get_seq_length(sequence);
      }
      // (Add one column for repeated match algorithm.)
      dp_matrix = allocate_matrix(dp_rows, dp_cols + 1);
      trace_matrix = allocate_matrix(dp_rows, dp_cols + 1);
    }

    // Compute the motif scoring matrix.
    if (options->motif_scoring) {

      motif_score_matrix = allocate_matrix(
        log_hmm->num_motifs,
        get_seq_length(sequence)
      );

      compute_motif_score_matrix(
        options->use_pvalues,
        options->motif_pthresh,
        get_int_sequence(sequence),
        get_seq_length(sequence),
        log_hmm,
        &motif_score_matrix
      );
    }

    // Fill in the DP matrix.
    repeated_match_algorithm(
      options->dp_thresh,
      get_int_sequence(sequence),
      get_seq_length(sequence),
      log_hmm,
      motif_score_matrix,
      dp_matrix,
      trace_matrix,
      complete_match
    );

    // Find all matches in the matrix.
    while (
      find_next_match(
        is_complete(sequence),
        start_pos,
        dp_matrix, 
        log_hmm,
        complete_match,
        partial_match
      )
    ) {

      // If this match starts in the overlap region, put if off until
      // the next segment.
      if (!is_complete(sequence) 
        && (get_start_match(partial_match) 
          > (get_seq_length(sequence) - OVERLAP_SIZE))
      ) {
        break;
      }

      // If this match starts at the beginning, then it's part of a
      // longer match from the previous matrix.  Skip it.  
      if (get_start_match(partial_match) == start_pos) {
        // fprintf(stderr, "Skipping match at position %d.\n", start_pos);

        // Change the starting position so that next call to find_next_match
        // will start to right of last match.
        start_pos = get_end_match(partial_match) + 1;
        continue;
      }
      start_pos = get_end_match(partial_match) + 1;

      int length = get_match_seq_length(partial_match);
      double viterbi_score = get_score(partial_match) - options->dp_thresh;
      int span = get_end_match(partial_match) - get_start_match(partial_match) + 1;
      int nhits = get_nhits(partial_match);
      double gc = 0; // GC-content around match.
      char *seq_name = get_seq_name(sequence);
      //
      // Save the score and sequence length.  Keep track of maximum length
      // and smallest score.
      //
      #define GC_WIN_SIZE 500	    // Size of GC window left and right of match.
      if (viterbi_score > LOG_SMALL) {		/* don't save tiny scores */

        // Save unscaled score and sequence length for calculating EVD. 
        // For this application we're going to use reservoir sampling
        // to store a uniform sample of scores, rather than saving all scores.
        if (score_set->max_scores_saved == 0) {
          // We begin with a one time allocation for the array of scores
          mm_resize(score_set->scores, EVD_NUM_SEQS, SCORE);
          score_set->max_scores_saved = EVD_NUM_SEQS;
        }

        int score_index = 0;
        BOOLEAN_T sample_score = FALSE;
        ++score_set->num_scores_seen;

        if (score_set->n < score_set->max_scores_saved) {
          // The first samples go directory into the reservoir
          // until it is filled.
          sample_score = TRUE;
          score_index = score_set->n;
          ++score_set->n;
        }
        else {
          // Now we're sampling
          score_index = score_set->num_scores_seen * my_drand();
          if (score_index < score_set->max_scores_saved) {
            sample_score = TRUE;
          }
        }

        if (sample_score) {

          score_set->scores[score_index].s = viterbi_score;	/* unscaled score */
          score_set->scores[score_index].t = length;
          score_set->scores[score_index].nhits = nhits;
          score_set->scores[score_index].span = span;
          if (length > score_set->max_t) {
            score_set->max_t = length;
          }

          // FIXME:  Bill: do I need to worry about the sequence offset if
          // the match doesn't contain the whole sequence?
          if (log_hmm->alph == DNA_ALPH) {		// save GC content
            // the GC count is (gc[match_end] - gc[match_start-1])
            int start = MAX(0, get_start_match(partial_match)-GC_WIN_SIZE);
            int end = MIN(get_seq_length(sequence)-1, get_end_match(partial_match)+GC_WIN_SIZE);
            double n = end - start;
            gc = (get_seq_gc(end, sequence) - get_seq_gc(start, sequence)) / n;
            score_set->scores[score_index].gc = gc; 
          }
        }
      }
      else {
        continue;
      }

      /* Calculate the initial E-value distribution if the required
       * number sequences has been saved.  This will allow the
       * descriptions of low-scoring sequences not to be stored.  The
       * distribution will be recomputed using all scores when all
       * sequences have been read.  */
      if (score_set->n == EVD_NUM_SEQS && got_evd == FALSE) {
        evd_set = calc_distr(
          *score_set,   // Set of scores.  
          options->use_pvalues,  // Use exponential distribution?
          TRUE          // Use match E-values.
        );
        if (evd_set.n > 0) {
          got_evd = TRUE;
        }
      }

      /* Use p-value to determine if score should be skipped. 
       * The threshold is multiplied by 10 because p-values may be fairly
       * inaccurate at this point.  */

      // Use length or GC-content of sequence.
      double t_or_gc = (got_evd && evd_set.exponential) ? gc : length;
      if (got_evd 
            && (evd_set_pvalue(viterbi_score, t_or_gc, 1, evd_set) 
            > 10.0 * options->p_thresh)
       ) {
        continue;
      }

      char* raw_seq = get_raw_sequence(sequence);
      long seq_offset =  get_seq_offset(sequence);
      MCAST_MATCH_T *mcast_match = allocate_mcast_match();

      int i_match = 0;
      int start_match = get_start_match(partial_match);
      int end_match = get_end_match(partial_match) + 1;
      for (i_match = start_match; i_match < end_match; i_match++) {

        int i_state = get_trace_item(i_match, partial_match);
        MHMM_STATE_T* this_state = &((log_hmm->states)[i_state]);

        // Are we at the start of a motif?
        if (this_state->type == START_MOTIF_STATE) {
          char *motif_id = get_state_motif_id(FALSE, this_state);
          int start = seq_offset + i_match + 1;
          int stop = start + this_state->w_motif - 1;
          char strand = get_strand(this_state);
          double s = get_matrix_cell(this_state->i_motif, i_match, motif_score_matrix);
          double pvalue = pow(2.0, -s) * options->motif_pthresh;
          char* hit_seq = mm_malloc((this_state->w_motif + 1) * sizeof(char));
          strncpy(hit_seq, raw_seq + i_match, this_state->w_motif);
          hit_seq[this_state->w_motif] = 0;
          MOTIF_HIT_T *motif_hit = allocate_motif_hit(
            motif_id, 
            hit_seq, 
            strand, 
            start, 
            stop, 
            pvalue
          );
          myfree(hit_seq);
          add_mcast_match_motif_hit(motif_hit, mcast_match);
        }
      }

       
      // Extract the matched sub-sequence
      // from the full sequence.
      int match_length = end_match - start_match + 1;
      char* match_seq = mm_malloc((match_length + 1) * sizeof(char));
      strncpy(match_seq, raw_seq + start_match - 1, match_length);
      match_seq[match_length] = 0;
      set_mcast_match_seq_name(seq_name, mcast_match);
      set_mcast_match_sequence(match_seq, mcast_match);
      set_mcast_match_score(viterbi_score, mcast_match);
      set_mcast_match_start(seq_offset + start_match, mcast_match);
      set_mcast_match_stop(seq_offset + end_match, mcast_match);
      int num_nodes = get_num_nodes(mcast_match_heap);
      int max_nodes = get_max_size(mcast_match_heap);
      if (num_nodes == max_nodes) {
        purge_match_heap(mcast_match_heap);
      }
      add_node_heap(mcast_match_heap, mcast_match);
      myfree(match_seq);

      num_matches++;
      user_update(
        num_matches, 
        *num_seqs, 
        num_segments, 
        num_matches, 
        get_num_nodes(mcast_match_heap),
        options->progress_every
      );
    }

    // Does the input file contain more of this sequence?
    if (!is_complete(sequence)) {
      int size_to_remove;

      // Compute the width of the buffer.
      size_to_remove = get_seq_length(sequence) - OVERLAP_SIZE;

      // Adjust the sequence accordingly.
      // remove_flanking_xs(sequence);
      remove_flanking_xs(sequence);
      shift_sequence(size_to_remove, sequence);
      if (SHIFT_DEBUG) {
        fprintf(stderr, "Retained %d bases.\n", get_seq_length(sequence));
      }

      // Remove the left-over portion from the total database size.
      score_set->total_length -= get_seq_length(sequence);

      // Next time, start looking for a match after the buffer.
      start_pos -= size_to_remove;
      if (start_pos < 0) {
        start_pos = 0;
      }

    } else {

      free_seq(sequence);
      sequence = NULL;
      start_pos = 0;
      *num_seqs += 1;

    }

    // Free the motif score matrix.
    free_matrix(motif_score_matrix);
    num_segments++;
    user_update(
      num_segments, 
      *num_seqs, 
      num_segments, 
      num_matches,
      get_num_nodes(mcast_match_heap),
      options->progress_every
    );

    if (seq_file == NULL) {
      break;
    }
  }

  //
  // Free structures.
  //
  free_matrix(dp_matrix);
  free_matrix(trace_matrix);
  free_match(complete_match);
  free_match(partial_match);

} // mcast_read_and_score

/**************************************************************************
 * calculate_statistics
 *
 * This function estimates the distribution for the MCAST scores.
 * and uses that to calculate p-values, q-values, and e-values for the
 * observed scores.
 **************************************************************************/
static BOOLEAN_T calculate_statistics(
  MHMMSCAN_OPTIONS_T *options,
  int num_matches,
  MCAST_MATCH_T **mcast_matches,
  SCORE_SET *score_set
) {

  BOOLEAN_T stats_available = FALSE;
  EVD_SET evd_set;

  /***********************************************
   * Calculate the E-values and store them as the keys.
   ***********************************************/
  evd_set = calc_distr(
    *score_set, // Set of scores.
    options->use_pvalues,  // Use exponential distribution?
    TRUE // Use match E-values.
  );

  // Distribution based on synthetic scores?
  evd_set.negatives_only = options->use_synth; 
  if (evd_set.n >= 1) {      // Found a valid score distribution.
    int q, t, N;
    q = 1; // Ignore query length.
    // Get p-value multiplier.
    if (options->use_synth) {
      N = evd_set.non_outliers = get_n(*score_set, evd_set);
    } else {
      N = evd_set.non_outliers; // Use number of non-outliers.
    }
    // p-value multiplier for E-value.
    evd_set.N = N;      
    // Record number of real scores in evd_set.
    evd_set.nscores = score_set->n;  

    // Get sequence length.
    if (options->use_pvalues) { // Exponential.
      t = 0; // Ignore sequence length.
    } else { // EVD
      t = score_set->total_length/score_set->n;  // average length
    }

    calc_pq_values(num_matches, mcast_matches, &evd_set, score_set, N, q, t);
    stats_available = TRUE;
  }

  myfree(evd_set.evds);
  return stats_available;
}

/******************************************************************************
 * write_mcast_results
 *
 * This function writes out the MCAST results as a pair of XML files and 
 * then uses XSLT to transform the XML files into HTML and text files.
******************************************************************************/
static void write_mcast_results(
  MHMMSCAN_OPTIONS_T *options, 
  BOOLEAN_T stats_available,
  int num_matches,
  MCAST_MATCH_T **mcast_matches,
  SCORE_SET *score_set, 
  MHMM_T *hmm, 
  int num_motifs, 
  MOTIF_T *motifs,
  int num_seqs
) {

  // Create output directory
  if (create_output_directory(
       options->output_dirname,
       options->allow_clobber,
       FALSE /* Don't print warning messages */
      )
    ) {
    // Failed to create output directory.
    die("Couldn't create output directory %s.\n", options->output_dirname);
  }

  // Open the MHMMSCAN XML file for output
  FILE *mhmmscan_file = fopen(options->mhmmscan_path, "w");
  if (!mhmmscan_file) {
    die("Couldn't open file %s for output.\n", options->mhmmscan_path);
  }
  print_mhmmscan_xml_file(
    mhmmscan_file,
    options,
    hmm->alph,
    num_motifs,
    motifs,
    hmm->background,
    NULL,
    num_seqs,
    score_set->total_length
  );
  fclose(mhmmscan_file);

  // Open the CisML XML file for output
  FILE *cisml_file = fopen(options->cisml_path, "w");
  if (!cisml_file) {
    die("Couldn't open file %s for output.\n", options->cisml_path);
  }

  // Print the CisML XML
  print_mcast_matches_as_cisml(cisml_file, stats_available, num_matches, mcast_matches, options);
  fclose(cisml_file);

  // Copy XML to HTML and CSS stylesheets to output directory
  copy_file(options->html_stylesheet_path, options->html_stylesheet_local_path);
  copy_file(options->css_stylesheet_path, options->css_stylesheet_local_path);
  copy_file(options->diagram_stylesheet_path, options->diagram_stylesheet_local_path);

  // Output HTML
  print_xml_filename_to_filename_using_stylesheet(
    options->mhmmscan_path,
    options->html_stylesheet_local_path,
    options->html_path
  );

  // Copy CisML to text stylesheets to output directory
  copy_file(options->text_stylesheet_path, options->text_stylesheet_local_path);

  // Output text
  print_xml_filename_to_filename_using_stylesheet(
    options->cisml_path,
    options->text_stylesheet_local_path,
    options->text_path
  );
}

/******************************************************************************
 * mcast
 *
 * This function is based on mhmmscan.c:mhmmscan(), but has been modifed to
 * use the CISML schema for writing the results as XML.
 * CONSIDER merging code back when we update beadstring.
 *
******************************************************************************/
static void mcast(
  int argc, 
  char* argv[], 
  MCAST_OPTIONS_T *mcast_options, 
  MHMM_T *hmm, 
  int num_motifs,
  MOTIF_T *motifs,
  FILE* seq_file
) {

  MHMM_T*   log_hmm;                 // The HMM, with probs converted to logs. 
  EVD_SET   evd_set;                 // EVD data structure 
  BOOLEAN_T got_evd = FALSE;         // no EVD found yet 
  SCORE_SET *score_set = NULL;       // Set of scores for computing distribution.
  SCORE_SET *synth_score_set = NULL; // Synthetic sequence scores.

  // Set the mhmmscan options from the mcast options
  MHMMSCAN_OPTIONS_T scan_options;
  build_scan_options_from_mcast_options(argc, argv, mcast_options, &scan_options);

  double    start_time;  // Time at start of sequence processing. 
  double    end_time;    // Time at end of sequence processing. 
  int       num_seqs;    // Number of sequences processed. 

  // Record CPU time. 
  myclock();

  //
  // Update options with info from the HMM.
  //
  if (scan_options.max_gap_set) {

    scan_options.dp_thresh = 1e-6;  // Very small number.

    // 
    // If using egcost, we must get model statistics
    // in order to set dp_thresh.
    //
    if (scan_options.egcost > 0.0) {    // Gap cost a fraction of expected hit score.
      score_set = set_up_score_set(
        scan_options.motif_pthresh, 
        scan_options.dp_thresh, 
        scan_options.max_gap, 
        FALSE,
        hmm
      );
      scan_options.dp_thresh = 
        (scan_options.egcost * scan_options.max_gap * score_set->e_hit_score) / score_set->egap;

      // This score set object is not used any further
      myfree(score_set->scores);
      myfree(score_set);
    } 

    //
    // Set gap costs.
    //
    scan_options.gap_extend = scan_options.dp_thresh/scan_options.max_gap;
    scan_options.gap_open = scan_options.gap_extend;
    scan_options.zero_spacer_emit_lo = TRUE;
  }

  if (scan_options.pam_distance == -1) {
    scan_options.pam_distance 
      = (hmm->alph == PROTEIN_ALPH) ? DEFAULT_PROTEIN_PAM : DEFAULT_DNA_PAM;
  }
  if (!scan_options.beta_set)  {
    scan_options.beta 
      = (hmm->alph == PROTEIN_ALPH) ? DEFAULT_PROTEIN_BETA : DEFAULT_DNA_BETA;
  }

  free_array(hmm->background);
  hmm->background = get_background(hmm->alph, scan_options.bg_filename);
  log_hmm = allocate_mhmm(hmm->alph, hmm->num_states);
  convert_to_from_log_hmm(
    TRUE, // Convert to logs.
    scan_options.zero_spacer_emit_lo,
    scan_options.gap_open,
    scan_options.gap_extend,
    hmm->background,
    scan_options.sc_filename,
    scan_options.pam_distance,
    scan_options.beta,
    hmm, 
    log_hmm
  );

  // Set up PSSM matrices if doing motif_scoring
  // and pre-compute motif p-values if using p-values.
  // Set up the hot_states list.
  set_up_pssms_and_pvalues(
    scan_options.motif_scoring,
    scan_options.motif_pthresh,
    scan_options.both_strands,
    scan_options.allow_weak_motifs,
    log_hmm
  );

  // Set thow many characters to read in a block.
  scan_options.max_chars = (int) (MAX_MATRIX_SIZE / log_hmm->num_states);
  if (verbosity >= NORMAL_VERBOSE) {
    fprintf(
      stderr, 
      "Reading sequences in blocks of %d characters.\n", 
      scan_options.max_chars
    );
  }

  start_time = myclock();

  // Set up for computing score distribution.
  score_set = set_up_score_set(
    scan_options.motif_pthresh, 
    scan_options.dp_thresh, 
    scan_options.max_gap, 
    FALSE, // Positives and negatives
    log_hmm
  );

  HEAP *mcast_match_heap = create_heap(
    mcast_options->max_stored_scores,
    compare_mcast_matches,
    copy_mcast_match,
    free_mcast_match,
    NULL,
    print_mcast_match
  );

  //
  // Read and score the real sequences using the MCAST algorithm.
  //
  SEQ_T *seq = NULL;
  mcast_read_and_score(
    mcast_match_heap,
    score_set,
    seq_file,
    seq,
    FALSE,      // Positives and negatives.
    log_hmm,
    &scan_options,
    got_evd,
    evd_set,
    &num_seqs
  );

  free_seq(seq);

  // Convert heap into an array of mcast matches, 
  // sorted by score in ascending order
  int num_matches = get_num_nodes(mcast_match_heap);
  MCAST_MATCH_T **mcast_matches = mm_malloc(num_matches * sizeof(MCAST_MATCH_T *));
  int i = 0;
  for (i = num_matches - 1; i >= 0; --i) {
    // The match heap is sorted in ascending order by score
    // Fill array in reverse order to chnage to ascending order.
    mcast_matches[i] = pop_heap_root(mcast_match_heap);
  }
  destroy_heap(mcast_match_heap);

  BOOLEAN_T stats_available = calculate_statistics(&scan_options, num_matches, mcast_matches, score_set);

  end_time = myclock();

  write_mcast_results(
    &scan_options, 
    stats_available,
    num_matches,
    mcast_matches, 
    score_set, 
    hmm, 
    num_motifs, 
    motifs, 
    num_seqs
  );

  // Tie up loose ends.
  
  for (i = 0; i < num_matches; ++i) {
    free_mcast_match(mcast_matches[i]);
  }
  myfree(mcast_matches);
  myfree(score_set->scores);
  myfree(score_set);
  free_mhmm(log_hmm);
  cleanup_mhmmscan_options(&scan_options);

}


/***********************************************************************
 * main
 *
 * Entry point for the MCAST program.
 ***********************************************************************/
int main(int argc, char* argv[]) {

  // Fill in the MCAST options from the command line.
  MCAST_OPTIONS_T mcast_options;
  process_command_line(argc, argv, &mcast_options);

  // Read the motifs from the file.
  ARRAY_T *background;
  int num_motifs = 0;
  MOTIF_T *motifs = NULL;
  MHMM_T *hmm = NULL;
  read_motifs(&mcast_options, &background, &num_motifs, &motifs);

  // Build the HMM from the motifs.
  build_hmm_from_motifs(&mcast_options, background, num_motifs, motifs, &hmm);

  // Open the sequence file.
  FILE *seq_file = NULL;
  if (open_file(
        mcast_options.seq_filename,
        "r",
        TRUE,
        "sequence",
        "sequences",
        &seq_file
      ) == 0) {
    die("Unable to continue without sequence data.\n");
  }

  // Read and score the sequences.
  mcast(argc, argv, &mcast_options, hmm, num_motifs, motifs, seq_file);

  // Cleanup

  free_motif_array(motifs, num_motifs);
  free_array(background);
  free_mhmm(hmm);
  fclose(seq_file);

  return 0;

}
