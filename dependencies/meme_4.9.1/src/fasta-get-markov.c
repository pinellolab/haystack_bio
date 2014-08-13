#ifndef _LARGEFILE_SOURCE
#define _LARGEFILE_SOURCE
#endif
#ifndef _LARGEFILE64_SOURCE
#define _LARGEFILE64_SOURCE
#endif
#ifndef _FILE_OFFSET_BITS
#define _FILE_OFFSET_BITS 64
#endif

#include <errno.h>
#include <getopt.h>
#include <inttypes.h>
#include <signal.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#define BAD -1
#define SPECIAL -2
#define NEWLINE -3
#define WHITESPACE -4

// GLOBAL CONSTANTS
int ALEN_DNA = 4;
int ALEN_PROT = 20;
char * ALPHA_DNA[] = {"Aa", "Cc", "Gg", "TtUu"};
char * ALPHA_PROT[] = {"Aa", "Cc", "Dd", "Ee", "Ff", "Gg", "Hh",
  "Ii", "Kk", "Ll", "Mm", "Nn", "Pp", "Qq", "Rr", "Ss", "Tt", "Vv", "Ww", "Yy"};
char * SPECIAL_DNA = "RrYyKkMmSsWwBbDdHhVvNnXx-";
// note that O (Pyrrolysine) and U (Selenocysteine) are not supported by
// MEME so I treat them as special characters 
char * SPECIAL_PROT = "OoUuBbZzXx*-";
int8_t ALPHA_RC_DNA[] = {3, 2, 1, 0};

// GLOBAL VARIABLE
volatile bool update_status = false;

// STRUCTURES
typedef struct fmkv_options {
  char* fasta;
  char* bg;
  char* seqc; // sequence count output file
  int order;
  double pseudo;
  bool prot;
  bool norc;
  bool status;
  bool summary;
  int alen; // number of entries in the alphabet
  char **alpha; // array of strings, each string contains all the different symbols for one letter
  char *special; // string containing valid special characters
  int8_t *alpha_rc;
} FMKV_OPTIONS_T;

typedef struct fmkv_count FMKV_COUNT_T;

struct fmkv_count {
  int64_t count;
  FMKV_COUNT_T *letters;
};

typedef struct fmkv_stats {
  off_t fasta_size;
  off_t fasta_proc;
  int64_t total_seqs;
  int64_t total_slen;
  int64_t slen_min;
  int64_t slen_max;
  FMKV_COUNT_T *letters;
  int64_t *totals; // totals for order groupings
  bool bad; // bad characters detected
  off_t bad_pos;
} FMKV_STATS_T;

typedef struct fmkv_state FMKV_STATE_T;

struct fmkv_state {
  bool nl;
  int64_t slen;
  size_t (*routine)(FMKV_OPTIONS_T*, FMKV_STATS_T*, FMKV_STATE_T*, char*, size_t);
  int8_t hasher[128];
  FMKV_COUNT_T **history; // pointers into the hierarchical letter counts, this is a circular buffer
  int oldest; // history entry to overwrite next
};

static void cleanup_options(FMKV_OPTIONS_T *options) {
}

static void cleanup_counts(int alen, FMKV_COUNT_T *counts) {
  FMKV_COUNT_T *count;
  int i;
  for (i = 0; i < alen; i++) {
    count = counts+i;
    if (count->letters != NULL) {
      cleanup_counts(alen, count->letters);
    }
  }
  free(counts);
}

static void cleanup_stats(FMKV_OPTIONS_T *options, FMKV_STATS_T *stats) {
  free(stats->totals);
  cleanup_counts(options->alen, stats->letters);
}

