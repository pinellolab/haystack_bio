/********************************************************************
 * FILE: centrimo.c
 * AUTHOR: Timothy Bailey, Philip Machanick
 * CREATE DATE: 06/06/2011
 * PROJECT: MEME suite
 * COPYRIGHT: 2011, UQ
 ********************************************************************/

#define DEFINE_GLOBALS
#include <assert.h>
#include <stdio.h>
#include <stddef.h>
#include <string.h>
#include <math.h>
#include <stdlib.h>
#include "matrix.h"
#include "alphabet.h"
#include "binomial.h"
#include "config.h"
#include "dir.h"
#include "fasta-io.h"
#include "hash_alph.h"
#include "html-monolith.h"
#include "io.h"
#include "motif-in.h"
#include "projrel.h"
#include "pssm.h"
#include "seq-reader-from-fasta.h"
#include "simple-getopt.h"
#include "string-list.h"
#include "utils.h"
#include "background.h"
#include "seq.h"
#include "fisher_exact.h"

#include <sys/resource.h>
#include <unistd.h>

const double DEFAULT_PSEUDOCOUNT = 0.1;
const double DEFAULT_SCORE_THRESH = 5.0;
const double DEFAULT_EVALUE_THRESH = 10.0;
const double ALPHA = 1.0; // Non-motif specific scale factor.
const char* TEMPLATE_FILENAME = "centrimo_template.html";
const char* SITES_FILENAME = "site_counts.txt"; // Name of plain-text sites
const char* TEXT_FILENAME = "centrimo.txt"; // Name of plain-text centrimo output
const char* HTML_FILENAME = "centrimo.html"; // Name of HTML centrimo output
const char* CENTRIMO_USAGE = 
  "USAGE: centrimo [options] <sequence file> <motif file>+\n"
  "\n"
  "   Options:\n"
  "     --o <output dir>         output directory; default: 'centrimo_out'\n"
  "     --oc <output dir>        allow overwriting; default: 'centrimo_out'\n"
  "     --neg <fasta file>       plot motif distributions in this set as well\n"
  "                                and compare enrichment of the best regions\n"
  "                                in the <sequence file> (positive sequences) with\n"
  "                                the same regions in these sequences\n"
  "                                (negative sequences) using the Fisher exact test\n"
  "     --bgfile <background>    background frequency model for PWMs;\n"
  "                                default: base frequencies in input sequences\n"
  "     --local                  compute the enrichment of all regions;\n"
  "                                default: enrichment of central regions only\n"
  "     --maxreg <maxreg>        maximum width of any region to consider;\n"
  "                                default: use the sequence length\n"
  "     --minreg <minreg>        minimum width of any region to consider;\n"
  "                                must be less than <maxreg>;\n"
  "                                default: 1 bp\n"
  "     --score <S>              score threshold for PWMs, in bits;\n"
  "                                sequences without a site with score >= <S>\n"
  "                                are ignored;\n"
  "                                default: %.1g\n"
  "     --optimize_score         search for optimized score above the threshold\n"
  "                                given by '--score' argument. Slow computation\n"
  "                                due to multiple tests.\n"
  "                                default: use fixed score threshold\n"
  "     --norc                   do not scan with the reverse complement motif\n"
  "     --sep                    scan separately with reverse complement motif;\n"
  "                                (requires --norc)\n"
  "     --flip                   'flip' sequences so that matches on the \n"
  "                                reverse strand are 'reflected' around center;\n"
  "                                default: do not flip sequences\n"
  "     --seqlen <length>        use sequences with this length; default: use\n"
  "                                sequences with the same length as the first\n"
  "     --motif <ID>             only scan with this motif; options may be\n"
  "                                repeated to specify more than one motif;\n"
  "                                default: scan with all motifs\n"
  "     --motif-pseudo <pseudo>  pseudo-count to use creating PWMs;\n"
  "                                default: %.3g\n"
  "     --ethresh <thresh>       evalue threshold for including in results;\n"
  "                                default: %g\n"
  "     --desc <description>     include the description in the output;\n"
  "                                default: no description\n"
  "     --dfile <desc file>      use the file content as the description;\n"
  "                                default: no description\n"
  "     --noseq                  do not store sequence IDs in HTML output;\n"
  "                                default: IDs are stored in the HTML output\n"
  "     --disc                   use the Fisher exact test instead of the\n"
  "                                binomial test to compare the control and\n"
  "                                the treatment (requires --neg <fasta file>)\n"
  "     --verbosity [1|2|3|4]    verbosity of output: 1 (quiet) - 4 (dump);\n"
  "                                default: %d\n"
  "     --version                print the version and exit\n"
  "\n";

VERBOSE_T verbosity = NORMAL_VERBOSE;
// output a message provided the verbosity is set appropriately
#define DEBUG_MSG(debug_level, debug_msg) { \
  if (verbosity >= debug_level) { \
    fprintf(stderr, debug_msg); \
  } \
}

#define DEBUG_FMT(debug_level, debug_msg_format, ...) { \
  if (verbosity >= debug_level) { \
    fprintf(stderr, debug_msg_format, __VA_ARGS__); \
  } \
}

// Structure for tracking centrimo command line parameters.
typedef struct options {
  ALPH_T alphabet; // Alphabet (DNA only for the moment)

  BOOLEAN_T allow_clobber; // Allow overwriting of files in output directory.
  BOOLEAN_T scan_both_strands; // Scan forward and reverse strands.
  BOOLEAN_T scan_separately; // Scan with reverse complement motif separately
  BOOLEAN_T flip; // score given and RC strand with given motif
  BOOLEAN_T local; // Compute the enrichment of all bins
  BOOLEAN_T noseq; // Do not store sequences in HTML output
  BOOLEAN_T neg_sequences; // Use a set of negative sequences
  BOOLEAN_T disc; // Use Fisher Exact Test
  BOOLEAN_T mcc; // Calculate Matthews Corrlelation Coefficent
  BOOLEAN_T optimize_score;
  RBTREE_T *selected_motifs; // IDs of requested motifs.

  char* description; // description of job
  char* desc_file; // file containing description
  char* bg_source; // Name of file file containing background freq.
  char* output_dirname; // Name of the output directory
  char* seq_source; // Name of file containing sequences.
  char* negseq_source; // Name of file containing sequences.

  ARRAYLST_T* motif_sources; // Names of files containing motifs.
  int seq_len; // Only accept sequences with this length

  double score_thresh; // Minimum value of site score (bits).
  double pseudocount; // Pseudocount added to motif PSPM.
  double evalue_thresh; // Don't report results with worse evalue

  int min_win; // Minimum considered central window size
  int max_win; // Maximum considered central window size
} CENTRIMO_OPTIONS_T;

typedef struct motif_db {
  int id;
  char* source;
  ARRAYLST_T* motifs;
} MOTIF_DB_T;

typedef struct site {
  int start;
  char strand;
} SEQ_SITE_T;

typedef struct sites {
  double best;
  int allocated;
  int used;
  SEQ_SITE_T *sites;
} SEQ_SITES_T;

typedef struct counts {
  long total_sites;
  int allocated;
  double *sites; // there are ((2 * seqlen) - 1) sites
} SITE_COUNTS_T;

typedef struct win_stats {
  // what is the minimum site score
  double site_score_threshold;
  // where is the window
  int center;
  int spread; // this value is >= 1 and odd
  // how many bins are in the window
  int n_bins;
  // how many sites appear in the window?
  double sites;
  double neg_sites;
  // how significant is the window? (possibly compared to the negatives)
  double log_pvalue;
  double log_adj_pvalue;
  // significance of the same window in the negative set if it exists
  double neg_log_pvalue;
  double neg_log_adj_pvalue;
  // Fisher Exact test p-value comparing positive and negative enrichment
  double fisher_log_pvalue;
  double fisher_log_adj_pvalue;
  // how different is the negative set?
  double mcc; // Matthews correlation coefficient
} WIN_STATS_T;

typedef struct motif_stats {
  MOTIF_DB_T* db; // motif database
  MOTIF_T* motif;
  ARRAYLST_T *windows;
  RBTREE_T* seq_ids;
  // how many tests have been done?
  int n_tests;
  // how many bins could this motif land in
  int n_bins;
  // what was the scoring threshold used?
  double score_threshold;
  // how many sites are found for this motif?
  long sites;
  long neg_sites;
} MOTIF_STATS_T;

typedef struct struc_score {
  double score;
  int sequence_number;
  int* position;
  int num_positions;
} SCORE_T;

typedef struct buffer {
  SEQ_SITES_T* sites;
  SITE_COUNTS_T* pos_counts;
  SITE_COUNTS_T* neg_counts;
  SITE_COUNTS_T* pos_c_counts;
  SITE_COUNTS_T* neg_c_counts;
} CENTRIMO_BUFFER_T;

#define SCORE_BLOCK 20

/*************************************************************************
 * Routines to destroy data
 *************************************************************************/
static void destroy_score(void *score) {
  SCORE_T *score_cast = (SCORE_T*) score;
  free(score_cast->position);
  free(score_cast);
}
static void destroy_window(void *w) {
  WIN_STATS_T *win = (WIN_STATS_T*)w;
  memset(win, 0, sizeof(WIN_STATS_T));
  free(win);
}
static void destroy_stats(void *s) {
  MOTIF_STATS_T *stats;
  if (!s) return;
  stats = (MOTIF_STATS_T*)s;
  arraylst_destroy(destroy_window, stats->windows);
  if (stats->seq_ids) rbtree_destroy(stats->seq_ids);
  memset(stats, 0, sizeof(MOTIF_STATS_T));
  free(stats);
}

