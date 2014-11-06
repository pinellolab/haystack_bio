/********************************************************************
 * FILE: mhmm.c
 * AUTHOR: William Stafford Noble
 * CREATE DATE: 06-23-96
 * PROJECT: MHMM
 * COPYRIGHT: 1996, WNG
 * DESCRIPTION: Given MEME output, write a motif-based HMM.
 ********************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "binary-search.h"
#include "utils.h"           // Generic utilities. 
#include "matrix.h"          // Matrix manipulation routines. 
#include "metameme.h"        // Global Meta-MEME functions. 
#include "motif.h"           // Data structure for holding motifs. 
#include "motif-in.h"
#include "order.h"           // Linear motif order and spacing. 
#include "simple-getopt.h"
#include "utils.h"           // Generic utilities. 
#include "mhmm.h"
#include "mhmm-state.h"      // Data structure for HMMs. 
#include "build-hmm.h"       // Construct an HMM in memory. 
#include "write-mhmm.h"      // Functions for producing MHMM output. 

#include "scanned-sequence.h"

#define SLOP 1E-5

#define SPACER_LENGTH 9.0 // Expected length of a spacer.

// Forward declarations
static int motif_num_cmp(const void *p1, const void *p2);

typedef struct reqmot REQMOT_T;
struct reqmot {
  RBTREE_T *nums;
  RBTREE_T *ids;
};

char* program = "mhmm";
extern int END_TRANSITION;

typedef struct mhmm_options {
  char *    motif_filename;      /* Input file containg motifs. */
  char *    hmm_type_str;       /* HMM type. */
  HMM_T     hmm_type;
  RBTREE_T *request_nums;       /* Indexes of requested motifs */
  RBTREE_T *request_ids;        /* Identifiers of requested motifs */
  int       request_n;          /* The user asked for the first n motifs. */
  double    e_threshold;        /* E-value threshold for motif inclusion. */
  double    complexity_threshold; // For eliminating low-complexity motifs.
  double    p_threshold;        /* p-value threshold for motif occurences. */
  char*     order_string;       /* Motif order and spacing. */
  int       spacer_states;      /* Number of states in each spacer. */
  BOOLEAN_T fim;                /* Represent spacers as free insertion modules? */
  BOOLEAN_T keep_unused;        // Drop unused inter-motif transitions?
  double    trans_pseudo;       /* Transition pseudocount. */
  double    spacer_pseudo;      // Spacer (self-loop) pseudocount. */
  char*     description;        // Descriptive text to be stored in model.
  BOOLEAN_T print_header;       /* Print file header? */
  BOOLEAN_T print_params;       /* Print parameter summary? */
} MHMM_OPTIONS_T;

