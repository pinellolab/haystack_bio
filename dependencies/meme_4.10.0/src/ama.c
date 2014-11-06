/********************************************************************
 * FILE: ama.c
 * AUTHOR: Fabian Buske and Timothy L. Bailey
 * CREATE DATE: 03/07/2008
 * PROJECT: MEME suite
 * COPYRIGHT: 2008, UQ
 *
 * AMA (average motif affinity) is part of an implementation of the
 * algorithm described in
 * "Associating transcription factor binding site motifs with target
 * Go terms and target genes"
 * authors: Mikael Boden and Timothy L. Bailey
 * published: Nucl. Acids Res (2008)
 *
 * The implementation is based on the fimo class in order to
 * use the scoring schemes described in the publication.
 *
 * AMA works on DNA data only.
 ********************************************************************/

#define DEFINE_GLOBALS
#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <float.h>
#include <getopt.h>
#include <math.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include "matrix.h"
#include "alphabet.h"
#include "cisml.h"
#include "fasta-io.h"
#include "motif-in.h"
#include "io.h"
#include "string-list.h"
#include "simple-getopt.h"
#include "array.h"
#include "macros.h"
#include "motif.h"
#include "matrix.h"
#include "gendb.h"
#include "pssm.h"
#include "background.h"
#include "hash_alph.h"
#include "red-black-tree.h"

typedef enum {GFF_FORMAT, CISML_FORMAT, DIRECTORY_FORMAT} OUT_FORMAT_EN;
typedef enum {AVG_ODDS, MAX_ODDS, SUM_ODDS} SCORING_EN;

const char * AMA_USAGE =
  "USAGE: ama [options] <motif file> <sequence file> [<background file>]\n"
  "\n"
  "   Options:\n"
  "     --sdbg <order>           Use Markov background model of order <order>\n"
  "                                derived from the sequence to compute its\n"
  "                                likelihood ratios;\n"
  "                                overrides --pvalues, --gcbins and --rma;\n"
  "                                <background file> is required unless\n"
  "                                --sdbg is given.\n"
  "     --motif <id>             Use only the motif identified by <id>.\n"
  "                                This option may be repeated.\n"
  "     --motif-pseudo <float>   The value <float> times the background\n"
  "                                frequency is added to the count of each\n"
  "                                letter when creating the likelihood \n"
  "                                ratio matrix (default: %g).\n"
  "     --norc                   Disables the scanning of the reverse\n"
  "                                complement strand.\n"
  "     --scoring [avg-odds|max-odds]\n"
  "                              Indicates whether the average or \n"
  "                                the maximum odds should be calculated\n"
  "                                (default: avg-odds)\n"
  "     --rma                    Scale motif scores to the range 0-1.\n"
  "                                (Relative Motif Affinity).\n"
  "                                Motif scores are scaled by the maximum\n"
  "                                score achievable by that PWM. (default:\n"
  "                                motif scores are not normalized)\n"
  "     --pvalues                Print p-value of avg-odds score in cisml\n"
  "                                output. Ignored for max-odds scoring.\n"
  "                                (default: p-values are not printed)\n"
  "     --gcbins <bins>          Compensate p-values for GC content of\n"
  "                                each sequence using given number of \n"
  "                                GC range bins. Recommended bins: 41.\n"
  "                                (default: p-values are based on\n"
  "                                frequencies in background file)\n"
  "     --cs                     Enable combining sequences with same\n"
  "                                identifier by taking the average score\n"
  "                                and the Sidac corrected p-value.\n"
  "     --o-format [gff|cisml]   Output file format (default: cisml)\n"
  "                                ignored if --o or --oc option used\n"
  "     --o <directory>          Output all available formats to\n"
  "                                <directory>; give up if <directory>\n"
  "                                exists\n"
  "     --oc <directory>         Output all available formats to\n"
  "                                <directory>; if <directory> exists\n"
  "                                overwrite contents\n"
  "     --verbosity [1|2|3|4]    Controls amount of screen output\n"
  "                                (default: 2)\n"
  "     --max-seq-length <int>   Set the maximum length allowed for \n"
  "                                input sequences. (default: %d)\n"
  "     --last <int>             Use only scores of (up to) last <n>\n"
  "                                sequence positions to compute AMA.\n"
  "     --version                Print version and exit.\n"
  "\n";

const char* PROGRAM_NAME = "ama";

#define DEFAULTDIR "ama_out"
#define sqrt2 sqrt(2.0)

static char *default_output_dirname = DEFAULTDIR;  // default output directory name
static const char *text_filename = "ama.txt";
static const char *cisml_filename = "ama.xml";

VERBOSE_T verbosity = NORMAL_VERBOSE;

// Structure for tracking ama command line parameters.
typedef struct options {
  int max_seq_length;
  bool scan_both_strands;
  bool combine_duplicates;
  RBTREE_T *selected_motifs;
  double pseudocount;
  OUT_FORMAT_EN output_format;
  bool clobber;
  char *out_dir;
  SCORING_EN scoring;
  bool pvalues;
  bool normalize_scores;
  int num_gc_bins;
  int sdbg_order;
  int last;
  char *motif_filename;
  char *fasta_filename;
  char *bg_filename;
} AMA_OPTIONS_T;