/*************************************************************************
 * Compare two window statistics in an arraylst
 * Takes pointers to pointers to WIN_STATS_T.
 *************************************************************************/
static int window_stats_compare_pvalue(const void *w1, const void *w2) {
  WIN_STATS_T *win1, *win2;
  double pv_diff;
  int spread_diff;
  win1 = *((WIN_STATS_T**) w1);
  win2 = *((WIN_STATS_T**) w2);
  // sort by p-value ascending
  pv_diff = win1->log_adj_pvalue - win2->log_adj_pvalue;
  if (pv_diff != 0) return (pv_diff < 0 ? -1 : 1);
  // sort by window size ascending
  spread_diff = win1->spread - win2->spread;
  if (spread_diff != 0) return spread_diff;
  // sort by position
  return win1->center - win2->center;
}

/*************************************************************************
 * Routines for item comparison
 *************************************************************************/
// Compare scores (based on score attribute)
static int compare_score(const void * a, const void * b) {
  const SCORE_T *a_score, *b_score;
  a_score = *((SCORE_T**) a);
  b_score = *((SCORE_T**) b);
  if (a_score->score < b_score->score) {
    return 1;
  }
  else if (b_score->score < a_score->score) {
    return -1;
  }
  return 0;
}

/***********************************************************************
 Free memory allocated in options processing
 ***********************************************************************/
static void cleanup_options(CENTRIMO_OPTIONS_T *options) {
  rbtree_destroy(options->selected_motifs);
  arraylst_destroy(NULL, options->motif_sources);
}

/***********************************************************************
 Process command line options
 ***********************************************************************/
static void process_command_line(int argc, char* argv[], CENTRIMO_OPTIONS_T *options) {

  // Define command line options.
  const int num_options = 23;
  cmdoption const centrimo_options[] = { 
    {"bgfile", REQUIRED_VALUE}, 
    {"o", REQUIRED_VALUE}, 
    {"oc", REQUIRED_VALUE}, 
    {"seqlen", REQUIRED_VALUE},
    {"score", REQUIRED_VALUE}, 
    {"motif", REQUIRED_VALUE}, 
    {"motif-pseudo", REQUIRED_VALUE},
    {"ethresh", REQUIRED_VALUE},
    {"minreg", REQUIRED_VALUE},
    {"maxreg", REQUIRED_VALUE},
    {"norc", NO_VALUE},
    {"sep", NO_VALUE},
    {"flip", NO_VALUE},
    {"noflip", NO_VALUE}, // do nothing, maintain backwards compatibility
    {"desc", REQUIRED_VALUE},
    {"dfile", REQUIRED_VALUE},
    {"local", NO_VALUE},
    {"noseq", NO_VALUE},
    {"neg", REQUIRED_VALUE},
    {"disc", NO_VALUE},
    {"verbosity", REQUIRED_VALUE},
    {"optimize_score", NO_VALUE},
    {"version", NO_VALUE}
  };

  int option_index = 0;

  /* Make sure various options are set to NULL or defaults. */
  options->alphabet = DNA_ALPH;
  options->allow_clobber = TRUE;
  options->scan_both_strands = TRUE;
  options->scan_separately = FALSE;
  options->flip = FALSE;
  options->local = FALSE;
  options->noseq = FALSE;
  options->neg_sequences = FALSE;
  options->disc = FALSE;
  options->mcc = FALSE;
  options->optimize_score = FALSE;
  options->description = NULL;
  options->desc_file = NULL;
  options->bg_source = NULL;
  options->output_dirname = "centrimo_out";
  options->seq_source = NULL;
  options->motif_sources = arraylst_create();
  options->seq_len = 0; // get the length from the first sequence
  options->score_thresh = DEFAULT_SCORE_THRESH;
  options->pseudocount = DEFAULT_PSEUDOCOUNT;
  options->evalue_thresh = DEFAULT_EVALUE_THRESH;
  options->min_win = 0;
  options->max_win = 0;

  // no need to copy, as string is declared in argv array
  options->selected_motifs = rbtree_create(rbtree_strcmp, NULL, NULL, NULL, NULL);

  verbosity = NORMAL_VERBOSE;

  simple_setopt(argc, argv, num_options, centrimo_options);

  // Parse the command line.
  while (TRUE) {
    int c = 0;
    char* option_name = NULL;
    char* option_value = NULL;
    const char * message = NULL;

    // Read the next option, and break if we're done.
    c = simple_getopt(&option_name, &option_value, &option_index);
    if (c == 0) {
      break;
    }
    else if (c < 0) {
      (void) simple_getopterror(&message);
      fprintf(stderr, "Error processing command line options (%s)\n", message);
      fprintf(stderr, CENTRIMO_USAGE, DEFAULT_PSEUDOCOUNT, DEFAULT_SCORE_THRESH,
          DEFAULT_EVALUE_THRESH, NORMAL_VERBOSE);
      exit(EXIT_FAILURE);
    }
    if (strcmp(option_name, "bgfile") == 0) {
      options->bg_source = option_value;
    }
    else if (strcmp(option_name, "ethresh") == 0) {
      options->evalue_thresh = atof(option_value);
    }
    else if (strcmp(option_name, "minreg") == 0) {
      options->min_win = atoi(option_value);
    }
    else if (strcmp(option_name, "maxreg") == 0) {
      // max_window is one less than the number of places a motif can align
      // within the central window
      options->max_win = atoi(option_value);
    }
    else if (strcmp(option_name, "motif") == 0) {
      rbtree_put(options->selected_motifs, option_value, NULL);
    }
    else if (strcmp(option_name, "motif-pseudo") == 0) {
      options->pseudocount = atof(option_value);
    }
    else if (strcmp(option_name, "norc") == 0) {
      options->scan_both_strands = FALSE;
    }
    else if (strcmp(option_name, "sep") == 0) {
      options->scan_separately = TRUE;
    }
    else if (strcmp(option_name, "flip") == 0) {
      options->flip = TRUE;
    }
    else if (strcmp(option_name, "noflip") == 0) {
      // this is default now, just leaving this
      // to avoid error messages with people
      // running old commands
    }
    else if (strcmp(option_name, "o") == 0) {
      // Set output directory with no clobber
      options->output_dirname = option_value;
      options->allow_clobber = FALSE;
    }
    else if (strcmp(option_name, "oc") == 0) {
      // Set output directory with clobber
      options->output_dirname = option_value;
      options->allow_clobber = TRUE;
    }
    else if (strcmp(option_name, "seqlen") == 0) {
      options->seq_len = atoi(option_value);
    }
    else if (strcmp(option_name, "score") == 0) {
      options->score_thresh = atof(option_value);
    }
    else if (strcmp(option_name, "optimize_score") == 0) {
      options->optimize_score = TRUE;
    }
    else if (strcmp(option_name, "desc") == 0) {
      options->description = option_value;
    }
    else if (strcmp(option_name, "dfile") == 0) {
      options->desc_file = option_value;
    }
    else if (strcmp(option_name, "local") == 0) {
      options->local = TRUE;
    }
    else if (strcmp(option_name, "noseq") == 0) {
      options->noseq = TRUE;
    }
    else if (strcmp(option_name, "neg") == 0) {
      options->negseq_source = option_value;
      options->neg_sequences = TRUE;
    }
    else if (strcmp(option_name, "disc") == 0) {
      options->disc = TRUE;
      options->mcc = TRUE;
    }
    else if (strcmp(option_name, "verbosity") == 0) {
      verbosity = atoi(option_value);
    }
    else if (strcmp(option_name, "version") == 0) {
      fprintf(stdout, VERSION "\n");
      exit(EXIT_SUCCESS);
    }
  }
  // Must have sequence and motif file names
  if (argc < option_index + 2) {
    fputs("Sequences and motifs are both required\n", stderr);
    fprintf(stderr, CENTRIMO_USAGE, DEFAULT_PSEUDOCOUNT, DEFAULT_SCORE_THRESH,
        DEFAULT_EVALUE_THRESH, NORMAL_VERBOSE);
    exit(EXIT_FAILURE);
  }
  if (options->scan_separately && options->scan_both_strands) {
    fputs("You must specify --norc when you specificify --sep.\n", stderr);
    fprintf(stderr, CENTRIMO_USAGE, DEFAULT_PSEUDOCOUNT, DEFAULT_SCORE_THRESH,
        DEFAULT_EVALUE_THRESH, NORMAL_VERBOSE);
    exit(EXIT_FAILURE);
  }
  if (options->disc && !options->neg_sequences) {
    fputs("You must supply a FASTA file with the negative dataset (--neg <fasta file>)"
        " when you use discriminative mode (--disc).\n", stderr);
    fprintf(stderr, CENTRIMO_USAGE, DEFAULT_PSEUDOCOUNT, DEFAULT_SCORE_THRESH,
        DEFAULT_EVALUE_THRESH, NORMAL_VERBOSE);
    exit(EXIT_FAILURE);
  }
  if (options->mcc && !options->neg_sequences) {
    fputs("You must supply a FASTA file with the negative dataset (--neg <fasta file>)"
        " to calculate Matthews correlation coefficient (--mcc).\n", stderr);
    fprintf(stderr, CENTRIMO_USAGE, DEFAULT_PSEUDOCOUNT, DEFAULT_SCORE_THRESH,
        DEFAULT_EVALUE_THRESH, NORMAL_VERBOSE);
    exit(EXIT_FAILURE);
  }
  if (options->max_win && options->min_win >= options->max_win) {
    fputs("You must specify --minreg smaller than --maxreg.\n", stderr);
    fprintf(stderr, CENTRIMO_USAGE, DEFAULT_PSEUDOCOUNT, DEFAULT_SCORE_THRESH,
        DEFAULT_EVALUE_THRESH, NORMAL_VERBOSE);
    exit(EXIT_FAILURE);
  }

  // Record the input file names
  options->seq_source = argv[option_index++];
  for (; option_index < argc; option_index++)
    arraylst_add(argv[option_index], options->motif_sources);

  // Set up path values for needed stylesheets and output files.
}