/**********************************************
* COMMAND LINE PROCESSING
**********************************************/
static void process_command_line(
  int argc,
  char* argv[],
  MHMM_OPTIONS_T *options
) {
  // Define command line options.
  cmdoption const cmdoptions[] = {
    {"type", OPTIONAL_VALUE},
    {"description", REQUIRED_VALUE},
    {"motif", REQUIRED_VALUE},
    {"motif-id", REQUIRED_VALUE},
    {"nmotifs", REQUIRED_VALUE},
    {"ethresh", REQUIRED_VALUE},
    {"lowcomp", REQUIRED_VALUE},
    {"pthresh", REQUIRED_VALUE},
    {"order", REQUIRED_VALUE},
    {"nspacer", REQUIRED_VALUE},
    {"fim", NO_VALUE},
    {"keep-unused", NO_VALUE},
    {"transpseudo", REQUIRED_VALUE},
    {"spacerpseudo", REQUIRED_VALUE},
    {"verbosity", REQUIRED_VALUE},
    {"noheader", NO_VALUE},
    {"noparams", NO_VALUE},
    {"quiet", NO_VALUE},
  };
  int option_count = 18;
  int option_index = 0;

  // Define the usage message.
  char *usage = 
    "USAGE: mhmm [options] <MEME file>\n"
    "\n"
    "   Options:\n"
    "     --type [linear|complete|star] (default=linear)\n"
    "     --description <string> (may be repeated)\n"
    "     --motif <motif #> (may be repeated)\n"
    "     --motif-id <string>\n"
    "     --nmotifs <#>\n"
    "     --ethresh <E-value>\n"
    "     --lowcomp <value>\n"
    "     --pthresh <p-value>\n"
    "     --order <string>\n"
    "     --nspacer <spacer length> (default=1)\n"
    "     --fim\n"
    "     --keep-unused\n"
    "     --transpseudo <pseudocount>\n"
    "     --spacerpseudo <pseudocount>\n"
    "     --verbosity 1|2|3|4|5 (default=2)\n"
    "     --noheader\n"
    "     --noparams\n"
    "     --quiet\n"
    "\n";

  /* Make sure various options are set to NULL or defaults. */
  options->motif_filename = NULL;
  options->hmm_type_str = NULL;
  options->hmm_type = INVALID_HMM;
  options->request_nums 
    = rbtree_create(motif_num_cmp, rbtree_intcpy, free, rbtree_intcpy, free);
  options->request_ids 
    = rbtree_create(rbtree_strcmp, rbtree_strcpy, free, rbtree_intcpy, free);
  options->request_n = 0;
  options->e_threshold = 0.0;
  options->complexity_threshold = 0.0;
  options->p_threshold = 0.0;
  options->order_string = NULL;
  options->spacer_states = DEFAULT_SPACER_STATES,
  options->fim = FALSE;
  options->keep_unused = FALSE;
  options->trans_pseudo = DEFAULT_TRANS_PSEUDO;
  options->spacer_pseudo = DEFAULT_SPACER_PSEUDO;
  options->description = NULL;
  options->print_header = TRUE;
  options->print_params = TRUE;

	simple_setopt(argc, argv, option_count, cmdoptions);

  // Parse the command line.
  while (1) { 
    int c = 0;
    char* option_name = NULL;
    char* option_value = NULL;
    const char * message = NULL;


    // Read the next option, and break if we're done.
    c = simple_getopt(&option_name, &option_value, &option_index);
    if (c == 0) {
      break;
    } else if (c < 0) {
    	simple_getopterror(&message);
      die("Error processing command line options (%s)\n", message);
    }

    if (strcmp(option_name, "type") == 0) {
			if (option_value != NULL) {
      	options->hmm_type_str = option_value;
			}
    } else if (strcmp(option_name, "description") == 0) {
      options->description = option_value;
    } else if (strcmp(option_name, "motif") == 0) {
      if (!store_requested_num(options->request_nums, option_value)) {
        die("Error processing \"-motif %s\". "
            "Option value should be a (non-zero) motif position. "
            "For example the first motif in the file has the "
            "position 1 and it's reverse complement has the position -1.\n", 
            option_value);
      }
    } else if (strcmp(option_name, "motif-id") == 0) {
      if (!store_requested_id(options->request_ids, option_value)) {
        die("Error processing \"-motif-id %s\"."
            "Option value should be a motif identifer and hence "
            "not just be a + or a -.\n", option_value);
      }
    } else if (strcmp(option_name, "nmotifs") == 0) {
      options->request_n = atoi(option_value);
    } else if (strcmp(option_name, "ethresh") == 0) {
      options->e_threshold = atof(option_value);
    } else if (strcmp(option_name, "lowcomp") == 0) {
      options->complexity_threshold = atof(option_value);
    } else if (strcmp(option_name, "pthresh") == 0) {
      options->p_threshold = atof(option_value);
    } else if (strcmp(option_name, "order") == 0) {
      options->order_string = option_value;
    } else if (strcmp(option_name, "nspacer") == 0) {
      options->spacer_states = atoi(option_value);
    } else if (strcmp(option_name, "fim") == 0) {
      options->fim = TRUE;
    } else if (strcmp(option_name, "keep-unused") == 0) {
      options->keep_unused = TRUE;
    } else if (strcmp(option_name, "transpseudo") == 0) {
      options->trans_pseudo = atof(option_value);
    } else if (strcmp(option_name, "spacerpseudo") == 0) {
      options->spacer_pseudo = atof(option_value);
    } else if (strcmp(option_name, "verbosity") == 0) {
      verbosity = (VERBOSE_T)atoi(option_value);
    } else if (strcmp(option_name, "noheader") == 0) {
      options->print_header = FALSE;
    } else if (strcmp(option_name, "noparams") == 0) {
      options->print_params = FALSE;
    } else if (strcmp(option_name, "quiet") == 0) {
      options->print_header = options->print_params = FALSE;
      verbosity = QUIET_VERBOSE;
    }
  }

  /* Set the model type. */
  options->hmm_type = convert_enum_type_str(
    options->hmm_type_str, 
    LINEAR_HMM, 
    HMM_STRS, 
    NUM_HMM_T
  );

  /* Gotta have positive spacer length. */
  if (options->spacer_states <= 0) {
    die("Negative spacer length (%d).\n", options->spacer_states);
  }

  /* Make sure motifs weren't selected redundantly. */
  if ((options->order_string != NULL) && (options->e_threshold != 0.0)) {
    die("Can't use -ethresh and -order.");
  }

  /* Prevent trying to build a complete or star model with ordering. */
  if (options->order_string != NULL && options->hmm_type != LINEAR_HMM) {
    die("Can't specify motif order with a %s model.", options->hmm_type_str);
  }

  // Read the single required argument.
  if (option_index + 1 != argc) {
    fprintf(stderr, "%s", usage);
    exit(1);
  }
  options->motif_filename = argv[option_index];
}