static void usage(FMKV_OPTIONS_T *options, char *format, ...) {
  va_list argp;

  char *usage = 
    "Usage:\n"
    "    fasta-get-markov [options] [sequence file] [background file]\n\n"
    "Options:\n"
    "    [-m <order>]        order of Markov model to use; default 0\n\n"
    "    [-p]                use protein alphabet; default: use DNA alphabet\n\n"
    "    [-norc]             do not combine forward and reverse complement freqs;\n"
    "                        this is highly recommended for RNA sequences.\n\n"
    "    [-pseudo <count>]   pseudocount added to avoid probabilities of zero;\n"
    "                        default: use a pseudocount of 0.1 .\n\n"
    "    [-nostatus]         do not print constant status messages.\n\n"
    "    [-help]             display this help message.\n\n"
    "    Estimate a Markov model from a FASTA file of sequences.\n\n"
    "    Converts U to T for DNA alphabet for use with RNA sequences.\n"
    "    Skips tuples containing ambiguous characters.\n"
    "    Combines both strands of DNA unless -norc is set.\n\n"
    "    Reads standard input if a sequence file is not specified.\n"
    "    Writes standard output if a background file is not specified.\n\n"
    "    When the background file is specified the following report is made:\n"
    "    <sequence count> <min length> <max length> <average length> <summed length>\n\n";

  if (format) {
    va_start(argp, format);
    vfprintf(stderr, format, argp);
    va_end(argp);
    fprintf(stderr, "\n");
    fputs(usage, stderr);
    fflush(stderr);
  } else {
    puts(usage);
  }
  cleanup_options(options);
  if (format) exit(EXIT_FAILURE);
  exit(EXIT_SUCCESS);
}

#define MRKV 0
#define PROT 1
#define NORC 2
#define PSDO 3
#define NSTA 4
#define NSUM 5
#define HELP 6
#define SEQC 7
static void process_arguments(FMKV_OPTIONS_T *options, int argc, char **argv) {
  struct option fmkv_options[] = {
    {"m", required_argument, NULL, MRKV},
    {"seqc", required_argument, NULL, SEQC}, // sequence count, undocumented
    {"p", no_argument, NULL, PROT},
    {"norc", no_argument, NULL, NORC},
    {"pseudo", required_argument, NULL, PSDO},
    {"nostatus", no_argument, NULL, NSTA},
    {"nosummary", no_argument, NULL, NSUM},
    {"help", no_argument, NULL, HELP},
    {NULL, 0, NULL, 0} //boundary indicator
  };
  bool bad_argument = false;

  options->bg = "-";
  options->fasta = "-";
  options->seqc = NULL;
  options->order = 0;
  options->pseudo = 0.1;
  options->prot = false;
  options->norc = false;
  options->status = true;
  options->summary = true;

  while (1) {
    int opt = getopt_long_only(argc, argv, "", fmkv_options, NULL);
    if (opt == -1) break;
    switch (opt) {
      case MRKV: //-m <order>
        options->order = atoi(optarg);
        break;
      case SEQC: //-seqc <nseqs file>
        options->seqc = optarg;
        break;
      case PROT: //-p
        options->prot = true;
        break;
      case NORC: //-norc
        options->norc = true;
        break;
      case PSDO: //-pseudo <count>
        options->pseudo = strtod(optarg, NULL);
        break;
      case NSTA: //-nostatus
        options->status = false;
        break;
      case NSUM: //-nosummary
        options->summary = false;
        break;
      case HELP: //-help
        usage(options, NULL); // does not return
      case '?':           //unrecognised or ambiguous argument
        bad_argument = true;
    }
  }
  if (bad_argument) usage(options, "One or more unknown or ambiguous options were supplied.");

  if (options->order < 0) usage(options, "Model order can not be less than zero.");
  if (options->pseudo < 0) usage(options, "Pseudo-count must be positive.");

  if (optind < argc) options->fasta = argv[optind++];
  if (optind < argc) options->bg = argv[optind++];

  if (options->prot) {
    options->alen = ALEN_PROT;
    options->alpha = ALPHA_PROT;
    options->special = SPECIAL_PROT;
    options->alpha_rc = NULL;
    options->norc = true;
  } else {
    options->alen = ALEN_DNA;
    options->alpha = ALPHA_DNA;
    options->special = SPECIAL_DNA;
    options->alpha_rc = ALPHA_RC_DNA;
  }
}