/*************************************************************************
 * score_motif_site
 *
 * Calculate the log odds score for a single motif-sized window.
 * return the score in the variable score.
 *************************************************************************/
/*
// FIXME Replaced because of failed smoke-test - JIJ
static inline BOOLEAN_T score_motif_site(ALPH_T alph, char *seq, PSSM_T *pssm,
    double *score) {
  int asize = alph_size(alph, ALPH_SIZE);
  MATRIX_T* pssm_matrix = pssm->matrix;
  double scaled_log_odds = 0.0;

  // For each position in the site
  int motif_position;
  for (motif_position = 0; motif_position < pssm->w; motif_position++) {
    char c = seq[motif_position];
    if (c >= 'a' && c <= 'z')
      c = toupper(c);
    int aindex = (c < 'A' || c > 'Z') ? -1 : ALPH_INDEX[alph][(int) (c - 'A')];
    // Check for gaps and ambiguity codes at this site
    if (aindex == -1 || aindex >= asize)
      return FALSE;
    scaled_log_odds += get_matrix_cell(motif_position, aindex, pssm_matrix);
  }
  // Handle scores that are out of range
  if (((int)scaled_log_odds) >= get_array_length(pssm->pv)) {
    scaled_log_odds = (double)(get_array_length(pssm->pv) - 1);
  }

  // *score = get_unscaled_pssm_score(scaled_log_odds, pssm);
  *score = scaled_to_raw(scaled_log_odds, get_pssm_w(pssm), 
      get_pssm_scale(pssm), get_pssm_offset(pssm));

  return TRUE;
}
*/
static inline BOOLEAN_T score_motif_site(ALPH_T alph, char *seq, PSSM_T *pssm,
    double *score) {
  int asize = alph_size(alph, ALPH_SIZE);
  MATRIX_T* pssm_matrix = pssm->matrix;
  double scaled_log_odds = 0.0;

  // For each position in the site
  int motif_position;
  for (motif_position = 0; motif_position < pssm->w; motif_position++) {

    char c = seq[motif_position];
    int aindex = alph_index(alph, c);
    // Check for gaps and ambiguity codes at this site
    if(aindex == -1 || aindex >= asize) return FALSE;

    scaled_log_odds += get_matrix_cell(motif_position, aindex, pssm_matrix);
  }

  *score = get_unscaled_pssm_score(scaled_log_odds, pssm);

  // Handle scores that are out of range
  if ((int) scaled_log_odds >= get_array_length(pssm->pv)) {
    scaled_log_odds = (float)(get_array_length(pssm->pv) - 1);
    *score = scaled_to_raw(scaled_log_odds, pssm->w, pssm->scale, pssm->offset);
  }
  return TRUE;
}
/*************************************************************************
 * track_site
 *
 * Keep track of any sites which have the best score seen
 *************************************************************************/
static inline void track_site(SEQ_SITES_T* seq_sites, double score, int start, char strand) {
  SEQ_SITE_T *site;
  // don't bother recording worse scores
  if (score < seq_sites->best)
    return;
  // better scores clear the list
  if (score > seq_sites->best) {
    seq_sites->used = 0;
    seq_sites->best = score;
  }
  // allocate memory on demand
  if (seq_sites->allocated <= seq_sites->used) {
    seq_sites->allocated += SCORE_BLOCK;
    mm_resize(seq_sites->sites, seq_sites->allocated, SEQ_SITE_T);
  }
  // store the site
  site = seq_sites->sites + (seq_sites->used++);
  site->start = start;
  site->strand = strand;
}

/*************************************************************************
 * score_sequence
 *
 * Calculate the log-odds score for each possible motif site in the
 * sequence and record the sites of the best. Apply a count to each
 * best site and increment the total site count.
 *************************************************************************/
static void score_sequence(CENTRIMO_OPTIONS_T *options, SEQ_T* sequence, 
    PSSM_T* pssm, PSSM_T* rev_pssm, SEQ_SITES_T* seq_sites, int seq_number,
    ARRAYLST_T* scores) {
  char *raw_seq, *seg;
  int i, L, w, pos;
  double score;
  double count;
  SEQ_SITE_T *site;
  // check we got passed stuff
  assert(options != NULL);
  assert(sequence != NULL);
  assert(pssm != NULL);
  assert(seq_sites != NULL);
  // make Mac OS compiler happy.
  score = -BIG;
  // Score and record each possible motif site in the sequence
  raw_seq = get_raw_sequence(sequence);
  L = get_seq_length(sequence);
  w = pssm->w;
  // Reset the sequence stats structure
  seq_sites->best = -BIG;
  seq_sites->used = 0;
  // Read and score each position in the sequence.
  for (i = 0; i < L - w + 1; i++) {
    seg = raw_seq + i;
    // Score and record forward strand
    if (score_motif_site(options->alphabet, seg, pssm, &score))
      track_site(seq_sites, score, i, '+');
    // Score and record reverse strand if appropriate.
    if (rev_pssm && score_motif_site(options->alphabet, seg, rev_pssm, &score))
      track_site(seq_sites, score, i, '-');
  }
  // Record the position of best site, averaging ties
  // and using position in RC of sequence if site on reverse strand
  // unless flip is false.
  if (seq_sites->used && seq_sites->best >= options->score_thresh) {
    SCORE_T* new_score = mm_malloc(sizeof(SCORE_T));
    new_score->sequence_number = seq_number;
    new_score->score = seq_sites->best;
    new_score->position = mm_malloc(sizeof(int) * seq_sites->used);
    new_score->num_positions = seq_sites->used;
    for (i = 0; i < seq_sites->used; i++) {
      site = seq_sites->sites + i;
      if (!options->flip || site->strand == '+') {
        //pos = 2 * (site->start + w/2 - 1/2); // a motif of width 1 can have sites at the first index
        pos = 2 * site->start + w - 1; // a motif of width 1 can have sites at the first index
      } else {
        //pos = 2 * (L - (site->start + w/2) - 1; // a motif of width 1 can have sites at the first index
        pos = 2 * (L - site->start) - w - 1;
      }
      new_score->position[i] = pos;
    }
    arraylst_add(new_score, scores);
  }
}

/*************************************************************************
 * score_sequences
 *
 * Score all the sequences and store the best location(s) in each sequence
 * that score better than the threshold in a list. Sort and return the list
 * so the best scores are at the start.
 **************************************************************************/
static ARRAYLST_T* score_sequences(CENTRIMO_OPTIONS_T *options, 
    CENTRIMO_BUFFER_T* buffer, SEQ_T** sequences, int seqN, 
    PSSM_T* pos_pssm, PSSM_T* rev_pssm) {
  ARRAYLST_T *scores;
  int i;
  scores = arraylst_create();
  for (i = 0; i < seqN; i++) {
    score_sequence(options, sequences[i], 
        pos_pssm, rev_pssm, buffer->sites, i, scores);
  }
  arraylst_qsort(compare_score, scores);
  return scores;
}

/*************************************************************************
 * window_binomial
 *
 * Compute the binomial enrichment of a window
 *************************************************************************/
static inline double window_binomial(double window_sites, long total_sites,
    int bins, int max_bins) {
  // p-value vars
  double log_p_value, n_trials, n_successes, p_success;
  // calculate the log p-value
  if (window_sites == 0 || bins == max_bins) {
    log_p_value = 0; // pvalue of 1
  } else {
    n_trials = (double) total_sites;
    n_successes = window_sites;
    p_success = (double) bins / (double) max_bins;
    log_p_value = log_betai(n_successes, n_trials - n_successes + 1.0, p_success);
  }
  return log_p_value;
}

/*************************************************************************
 * window_FET
 *
 * Compute the fisher exact test on a window
 *************************************************************************/
static inline double window_FET(double window_sites, long total_sites, 
    double neg_window_sites, long neg_total_sites) {
  // I'm not sure the rounds below are required, and if they are,
  // shouldn't they be in the function itself as it takes double type?
  // They were in Tom's implementation so for now I'm keeping them.
  return getLogFETPvalue(
      ROUND(window_sites),
      total_sites, 
      ROUND(neg_window_sites), 
      neg_total_sites, 
      1 // skips expensive calculation for p-values > 0.5, just returns 0.
    );
}

/*************************************************************************
 * window_MCC
 *
 * Compute the Matthews Correlation Coefficient of a window
 *************************************************************************/
static inline double window_MCC(double pos_window_sites, double neg_window_sites,
    int seqN, int neg_seqN) {
  double TP = (int) pos_window_sites;  // round down
  double FP = (int) neg_window_sites;  // round down
  double TN = neg_seqN-FP;
  double FN = seqN - TP;
  double base_sqr;
  if ((base_sqr = (TP + FP) * (TP + FN) * (TN + FP) * (TN + FN)) == 0) {
    return 0;
  } else {
    return ((TP * TN) - (FP * FN)) / sqrt(base_sqr);
  }
}


/*************************************************************************
 * Accumulate a counts struct
 *************************************************************************/
static inline void accumulate_site_counts(SITE_COUNTS_T* counts, 
    SITE_COUNTS_T* target) {
  int i;
  assert(counts->allocated == target->allocated);
  target->total_sites = counts->total_sites;
  target->sites[0] = counts->sites[0];
  for (i = 1; i < counts->allocated; i++) {
    target->sites[i] = target->sites[i-1] + counts->sites[i];
  }
}
/*************************************************************************
 * Reset a counts struct
 *************************************************************************/