/***********************************************************************
 * Should the given motif be inserted into the model?
 * By default all are included
 ***********************************************************************/
static int select_all(void *context, int motif_i, MOTIF_T *motif) {
  if (get_motif_strands(motif)) return MOTIF_PLUS | MOTIF_MINUS;
  else return MOTIF_PLUS;
}

/***********************************************************************
 * Should the given motif be kept for the model?
 * Select the requested motifs
 ***********************************************************************/
static int select_requested(void *context, int motif_i, MOTIF_T *motif) {
  REQMOT_T *requested;
  RBNODE_T *node;
  int which;
  requested = (REQMOT_T *)context;
  which = 0;
  if ((node = rbtree_find(requested->nums, &motif_i)) != NULL) {
    which |= *((int*)rbtree_value(node));
  }
  if ((node = rbtree_find(requested->ids, get_motif_id(motif))) != NULL) {
    which |= *((int*)rbtree_value(node));
  }
  return which;
}

/***********************************************************************
 * Should the given motif be kept for the model?
 * Select the top n motifs
 ***********************************************************************/
static int select_top_n(void *context, int motif_i, MOTIF_T *motif) {
  if (*((int*)context) >= motif_i) {
    return select_all(NULL, motif_i, motif);
  }
  return 0;
}

/***********************************************************************
 * Should the given motif be kept for the model?
 * Select any motif with an evalue at least as high as passed
 ***********************************************************************/
static int select_better_e_value(void *context, int motif_i, MOTIF_T *motif) {
  if (get_motif_evalue(motif) <= *((double*)context)) {
    return select_all(NULL, motif_i, motif);
  }
  return 0;
}

/***********************************************************************
 * Should the given motif be kept for the model?
 * Select any motif with a complexity value at least as high as passed.
 ***********************************************************************/
static int select_more_complex(void *context, int motif_i, MOTIF_T *motif) {
  if (get_motif_complexity(motif) <= *((double*)context)) {
    return select_all(NULL, motif_i, motif);
  }
  return 0;
}

/***********************************************************************
 * Allows sorting motif nums.
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

/***********************************************************************
 * Calculate the motif transitions
 ***********************************************************************/