typedef struct motif_and_pssm {
  MOTIF_T *motif;
  PSSM_PAIR_T *pssm_pair;
} MOTIF_AND_PSSM_T;


#define min(a,b)      (a<b)?a:b
#define max(a,b)      (a>b)?a:b

/*************************************************************************
 * Calculate the odds score for each motif-sized window at each
 * site in the sequence using the given nucleotide frequencies.
 *
 * This function is a lightweight version based on the one contained in
 * motiph-scoring. Several calculations that are unnecessary for gomo
 * have been removed in order to speed up the process.
 * Scores sequence with up to two motifs.
 *************************************************************************/
static double score_sequence(
  ALPH_T        alph,         // alphabet (IN)
  SEQ_T*        seq,          // sequence to scan (IN)
  double        *logcumback,  // cumulative bkg probability of sequence (IN)
  PSSM_PAIR_T   *pssm_pair,   // pos and neg pssms (IN)
  SCORING_EN    method,       // method used for scoring (IN)
  int           last,         // score only last <n> or score all if <n> 
                              //                                  is zero (IN)
  BOOLEAN_T* isFeasible       // FLAG indicated if there is at least one position
                              // where the motif could be matched against (OUT)
)
{
  PSSM_T *pos_pssm, *neg_pssm, *pssm;
  int strands, seq_length, w, n, asize, strand, start, N_scored, s_pos, m_pos;
  double max_odds, sum_odds, requested_odds, odds, adjust, log_p;
  int8_t *isequence, *iseq;

  assert(pssm_pair != NULL);
  assert(seq != NULL);

  asize = alph_size(alph, ALPH_SIZE);
  pos_pssm = pssm_pair->pos_pssm;
  assert(pos_pssm != NULL);
  neg_pssm = pssm_pair->neg_pssm;
  strands = neg_pssm ? 2 : 1;

  isequence = get_isequence(seq);
  seq_length = get_seq_length(seq);
  w = get_num_rows(pos_pssm->matrix);
  n = seq_length - w + 1;

  if (verbosity >= DUMP_VERBOSE) {
    fprintf(stderr, "Debug strands: %d seq_length: %d w: %d n: %d.\n", 
        strands, seq_length, w, n);
  }
  // Dependent on the "last" parameter, change the starting point
  if (last > 0 && last < seq_length) {
    start = seq_length - last;
    N_scored  = strands * (last - w + 1); // number of sites scored
  } else {
    start = 0;
    N_scored  = strands * n; // number of sites scored
  }

  // For each motif (positive and reverse complement)
  max_odds = 0.0;
  sum_odds = 0.0;

  if (verbosity >= HIGHER_VERBOSE) {
    fprintf(stderr, "Starting scan at position %d .\n", start);
  }

  for (strand = 0; strand < strands; strand++) { // pos (and negative) motif
   pssm = (strand == 0 ? pos_pssm : neg_pssm); // choose +/- motif
    // For each site in the sequence
    for (s_pos = start; s_pos < n; s_pos++) {
      odds = 1.0;
      // For each position in the motif window
      for (m_pos = 0, iseq = isequence+s_pos; m_pos < w; m_pos++, iseq++) {
        if (*iseq == -1) {
          N_scored--; 
          odds = 0; 
          break; 
        }
        // multiple odds by value in appropriate motif cell
        odds *= get_matrix_cell(m_pos, *iseq, pssm->matrix);
      }
      // Apply sequence-dependent background model.
      if (logcumback) {
        log_p = logcumback[s_pos+w] - logcumback[s_pos]; // log Pr(x | background)
        //printf("log_p:: %g motif_pos %d\n", log_p, m_pos);
        adjust = exp(w*log(1/4.0) - log_p); // Pr(x | uniform) / Pr(x | background)
        odds *= adjust;
      }
      // Add odds to growing sum.
      sum_odds += odds; // sum of odds
      if (odds > max_odds) max_odds = odds; // max of odds
    } // site
  } // strand

  if (verbosity >= HIGHER_VERBOSE) {
    fprintf(stderr, "Scored %d positions with the sum odds %f and the "
        "max odds %f.\n", N_scored, sum_odds, max_odds);
  }

  // has there been anything matched at all?
  if (N_scored == 0) {
    if (verbosity >= NORMAL_VERBOSE) {
      fprintf(stderr,"Sequence \'%s\' offers no location to match "
          "the motif against (sequence length too short?)\n",
          get_seq_name(seq));
    }
    *isFeasible = false;
    return 0.0;
    // return odds as requested (MAX or AVG scoring)
  } else if (method == AVG_ODDS) {
    return sum_odds / N_scored;  // mean
  } else if (method == MAX_ODDS) {
    return max_odds;             // maximum
  } else if (method == SUM_ODDS) {
    return sum_odds;             // sum
  } else {
    die("Unknown scoring method");
    // should not get here... but the compiler will complain if I don't handle this case
    *isFeasible = false;
    return 0.0;
  }
} // score_sequence