static inline void reset_site_counts(SITE_COUNTS_T* counts) {
  int i;
  for (i = 0; i < counts->allocated; i++) counts->sites[i] = 0;
  counts->total_sites = 0;
}
/*************************************************************************
 * Create and zero a counts struct
 *************************************************************************/
static inline SITE_COUNTS_T* create_site_counts(int seqlen) {
  int i;
  SITE_COUNTS_T* counts;
  counts = mm_malloc(sizeof(SITE_COUNTS_T));
  counts->allocated = ((2 * seqlen) - 1);
  counts->sites = mm_malloc(sizeof(double) * counts->allocated);
  reset_site_counts(counts);
  return counts;
}

/*************************************************************************
 * Destroy the site counts struct
 *************************************************************************/
static inline void destroy_site_counts(SITE_COUNTS_T* counts) {
  free(counts->sites);
  memset(counts, 0, sizeof(SITE_COUNTS_T));
}

/*************************************************************************
 * remove_redundant
 *
 * Given a list of windows sorted best to worst, remove any that overlap
 * with a better one.
 *************************************************************************/
static void remove_redundant(ARRAYLST_T* windows) {
  int i, j, end, extent, removed;
  int best_left, best_right, current_left, current_right;
  WIN_STATS_T *current, *best;
  // a single stat can't be redundant
  if (arraylst_size(windows) <= 1) return;
  removed = 0;
  best = NULL;
  for (i = 1; i < arraylst_size(windows); i++) { // For each stats (tested stats)
    current = (WIN_STATS_T*) arraylst_get(i, windows);
    extent = (current->spread - 1) / 2;
    current_left = current->center - extent;
    current_right = current->center + extent;
    // For each stats better than the tested one (better stats)
    end = i - removed;
    for (j = 0; j < end; j++) {
      best = (WIN_STATS_T*) arraylst_get(j, windows);
      extent = (best->spread - 1) / 2;
      best_left = best->center - extent;
      best_right = best->center + extent;
      if (best_right >= current_left && current_right >= best_left) break;
    }
    if (j < end) { // the window overlaps with a better one
      DEBUG_FMT(HIGH_VERBOSE, "Discarding redundant region "
          "(center: %d spread: %d) because it overlaps with a better "
          "region (center: %d spread: %d).\n", current->center,
          current->spread, best->center, best->spread);
      destroy_window(current);
      removed++;
    } else if (removed) {
      arraylst_set(i-removed, arraylst_get(i, windows), windows);
    }
  }
  if (removed) {
    arraylst_remove_range(arraylst_size(windows) - removed, removed, NULL, windows);
  }
}

/*************************************************************************
 * Output motif site counts
 *************************************************************************/
static void output_site_counts(FILE* fh, BOOLEAN_T scan_separately,
    int sequence_length, MOTIF_DB_T* db, MOTIF_T* motif, SITE_COUNTS_T* counts) {
  // vars
  int i, w, end;
  char *alt;
  fprintf(fh, "DB %d MOTIF\t%s", db->id, 
      (scan_separately ? get_motif_st_id(motif) : get_motif_id(motif)));
  alt = get_motif_id2(motif);
  if (alt[0]) fprintf(fh, "\t%s", alt);
  fprintf(fh, "\n");
  w = get_motif_length(motif);
  end = counts->allocated - (w - 1);
  for (i = (w - 1); i < end; i += 2) {
    fprintf(fh, "% 6.1f\t%g\n", ((double) (i - sequence_length + 1)) / 2.0, counts->sites[i]);
  }
}

/*************************************************************************
 * Setup the JSON writer and output a lot of pre-calculation data
 *************************************************************************/
static void start_json(CENTRIMO_OPTIONS_T* options, int argc, char** argv, 
    SEQ_T** sequences, int seqN, int seq_skipped, int neg_seqN, 
    int neg_seq_skipped, MOTIF_DB_T** dbs, int motifN, int seqlen,
    HTMLWR_T** html_out, JSONWR_T** json_out) {
  int i;
  MOTIF_DB_T* db;
  HTMLWR_T *html;
  JSONWR_T *json;
  // setup html monolith writer
  json = NULL;
  if ((html = htmlwr_create(get_meme_etc_dir(), TEMPLATE_FILENAME))) {
    htmlwr_set_dest_name(html, options->output_dirname, HTML_FILENAME);
    htmlwr_replace(html, "centrimo_data.js", "data");
    json = htmlwr_output(html);
    if (json == NULL) die("Template does not contain data section.\n");
  } else {
    DEBUG_MSG(QUIET_VERBOSE, "Failed to open html template file.\n");
    *html_out = NULL;
    *json_out = NULL;
    return;
  }
  // now output some json
  // output some top level variables
  jsonwr_str_prop(json, "version", VERSION);
  jsonwr_str_prop(json, "revision", REVISION);
  jsonwr_str_prop(json, "release", ARCHIVE_DATE);
  //jsonwr_str_prop(json, "program", options->local ? "LocoMo" : "CentriMo");
  jsonwr_str_prop(json, "program", "CentriMo");
  // output cmd, have
  jsonwr_args_prop(json, "cmd", argc, argv);
  jsonwr_property(json, "options");
  jsonwr_start_object_value(json);
  jsonwr_dbl_prop(json, "motif-pseudo", options->pseudocount);
  jsonwr_dbl_prop(json, "score", options->score_thresh);
  jsonwr_bool_prop(json, "optimize_score", options->optimize_score);
  jsonwr_dbl_prop(json, "ethresh", options->evalue_thresh);
  jsonwr_lng_prop(json, "minbin", options->min_win);
  jsonwr_lng_prop(json, "maxbin", options->max_win);
  jsonwr_bool_prop(json, "local", options->local);
  jsonwr_bool_prop(json, "norc", !options->scan_both_strands);
  jsonwr_bool_prop(json, "sep", !options->scan_separately);
  jsonwr_bool_prop(json, "flip", options->flip);
  jsonwr_bool_prop(json, "noseq", options->noseq);
  jsonwr_bool_prop(json, "neg_sequences", options->neg_sequences);
  jsonwr_bool_prop(json, "disc", options->disc);
  jsonwr_bool_prop(json, "mcc", options->mcc);
  jsonwr_end_object_value(json);
  // output description
  jsonwr_desc_prop(json, "job_description", options->desc_file, options->description);
  // output size metrics
  jsonwr_lng_prop(json, "seqlen", seqlen);
  jsonwr_lng_prop(json, "tested", motifN);
  // output the fasta db
  jsonwr_property(json, "sequence_db");
  jsonwr_start_object_value(json);
  jsonwr_str_prop(json, "source", options->seq_source);
  jsonwr_lng_prop(json, "count", seqN);
  jsonwr_lng_prop(json, "skipped", seq_skipped);
  jsonwr_end_object_value(json);
  if (options->neg_sequences) {
    jsonwr_property(json, "negative_sequence_db");
    jsonwr_start_object_value(json);
    jsonwr_str_prop(json, "source", options->negseq_source);
    jsonwr_lng_prop(json, "count", neg_seqN);
    jsonwr_lng_prop(json, "skipped", neg_seq_skipped);
    jsonwr_end_object_value(json);
  }
  // output the motif dbs
  jsonwr_property(json, "motif_dbs");
  jsonwr_start_array_value(json);
  for (i = 0; i < arraylst_size(options->motif_sources); i++) {
    db = dbs[i];
    jsonwr_start_object_value(json);
    jsonwr_str_prop(json, "source", db->source);
    jsonwr_lng_prop(json, "count", arraylst_size(db->motifs));
    jsonwr_end_object_value(json);
  }
  jsonwr_end_array_value(json);
  if (!options->noseq) {
    // output the sequences ID
    jsonwr_property(json, "sequences");
    jsonwr_start_array_value(json);
      for (i = 0; i < seqN; i++) {
        jsonwr_str_value(json, get_seq_name(sequences[i]));
      }
    jsonwr_end_array_value(json);
  }
  // start the motif array
  jsonwr_property(json, "motifs");
  jsonwr_start_array_value(json);
  // return the html and json writers
  *html_out = html;
  *json_out = json;
}

/*************************************************************************
 * Output JSON data for a motif.
 *************************************************************************/