static void compute_transitions_and_spacers(
  RBTREE_T  *motifs,              // Tree of motifs keyed by file order (1 and up) IN
  ARRAYLST_T*  motif_occurrences, // List of motif occurences IN
  BOOLEAN_T  has_reverse_strand,  // File included both strands? IN
  double     p_threshold,         // P-value to include motif occurences. IN 
  ORDER_T**  order_spacing,       // Motif order and spacing (linear HMM) IN OUT
  MATRIX_T** transp_freq,         // Motif-to-motif transitions. OUT
  MATRIX_T** spacer_ave           // Average inter-motif distances. OUT
) {
  RBNODE_T *node;
  ORDER_T *new_order;      // New order and spacing. 
  BOOLEAN_T find_order;     // Should we look for the motif order? 
  int *m_nums;
  int m_count, i;

  // create a list of the nums of the remaining motifs
  // this allows log-n lookup of the motif's position in the matrix
  m_count = rbtree_size(motifs);
  m_nums = mm_malloc(sizeof(int) * m_count);
  for (i = 0, node = rbtree_first(motifs); node != NULL; i++, node = rbtree_next(node)) {
    m_nums[i] = *((int*)rbtree_key(node));
  }

  // If we already have a motif order and spacing, don't find any more. 
  if (*order_spacing == NULL) {
    find_order = TRUE;
  } else {
    find_order = FALSE;
  }
  new_order = NULL;
  
  // Allocate the matrices. 
  *transp_freq = allocate_matrix(m_count + 2, m_count + 2);
  *spacer_ave = allocate_matrix(m_count + 2, m_count + 2);
  init_matrix(0.0, *transp_freq);
  init_matrix(0.0, *spacer_ave);

  for (i = 0; i < arraylst_size(motif_occurrences); i++) {
    SCANNED_SEQ_T *sseq;
    int i_occur;
    int prev_loc, loc, gap;
    long prev_position;     // Location of the right edge of previous motif. 

    sseq = (SCANNED_SEQ_T*)arraylst_get(i, motif_occurrences);

    if (verbosity > NORMAL_VERBOSE) {
      fprintf(stderr, "Reading motif occurrences for sequence %s.\n", 
        sseq->seq_id);
    }

    // If requested, try to create an order string. 
    if (find_order) {
      new_order = create_empty_order(sseq->site_count, sseq->pvalue);
    }

    prev_loc = 0;
    prev_position = 0;
    for (i_occur = 0; i_occur < sseq->site_count; i_occur++) {
      MOTIF_T *motif;
      SCANNED_SITE_T *ssite;
      int motif_num; 
      ssite = sseq->sites+i_occur;
      motif_num = (ssite->strand == '-' ? -(ssite->m_num) : ssite->m_num);
      motif = rbtree_get(motifs, &motif_num);
      // Only include motifs that have been retained
      if (motif == NULL) {
        if (verbosity > NORMAL_VERBOSE) {
          fprintf(stderr, "Skipping motif number %d which appears in sequence %s.\n", 
              motif_num, sseq->seq_id);
        }
        continue;
      }
      if ((p_threshold > 0.0) && (ssite->pvalue > p_threshold)) {
        if (verbosity > NORMAL_VERBOSE) {
          fprintf(stderr, "Skipping motif %s in sequence %s (%g > %g).\n", 
              get_motif_st_id(motif), sseq->seq_id, ssite->pvalue, p_threshold);
        }
        continue;
      }
  
      if (verbosity > NORMAL_VERBOSE) {
        fprintf(stderr, "Adding motif %s in sequence %s.\n", 
            get_motif_st_id(motif), sseq->seq_id);
      }

      gap = ssite->position - prev_position;
      loc = binary_search(&motif_num, m_nums, m_count, sizeof(int), motif_num_cmp) + 1;
      assert(loc > 0);
      // Increment the transition count and spacer length matrices.
      incr_matrix_cell(prev_loc, loc, 1, *transp_freq);
      incr_matrix_cell(prev_loc, loc, gap, *spacer_ave);

      // Add the occurrence to the order list.
      add_occurrence(motif_num, gap, new_order);

      // store as the previous accepted site
      prev_position = ssite->position + get_motif_length(motif);
      prev_loc = loc;
    }
    // Record the transition to the end state.
    if (verbosity > NORMAL_VERBOSE) {
      fprintf(stderr, "Adding transition to end in sequence %s.\n",
        sseq->seq_id);
    }

    // we're at the end, store in the last column of the matrix.
    gap = sseq->length - prev_position;
    loc = m_count + 1;

    // Increment the transition count and spacer length matrices.
    incr_matrix_cell(prev_loc, loc, 1, *transp_freq);
    incr_matrix_cell(prev_loc, loc, gap, *spacer_ave);

    // Add the occurrence to the order list.
    add_occurrence(END_TRANSITION, gap, new_order);

    if (find_order) {
      int nd_new, nd_old;
      double pv_new, pv_old;
      nd_new = get_num_distinct(new_order);
      nd_old = get_num_distinct(*order_spacing);
      pv_new = get_pvalue(new_order);
      pv_old = get_pvalue(*order_spacing);

      // Decide whether to keep the new order object. 
      if ((nd_new > nd_old) || (((nd_new == nd_old) && (pv_new < pv_old)))) {
        if (verbosity > NORMAL_VERBOSE) {
          fprintf(stderr, "Storing order from sequence %s (%g < %g).\n",
            sseq->seq_id, pv_new, pv_old);
          print_order_and_spacing(stderr, new_order);
        }
        free_order(*order_spacing);
        *order_spacing = new_order;
      } else {
        free_order(new_order);
      }
    }
  }
  free(m_nums);
}