static void display_status_update(FMKV_OPTIONS_T *opts, FMKV_STATS_T *stats) {
  long double total, processed, fraction;
  if (stats->fasta_size) {
    total = stats->fasta_size;
    processed = stats->fasta_proc;
    fraction = (processed / total) * 100;
    fprintf(stderr, "\rprocessed: %.1Lf%%", fraction);
  } else {
    fprintf(stderr, "\rsequences: %" PRId64, stats->total_seqs);
  }
  update_status = false;
}

// predeclare routines so they can reference each-other.
static size_t routine_seq_data(FMKV_OPTIONS_T*, FMKV_STATS_T*, FMKV_STATE_T*, char*, size_t);
static size_t routine_seq_name(FMKV_OPTIONS_T*, FMKV_STATS_T*, FMKV_STATE_T*, char*, size_t);
static size_t routine_find_start(FMKV_OPTIONS_T*, FMKV_STATS_T*, FMKV_STATE_T*, char*, size_t);

// routine to read sequence data
static size_t routine_seq_data(FMKV_OPTIONS_T *opts, FMKV_STATS_T *stats,
    FMKV_STATE_T *state, char *buffer, size_t len) {
  size_t i, j, k, chain;
  int pos, order_plus;
  order_plus = opts->order + 1;
  if (state->nl) {
    state->nl = false;
    if (buffer[0] == '>') {
      state->routine = routine_seq_name;
      if (state->slen < stats->slen_min) stats->slen_min = state->slen;
      if (state->slen > stats->slen_max) stats->slen_max = state->slen;
      stats->total_slen += state->slen;
      return 1;
    }
  }
  for (i = 0; i < len; i++) {
    // check that byte is ASCII before using the hasher
    switch ((pos = (buffer[i] & 0x80 ? -1 : state->hasher[buffer[i] - '\0']))) {
      case BAD: // illegal but just set flag and treat as a specal character
        if (!stats->bad) {
          // record the position of the first bad character sighted
          stats->bad_pos = stats->fasta_proc + i;
        }
        stats->bad = true;
        // fall through
      case SPECIAL: // special character
        // chain interrupted -> erase history
        // oldest + 1 == newest, if newest is unset then no history to erase
        if (state->history[(state->oldest + 1) % order_plus]) {
          for (j = 0; j < order_plus; j++) {
            state->history[j] = NULL;
          }
          state->oldest = opts->order;
        }
        state->slen++;
        break;
      case NEWLINE: // newline character
        if ((i + 1) < len) {
          // don't bother using the flag as we can look ahead
          if (buffer[i+1] == '>') {
            state->nl = false;
            state->routine = routine_seq_name;
            if (state->slen < stats->slen_min) stats->slen_min = state->slen;
            if (state->slen > stats->slen_max) stats->slen_max = state->slen;
            stats->total_slen += state->slen;
            return i + 2;
          }
        } else { // last character in buffer so we need to use the flag
          state->nl = true;
        }
        // fall through
      case WHITESPACE: // whitespace charater
        break;
      default:
        // process the alphabet character
        ( state->history[state->oldest] = stats->letters+pos )->count++;
        stats->totals[0]++;
        chain = 1;
        for (j = state->oldest + 1; j < order_plus; j++, chain++) {
          if (state->history[j] == NULL) goto done_extend_chain;
          if (state->history[j]->letters == NULL) {
            if ((state->history[j]->letters = malloc(sizeof(FMKV_COUNT_T) * opts->alen)) == NULL) {
              fprintf(stderr, "Failed to allocate memory for letter counts"
                  " at chain depth %d: ", chain);
              perror(NULL);
              exit(EXIT_FAILURE);
            }
            for (k = 0; k < opts->alen; k++) {
              state->history[j]->letters[k].count = 0;
              state->history[j]->letters[k].letters = NULL;
            }
          }
          ( state->history[j] = state->history[j]->letters+pos )->count++;
          stats->totals[chain]++;
        }
        for (j = 0; j < state->oldest; j++, chain++) {
          if (state->history[j] == NULL) goto done_extend_chain;
          if (state->history[j]->letters == NULL) {
            if ((state->history[j]->letters = malloc(sizeof(FMKV_COUNT_T) * opts->alen)) == NULL) {
              fprintf(stderr, "Failed to allocate memory for letter counts"
                  " at chain depth %d: ", chain);
              perror(NULL);
              exit(EXIT_FAILURE);
            }
            for (k = 0; k < opts->alen; k++) {
              state->history[j]->letters[k].count = 0;
              state->history[j]->letters[k].letters = NULL;
            }
          }
          ( state->history[j] = state->history[j]->letters+pos )->count++;
          stats->totals[chain]++;
        }
done_extend_chain:
        state->oldest = state->oldest - 1;
        if (state->oldest < 0) state->oldest = opts->order;
        state->slen++;
        break;
    }
  }
  return len;
}