static void output_motif_json(JSONWR_T* json, int sequence_length,
    MOTIF_STATS_T *stats, SITE_COUNTS_T* counts, SITE_COUNTS_T* neg_counts,
    BOOLEAN_T store_sequences, BOOLEAN_T negative_sequences,
    BOOLEAN_T discriminative, BOOLEAN_T mcc, BOOLEAN scan_separately) {
  WIN_STATS_T *window;
  MOTIF_T *motif;
  MATRIX_T *freqs;
  int i, j, mlen, asize, end, index;
  RBNODE_T *seq;
  motif = stats->motif;
  freqs = get_motif_freqs(motif);
  asize = alph_size(get_motif_alph(motif), ALPH_SIZE);
  jsonwr_start_object_value(json);
  jsonwr_lng_prop(json, "db", stats->db->id);
  jsonwr_str_prop(json, "id", scan_separately ? get_motif_st_id(motif) : get_motif_id(motif));
  if (*(get_motif_id2(motif))) jsonwr_str_prop(json, "alt", get_motif_id2(motif));
  mlen = get_motif_length(motif);
  jsonwr_lng_prop(json, "len", mlen);
  jsonwr_log10num_prop(json, "motif_evalue", get_motif_log_evalue(motif), 1);
  jsonwr_dbl_prop(json, "motif_nsites", get_motif_nsites(motif));
  jsonwr_lng_prop(json, "n_tested", stats->n_tests);
  //jsonwr_dbl_prop(json, "max_prob", stats->max_prob);
  jsonwr_dbl_prop(json, "score_threshold", stats->score_threshold);
  if (get_motif_url(motif) && *get_motif_url(motif)) jsonwr_str_prop(json, "url", get_motif_url(motif));
  jsonwr_property(json, "pwm");
  jsonwr_start_array_value(json);
  for (i = 0; i < mlen; i++) {
    jsonwr_start_array_value(json);
    for (j = 0; j < asize; j++) {
      jsonwr_dbl_value(json, get_matrix_cell(i, j, freqs));
    }
    jsonwr_end_array_value(json);
  }
  jsonwr_end_array_value(json);
  jsonwr_lng_prop(json, "total_sites", counts->total_sites);
  jsonwr_property(json, "sites");
  jsonwr_start_array_value(json);
  end = counts->allocated - (mlen - 1);
  for (i = (mlen - 1); i < end; i += 2) {
    jsonwr_dbl_value(json, counts->sites[i]);
  }
  jsonwr_end_array_value(json);
  if (negative_sequences) {
    jsonwr_lng_prop(json, "neg_total_sites", neg_counts->total_sites);
    jsonwr_property(json, "neg_sites");
    jsonwr_start_array_value(json);
    end = neg_counts->allocated - (mlen - 1);
    for (i = (mlen - 1); i < end; i += 2) {
      jsonwr_dbl_value(json, neg_counts->sites[i]);
    }
    jsonwr_end_array_value(json);
  }
  jsonwr_property(json, "seqs");
  jsonwr_start_array_value(json);
  if (store_sequences) {
    for (seq = rbtree_first(stats->seq_ids); seq; seq = rbtree_next(seq)) {
      jsonwr_lng_value(json, (long)(*((int*)rbtree_key(seq))));
    }
  }
  jsonwr_end_array_value(json);
  jsonwr_property(json, "peaks"); // There are several possible peaks for LocoMo output
  jsonwr_start_array_value(json);
  for (index = 0; index < arraylst_size(stats->windows); index++) {
    jsonwr_start_object_value(json);
    window = (WIN_STATS_T *) arraylst_get(index, stats->windows);
    jsonwr_dbl_prop(json, "center", ((double) (window->center - sequence_length + 1)) / 2.0);
    jsonwr_lng_prop(json, "spread", (window->spread + 1) / 2);
    jsonwr_dbl_prop(json, "sites", window->sites);
    jsonwr_dbl_prop(json, "log_adj_pvalue", window->log_adj_pvalue);
    if (negative_sequences) {
      jsonwr_dbl_prop(json, "neg_sites", window->neg_sites);
      jsonwr_dbl_prop(json, "neg_log_adj_pvalue", window->neg_log_adj_pvalue);
      // Compute Fisher Exact Test for peak
      window->fisher_log_pvalue = window_FET(window->sites, counts->total_sites, window->neg_sites,
        neg_counts->total_sites); 
      window->fisher_log_adj_pvalue = LOGEV(log(stats->n_tests), window->fisher_log_pvalue);
      jsonwr_dbl_prop(json, "fisher_log_adj_pvalue", window->fisher_log_adj_pvalue);
      jsonwr_dbl_prop(json, "fisher_adj_pvalue", window->fisher_log_adj_pvalue);
      if (mcc) jsonwr_dbl_prop(json, "mcc", window->mcc);
    }
    jsonwr_end_object_value(json);
  }
  jsonwr_end_array_value(json);
  jsonwr_end_object_value(json);
}

/*************************************************************************
 * Finish up JSON output and HTML output
 *************************************************************************/
static void end_json(HTMLWR_T* html, JSONWR_T* json) {
  // finish writing motifs
  if (json) jsonwr_end_array_value(json);
  // finish writing html file
  if (html) {
    if (htmlwr_output(html) != NULL) {
      die("Found another JSON replacement!\n");
    }
    htmlwr_destroy(html);
  }
}

/*************************************************************************
 * Output a log value in scientific notation
 *************************************************************************/
static void print_log_value(FILE *file, double loge_val, int prec) {
  double log10_val, e, m;
  log10_val = loge_val / log(10);
  e = floor(log10_val);
  m = pow(10.0, (log10_val - e));
  if ((m + (0.5 * pow(10, -prec))) >= 10) {
    m = 1;
    e += 1;
  }
  fprintf(file, "%.*fe%04.0f", prec, m, e);
}

/*************************************************************************
 * CentriMo sites (create file)
 *************************************************************************/
static FILE* start_centrimo_sites(CENTRIMO_OPTIONS_T *options) {
  char *sites_path;
  FILE *sites_file;
  sites_path = make_path_to_file(options->output_dirname, SITES_FILENAME);
  sites_file = fopen(sites_path, "w");
  free(sites_path);
  return sites_file;
}

/*************************************************************************
 * CentriMo text (create file)
 *************************************************************************/
static FILE* start_centrimo_text(CENTRIMO_OPTIONS_T *options) {
  char *file_path;
  FILE *text_file;
// open centrimo text file
  file_path = make_path_to_file(options->output_dirname, TEXT_FILENAME);
  text_file = fopen(file_path, "w");
  free(file_path);
  fputs("# WARNING: this file is not sorted!\n"
      "# db\tmotif                         \t"
      " E-value\tadj_p-value\tlog_adj_p-value\t"
      "bin_location\tbin_width\ttotal_width\tsites_in_bin\ttotal_sites\t"
      "p_success\t p-value\tmult_tests\n", text_file);
  return text_file;
}

/*************************************************************************
 * CentriMo text (output motif stats)
 *************************************************************************/
static void output_centrimo_text(FILE* text_file, BOOLEAN_T scan_separately,
    MOTIF_STATS_T* stats, int motifN, int sequence_length) {
  //vars
  MOTIF_DB_T *db;
  MOTIF_T *motif;
  WIN_STATS_T *window;
  double log_motifN, actual_center, window_prob;
  int pad;
  char* (*get_id)(MOTIF_T*);

  get_id = (scan_separately ? get_motif_st_id : get_motif_id);
  log_motifN = log(motifN);
  motif = stats->motif;
  window = arraylst_get(0, stats->windows);
  db = stats->db;
  // convert from the internal coordinate system
  // adjust so the center of the sequence is reported as 0
  actual_center = ((double) (window->center - sequence_length + 1)) / 2.0;
  // calculate the probability of landing in the window
  window_prob = (double) window->n_bins / stats->n_bins;

  // write the motif DB
  fprintf(text_file, "%4d\t", db->id + 1);
  // write the motif name and alternate name, pad it so things nicely align
  pad = 30 - strlen(get_id(motif)) - 1;
  if (pad < 0) pad = 0;
  fprintf(text_file, "%s %-*s\t", get_id(motif), pad, get_motif_id2(motif));
  // write the best window E-value
  print_log_value(text_file, window->log_adj_pvalue + log_motifN, 1);
  fputs("\t", text_file);
  // write the best window adjusted p-value
  fputs("   ", text_file);
  print_log_value(text_file, window->log_adj_pvalue, 1);
  fputs("\t", text_file);
  // write the best window log adjusted p-value
  fprintf(text_file, "%15.2f\t", window->log_adj_pvalue);
  // write the best window stats as follows:
  // center, included bins, total bins, included sites, total sites, probability
  fprintf(text_file, "%12.1f\t%9d\t%11d\t%12.0f\t%11ld\t%9.5f\t",
      actual_center, window->n_bins, stats->n_bins, 
      window->sites, stats->sites, window_prob);
  // write the best window log raw p-value
  print_log_value(text_file, window->log_pvalue, 1);
  fputs("\t", text_file);
  // write the number of windows tested
  fprintf(text_file, "%10d\n", stats->n_tests);
}

/*************************************************************************
 * Read all the sequences into an array of SEQ_T
 *************************************************************************/
static void read_sequences(ALPH_T alph, char *seq_file_name,
    SEQ_T ***sequences, int *seq_num, int *seq_skipped, int *seq_len) {
  const int max_sequence = 32768; // unlikely to be this big
  int i, move;
  FILE * seq_fh = fopen(seq_file_name, "r");
  if (!seq_fh) die("failed to open sequence file `%s'", seq_file_name);
  read_many_fastas(alph, seq_fh, max_sequence, seq_num, sequences);
  if (fclose(seq_fh) != 0) die("failed to close sequence file\n");
  if (*seq_len == 0) *seq_len = get_seq_length((*sequences)[0]);
  move = 0;
  for (i = 0; i < *seq_num; i++) {
    if (*seq_len == get_seq_length((*sequences)[i])) {
      if (move > 0) (*sequences)[i - move] = (*sequences)[i];
    } else {
      fprintf(stderr, "Skipping sequence %s as its length (%d) does not "
          "match the expected length (%d).\n", get_seq_name((*sequences)[i]), 
          get_seq_length((*sequences)[i]), *seq_len);
      free_seq((*sequences)[i]);
      move++;
    }
  }
  *seq_num -= move;
  *seq_skipped = move;
  for (i--; i >= *seq_num; i--) (*sequences)[i] = NULL;
  *sequences = mm_realloc(*sequences, sizeof(SEQ_T*) * (*seq_num));
  if (*seq_num == 0) die("Failed to find a single sequence with the"
      " expected length %d\n", *seq_len);  
}

/*************************************************************************
 * Read a motif database
 *************************************************************************/