/*************************************************************************
 * Setup motif-to-motif occurrence and spacer length frequency
 * transition matrices in a naive fashion, without taking into account
 * any motif occurrence information.
 *************************************************************************/
static void compute_naive_transitions_and_spacers
  (const int  nmotifs,     // The number of motifs.
   MATRIX_T** transp_freq, // Motif-to-motif occurrence matrix.
   MATRIX_T** spacer_ave)  // Average spacer length matrix.
{
  int   i;
  int   j;
  PROB_T prob;

  // Allocate the matrices.
  *transp_freq = allocate_matrix(nmotifs + 2, nmotifs + 2);
  *spacer_ave = allocate_matrix(nmotifs + 2, nmotifs + 2);

  // Set up transition frequencies and spacer lengths from start state.
  prob = 1.0 / (PROB_T)nmotifs;
  for (j = 1; j <= nmotifs; j++) {
    set_matrix_cell(0, j, prob, *transp_freq);
    set_matrix_cell(0, j, SPACER_LENGTH, *spacer_ave);
  }

  /* Set up transition frequencies and spacer lengths between motifs
     and to the end state. */
  prob = 1.0 / ((PROB_T)(nmotifs + 1));
  for (i = 1; i <= nmotifs; i++) {
    for (j = 1; j <= nmotifs+1; j++) {
      set_matrix_cell(i, j, prob, *transp_freq);
      set_matrix_cell(i, j, SPACER_LENGTH, *spacer_ave);
    }
  }
}

/***********************************************************************
 * Convert transition counts to transition probabilities, and compute
 * average spacer lengths.
 *
 * Each matrix is indexed 0 ... n+1, where n is the number of motifs.
 * The entry at [i,j] corresponds to the transition from motif i to
 * motif j.  Hence, after normalization, each row in the transition
 * matrix should sum to 1.
 ***********************************************************************/
static void normalize_spacer_counts(
  double    trans_pseudo,
  double    spacer_pseudo,    // Pseudocount for self-loop.
  BOOLEAN_T keep_unused,
  MATRIX_T* transp_freq,
  MATRIX_T* spacer_ave
) {
  int i_row;
  int i_col;
  int num_rows;
  double total_spacer;
  double num_transitions;
  double ave_spacer;
  
  /* Divide the spacer lengths by the number of occurrences. */
  num_rows = get_num_rows(transp_freq);
  for (i_row = 0; i_row < num_rows; i_row++) {
    for (i_col = 0; i_col < num_rows; i_col++) {
      total_spacer = get_matrix_cell(i_row, i_col, spacer_ave) + spacer_pseudo;
      num_transitions = get_matrix_cell(i_row, i_col, transp_freq);
      if (spacer_pseudo > 0) num_transitions++;
      if (num_transitions != 0.0) {
        ave_spacer = total_spacer / num_transitions;
        set_matrix_cell(i_row, i_col, ave_spacer, spacer_ave);
      }
    }
  }

  // Add pseudocounts.
  for (i_row = 0; i_row < num_rows; i_row++) {
    for (i_col = 0; i_col < num_rows; i_col++) {

      // Force some transitions to zero.
      if (// No transitions to the start state.
        (i_col == 0) || 
        // No transitions from the end state.
        (i_row == num_rows - 1) ||
        // No transition from start to end.
        ((i_row == 0) && (i_col == num_rows - 1))) {
        set_matrix_cell(i_row, i_col, 0.0, transp_freq);
      }
      else {
        // Only increment the used transitions.
        if ((keep_unused) || 
            (get_matrix_cell(i_row, i_col, transp_freq) > 0.0)) {
          incr_matrix_cell(i_row, i_col, trans_pseudo, transp_freq);
        }
      }
    }
  }

  // Normalize rows.
  for (i_row = 0; i_row < num_rows - 1; i_row++) {
    if (array_total(get_matrix_row(i_row, transp_freq)) > 0.0) {
      normalize(SLOP, get_matrix_row(i_row, transp_freq));
    }
  }
}