/**********************************************************************
  ama_sequence_scan()

  Scan a given sequence with a specified motif using either
  average motif affinity scoring or maximum one. In addition z-scores
  may be calculated. Also the scan can be limited to only the end of
  the passed sequences.

  The motif has to be converted to odds in advance (in order
  to speed up the scanning).

  The result will be stored in the scanned_sequence parameter.
 **********************************************************************/
void ama_sequence_scan(
  ALPH_T      alph,         // alphabet
  SEQ_T       *sequence,    // the sequence to scan (IN)
  double      *logcumback,  // cumulative bkg probability of sequence (IN)
  PSSM_PAIR_T *pssm_pair,   // the pos/neg pssms (IN)
  int         scoring,      // AVG_ODDS or MAX_ODDS (IN)
  BOOLEAN_T   pvalues,      // compute p-values (IN)
  int         last,         // use only last <n> sequence positions
                            // or 0 if all positions should be used
  SCANNED_SEQUENCE_T* scanned_seq,// the scanned sequence results (OUT)
  BOOLEAN_T* need_postprocessing // Flag indicating the need for postprocessing (OUT)
)
{
  assert(sequence != NULL);
  assert(pssm_pair != NULL);

  // FLAG indicates if sequence was suitable for motif matching
  BOOLEAN_T isFeasible = true;

  // Score the sequence.
  double odds = score_sequence(alph, sequence, logcumback, 
      pssm_pair, scoring, last, &isFeasible);
        
  // Compute the p-value of the AVG_ODDS score.
  if (get_scanned_sequence_num_scanned_positions(scanned_seq) == 0L) {
    set_scanned_sequence_score(scanned_seq, odds);
    // sequence has not been scanned before
    if (!isFeasible) {
      if (verbosity >= NORMAL_VERBOSE) {
      fprintf(stderr,"Sequence '%s' not suited for motif. P-value "
        "set to 1.0!\n", get_scanned_sequence_accession(scanned_seq));
      }
      set_scanned_sequence_pvalue(scanned_seq, 1.0);
    } else if (odds < 0.0){
      if (verbosity >= NORMAL_VERBOSE) {
        fprintf(stderr,"Sequence '%s' got invalid (negative) odds "
            "score. P-value set to 1.0!\n",
            get_scanned_sequence_accession(scanned_seq));
      }
      set_scanned_sequence_pvalue(scanned_seq, 1.0);
    } else if (pvalues && scoring == AVG_ODDS) {
      double pvalue = get_ama_pv(odds, get_scanned_sequence_length(scanned_seq),
          get_total_gc_sequence(sequence), pssm_pair);
      set_scanned_sequence_pvalue(scanned_seq, pvalue);
    }
    // scanned_position is used to keep track how often a sequence has been scored
    // this feature is used in downstream gomo where a one2many homolog relationship
    // is encoded through the same sequence identifier
    add_scanned_sequence_scanned_position(scanned_seq); 
  } else {
    // sequence has been scored before
    if(!has_scanned_sequence_score(scanned_seq)) {
      // no score set yet, so do
      set_scanned_sequence_score(scanned_seq, odds);
    } else {
      // sum scores (take average later)
      set_scanned_sequence_score(scanned_seq, odds + 
          get_scanned_sequence_score(scanned_seq));
    }
    if (!isFeasible) {
      if (verbosity >= NORMAL_VERBOSE) {
        fprintf(stderr,"Sequence '%s' not suited for motif. P-value set "
            "to 1.0!\n", get_scanned_sequence_accession(scanned_seq));
      }
      if (!has_scanned_sequence_pvalue(scanned_seq)) {
        set_scanned_sequence_pvalue(scanned_seq, 1.0);
      }
    } else if (odds < 0.0) {
      if (verbosity >= NORMAL_VERBOSE) {
        fprintf(stderr,"Sequence '%s' got invalid (negative) odds score. "
            "P-value set to 1.0!\n", get_scanned_sequence_accession(scanned_seq));
      }
      if (!has_scanned_sequence_pvalue(scanned_seq)) { 
        set_scanned_sequence_pvalue(scanned_seq, 1.0);
      }
    } else if (pvalues && scoring == AVG_ODDS) {
      double pvalue = get_ama_pv(odds, get_scanned_sequence_length(scanned_seq),
          get_total_gc_sequence(sequence), pssm_pair);
      if (!has_scanned_sequence_pvalue(scanned_seq)) {
        set_scanned_sequence_pvalue(scanned_seq, pvalue);
      } else {
        // keep minimum p-value only
        set_scanned_sequence_pvalue(scanned_seq, min(pvalue, 
              get_scanned_sequence_pvalue(scanned_seq)));
      }
    }
    add_scanned_sequence_scanned_position(scanned_seq); 
    *need_postprocessing = true;
  }
} // ama_sequence_scan
/**********************************************************************
 print_score()

 outputs the scores in a gff format
 **********************************************************************/