static MOTIF_DB_T* read_motifs(int id, char* motif_source, char* bg_source,
    ARRAY_T** bg, double pseudocount, RBTREE_T *selected, ALPH_T alph, 
    BOOLEAN scan_separately) {
  // vars
  int read_motifs, i;
  MOTIF_DB_T* motifdb;
  MREAD_T *mread;
  MOTIF_T *motif;
  ARRAYLST_T *motifs;
  RBTREE_T *seen;
  // open the motif file for reading
  mread = mread_create(motif_source, OPEN_MFILE);
  mread_set_pseudocount(mread, pseudocount);
  // determine background to use
  if (*bg != NULL) mread_set_background(mread, *bg);
  else mread_set_bg_source(mread, bg_source);
  // load motifs
  read_motifs = 0;
  if (rbtree_size(selected) > 0) {
    motifs = arraylst_create();
    while (mread_has_motif(mread)) {
      motif = mread_next_motif(mread);
      read_motifs++;
      if (rbtree_find(selected, get_motif_id(motif))) {
        arraylst_add(motif, motifs);
      } else {
        DEBUG_FMT(NORMAL_VERBOSE, "Discarding motif %s in %s (not selected).\n",
            get_motif_id(motif), motif_source);
        destroy_motif(motif);
      }
    }
  } else {
    motifs = mread_load(mread, NULL);
    read_motifs = arraylst_size(motifs);    
  }
  // remove duplicate ID motifs
  seen = rbtree_create(rbtree_strcmp, rbtree_strcpy, free, NULL, NULL);
  for (i = 0; i < arraylst_size(motifs); i++) {
    motif = (MOTIF_T*)arraylst_get(i, motifs);
    if (!rbtree_make(seen, get_motif_id(motif), NULL)) {
      DEBUG_FMT(NORMAL_VERBOSE, "Discarding motif %s in %s (non-unique ID).\n",
          get_motif_id(motif), motif_source);
      arraylst_remove(i--, motifs);
      destroy_motif(motif);
    }
  }
  rbtree_destroy(seen);

  // Add reverse complements if scanning strands separately
  if (scan_separately) add_reverse_complements(motifs);

  arraylst_fit(motifs);
  if (read_motifs > 0) {
    // check the alphabet
    if (mread_get_alphabet(mread) != alph) {
      die("Expected %s alphabet motifs\n", alph_name(alph));
    }
    // get the background
    if (*bg == NULL) *bg = mread_get_background(mread);
  } else {
    fprintf(stderr, "Warning: Motif file %s contains no motifs.\n", motif_source);
  }
  // clean up motif reader
  mread_destroy(mread);
  // create motif db
  motifdb = mm_malloc(sizeof(MOTIF_DB_T));
  memset(motifdb, 0, sizeof(MOTIF_DB_T));
  motifdb->id = id;
  motifdb->source = strdup(motif_source);
  motifdb->motifs = motifs;
  return motifdb;
}

/*************************************************************************
 * Free a motif database
 *************************************************************************/
static void free_db(MOTIF_DB_T* db) {
  free(db->source);
  arraylst_destroy(destroy_motif, db->motifs);
  memset(db, 0, sizeof(MOTIF_DB_T));
  free(db);
}

/*************************************************************************
 * Make a PSSM for centrimo
 *************************************************************************/
static PSSM_T* make_pssm(ARRAY_T* bg_freqs, MOTIF_T *motif) {
  PSSM_T *pssm;

// Build PSSM for motif and tables for p-value calculation.
// p-values are not used in centrimo, only scores
  pssm = build_motif_pssm(motif, bg_freqs, bg_freqs, //p-value background
      NULL, // prior distribution
      ALPHA, //non-motif specific scale factor
      PSSM_RANGE, 0, // no GC bins
      FALSE // make log-likelihood pssm
      );
  return pssm;
}

/*************************************************************************
 * test_counts_sites
 *
 * For a given counts struct, make sure that the sum of each site is equals to total sites value
 *************************************************************************/
static void test_counts_sites(SITE_COUNTS_T *counts) {
  int i;
  double sum_check = 0, sum_diff;
  for (i = 0; i < counts->allocated; i++)
    sum_check += counts->sites[i];
  sum_diff = counts->total_sites - sum_check;
  if (sum_diff < 0)
    sum_diff = -sum_diff;
  if (sum_diff > 0.1) {
    fprintf(stderr, "Warning: site counts don't sum to accurate value! %g != %ld\n", sum_check, counts->total_sites);
  }
}

/*************************************************************************
 * compute_counts
 *
 *************************************************************************/
static void compute_counts(ARRAYLST_T* scores, SITE_COUNTS_T *counts, int* start_index, double score_thresh) {
  int i, j;
  double count;
  SCORE_T *score;
  for (i = (start_index ? *start_index : 0); i < arraylst_size(scores); i++) { // For each score
    score = (SCORE_T*) arraylst_get(i, scores);
    if (score->score >= score_thresh) {
      count = 1.0 / score->num_positions; // Compute count value to add at every position
      for (j = 0; j < score->num_positions; j++) { // For each position of the current score
        counts->sites[score->position[j]] += count; // Set values into counts
      }
      counts->total_sites++; // Increase total sites
    } else {
      break;
    }
  }
  if (start_index) *start_index = i;
  test_counts_sites(counts);
}

/*************************************************************************
 * all_sequences_in_window
 *
 * Given a window, an arraylst of scores and a score threshold; this function
 * will find all sequences where the best motif site is in the window.
 *
 * Assumes the list of scores is sorted best to worst.
 *************************************************************************/
static RBTREE_T* all_sequences_in_window(int window_center, int window_spread,
    double min_score_threshold, ARRAYLST_T *scores) {
  RBTREE_T *seq_ids;
  int window_extent, window_left, window_right, i , j, pos;
  SCORE_T *score;
  seq_ids = rbtree_create(rbtree_intcmp, rbtree_intcpy, free, NULL, NULL);
  // Get the minimum and maximum locations of the window
  window_extent = (window_spread - 1) / 2;
  window_left = window_center - window_extent;
  window_right = window_center + window_extent;
  // now see if any positions in any scores fit in the window
  for (i = 0; i < arraylst_size(scores); i++) {
    score = (SCORE_T*) arraylst_get(i, scores);
    if (score->score < min_score_threshold) {
      // as this list is sorted there won't be any better scores
      break;
    }
    for (j = 0; j < score->num_positions; j++) { 
      pos = score->position[j];
      if (pos >= window_left && pos <= window_right) { 
        // the position is inside the given window
        rbtree_make(seq_ids, &(score->sequence_number), NULL);
      }
    }
  }
  return seq_ids;
}

/*************************************************************************
 * smallest_score
 *
 * Helper function to get the smallest score from the positive and negative
 * scores. Assumes scores are sorted largest to smallest.
 *************************************************************************/
static inline double smallest_score(ARRAYLST_T* pos, ARRAYLST_T* neg) {
  SCORE_T *score;
  int len;
  double value;
  value = BIG;
  if (pos && (len = arraylst_size(pos)) > 0) {
    score = arraylst_get(len - 1, pos);
    value = score->score;
  }
  if (neg && (len = arraylst_size(neg)) > 0) {
    score = arraylst_get(len - 1, neg);
    value = MIN(value, score->score);
  }
  return value;
}

/*************************************************************************
 * largest_score
 *
 * Helper function to get the largest score at an index from the 
 * positive and negative scores. Assumes scores are sorted largest to smallest.
 *************************************************************************/
static inline double largest_score(ARRAYLST_T* pos, int pos_i, ARRAYLST_T* neg, int neg_i) {
  SCORE_T *score;
  double value;
  value = -BIG;
  if (pos && pos_i < arraylst_size(pos)) {
    score = arraylst_get(pos_i, pos);
    value = score->score;
  }
  if (neg && neg_i < arraylst_size(neg)) {
    score = arraylst_get(neg_i, neg);
    value = MAX(value, score->score);
  }
  return value;
}


/*************************************************************************
 * count_unique_scores
 *
 * Counts the number of unique scores. This information is then used to do
 * a multiple test correction.
 *************************************************************************/
static int count_unique_scores(ARRAYLST_T* pos, ARRAYLST_T* neg) {
  double last;
  int p_i, n_i, count;
  p_i = n_i = 0;
  count = 0;
  last = largest_score(pos, p_i, neg, n_i);
  while (last != -BIG) {
    count++;
    // skip over scores with the same value
    for (; pos && p_i < arraylst_size(pos); p_i++) {
      if (((SCORE_T*)arraylst_get(p_i, pos))->score != last) break;
    }
    for (; neg && n_i < arraylst_size(neg); n_i++) {
      if (((SCORE_T*)arraylst_get(n_i, neg))->score != last) break;
    }
    // get the next largest score
    last = largest_score(pos, p_i, neg, n_i);
  }
  return count;
}

/*************************************************************************
 * test_window
 *
 * Perform statistical tests on a window.
 *************************************************************************/