/***********************************************************************
 * Discard motifs that are not connected.
 ***********************************************************************/
static void throw_out_unused_motifs
  (RBTREE_T *motifs,
   MATRIX_T *transp_freq,
   MATRIX_T *spacer_ave)
{
  int i_motif_loc, j_motif;
  ARRAY_T* row_sums;
  ARRAY_T* col_sums;
  RBNODE_T *node, *next_node;

  // Extract the margins of the transition matrix.
  row_sums = get_matrix_row_sums(transp_freq);
  col_sums = get_matrix_col_sums(transp_freq);

  for (i_motif_loc = 1, node = rbtree_first(motifs); 
      node != NULL; node = next_node, i_motif_loc++) {
    // calculate the next node before we delete anything
    next_node = rbtree_next(node);

    // Is this row or column empty?
    if ((get_array_item(i_motif_loc, row_sums) == 0.0) ||
      (get_array_item(i_motif_loc, col_sums) == 0.0)) {

      if (verbosity >= NORMAL_VERBOSE) {
        fprintf(stderr, "Removing unused motif %s. "
            "No occurrences of this motif were found.\n",
            get_motif_st_id((MOTIF_T*)rbtree_value(node)));
      }

      // Remove the row and column from the transition matrix.
      remove_matrix_row(i_motif_loc, transp_freq);
      remove_matrix_col(i_motif_loc, transp_freq);
      assert(get_num_rows(transp_freq) == get_num_cols(transp_freq));

      remove_matrix_row(i_motif_loc, spacer_ave);
      remove_matrix_col(i_motif_loc, spacer_ave);
      assert(get_num_rows(spacer_ave) == get_num_cols(spacer_ave));

      // Remove the motif from the tree
      rbtree_delete(motifs, node, NULL, NULL);
      i_motif_loc--;

      // Recalculate the row and column sums.
      free_array(row_sums);
      free_array(col_sums);
      row_sums = get_matrix_row_sums(transp_freq);
      col_sums = get_matrix_col_sums(transp_freq);
    }
  }

  free_array(row_sums);
  free_array(col_sums);
}



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
) {
  int (*filter)(void *context, int motif_num, MOTIF_T *motif);
  void *filter_context;
  char *filter_name;
  REQMOT_T requested;
  RBTREE_T *ids2nums; // mapping of motif IDs to numbers
  RBNODE_T *node;
  MREAD_T *mread;
  MOTIF_T *motif;
  int motif_i, rc_motif_i, strands, i;
  BOOLEAN_T created, has_reverse_strand;
  ARRAYLST_T *motif_occurrences;

  /**********************************************
   * SELECT MOTIF FILTER
   **********************************************/
  filter = NULL;
  filter_context = NULL;
  filter_name = "<no filter>";
  // filter top n
  if (request_n != 0) {
    filter = select_top_n;
    filter_context = &request_n;
    filter_name = "-nmotifs";
  }
  // filter requested
  if (rbtree_size(request_nums) != 0 || rbtree_size(request_ids) != 0) {
    if (filter != NULL) {
      die("Can't use -motif or -motif-id with %s.\n", filter_name);
    }
    requested.nums = request_nums;
    requested.ids = request_ids;
    filter = select_requested;
    filter_context = &requested;
    filter_name = "-motif or -motif-id";
  }
  // filter e threshold
  if (request_ev != 0.0) {
    if (filter != NULL) {
      die("Can't use -ethresh with %s.\n", filter_name);
    }
    filter = select_better_e_value;
    filter_context = &request_ev;
  }
  // filter complexity threshold
  if (request_complexity != 0.0) {
    if (filter != NULL) {
      die("Can't use -lowcomp with %s.\n", filter_name);
    }
    filter = select_more_complex;
    filter_context = &request_complexity;
  }
  // filter order (but not when reading motifs)
  if (request_order != NULL && filter != NULL) {
    die("Can't use -order with %s.\n", filter_name);
  }
  if (filter == NULL) filter = select_all;

  /**********************************************
   * READING THE MOTIFS
   **********************************************/

  mread = mread_create(motif_filename, OPEN_MFILE | CALC_AMBIGS | SCANNED_SITES);
  mread_set_bg_source(mread, bg_source);
  mread_set_pseudocount(mread, motif_pseudo);

  has_reverse_strand = (mread_get_strands(mread) == 2);

  *motifs = rbtree_create(motif_num_cmp, rbtree_intcpy, free, NULL, destroy_motif);
  ids2nums = rbtree_create(rbtree_strcmp, rbtree_strcpy, free, rbtree_intcpy, free);
  for (motif_i = 1, rc_motif_i = -1; mread_has_motif(mread); ++motif_i, --rc_motif_i) {
    motif = mread_next_motif(mread);
    strands = filter(filter_context, motif_i, motif);
    if (strands & MOTIF_PLUS) rbtree_make(*motifs, &motif_i, motif);
    if (strands & MOTIF_MINUS) rbtree_make(*motifs, &rc_motif_i, dup_rc_motif(motif));
    if (strands) rbtree_make(ids2nums, get_motif_id(motif), &motif_i);
    else destroy_motif(motif);
  }

  *background = mread_get_background(mread);
  motif_occurrences = mread_get_motif_occurrences(mread);

  mread_destroy(mread);

  // Parse the order string. 
  *order_spacing = create_order_from_ids(request_order, ids2nums);
  rbtree_destroy(ids2nums);

  if (*order_spacing) {
    // Remove unused motifs
    for (node = rbtree_first(*motifs); node != NULL; node = rbtree_next(node)) {
      motif_i = *((int*)rbtree_key(node));
      if (!order_contains(motif_i, *order_spacing)) {
        rbtree_delete(*motifs, node, NULL, NULL);
      }
    }
  }

  // Turn the raw motifs and motif occurrences into the elements of the model
  if (motif_occurrences != NULL && arraylst_size(motif_occurrences) > 0) {
    compute_transitions_and_spacers(
        *motifs,
        motif_occurrences,
        has_reverse_strand,
        hit_pv,
        order_spacing,
        transp_freq,
        spacer_ave);
  } else {
    // If no occurrences are found, initialize matrices uniformly.
    compute_naive_transitions_and_spacers(
        rbtree_size(*motifs), 
        transp_freq, 
        spacer_ave);
  }

  // Convert spacer info to probabilities.
  normalize_spacer_counts(
      trans_pseudo, 
      spacer_pseudo,
      keep_unused,
      *transp_freq, 
      *spacer_ave);

  // Throw out unused motifs.
  throw_out_unused_motifs(*motifs, *transp_freq, *spacer_ave);

  destroy_motif_occurrences(motif_occurrences);
}