void print_score(CISML_T* cisml, FILE* gff_file) {

  // iterate over all patterns
  int num_patterns = get_cisml_num_patterns(cisml);
  if (num_patterns > 0) {
    PATTERN_T **patterns = get_cisml_patterns(cisml);
    int i = 0;
    for (i = 0; i < num_patterns; ++i) {
          char* pattern_id = get_pattern_accession(patterns[i]);
          // iterate over all sequences
      int num_seq = 0;
      num_seq = get_pattern_num_scanned_sequences(patterns[i]);
      SCANNED_SEQUENCE_T **sequences = get_pattern_scanned_sequences(
                      patterns[i]);
      int j = 0;
      for (j = 0; j < num_seq; ++j) {
        double score = 0.0;
        double pvalue = 1.0;

        if (has_scanned_sequence_score(sequences[j])) {
          score = get_scanned_sequence_score(sequences[j]);
        }
        if (has_scanned_sequence_pvalue(sequences[j])) {
          pvalue = get_scanned_sequence_pvalue(sequences[j]);
        }
        fprintf(gff_file, "%s", get_scanned_sequence_accession(
                        sequences[j]));
        fprintf(gff_file, "\t%s", PROGRAM_NAME);
        fprintf(gff_file, "\tsequence");
        fprintf(gff_file, "\t1"); // Start
        fprintf(gff_file, "\t%d", get_scanned_sequence_length(
                        sequences[j])); // End
        fprintf(gff_file, "\t%g", score); // Score
        fprintf(gff_file, "\t%g", pvalue); // Score
        fprintf(gff_file, "\t."); // Strand
        fprintf(gff_file, "\t."); // Frame
        fprintf(gff_file, "\t%s\n",pattern_id); // Comment
      } // num_seq
    } // num_pattern
  } // pattern > 0
}

/**********************************************************************
 post_process()
 
 adjust/normalize scores and p-values
 **********************************************************************/
void post_process(CISML_T* cisml, ARRAYLST_T* motifs, BOOLEAN_T normalize_scores){
  int m_index, seq_index;
  MOTIF_AND_PSSM_T *combo;
  for (m_index = 0; m_index < get_cisml_num_patterns(cisml); ++m_index) {
    PATTERN_T* pattern = get_cisml_patterns(cisml)[m_index];
    double maxscore = 1;
    
    // FIXME: This should be done to the PSSM, not the individual scores!!!
    // Normalize the scores to RMA format if necessary.
    if (normalize_scores) {
      int k;
      combo = (MOTIF_AND_PSSM_T*)arraylst_get(m_index, motifs);
      PSSM_T* pssm = combo->pssm_pair->pos_pssm;
      for (k = 0; k < pssm->w; k++) {
        double maxprob = -BIG;    // These are scores, not probabilities!!!
        int a;
        for (a = 0; a < alph_size(pssm->alph, ALPH_SIZE); a++) {
          double prob = get_matrix_cell(k, a, pssm->matrix);
          if (maxprob < prob) maxprob = prob;
        }
        maxscore *= maxprob;
      }
    }
    
    // adjust each scanned sequence
    for (seq_index = 0; seq_index < get_pattern_num_scanned_sequences(pattern); 
        ++seq_index) {
      SCANNED_SEQUENCE_T* scanned_seq = 
        get_pattern_scanned_sequences(pattern)[seq_index];
      // only adjust scores and p-values if more than one copy was scored
      // num_scanned_positions is (mis-)used in ama to indicate the number of times 
      // a sequence identifier 0occured in the set
      if (get_scanned_sequence_num_scanned_positions(scanned_seq) > 1L){
        // take average score
        if(has_scanned_sequence_score(scanned_seq)){
          double avg_odds = get_scanned_sequence_score(scanned_seq) / 
            get_scanned_sequence_num_scanned_positions(scanned_seq);
          set_scanned_sequence_score(scanned_seq, avg_odds);
        }
        // adjust the minimum p-value for multiple hypothesis testing
        if(has_scanned_sequence_pvalue(scanned_seq)){
          double corr_pvalue = 1.0 - pow(
              1.0 - get_scanned_sequence_pvalue(scanned_seq),
              get_scanned_sequence_num_scanned_positions(scanned_seq)
              );
          set_scanned_sequence_pvalue(scanned_seq, corr_pvalue);
        }
      }
      
      // normalize if requested
      if (normalize_scores) {
        set_scanned_sequence_score(scanned_seq, 
            get_scanned_sequence_score(scanned_seq) / maxscore
            );
      }
    }
  }
}

static void usage(char *format, ...) {
  va_list argp;

  if (format) {
    va_start(argp, format);
    vfprintf(stderr, format, argp);
    va_end(argp);
    fputs("\n", stderr);
    fputs(AMA_USAGE, stderr);
    fflush(stderr);
  } else {
    puts(AMA_USAGE);
  }
  if (format) exit(EXIT_FAILURE);
  exit(EXIT_SUCCESS);
}