// routine to skip over the name/description line of the sequence
static size_t routine_seq_name(FMKV_OPTIONS_T *opts, FMKV_STATS_T *stats,
    FMKV_STATE_T *state, char *buffer, size_t len) {
  size_t i, j;
  for (i = 0; i < len; i++) {
    if (buffer[i] == '\n' || buffer[i] == '\r') {
      state->nl = true;
      state->routine = routine_seq_data;
      state->slen = 0;
      // reset history
      for (j = 0; j <= opts->order; j++) state->history[j] = NULL;
      state->oldest = opts->order;
      // increment sequence count
      stats->total_seqs++;
      return i + 1;
    }
  }
  return len;
}

// routine to find the first sequence
static size_t routine_find_start(FMKV_OPTIONS_T *opts, FMKV_STATS_T *stats,
    FMKV_STATE_T *state, char *buffer, size_t len) {
  size_t i;
  for (i = 0; i < len;) {
    if (state->nl && buffer[i] == '>') {
      state->routine = routine_seq_name;
      state->nl = false;
      return i + 1;
    } else {
      state->nl = false;
      for (; i < len; i++) {
        if (buffer[i] == '\n' || buffer[i] == '\r') {
          state->nl = true;
          i++;
          break;
        }
      }
    }
  }
  return len;
}

// assume inputs are ASCII 7-bit in 8 bit bytes.
static void setup_hasher(int8_t *hasher, int alen, char **alpha, char *special) {
  int i;
  char *c;
  // set everything to illegal by default
  for (i = 0; i < 128; i++) {
    hasher[i] = BAD;
  }
  // add special
  for (c = special; *c != '\0'; c++) {
    hasher[*c -'\0'] = SPECIAL;
  }
  // add newlines
  hasher['\r'] = NEWLINE;
  hasher['\n'] = NEWLINE;
  // add whitespace
  hasher[' '] = WHITESPACE;
  hasher['\t'] = WHITESPACE;
  hasher['\v'] = WHITESPACE; //vertical tab, rather unlikely

  // add alphabet
  for (i = 0; i < alen; i++) {
    for (c = alpha[i]; *c != '\0'; c++) {
      hasher[*c - '\0'] = i;
    }
  }
}