/*************************************************************************
 * store a motif number in a passed set with the strands of it that
 * have been requested.
 * Returns true on success.
 *************************************************************************/
BOOLEAN_T store_requested_num(RBTREE_T *request_nums, const char *value) {
  int strands, motif_i;
  char *end;
  RBNODE_T *node;
  BOOLEAN_T created;

  strands = MOTIF_PLUS;
  motif_i = strtol(value, &end, 10);
  if (*end != '\0' || motif_i == 0) {
    return FALSE;
  }
  if (motif_i < 0) {
    strands = MOTIF_MINUS;
    motif_i = -motif_i;
  }
  node = rbtree_lookup(request_nums, &motif_i, TRUE, &created);
  if (!created) strands |= *((int*)rbtree_value(node));
  rbtree_set(request_nums, node, &strands);
  return TRUE;
}

/*************************************************************************
 * store a motif identifier in a passed set with the strands of it that
 * have been requested.
 * Returns true on success.
 *************************************************************************/
BOOLEAN_T store_requested_id(RBTREE_T *request_ids, char *value) {
  int strands;
  RBNODE_T *node;
  BOOLEAN_T created;

  strands = MOTIF_PLUS; // assume positive strand
  if (value[0] == '+') {
    value++;
  } else if (value[0] == '-') {
    value++;
    strands = MOTIF_MINUS;
  }
  if (value[0] == '\0') return FALSE;
  node = rbtree_lookup(request_ids, value, TRUE, &created);
  if (!created) strands |= *((int*)rbtree_value(node));
  rbtree_set(request_ids, node, &strands);
  return TRUE;
}

#ifdef MAIN
#include "simple-getopt.h"

VERBOSE_T verbosity = NORMAL_VERBOSE;


/*************************************************************************
 * int main
 *************************************************************************/