#define OPT_MAX_SEQ_LENGTH 0
#define OPT_MOTIF 1
#define OPT_MOTIF_PSEUDO 2
#define OPT_RMA 3
#define OPT_PVALUES 4
#define OPT_SDBG 5
#define OPT_NORC 6
#define OPT_CS 7
#define OPT_O_FORMAT 8
#define OPT_O 9
#define OPT_OC 10
#define OPT_SCORING 11
#define OPT_VERBOSITY 12
#define OPT_GCBINS 13
#define OPT_LAST 14
#define OPT_VERSION 15
static void process_command_line(int argc, char **argv, AMA_OPTIONS_T *options) {
  int option_index = 0;
  const int num_options = 16;
  struct option ama_options[] = {
    {"max-seq-length",  required_argument,  NULL, OPT_MAX_SEQ_LENGTH},
    {"motif",           required_argument,  NULL, OPT_MOTIF},
    {"motif-pseudo",    required_argument,  NULL, OPT_MOTIF_PSEUDO},
    {"rma",             no_argument,        NULL, OPT_RMA},
    {"pvalues",         no_argument,        NULL, OPT_PVALUES},
    {"sdbg",            required_argument,  NULL, OPT_SDBG},
    {"norc",            no_argument,        NULL, OPT_NORC},
    {"cs",              no_argument,        NULL, OPT_CS},
    {"o-format",        required_argument,  NULL, OPT_O_FORMAT},
    {"o",               required_argument,  NULL, OPT_O},
    {"oc",              required_argument,  NULL, OPT_OC},
    {"scoring",         required_argument,  NULL, OPT_SCORING},
    {"verbosity",       required_argument,  NULL, OPT_VERBOSITY},
    {"gcbins",          required_argument,  NULL, OPT_GCBINS},
    {"last",            required_argument,  NULL, OPT_LAST},
    {"version",         no_argument,        NULL, OPT_VERSION},
    {NULL, 0, NULL, 0} //boundary indicator
  };
  bool out_set = false;
  bool format_set = false;
  // set option defaults
  options->max_seq_length = MAX_SEQ;
  options->scan_both_strands = true;
  options->combine_duplicates = false;
  options->selected_motifs = rbtree_create(rbtree_strcmp, NULL, NULL, NULL, NULL);
  options->pseudocount = 0.01;
  options->output_format = CISML_FORMAT;
  options->clobber = false;
  options->out_dir = NULL;
  options->scoring = AVG_ODDS;
  options->pvalues = false;
  options->normalize_scores = false;
  options->num_gc_bins = 1;
  options->sdbg_order = -1;
  options->last = 0;
  options->motif_filename = NULL;
  options->fasta_filename = NULL;
  options->bg_filename = NULL;

  // parse command line
  while (1) {
    int opt = getopt_long_only(argc, argv, "", ama_options, NULL);
    if (opt == -1) break;
    switch (opt) {
      case OPT_MAX_SEQ_LENGTH:
        options->max_seq_length = atoi(optarg);
        break;
      case OPT_MOTIF:
        rbtree_make(options->selected_motifs, optarg, NULL);
        break;
      case OPT_MOTIF_PSEUDO:
        options->pseudocount = atof(optarg);
        break;
      case OPT_RMA:
        options->normalize_scores = true;
        break;
      case OPT_PVALUES:
        options->pvalues = true;
        break;
      case OPT_SDBG:
        options->sdbg_order = atoi(optarg); // >=0 means use sequence bkg
        break;
      case OPT_NORC:
        options->scan_both_strands = false;
        break;
      case OPT_CS:
        options->combine_duplicates = true;
        break;
      case OPT_O_FORMAT:
        if (out_set) {
          usage("Option -o-format is incompatible with option -o/-oc");
        } else {
          format_set = true;
          if (strcmp(optarg, "gff") == 0) {
            options->output_format = GFF_FORMAT;
          } else if (strcmp(optarg, "cisml") == 0) {
            options->output_format = CISML_FORMAT;
          } else {
            usage("Output format \"%s\" is not recognised. "
                "Expected \"gff\" or \"cisml\".", optarg);
          }
        }
        break;
      case OPT_OC:
        options->clobber = true;
      case OPT_O:
        if (format_set) {
          usage("Option -o/-oc is incompatible with option -o-format");
        } else {
          out_set = true;
          options->out_dir = optarg;
          options->output_format = DIRECTORY_FORMAT;
        }
        break;
      case OPT_SCORING:
        if (strcmp(optarg, "max-odds") == 0) {
          options->scoring = MAX_ODDS;
        } else if (strcmp(optarg, "avg-odds") == 0) {
          options->scoring = AVG_ODDS;
        } else if (strcmp(optarg, "sum-odds") == 0) {
          options->scoring = SUM_ODDS;
        } else {
          usage("Scoring method \"%s\" is not recognised. "
              "Expected \"max-odds\", \"avg-odds\" or \"sum-odds\".", optarg);
        }
        break;
      case OPT_VERBOSITY:
        verbosity = atoi(optarg);
        break;
      case OPT_GCBINS:
        options->num_gc_bins = atoi(optarg);
        options->pvalues = true;
        if (options->num_gc_bins <= 1)
          usage("Number of bins in --gcbins must be greater than 1.");
        break;
      case OPT_LAST:
        options->last = atoi(optarg);
        if (options->last < 0) usage("Option --last must not be negative.");
        break;
      case OPT_VERSION:
        fprintf(stdout, VERSION "\n");
        exit(EXIT_SUCCESS);
        break;
      case '?':           //unrecognised or ambiguous argument
        usage("Unrecognized or ambiguous option.");
    }
  }

  // --sdbg overrides --pvalues and --gcbins and --rma
  if (options->sdbg_order >= 0) {
    options->pvalues = false;
    options->normalize_scores = false;
    options->num_gc_bins = 1;
  }

  if (argc <= optind) usage("Expected motif file.");
  options->motif_filename = argv[optind++];
  if (argc <= optind) usage("Expected fasta file.");
  options->fasta_filename = argv[optind++];
  if (argc > optind) options->bg_filename = argv[optind++];

  if (options->sdbg_order >= 0) {
    if (options->bg_filename) usage("A background file can not be used together with --sdbg.");
  } else {
    if (!options->bg_filename) usage("Expected background file");
  }
  if (argc > optind) usage("Too many parameters");
  // for now, use uniform to mimic old implementation. I will probably remove this later
  if (!options->bg_filename) options->bg_filename = "--uniform--";
}