static void test_window(CENTRIMO_OPTIONS_T* options, double log_pvalue_thresh,
    int n_bins, double log_n_tests,
    SITE_COUNTS_T* pve_c_counts, SITE_COUNTS_T* neg_c_counts, 
    int center, int spread, 
    double* best_ignore, ARRAYLST_T* windows, int seqN, int neg_seqN) {
  WIN_STATS_T *window;
  int expand, left, right, bins;
  double pve_sites, neg_sites, log_pvalue, log_adj_pvalue;
  neg_sites = 0;
  expand = (spread - 1) / 2; // spread is guaranteed to be +ve and odd
  left = center - expand;
  right = center + expand;
  // Warning: assumes that a window with empty outermost bins will not be passed
  bins = expand + 1;

  // calculate the number of sites using the cumulative counts
  pve_sites = pve_c_counts->sites[right] - (left ? pve_c_counts->sites[left - 1] : 0);
  if (options->neg_sequences) {
    neg_sites = neg_c_counts->sites[right] - (left ? neg_c_counts->sites[left - 1] : 0);
  }
  // calculate window significance
  if (options->disc) {
    // use Fisher Exact Test
    log_pvalue = window_FET(pve_sites, pve_c_counts->total_sites, neg_sites,
        neg_c_counts->total_sites); 
  } else {
    // use Binomial Test
    if (pve_sites <= *best_ignore) {
      // we've previously seen a smaller or equal window that had at least this
      // many sites and we skipped it because it did not meet the threshold
      // it follows that we can skip this one too because it won't do better
      return;
    }
    log_pvalue = window_binomial(pve_sites, pve_c_counts->total_sites, 
        bins, n_bins);
  }
  log_adj_pvalue = LOGEV(log_n_tests, log_pvalue);
  if (log_adj_pvalue > log_pvalue_thresh) {
    // Note: best_ignore only used as a shortcut for binomial testing
    *best_ignore = pve_sites;
    return;
  }
  // this looks like an interesting window! Record some statistics...
  window = mm_malloc(sizeof(WIN_STATS_T));
  memset(window, 0, sizeof(WIN_STATS_T));
  window->center = center;
  window->spread = spread;
  window->n_bins = bins;
  window->sites = pve_sites;
  window->log_pvalue = log_pvalue;
  window->log_adj_pvalue = log_adj_pvalue;
  if (options->neg_sequences) {
    window->neg_sites = neg_sites;
    window->neg_log_pvalue = window_binomial(neg_sites, 
      neg_c_counts->total_sites, bins, n_bins);
    window->neg_log_adj_pvalue = LOGEV(log_n_tests, window->neg_log_pvalue);
    if (options->mcc) {
      window->mcc = window_MCC(pve_sites, neg_sites, seqN, neg_seqN);
    }
  }
  arraylst_add(window, windows);
} // test_window

/*************************************************************************
 * calculate_best_windows
 *
 *************************************************************************/
static MOTIF_STATS_T* calculate_best_windows(CENTRIMO_OPTIONS_T* options, 
    CENTRIMO_BUFFER_T* buffers, double log_pvalue_thresh, 
    MOTIF_DB_T *db, MOTIF_T* motif, int seq_len, 
    ARRAYLST_T* pve_scores, ARRAYLST_T* neg_scores, int seqN, int neg_seqN) {
  int n_bins, n_minus, min_win, max_win, n_windows, n_scores, n_tests, n_actual_tests;
  int spread, max_spread, pos_first, pos_last, pos;
  int i, pve_index, neg_index;
  long best_total_sites, best_neg_total_sites;
  double score_thresh_min, best_score_thresh, log_n_tests;
  ARRAYLST_T *best_windows, *current_windows, *temp;
  WIN_STATS_T *win_stats;
  MOTIF_STATS_T *motif_stats;
  double best_log_pv, current_log_pv, best_ignore_count;
  // initilise vars
  best_total_sites = 0;
  best_neg_total_sites = 0;
  best_score_thresh = BIG;
  if (!arraylst_size(pve_scores)) {
    // no scores in the positive set, hence no best windows
    return NULL;
  }
  //
  // calculate the multiple test correction
  //
  n_bins = seq_len - get_motif_length(motif) + 1;
  // Insure that we have at least one "negative bin", max_win < n_bins
  max_win = n_bins - 1;
  if (options->max_win && options->max_win >= n_bins) {
    fprintf(stderr, "--maxreg (%d) too large for motif; setting to %d\n", options->max_win, max_win);
  } else if (options->max_win) {
    max_win = options->max_win;
  }
  // Insure that min_win is less than max_win so that even/odd parity situations both work.
  min_win = 1;
  if (options->min_win && options->min_win >= max_win) {
    min_win = max_win - 1;
    fprintf(stderr, "--minreg (%d) too large; setting to %d\n", options->min_win, min_win);
  } else if (options->min_win) {
    min_win = options->min_win;
  }
  // If the sequence_length:motif_width parity is EVEN, then
  // the allowable window sizes are ODD: 1, 3, 5, ...
  // If the sequence_length:motif_width parity is ODD, then
  // the allowable window sizes are EVEN: 2, 4, 6, ...
  // So we adjust min_win UP and max_win DOWN so that they have correct ODD/EVENness.
  int even = (n_bins % 2);
  if (even == ((min_win+1) % 2)) min_win++;
  if (even == ((max_win+1) % 2)) max_win--;
  n_scores = (options->optimize_score ? count_unique_scores(pve_scores, neg_scores) : 1);
  if (options->local) {
    // A_w = n_bins - w + 1
    // n_windows = \sum_w={min_win}^{max_win} A_w
    //  = \sum_w={1}^{max_win} A_w + \sum_w={min_win-1}^{max_win} A_w
    // algebra gives the following:
    n_windows = (n_bins+1)*(max_win-min_win+1) - (max_win*(max_win+1) - min_win*(min_win-1))/2;
  } else {
    //n_windows = ((max_win - (n_bins % 2)) / 2) + (n_bins % 2);
    // adjust max_win, min_win down if seq_length:motif_width parity is even
    // add in central bin if parity is even and min_win == 1
    n_windows = ((max_win - even) / 2) - ((min_win - 1 - even) / 2) + ((even && min_win==1) ? 1 : 0);
  }
  n_tests = n_windows * n_scores;
  log_n_tests = log(n_tests);
  // fprintf(stderr, "w %d l %d min_win %d max_win %d n_windows %d n_scores %d n_tests %d\n", get_motif_length(motif), seq_len, min_win, max_win, n_windows, n_scores, n_tests);
  // reset vars
  pve_index = neg_index = 0;
  reset_site_counts(buffers->pos_counts);
  reset_site_counts(buffers->neg_counts);
  best_windows = arraylst_create();
  current_windows = arraylst_create();
  best_log_pv = BIG;
  n_actual_tests = 0;
  max_spread = 2 * max_win - 1;
  // try each score threshold in turn
  while (1) {
    best_ignore_count = -1;
    // get the scoring threshold
    if (options->optimize_score) {
      score_thresh_min = largest_score(pve_scores, pve_index, neg_scores, neg_index);
      if (score_thresh_min == -BIG) break;
    } else {
      score_thresh_min = smallest_score(pve_scores, neg_scores);
    }
    // compute counts and advance the index to the first score worse than the threshold
    compute_counts(pve_scores, buffers->pos_counts, &pve_index, score_thresh_min);
    if (neg_scores) {
      compute_counts(neg_scores, buffers->neg_counts, &neg_index, score_thresh_min);
    }
    //convert to cumulative site counts
    accumulate_site_counts(buffers->pos_counts, buffers->pos_c_counts);
    accumulate_site_counts(buffers->neg_counts, buffers->neg_c_counts);

    if (options->local) {
      // calculate the first and last position for a window size of 1
      // pos_first = get_motif_length(motif) - 1;
      // pos_last = 2 * seq_len - get_motif_length(motif) - 1;
      // calculate the first and last position for a window size of min_win 
      pos_first = get_motif_length(motif) - 1 + (min_win-1);
      pos_last = 2 * seq_len - get_motif_length(motif) - 1 - (min_win-1);
      // loop over window sizes increasing
      //for (spread = 1; spread <= max_spread; spread += 2, pos_first++, pos_last--) {
      //fprintf(stderr, "pos_first %d pos_last %d max_spread %d n_bins %d min_win %d max_win %d\n", pos_first, pos_last, max_spread, n_bins, min_win, max_win);
      for (spread = 2*min_win-1; spread <= max_spread; spread += 2, pos_first++, pos_last--) {
        //loop over center positions
        for (pos = pos_first; pos <= pos_last; pos += 2) {
          test_window(options, log_pvalue_thresh, n_bins, log_n_tests,
              buffers->pos_c_counts, buffers->neg_c_counts, pos, spread,
              &best_ignore_count, current_windows, seqN, neg_seqN);
          n_actual_tests++;
        }
      }
    } else {
      pos = seq_len - 1;
      // loop over centered window sizes increasing
      for (spread=2*min_win-1; spread <= max_spread; spread += 4) {
        test_window(options, log_pvalue_thresh, n_bins, log_n_tests,
            buffers->pos_c_counts, buffers->neg_c_counts, pos, spread,
            &best_ignore_count, current_windows, seqN, neg_seqN);
        n_actual_tests++;
      }
    }
    if (!options->optimize_score) {
      // swap
      temp = best_windows;best_windows = current_windows;current_windows = temp;
      // keep track of best windows stats
      best_score_thresh = score_thresh_min;
      best_total_sites = buffers->pos_counts->total_sites;
      best_neg_total_sites = buffers->neg_counts->total_sites;
      break;
    }

    // find the best score for the current threshold
    current_log_pv = BIG;
    for (i = 0; i < arraylst_size(current_windows); i++) {
      win_stats = (WIN_STATS_T*) arraylst_get(i, current_windows);
      if (win_stats->log_adj_pvalue < current_log_pv) {
        current_log_pv = win_stats->log_adj_pvalue;
      }
    }
    // see if we found something better!
    if (current_log_pv < best_log_pv) {
      // swap
      temp = best_windows;best_windows = current_windows;current_windows = temp;
      // update pvalue
      best_log_pv = current_log_pv;
      // keep track of best windows stats
      best_score_thresh = score_thresh_min;
      best_total_sites = buffers->pos_counts->total_sites;
      best_neg_total_sites = buffers->neg_counts->total_sites;
    }
    arraylst_clear(destroy_window, current_windows);
  }
  // fprintf(stderr, "n_tests %d n_actual %d\n", n_tests, n_actual_tests);
  assert(n_tests == n_actual_tests);
  arraylst_destroy(destroy_window, current_windows);
  arraylst_qsort(window_stats_compare_pvalue, best_windows);
  // remove any weaker overlapping windows
  remove_redundant(best_windows);
  arraylst_fit(best_windows);
  // check if we found at least one good window
  if (arraylst_size(best_windows) == 0) {
    // nothing useful found
    arraylst_destroy(destroy_window, best_windows);
    return NULL;
  }
  // found some good windows, make motif stats
  motif_stats = mm_malloc(sizeof(MOTIF_STATS_T));
  memset(motif_stats, 0, sizeof(MOTIF_STATS_T));
  motif_stats->db = db;
  motif_stats->motif = motif;
  motif_stats->windows = best_windows;
  if (!options->noseq) { 
    win_stats = arraylst_get(0, best_windows);
    motif_stats->seq_ids = all_sequences_in_window(win_stats->center,
        win_stats->spread, best_score_thresh, pve_scores);
  }
  motif_stats->n_tests = n_tests;
  motif_stats->n_bins = n_bins;
  motif_stats->score_threshold = options->optimize_score ? best_score_thresh : options->score_thresh;
  motif_stats->sites = best_total_sites;
  motif_stats->neg_sites = best_neg_total_sites;
  return motif_stats;
} // calculate_best_windows