int main(int argc, char *argv[])
{
  /* Data structures. */
  RBTREE_T *motifs; // The motifs.
  int *motif_nums; // sorted list of motif nums to provide mapping to matricies
  ALPH_T alph = INVALID_ALPH;
  ARRAY_T*  background = NULL;    /* Background probs for alphabet. */
  ORDER_T*  order_spacing = NULL; /* Linear HMM order and spacing. */
  MATRIX_T* transp_freq = NULL;   /* Matrix of inter-motif transitions freqs. */
  MATRIX_T* spacer_ave = NULL;    /* Matrix of average spacer lengths. */
  MHMM_T *  the_hmm = NULL;       /* The HMM being constructed. */

  /* Process command line options. */
  MHMM_OPTIONS_T options;
  process_command_line(argc, argv, &options);

  /* Local variables. */
  int motif_i, strands, i;
  RBNODE_T *node;
  BOOLEAN_T created;

  /**********************************************
   * LOADING THE MOTIFS
   **********************************************/
  load_filter_process_motifs_for_hmm(
      options.request_n,
      options.request_nums,
      options.request_ids,
      options.e_threshold,
      options.complexity_threshold,
      options.order_string,
      options.p_threshold,
      options.motif_filename,
      "motif-file", // bg source
      0.0, // motif pseudocount
      options.trans_pseudo,
      options.spacer_pseudo,
      options.keep_unused,
      &alph,
      &motifs,
      &background,
      &order_spacing,
      &transp_freq,
      &spacer_ave
      );

  /**********************************************
   * BUILDING THE HMM
   **********************************************/

  /* Build the motif-based HMM. */
  if (options.hmm_type == LINEAR_HMM) {

    if (order_spacing == NULL) {
      die("No order specified for the motifs.\n"
          "For the linear model the motif file must contain motif occurence\n" 
          "data or the motif order must be specified using "
          "the --order option.");
    }

    build_linear_hmm(
      background,
		  order_spacing,
		  options.spacer_states,
		  motifs,
		  options.fim,
		  &the_hmm
    );

  } else  {
    // flatten motif list because we no longer need to handle the order
    int num_motifs;
    MOTIF_T *flat_motif_list;

    motif_tree_to_array(motifs, &flat_motif_list, &num_motifs);
    
    if (options.hmm_type == COMPLETE_HMM) {
      build_complete_hmm(
        background,
        options.spacer_states,
        flat_motif_list,
        num_motifs,
        transp_freq,
        spacer_ave,
        options.fim,
        &the_hmm
      );
    } else if (options.hmm_type == STAR_HMM) {
      build_star_hmm(
        background,
        options.spacer_states,
        flat_motif_list,
        num_motifs,
        options.fim,
        &the_hmm
      );
    }
    free_motif_array(flat_motif_list, num_motifs);
  }

  // Add some global information.
  copy_string(&(the_hmm->motif_file), options.motif_filename);

  /**********************************************
   * WRITING THE HMM
   **********************************************/

  /* Print the header. */
  if (options.print_header)
    write_header(
     program, 
     "",
		 options.description,
		 options.motif_filename,
		 NULL,
		 NULL, 
		 stdout
    );

  /* Write the HMM. */
  write_mhmm(verbosity, the_hmm, stdout);

  /* Print the program parameters. */
  if (options.print_params) {
    printf("Program parameters for mhmm\n");
    printf("  MEME file: %s\n", options.motif_filename);
    printf("  Motifs:");
    for (node = rbtree_first(motifs); node != NULL; node = rbtree_next(node)) {
      MOTIF_T* motif = (MOTIF_T*)rbtree_value(node);
      printf(" %s", (get_motif_strands(motif) ? get_motif_st_id(motif) : get_motif_id(motif)));
    }
    printf("\n");
    printf("  Model topology: %s\n",
        convert_enum_type(options.hmm_type, HMM_STRS, NUM_HMM_T));
    printf("  States per spacer: %d\n", options.spacer_states);
    printf("  Spacers are free-insertion modules: %s\n",
	  boolean_to_string(options.fim));
    printf("\n");
  }

  free_array(background);
  rbtree_destroy(options.request_ids);
  rbtree_destroy(options.request_nums);
  free_order(order_spacing);
  free_matrix(transp_freq);
  free_matrix(spacer_ave);
  rbtree_destroy(motifs);
  free_mhmm(the_hmm);
  return(0);
}
#endif /* MAIN */