static void process_fasta2(FMKV_OPTIONS_T *opts, FMKV_STATS_T *stats, FILE *fasta_fh, char *buffer, size_t buffer_len) {
  size_t bytes_read, offset, consumed, i;
  FMKV_STATE_T state;
  size_t (*prev_routine)(FMKV_OPTIONS_T*, FMKV_STATS_T*, FMKV_STATE_T*, char*, size_t);
  // init state
  state.nl = true;
  state.routine = routine_find_start;
  setup_hasher(state.hasher, opts->alen, opts->alpha, opts->special);
  state.history = malloc(sizeof(FMKV_COUNT_T*) * (opts->order + 1));
  for (i = 0; i <= opts->order; i++) state.history[i] = NULL;
  state.oldest = opts->order;
  //load data into buffer
  while ((bytes_read = fread(buffer, sizeof(char), buffer_len, fasta_fh)) > 0) {
    offset = 0;
    while (offset < bytes_read) {
      prev_routine = state.routine;
      consumed = state.routine(opts, stats, &state, buffer+offset, bytes_read - offset);
      if (consumed == 0 && state.routine == prev_routine) {
        fprintf(stderr, "Infinite loop detected!\n");
        abort(); // we'd want to debug this so an abort makes more sense than exit.
      }
      offset += consumed;
      stats->fasta_proc += consumed;
      if (update_status) display_status_update(opts, stats);
    }
  }
  if (state.routine == routine_seq_name) {
    stats->total_seqs++;
    stats->slen_min = 0;
  } else if (state.routine == routine_seq_data) {
    if (state.slen < stats->slen_min) stats->slen_min = state.slen;
    if (state.slen > stats->slen_max) stats->slen_max = state.slen;
    stats->total_slen += state.slen;
  } else if (state.routine == routine_find_start) {
    stats->slen_min = 0;
  }
  free(state.history);
}

static void process_fasta(FMKV_OPTIONS_T *opts, FMKV_STATS_T *stats) {
  FILE *fasta_fh;
  size_t buffer_len, i;
  char *buffer;
  struct stat fasta_stat;
  off_t fasta_size;
  // get file handle
  if (strcmp(opts->fasta, "-") == 0) {
    // read from stdin
    fasta_fh = stdin;
    fasta_size = 0;
  } else {
    // open file
    fasta_fh = fopen(opts->fasta, "r");
    // check it worked
    if (fasta_fh == NULL) {
      // report error and exit
      fprintf(stderr, "Failed to open fasta file \"%s\" for reading: ", opts->fasta);
      perror(NULL);
      exit(EXIT_FAILURE);
    }
    // get the file size
    if (fstat(fileno(fasta_fh), &fasta_stat) == -1) {
      perror("Failed to get size of the fasta file");
      exit(EXIT_FAILURE);
    }
    fasta_size = fasta_stat.st_size;
  }
  // allocate 1MB buffer
  buffer_len =  (1 << 20);
  if ((buffer = malloc(sizeof(char) * buffer_len)) == NULL) {
    perror("Memory allocation for 1MB buffer failed");
    exit(EXIT_FAILURE);
  }
  // init stats
  stats->fasta_size = fasta_size;
  stats->fasta_proc = 0;
  stats->bad = false;
  stats->total_seqs = 0;
  stats->total_slen = 0;
  stats->slen_min = 0x7FFFFFFFFFFFFFFF;
  stats->slen_max = 0;
  if ((stats->letters = malloc(sizeof(FMKV_COUNT_T) * opts->alen)) == NULL) {
    perror("Memory allocation for top-level letter counts failed");
    exit(EXIT_FAILURE);
  }
  for (i = 0; i < opts->alen; i++) {
    stats->letters[i].count = 0;
    stats->letters[i].letters = NULL;
  }
  stats->totals = malloc(sizeof(int64_t) * (opts->order + 1));
  for (i = 0; i < (opts->order + 1); i++) stats->totals[i] = 0;
  // read and process sequences
  process_fasta2(opts, stats, fasta_fh, buffer, buffer_len);
  if (opts->status) {
    display_status_update(opts, stats);
    fputs("\r\033[K", stderr);
  }
  // clean up
  free(buffer);
  if (strcmp(opts->fasta, "-") != 0) {
    fclose(fasta_fh);
  }
}