/*************************************************************************
 * Allocates memory for some buffers that centrimo needs
 *************************************************************************/
void create_buffers(CENTRIMO_BUFFER_T* buffers, int seqlen) {
  SEQ_SITES_T *sites;
  memset(buffers, 0, sizeof(CENTRIMO_BUFFER_T));
  sites = mm_malloc(sizeof(SEQ_SITES_T));
  memset(sites, 0, sizeof(SEQ_SITES_T));

  buffers->sites = sites;
  buffers->pos_counts = create_site_counts(seqlen);
  buffers->neg_counts = create_site_counts(seqlen);
  buffers->pos_c_counts = create_site_counts(seqlen);
  buffers->neg_c_counts = create_site_counts(seqlen);
}

/*************************************************************************
 * Frees memory for centrimo's buffers
 *************************************************************************/
void destroy_buffers(CENTRIMO_BUFFER_T* buffers) {
  SEQ_SITES_T *sites;
  sites = buffers->sites;
  destroy_site_counts(buffers->pos_counts);
  destroy_site_counts(buffers->neg_counts);
  destroy_site_counts(buffers->pos_c_counts);
  destroy_site_counts(buffers->neg_c_counts);
  free(sites->sites);
  memset(buffers, 0, sizeof(CENTRIMO_BUFFER_T)); 
}

/*************************************************************************
 * Entry point for centrimo
 *************************************************************************/
int main(int argc, char *argv[]) {
  CENTRIMO_OPTIONS_T options;
  CENTRIMO_BUFFER_T buffers;
  int seqlen, seqN, seq_skipped, neg_seqN, neg_seq_skipped, motifN;
  int i, db_i, motif_i;
  double log_pvalue_thresh, log_half;
  SEQ_T **sequences, **neg_sequences;
  ARRAY_T* bg_freqs;
  MOTIF_DB_T **dbs, *db;
  MOTIF_STATS_T *motif_stats;
  MOTIF_T *motif, *rev_motif;
  PSSM_T *pos_pssm, *rev_pssm;
  char *sites_path, *desc;
  FILE *sites_file, *text_out;
  HTMLWR_T *html;
  JSONWR_T *json;
  ARRAYLST_T *scores, *neg_scores;
  
  // command line processing
  process_command_line(argc, argv, &options);

  // load the positive sequences
  DEBUG_MSG(NORMAL_VERBOSE, "Loading sequences.\n");
  seqlen = options.seq_len;
  read_sequences(options.alphabet, options.seq_source, &sequences, &seqN, 
      &seq_skipped, &seqlen);
  bg_freqs = NULL;
  if (!options.bg_source) {
    bg_freqs = calc_bg_from_fastas(options.alphabet, seqN, sequences);
  }

  // load the negative sequences
  if (options.neg_sequences) {
    read_sequences(options.alphabet, options.negseq_source, &neg_sequences,
        &neg_seqN, &neg_seq_skipped, &seqlen);
  }

  // load the motifs
  DEBUG_MSG(NORMAL_VERBOSE, "Loading motifs.\n");
  dbs = mm_malloc(sizeof(MOTIF_DB_T*) * arraylst_size(options.motif_sources));
  for (motifN = 0, i = 0; i < arraylst_size(options.motif_sources); i++) {
    char* db_source;
    db_source = (char*) arraylst_get(i, options.motif_sources);
    dbs[i] = read_motifs(i, db_source, options.bg_source, &bg_freqs, 
        options.pseudocount, options.selected_motifs, options.alphabet, options.scan_separately);
    motifN += arraylst_size(dbs[i]->motifs);
  }
  // convert the evalue threshold into a pvalue threshold
  log_pvalue_thresh = log(options.evalue_thresh) - log(motifN);
  // if p-value threshold would be 1.0, reduce it slightly to
  // prevent jillions of absolutely non-significant peaks being printed
  if (log_pvalue_thresh >= 0) log_pvalue_thresh = log(0.999999999);

  // Setup some things for double strand scanning
  if (options.scan_both_strands == TRUE) {
    // Set up hash tables for computing reverse complement
    setup_hash_alph(DNAB);
    setalph(0);
    // Correct background by averaging on freq. for both strands.
    average_freq_with_complement(options.alphabet, bg_freqs);
    normalize_subarray(0, alph_size(options.alphabet, ALPH_SIZE), 0.0, bg_freqs);
    calc_ambigs(options.alphabet, FALSE, bg_freqs);
  }

  // Create output directory
  if (create_output_directory(options.output_dirname, options.allow_clobber,
        (verbosity >= NORMAL_VERBOSE))) {
    die("Couldn't create output directory %s.\n", options.output_dirname);
  }

  // open output files
  sites_file = start_centrimo_sites(&options);
  text_out = start_centrimo_text(&options);
  start_json(&options, argc, argv, sequences, seqN, seq_skipped, neg_seqN, 
      neg_seq_skipped, dbs, motifN, seqlen, &html, &json);

  // initialize local variables
  create_buffers(&buffers, seqlen);
  motif = rev_motif = NULL;
  pos_pssm = rev_pssm = NULL;
  scores = neg_scores = NULL;

  // calculate and output the best windows for each motif
  for (db_i = 0, i = 1; db_i < arraylst_size(options.motif_sources); db_i++) {
    db = dbs[db_i];
    for (motif_i = 0; motif_i < arraylst_size(db->motifs); motif_i++, i++) {
      motif = (MOTIF_T *) arraylst_get(motif_i, db->motifs);
      DEBUG_FMT(NORMAL_VERBOSE, "Using motif %s (%d/%d) of width %d.\n", 
          options.scan_separately ? get_motif_st_id(motif) : get_motif_id(motif), i, motifN, get_motif_length(motif));
      // create the pssm
      pos_pssm = make_pssm(bg_freqs, motif);
      // If required, do the same for the reverse complement motif.
      if (options.scan_both_strands) {
        rev_motif = dup_rc_motif(motif);
        rev_pssm = make_pssm(bg_freqs, rev_motif);
      }
      // score the sequences
      scores = score_sequences(&options, &buffers, sequences, seqN, pos_pssm, rev_pssm);
      if (options.neg_sequences) {
        neg_scores = score_sequences(&options, &buffers, neg_sequences, neg_seqN, pos_pssm, rev_pssm);
      }
      // calculate the best windows 
      motif_stats = calculate_best_windows(&options, &buffers,
          log_pvalue_thresh, db, motif, seqlen, scores, neg_scores, seqN, neg_seqN);
      // If there is a result to ouput
      if (motif_stats) {
        // recalculate the site counts for this specific score
        reset_site_counts(buffers.pos_counts);
        compute_counts(scores, buffers.pos_counts, NULL, motif_stats->score_threshold);
        if (options.neg_sequences) {
          reset_site_counts(buffers.neg_counts);
          compute_counts(neg_scores, buffers.neg_counts, NULL, motif_stats->score_threshold);
        }
        // Output JSON results
        if (json) {
          output_motif_json(json, seqlen, motif_stats, buffers.pos_counts, 
              buffers.neg_counts, !options.noseq, options.neg_sequences, 
              options.disc, options.mcc, options.scan_separately); // Write values into HTML file
        }
        output_site_counts(sites_file, options.scan_separately, seqlen, db, motif, buffers.pos_counts);
        output_centrimo_text(text_out, options.scan_separately, motif_stats, motifN, seqlen);
      }
      // Free memory associated with this motif.
      destroy_stats(motif_stats);
      arraylst_destroy(destroy_score, scores);
      if (options.neg_sequences) {
        arraylst_destroy(destroy_score, neg_scores);
      }
      free_pssm(pos_pssm);
      free_pssm(rev_pssm);
      destroy_motif(rev_motif);
    } // looping over motifs
  } // looping over motif databases

  end_json(html, json);
  // finish writing files
  fclose(sites_file);
  fclose(text_out);
  // Clean up.
  destroy_buffers(&buffers);
  for (i = 0; i < seqN; ++i) {
    free_seq(sequences[i]);
  }
  free(sequences);
  if (options.neg_sequences) {
    for (i = 0; i < neg_seqN; ++i) {
      free_seq(neg_sequences[i]);
    }
    free(neg_sequences);
  }
  for (i = 0; i < arraylst_size(options.motif_sources); i++) {
    free_db(dbs[i]);
  }
  free(dbs);
  free_array(bg_freqs);
  cleanup_options(&options);
  DEBUG_MSG(NORMAL_VERBOSE, "Program ends correctly\n");
  return EXIT_SUCCESS;
} // main