MOTIF_AND_PSSM_T* motif_and_pssm_create(MOTIF_T *motif, PSSM_T *pos_pssm, PSSM_T *neg_pssm) {
  MOTIF_AND_PSSM_T *motif_combo;
  // allocate memory for PSSM pairs
  motif_combo = mm_malloc(sizeof(MOTIF_AND_PSSM_T));
  motif_combo->motif = motif;
  motif_combo->pssm_pair = create_pssm_pair(pos_pssm, neg_pssm);
  return motif_combo;
}

void motif_and_pssm_destroy(void *value) {
  MOTIF_AND_PSSM_T *motif_combo;
  motif_combo = (MOTIF_AND_PSSM_T*)value;
  destroy_motif(motif_combo->motif);
  free_pssm_pair(motif_combo->pssm_pair);
  memset(motif_combo, 0, sizeof(MOTIF_AND_PSSM_T));
  free(motif_combo);
}

ARRAYLST_T* load_motifs(const char* motif_filename,
    const char* bg_filename, const double pseudocount, 
    RBTREE_T *selected_motifs,
    const int num_gc_bins, const bool scan_both_strands) {
  ARRAYLST_T *motifs;
  ARRAY_T *pos_bg_freqs, *rev_bg_freqs;
  MREAD_T *mread;
  MOTIF_T *motif, *motif_rc;
  double range;
  PSSM_T *pos_pssm, *neg_pssm;
  int total_motifs;

  //
  // Read the motifs and background model.
  //
  //this reads any meme file, xml, txt and html
  mread = mread_create(motif_filename, OPEN_MFILE);
  mread_set_bg_source(mread, bg_filename);
  mread_set_pseudocount(mread, pseudocount);

  // sanity check, since the rest of the code relies on the motifs being DNA
  if (mread_get_alphabet(mread) != DNA_ALPH)
    die("Expected motifs to use the DNA alphabet");

  pos_bg_freqs = mread_get_background(mread);
  rev_bg_freqs = NULL;
  if (scan_both_strands == true) {
    rev_bg_freqs = allocate_array(get_array_length(pos_bg_freqs));
    complement_dna_freqs(pos_bg_freqs, rev_bg_freqs);
  }

  // allocate memory for motifs
  motifs = arraylst_create();
  //
  // Convert motif matrices into log-odds matrices.
  // Scale them.
  // Compute the lookup tables for the PDF of scaled log-odds scores.
  //
  range = 300; // 100 is not very good; 1000 is great but too slow
  neg_pssm = NULL;
  total_motifs = 0;
  while (mread_has_motif(mread)) {
    motif = mread_next_motif(mread);
    total_motifs++;
    if (rbtree_size(selected_motifs) == 0 || rbtree_find(selected_motifs, get_motif_id(motif)) != NULL) {
      if (verbosity >= HIGH_VERBOSE) {
        fprintf(stderr, "Using motif %s of width %d.\n", get_motif_id(motif), get_motif_length(motif));
      }
      pos_pssm =
        build_motif_pssm(
          motif, 
          pos_bg_freqs, 
          pos_bg_freqs, 
          NULL, // Priors not used
          0.0L, // alpha not used
          range, 
          num_gc_bins, 
          true 
        );
      //
      //  Note: If scanning both strands, we complement the motif frequencies
      //  but not the background frequencies so the motif looks the same.
      //  However, the given frequencies are used in computing the p-values
      //  since they represent the frequencies on the negative strands.
      //  (If we instead were to complement the input sequence, keeping the
      //  the motif fixed, we would need to use the complemented frequencies
      //  in computing the p-values.  Is that any clearer?)
      //
      if (scan_both_strands) {
        motif_rc = dup_rc_motif(motif);
        neg_pssm =
          build_motif_pssm(
            motif_rc, 
            rev_bg_freqs, 
            pos_bg_freqs, 
            NULL, // Priors not used
            0.0L, // alpha not used
            range, 
            num_gc_bins, 
            true
          );
        destroy_motif(motif_rc);
      }
      arraylst_add(motif_and_pssm_create(motif, pos_pssm, neg_pssm), motifs);
    } else {
      if (verbosity >= HIGH_VERBOSE) fprintf(stderr, "Skipping motif %s.\n",
          get_motif_id(motif));
      destroy_motif(motif);
    }
  }
  mread_destroy(mread);
  free_array(pos_bg_freqs);
  free_array(rev_bg_freqs);
  if (verbosity >= NORMAL_VERBOSE) {
    fprintf(stderr, "Loaded %d/%d motifs from %s.\n", 
        arraylst_size(motifs), total_motifs, motif_filename);
  }
  return motifs;
}