static int64_t get_chain_count(FMKV_COUNT_T *counts, int order, int8_t *chain) {
  FMKV_COUNT_T *curr;
  int j;
  curr = counts+(chain[0]);
  for (j = 1; j <= order; j++) {
    if (curr->letters == NULL) return 0;
    curr = curr->letters+(chain[j]);
  }
  return curr->count;
}

static void rc_chain(int8_t *alpha_rc, int8_t *chain, int8_t *chain_rc, int order) {
  int i, j;
  for (i = 0, j = order; i < j; i++, j--) {
    chain_rc[i] = alpha_rc[chain[j]];
    chain_rc[j] = alpha_rc[chain[i]];
  }
  if (i == j) {
    chain_rc[i] = alpha_rc[chain[i]];
  }
}

static void output_chains(FILE *out_fh, FMKV_COUNT_T *counts, int64_t total_counts, 
    long double pseudo, long double pseudo_frac,
    int alen, char **alpha, int8_t *alpha_rc, bool do_rc,
    int order, int8_t *chain, int8_t *chain_rc_buf, char *tuple, int len) {
  int i;
  int64_t count, count_rc;
  long double prob;
  for (i = 0; i < alen; i++) {
    tuple[len] = alpha[i][0];
    chain[len] = i;
    if (len == order) {
      tuple[len+1] = '\0';
      // calculate probability
      count = get_chain_count(counts, order, chain);
      if (do_rc) {
        rc_chain(alpha_rc, chain, chain_rc_buf, order);
        count_rc = get_chain_count(counts, order, chain_rc_buf);
        prob = (pseudo_frac + count + count_rc) / (pseudo + (2 * total_counts));
      } else {
        count_rc = 0;
        prob = (pseudo_frac + count) / (pseudo + total_counts);
      }
#ifdef DEBUG
      fprintf(out_fh, "# %15" PRId64 " %15" PRId64 "\n", count, count_rc);
#endif
      fprintf(out_fh, "%s %9.3Le\n", tuple, prob);
    } else {
      output_chains(out_fh, counts, total_counts, pseudo, pseudo_frac,
          alen, alpha, alpha_rc, do_rc,
          order, chain, chain_rc_buf, tuple, len + 1);
    }
  }
}