double * log_cumulative_background(const int sdbg_order, SEQ_T *sequence) {
  double *logcumback, *a_cp;
  char *raw_seq;
  if (sdbg_order < 0) die("No such thing as a negative background order");

  logcumback = mm_malloc(sizeof(double) * (get_seq_length(sequence)+1));
  raw_seq = get_raw_sequence(sequence);
  a_cp = get_markov_from_sequence(raw_seq, alph_string(DNA_ALPH), false, sdbg_order, 0);
  log_cum_back(raw_seq, a_cp, sdbg_order, logcumback);
  myfree(a_cp);
  return logcumback;
}

/*************************************************************************
 * Entry point for ama
 *************************************************************************/
int main(int argc, char **argv) {
  AMA_OPTIONS_T options;
  ARRAYLST_T *motifs;
  clock_t c0, c1; // measuring cpu_time
  MOTIF_AND_PSSM_T *combo;
  CISML_T *cisml;
  PATTERN_T** patterns;
  PATTERN_T *pattern;
  FILE *fasta_file, *text_output, *cisml_output;
  int i, seq_loading_num, seq_counter, unique_seqs, seq_len, scan_len;
  char *seq_name, *path;
  bool need_postprocessing, created;
  SEQ_T *sequence;
  RBTREE_T *seq_ids;
  RBNODE_T *seq_node;
  double *logcumback;

  // process the command
  process_command_line(argc, argv, &options);

  // load DNA motifs
  motifs = load_motifs(options.motif_filename, options.bg_filename, 
      options.pseudocount, options.selected_motifs,
      options.num_gc_bins, options.scan_both_strands);

  // record starting time
  c0 = clock();

  // Set up DNA hash tables for computing reverse complement if doing --sdbg
  if (options.sdbg_order >= 0) setup_hash_alph(DNAB);

  // Create cisml data structure for recording results
  cisml = allocate_cisml(PROGRAM_NAME, options.motif_filename, options.fasta_filename);
  set_cisml_background_file(cisml, options.bg_filename);

  // make a CISML pattern to hold scores for each motif
  for (i = 0; i < arraylst_size(motifs); i++) {
    combo = (MOTIF_AND_PSSM_T*)arraylst_get(i, motifs);
    add_cisml_pattern(cisml, allocate_pattern(get_motif_id(combo->motif), ""));
  }

  // Open the FASTA file for reading.
  fasta_file = NULL;
  if (!open_file(options.fasta_filename, "r", false, "FASTA", "sequences", &fasta_file)) {
    die("Couldn't open the file %s.\n", options.fasta_filename);
  }
  if (verbosity >= NORMAL_VERBOSE) {
    if (options.last == 0) {
      fprintf(stderr, "Using entire sequence\n");
    } else {
      fprintf(stderr, "Limiting sequence to last %d positions.\n", options.last);
    }
  }

  //
  // Read in all sequences and score with all motifs
  //
  seq_loading_num = 0;  // keeps track on the number of sequences read in total
  seq_counter = 0;      // holds the index to the seq in the pattern
  unique_seqs = 0;      // keeps track on the number of unique sequences
  need_postprocessing = false;
  sequence = NULL;
  logcumback = NULL;
  seq_ids = rbtree_create(rbtree_strcasecmp,rbtree_strcpy,free,rbtree_intcpy,free);
  while (read_one_fasta(DNA_ALPH, fasta_file, options.max_seq_length, &sequence)) {
    ++seq_loading_num;
    seq_name = get_seq_name(sequence);
    seq_len = get_seq_length(sequence);
    scan_len = (options.last != 0 ? options.last : seq_len);
    // red-black trees are only required if duplicates should be combined
    if (options.combine_duplicates){
      //lookup seq id and create new entry if required, return sequence index
      seq_node = rbtree_lookup(seq_ids, get_seq_name(sequence), true, &created);
      if (created) { // assign it a loading number
        rbtree_set(seq_ids, seq_node, &unique_seqs);
        seq_counter = unique_seqs;
        ++unique_seqs;
      } else {
        seq_counter = *((int*)rbnode_get(seq_node));
      }
    }
          
    //
    // Set up sequence-dependent background model and compute
    // log cumulative probability of sequence.
    // This needs the sequence in raw format.
    //
    if (options.sdbg_order >= 0)
      logcumback = log_cumulative_background(options.sdbg_order, sequence);

    // Index the sequence, throwing away the raw format and ambiguous characters
    index_sequence(sequence, DNA_ALPH, SEQ_NOAMBIG);

    // Get the GC content of the sequence if binning p-values by GC
    // and store it in the sequence object.
    if (options.num_gc_bins > 1) {
      ARRAY_T *freqs = get_sequence_freqs(sequence, DNA_ALPH);
      set_total_gc_sequence(sequence,
        get_array_item(alph_index(DNA_ALPH, 'C'), freqs) +
        get_array_item(alph_index(DNA_ALPH, 'G'),freqs));     // f(C) + f(G)
      free_array(freqs);                        // clean up
    } else {
      set_total_gc_sequence(sequence, -1);      // flag ignore
    }

    // Scan with motifs.
    for (i = 0; i < arraylst_size(motifs); i++) {
      pattern = get_cisml_patterns(cisml)[i];
      combo = (MOTIF_AND_PSSM_T*)arraylst_get(i, motifs);
      if (verbosity >= HIGHER_VERBOSE) {
        fprintf(stderr, "Scanning %s sequence with length %d "
            "abbreviated to %d with motif %s with length %d.\n",
            seq_name, seq_len, scan_len, 
            get_motif_id(combo->motif), get_motif_length(combo->motif));
      }
      SCANNED_SEQUENCE_T* scanned_seq = NULL;
      if (!options.combine_duplicates || get_pattern_num_scanned_sequences(pattern) <= seq_counter) {
        // Create a scanned_sequence record and save it in the pattern.
        scanned_seq = allocate_scanned_sequence(seq_name, seq_name, pattern);
        set_scanned_sequence_length(scanned_seq, scan_len);
      } else {
        // get existing sequence record
        scanned_seq = get_pattern_scanned_sequences(pattern)[seq_counter];
        set_scanned_sequence_length(scanned_seq, max(scan_len, get_scanned_sequence_length(scanned_seq)));
      }
      
      // check if scanned component of sequence has sufficient length for the motif
      if (scan_len < get_motif_length(combo->motif)) {
        // set score to zero and p-value to 1 if not set yet
        if(!has_scanned_sequence_score(scanned_seq)){
          set_scanned_sequence_score(scanned_seq, 0.0);
        }
        if(options.pvalues && !has_scanned_sequence_pvalue(scanned_seq)){
          set_scanned_sequence_pvalue(scanned_seq, 1.0);
        } 
        add_scanned_sequence_scanned_position(scanned_seq); 
        if (get_scanned_sequence_num_scanned_positions(scanned_seq) > 0L) {
          need_postprocessing = true;
        }
        if (verbosity >= HIGH_VERBOSE) {
          fprintf(stderr, "%s too short for motif %s. Score set to 0.\n",
              seq_name, get_motif_id(combo->motif));
        }
      } else {
        // scan the sequence using average/maximum motif affinity
        ama_sequence_scan(DNA_ALPH, sequence, logcumback, combo->pssm_pair,
            options.scoring, options.pvalues, options.last, scanned_seq,
            &need_postprocessing);
      }
    } // All motifs scanned

    free_seq(sequence);
    if (options.sdbg_order >= 0) myfree(logcumback);

  } // read sequences

  fclose(fasta_file);
  if (verbosity >= HIGH_VERBOSE) fprintf(stderr, "(%d) sequences read in.\n", seq_loading_num);
  if (verbosity >= NORMAL_VERBOSE) fprintf(stderr, "Finished          \n");

        
  // if any sequence identifier was multiple times in the sequence set  then
  // postprocess of the data is required
  if (need_postprocessing || options.normalize_scores) {
    post_process(cisml, motifs, options.normalize_scores);
  }
        
  // output results
  if (options.output_format == DIRECTORY_FORMAT) {
    if (create_output_directory(options.out_dir, options.clobber, verbosity > QUIET_VERBOSE)) {
      // only warn in higher verbose modes
      fprintf(stderr, "failed to create output directory `%s' or already exists\n", options.out_dir);
      exit(1);
    }
    path = make_path_to_file(options.out_dir, text_filename);
    //FIXME check for errors: MEME doesn't either and we at least know we have a good directory
    text_output = fopen(path, "w");
    free(path);
    path = make_path_to_file(options.out_dir, cisml_filename);
    //FIXME check for errors
    cisml_output = fopen(path, "w");
    free(path);
    print_cisml(cisml_output, cisml, true, NULL, false);
    print_score(cisml, text_output);
    fclose(cisml_output);
    fclose(text_output);
  } else if (options.output_format == GFF_FORMAT) {
    print_score(cisml, stdout);
  } else if (options.output_format == CISML_FORMAT) {
    print_cisml(stdout, cisml, true, NULL, false);
  } else {
    die("Output format invalid!\n");
  }

  //
  // Clean up.
  //
  rbtree_destroy(seq_ids);
  arraylst_destroy(motif_and_pssm_destroy, motifs);
  free_cisml(cisml);
  rbtree_destroy(options.selected_motifs);
        
  // measure time
  if (verbosity >= NORMAL_VERBOSE) { // starting time
    c1 = clock();
    fprintf(stderr, "cycles (CPU);            %ld cycles\n", (long) c1);
    fprintf(stderr, "elapsed CPU time:        %f seconds\n", (float) (c1-c0) / CLOCKS_PER_SEC);
  }
  return 0;
}