static void output_stats(FMKV_OPTIONS_T *opts, FMKV_STATS_T *stats) {
  int level;
  int64_t total_chains;
  int8_t *chain, *chain_rc_buf;
  long double pseudo, pseudo_frac, avg;
  char *buffer;
  FILE *bg_fh, *seqc_fh;
  
  if (strcmp(opts->bg, "-") == 0) {
    // write to stdout
    bg_fh = stdout;
  } else {
    // open file
    bg_fh = fopen(opts->bg, "w");
    // check it worked
    if (bg_fh == NULL) {
      // report error and exit
      fprintf(stderr, "Failed to open background file \"%s\" for writing: ", opts->bg);
      perror(NULL);
      exit(EXIT_FAILURE);
    }
  }
  if (strcmp(opts->fasta, "-") != 0) {
    fprintf(bg_fh, "# %d-order Markov frequencies from file %s\n", opts->order, opts->fasta);
  } else {
    fprintf(bg_fh, "# %d-order Markov frequencies\n", opts->order);
  }
  if (stats->bad) {
    fprintf(bg_fh, "# Warning: unexpected characters were detected, "
        "first instance at byte offset %jd\n", (intmax_t)stats->bad_pos);
  }
  if (stats->total_seqs > 0) {
    avg = (long double)stats->total_slen / (long double)stats->total_seqs;
  } else {
    avg = 0;
  }
  fprintf(bg_fh, "# seqs: %" PRId64 "    min: %" PRId64 "    max: %"
      PRId64 "    avg: %.1Lf    sum: %" PRId64"\n", stats->total_seqs,
      stats->slen_min, stats->slen_max, avg, stats->total_slen);

  if ((chain = malloc(sizeof(int8_t) * (opts->order + 1))) == NULL) {
    perror("Memory allocation for chain failed");
    exit(EXIT_FAILURE);
  }
  if ((chain_rc_buf = malloc(sizeof(int8_t) * (opts->order + 1))) == NULL) {
    perror("Memory allocation for chain rc buffer failed");
    exit(EXIT_FAILURE);
  }

  if ((buffer = malloc(sizeof(char) * (opts->order + 2))) == NULL) {
    perror("Memory allocation for buffer failed");
    exit(EXIT_FAILURE);
  }

  total_chains = opts->alen;
  for (level = 0; level <= opts->order; level++) {
    fprintf(bg_fh, "# order %d\n", level);
    // when there are no entries for this level, force a pseudocount to avoid divide by zero
    pseudo = (stats->totals[level] == 0 ?  1.0 : opts->pseudo);
    pseudo_frac = pseudo / total_chains;
#ifdef DEBUG
    fprintf(bg_fh, "# %" PRId64 " %Le\n", stats->totals[level], pseudo_frac);
#endif
    output_chains(bg_fh, stats->letters, stats->totals[level],
        pseudo, pseudo_frac,
        opts->alen, opts->alpha, opts->alpha_rc, !opts->norc, level,
        chain, chain_rc_buf, buffer, 0);
    total_chains *= opts->alen;
  }

  free(buffer);
  free(chain_rc_buf);
  free(chain);

  if (opts->seqc != NULL) {
    // open file
    seqc_fh = fopen(opts->seqc, "w");
    // check it worked
    if (seqc_fh == NULL) {
      // report error and exit
      fprintf(stderr, "Failed to open sequence count file \"%s\" for writing: ", opts->seqc);
      perror(NULL);
      exit(EXIT_FAILURE);
    }
    // write out the sequence count
    fprintf(seqc_fh, "%" PRId64 "\n", stats->total_seqs);
    // close the file
    fclose(seqc_fh);
  }

  if (strcmp(opts->bg, "-") != 0 && opts->summary) {
    fclose(bg_fh);
    // when we don't have to print the background to stdout
    // print a report like getsize except without the alphabet
    printf("%" PRId64 " %" PRId64 " %" PRId64 " %.1Lf %" PRId64 "\n", 
      stats->total_seqs, stats->slen_min, stats->slen_max, avg,
      stats->total_slen);
  }
}

void status_update_alarm(int signum) {
  update_status = true;
}

static void start_status_update_alarm() {
  struct sigaction alarm_action;
  struct itimerval msg;

  alarm_action.sa_handler = status_update_alarm;
  sigemptyset(&alarm_action.sa_mask);
  alarm_action.sa_flags = SA_RESTART;

  sigaction(SIGALRM, &alarm_action, NULL);

  msg.it_interval.tv_usec = 500000; // half a second
  msg.it_interval.tv_sec = 0;
  msg.it_value.tv_usec = 500000; // half a second
  msg.it_value.tv_sec = 0;

  setitimer(ITIMER_REAL, &msg, NULL);

  update_status = true;
}

static void stop_status_update_alarm() {
  struct itimerval msg;
  msg.it_interval.tv_usec = 0;
  msg.it_interval.tv_sec = 0;
  msg.it_value.tv_usec = 0;
  msg.it_value.tv_sec = 0;
  setitimer(ITIMER_REAL, &msg, NULL);
}

int main(int argc, char **argv) {
  FMKV_OPTIONS_T opts;
  FMKV_STATS_T stats;

  process_arguments(&opts, argc, argv);
  if (opts.status) start_status_update_alarm();
  process_fasta(&opts, &stats);
  if (opts.status) stop_status_update_alarm();
  output_stats(&opts, &stats);
  
  cleanup_stats(&opts, &stats);
  cleanup_options(&opts);
  return EXIT_SUCCESS;
}
