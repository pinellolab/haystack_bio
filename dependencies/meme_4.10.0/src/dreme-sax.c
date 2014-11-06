
#include <errno.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>

#include "dreme-sax.h"
#include "linked-list.h"
#include "sax-parser-utils.h"
#include "utils.h"

/*****************************************************************************
 * Parser States
 ****************************************************************************/
enum parser_state {
  PS_ERROR,
  PS_START,
  PS_IN_DREME,
  PS_IN_MODEL,
  PS_IN_MOTIFS,
  PS_IN_RUN_TIME,
  PS_IN_COMMAND_LINE,
  PS_IN_POSITIVES,
  PS_IN_NEGATIVES,
  PS_IN_BACKGROUND,
  PS_IN_STOP,
  PS_IN_NGEN,
  PS_IN_ADD_PV_THRESH,
  PS_IN_SEED,
  PS_IN_HOST,
  PS_IN_WHEN,
  PS_IN_DESCRIPTION,
  PS_IN_MOTIF,
  PS_IN_POS,
  PS_IN_MATCH,
  PS_END
};
typedef enum parser_state PS_EN;

/*****************************************************************************
 * Parser State Names (for error messages)
 ****************************************************************************/
static char const * const state_names[] = {
  "ERROR",
  "START",
  "IN_DREME",
  "IN_MODEL",
  "IN_MOTIFS",
  "IN_RUN_TIME",
  "IN_COMMAND_LINE",
  "IN_POSITIVES",
  "IN_NEGATIVES",
  "IN_BACKGROUND",
  "IN_STOP",
  "IN_NGEN",
  "IN_ADD_PV_THRESH",
  "IN_SEED",
  "IN_HOST",
  "IN_WHEN",
  "IN_DESCRIPTION",
  "IN_MOTIF",
  "IN_POS",
  "IN_MATCH",
  "END"
};

/*****************************************************************************
 * Expected State Repeats
 ****************************************************************************/
enum expected_state {
  ES_ZERO_OR_ONE,
  ES_ONCE,
  ES_ONE_OR_MORE,
  ES_ANY
};
typedef enum expected_state ES_EN;

/*****************************************************************************
 * Used to keep track of what elements are expected and enforces limits on the
 * number of times a valid element can be repeated. Also avoids the 
 * possibility that elements could be left out.
 ****************************************************************************/
typedef struct es ES_T;
struct es {
  PS_EN state;    // the element that is expected
  ES_EN expected; // the number of times the element is expected (see ES codes)
  int found;    // the number of times the element has been found already
};

/*****************************************************************************
 * Stores the parser state.
 ****************************************************************************/
typedef struct ps PS_T;
struct ps {
  int state;                // the state code (see PS codes)
  int udepth;               // the depth of unknown tags (disabled)
  CHARBUF_T characters;     // a buffer to store characters between elements
  LINKLST_T *expected_stack;// a queue of expected states
  DREME_IO_XML_CALLBACKS_T *callbacks; // callbacks to the user
  DREME_ALPH_EN alphabet;   // alphabet of the motifs
  char *motif_id;           // motif id
  int motif_len;            // motif length
  int last_pos;             // last seen motif pos
  void *user_data;          // the user data
};



/*****************************************************************************
 * Report an error to the user.
 ****************************************************************************/
static void error(PS_T *ps, char *fmt, ...) {
  va_list  argp;
  if (ps->callbacks->error) {
    va_start(argp, fmt);
    ps->callbacks->error(ps->user_data, fmt, argp);
    va_end(argp);
  }
  ps->state = PS_ERROR;
}

/*****************************************************************************
 * Report an attribute parse error to the user.
 ****************************************************************************/
void dreme_attr_parse_error(void *state, int errcode, char *tag, char *attr, char *value) {
  PS_T *ps = (PS_T*)state;
  if (errcode == PARSE_ATTR_DUPLICATE) {
    error(ps, "Duplicate attribute %s::%s.\n", tag, attr);
  } else if (errcode == PARSE_ATTR_BAD_VALUE) {
    error(ps, "Bad value \"%s\" for attribute %s::%s.\n", value, tag, attr);
  } else if (errcode == PARSE_ATTR_MISSING) {
    error(ps, "Missing required attribute %s::%s.\n", tag, attr);
  }
}

/*****************************************************************************
 * Push an expected state on the stack
 ****************************************************************************/
void dreme_push_es(PS_T *ps, PS_EN expected_state, ES_EN expected_occurances) {
  ES_T *es;
  if (expected_state < PS_ERROR ||  expected_state > PS_END) {
    die("Bad state code!\n");
  }
  es = mm_malloc(sizeof(ES_T));
  es->state = expected_state;
  es->expected = expected_occurances;
  es->found = 0;
  linklst_push(es, ps->expected_stack);
}

/*****************************************************************************
 * At the start of a new element check that all previous expected elements
 * have been found and that the current element hasn't been repeated too 
 * many times.
 ****************************************************************************/
int dreme_update_es(PS_T *ps, PS_EN next_state) {
  ES_T *es, old_es;
  if (next_state < PS_ERROR ||  next_state > PS_END) {
    die("Bad state code!\n");
  }
  while ((es = (ES_T*)linklst_pop(ps->expected_stack)) != NULL && es->state != next_state) {
    old_es = *es;
    free(es);
    es = NULL;
    switch (old_es.expected) {
      case ES_ONCE:
      case ES_ONE_OR_MORE:
        if (old_es.found < 1){
          error(ps, "Expected state %s not found!\n", state_names[old_es.state]);
          return FALSE;
        }
      default:
        break;
    }
  }
  if (es != NULL) {
    es->found += 1;
    linklst_push(es, ps->expected_stack);
    switch (es->expected) {
      case ES_ONCE:
      case ES_ZERO_OR_ONE:
        if (es->found > 1) {
          error(ps, "Expected state %s only once!\n", state_names[es->state]);
          return FALSE;
        }
      default:
        break;
    }
  } else {
    error(ps, "The state %s was not expected!\n", state_names[next_state]);
    return FALSE;
  }
  return TRUE;
}


/*****************************************************************************
 * dreme
 * contains: model, motifs, run_time
 *
 *  release         the release date.
 *  version         the program version.
 ****************************************************************************/
static void start_ele_dreme(PS_T *ps, const xmlChar **attrs) {
  struct prog_version ver;
  char *release;

  char* names[2] = {"release", "version"};
  int (*parsers[2])(char*, void*) = {ld_str, ld_version};
  void *data[2] = {&release, &ver};
  BOOLEAN_T required[2] = {TRUE, TRUE};
  BOOLEAN_T done[2];

  parse_attributes(dreme_attr_parse_error, ps, "dreme", attrs, 2, names, parsers, data, required, done);
  
  if (ps->callbacks->start_dreme && ps->state != PS_ERROR) {
    ps->callbacks->start_dreme(ps->user_data, ver.major, ver.minor, ver.patch, release);
  }
  dreme_push_es(ps, PS_IN_RUN_TIME, ES_ONCE);
  dreme_push_es(ps, PS_IN_MOTIFS, ES_ONCE);
  dreme_push_es(ps, PS_IN_MODEL, ES_ONCE);
}

/*****************************************************************************
 * /dreme
 * contains: model, motifs, run_time
 ****************************************************************************/
static void end_ele_dreme(PS_T *ps) {
  if (ps->callbacks->end_dreme) {
    ps->callbacks->end_dreme(ps->user_data);
  }
}

/*****************************************************************************
 * dreme > model
 * contains: command_line, positives, negatives, background, stop, ngen, 
 *          add_pv_thresh, seed, host, when, description
 ****************************************************************************/
static void start_ele_model(PS_T *ps, const xmlChar **attrs) {
  if (ps->callbacks->start_model && ps->state != PS_ERROR) {
    ps->callbacks->start_model(ps->user_data);
  }
  dreme_push_es(ps, PS_IN_DESCRIPTION, ES_ZERO_OR_ONE);
  dreme_push_es(ps, PS_IN_WHEN, ES_ONCE);
  dreme_push_es(ps, PS_IN_HOST, ES_ONCE);
  dreme_push_es(ps, PS_IN_SEED, ES_ONCE);
  dreme_push_es(ps, PS_IN_ADD_PV_THRESH, ES_ONCE);
  dreme_push_es(ps, PS_IN_NGEN, ES_ONCE);
  dreme_push_es(ps, PS_IN_STOP, ES_ONCE);
  dreme_push_es(ps, PS_IN_BACKGROUND, ES_ONCE);
  dreme_push_es(ps, PS_IN_NEGATIVES, ES_ONCE);
  dreme_push_es(ps, PS_IN_POSITIVES, ES_ONCE);
  dreme_push_es(ps, PS_IN_COMMAND_LINE, ES_ONCE);
}

/*****************************************************************************
 * dreme > /model
 * contains: command_line, positives, negatives, background, stop, ngen, 
 *          add_pv_thresh, seed, host, when, description?
 ****************************************************************************/
static void end_ele_model(PS_T *ps) {
  if (ps->callbacks->end_model && ps->state != PS_ERROR) {
    ps->callbacks->end_model(ps->user_data);
  }
}

/*****************************************************************************
 * dreme > motifs
 * contains: motif*
 ****************************************************************************/
static void start_ele_motifs(PS_T *ps, const xmlChar **attrs) {
  if (ps->callbacks->start_motifs && ps->state != PS_ERROR) {
    ps->callbacks->start_motifs(ps->user_data);
  }
  dreme_push_es(ps, PS_IN_MOTIF, ES_ANY);
}

/*****************************************************************************
 * dreme > /motifs
 * contains: motif*
 ****************************************************************************/
static void end_ele_motifs(PS_T *ps) {
  if (ps->callbacks->end_motifs && ps->state != PS_ERROR) {
    ps->callbacks->end_motifs(ps->user_data);
  }
}

/*****************************************************************************
 * dreme > run_time
 *
 *  cpu             the time the program was running on a cpu.
 *  real            the real world time the program was running.
 *  stop            the reason dreme stopped.
 ****************************************************************************/
static void start_ele_run_time(PS_T *ps, const xmlChar **attrs) {
  double cpu_time, real_time;
  int stop;

  char* stop_options[3] = {"count", "evalue", "time"};
  int stop_values[3] = {DREME_STOP_COUNT, DREME_STOP_EVALUE, DREME_STOP_TIME};
  MULTI_T stop_multi = {.count = 3, .options = stop_options, .outputs = stop_values, .target = &(stop)};

  char* names[3] = {"cpu", "real", "stop"};
  int (*parsers[3])(char*, void*) = {ld_double, ld_double, ld_multi};
  void *data[3] = {&cpu_time, &real_time, &stop_multi};
  BOOLEAN_T required[3] = {TRUE, TRUE, TRUE};
  BOOLEAN_T done[3];

  parse_attributes(dreme_attr_parse_error, ps, "run_time", attrs, 3, names, parsers, data, required, done);

  if (ps->callbacks->handle_run_time && ps->state != PS_ERROR) {
    ps->callbacks->handle_run_time(ps->user_data, cpu_time, real_time, (DREME_STOP_EN)stop);
  }
}

/*****************************************************************************
 * dreme > model > /command_line
 * the command-line used to run the program.
 ****************************************************************************/
static void end_ele_command_line(PS_T *ps) {
  if (ps->callbacks->handle_command_line && ps->state != PS_ERROR) {
    ps->callbacks->handle_command_line(ps->user_data, ps->characters.buffer);
  }
}

/*****************************************************************************
 * dreme > model > positives
 *
 *  name            the name of the positive input set
 *  count           the number of sequences in the positive input set
 *  file            the file containing the positive input set
 *  last_mod_date   the time the positive input set was last modified
 ****************************************************************************/
static void start_ele_positives(PS_T *ps, const xmlChar **attrs) {
  char *name, *file, *lastmod;
  long count;

  char* names[4] = {"count", "file", "last_mod_date", "name"};
  int (*parsers[4])(char*, void*) = {ld_long, ld_str, ld_str, ld_str};
  void *data[4] = {&count, &file, &lastmod, &name};
  BOOLEAN_T required[4] = {TRUE, TRUE, TRUE, TRUE};
  BOOLEAN_T done[4];

  parse_attributes(dreme_attr_parse_error, ps, "positives", attrs, 4, names, parsers, data, required, done);

  if (ps->callbacks->handle_positives && ps->state != PS_ERROR) {
    ps->callbacks->handle_positives(ps->user_data, name, count, file, lastmod);
  }
}

/*****************************************************************************
 * dreme > model > negatives
 *
 *  name            the name of the negative dataset
 *  count           the number of sequences in the negative dataset
 *  from            the source of the negative dataset (eg shuffled positives)
 *  file            the file containing the negative dataset (optional)
 *  last_mod_date   the last modified date of the file (optional)
 ****************************************************************************/
static void start_ele_negatives(PS_T *ps, const xmlChar **attrs) {
  char *name, *file, *lastmod;
  long count;
  int from;

  file = NULL;
  lastmod = NULL;

  char* from_options[2] = {"file", "shuffled"};
  int from_values[2] = {DREME_NEG_FILE, DREME_NEG_SHUFFLED};
  MULTI_T from_multi = {.count = 2, .options = from_options, .outputs = from_values, .target = &(from)};

  char* names[5] = {"count", "file", "from", "last_mod_date", "name"};
  int (*parsers[5])(char*, void*) = {ld_long, ld_str, ld_multi, ld_str, ld_str};
  void *data[5] = {&count, &file, &from_multi, &lastmod, &name};
  BOOLEAN_T required[5] = {TRUE, FALSE, TRUE, FALSE, TRUE};
  BOOLEAN_T done[5];

  parse_attributes(dreme_attr_parse_error, ps, "negatives", attrs, 5, names, parsers, data, required, done);

  if (ps->state != PS_ERROR && from == DREME_NEG_FILE) {
    if (file == NULL) {
      dreme_attr_parse_error(ps, PARSE_ATTR_MISSING, "negatives", "file", NULL);
    } 
    if (lastmod == NULL) {
      dreme_attr_parse_error(ps, PARSE_ATTR_MISSING, "negatives", "last_mod_date", NULL);
    }
  }

  if (ps->callbacks->handle_negatives && ps->state != PS_ERROR) {
    ps->callbacks->handle_negatives(ps->user_data, name, count, (DREME_NEG_EN)from, file, lastmod);
  }
}

/*****************************************************************************
 * dreme > model > background
 *
 *  type            is the alphabet DNA or RNA
 *  A               frequency of A
 *  C               frequency of C
 *  G               frequency of G
 *  T/U             frequency of T or U depending on type
 *  from            from the negative dataset or a background file
 *  file            the background file (optional)
 *  last_mod_date   the last modified date of the background file (optional)
 ****************************************************************************/
static void start_ele_background(PS_T *ps, const xmlChar **attrs) {
  int type, from;
  double A, C, G, T;
  char *file, *lastmod;

  // set reasonable defaults
  type = DREME_BG_FROM_DATASET;
  from = DREME_ALPH_DNA;
  file = NULL;
  lastmod = NULL;

  char* type_options[2] = {"dna", "rna"};
  int type_values[2] = {DREME_ALPH_DNA, DREME_ALPH_RNA};
  MULTI_T type_multi = {.count = 2, .options = type_options, 
    .outputs = type_values, .target = &(type)};

  char* from_options[2] = {"dataset", "file"};
  int from_values[2] = {DREME_BG_FROM_DATASET, DREME_BG_FROM_FILE};
  MULTI_T from_multi = {.count = 2, .options = from_options, 
    .outputs = from_values, .target = &(from)};

  char* names[9] = {"A", "C", "G", "T", "U", "file", "from", 
    "last_mod_date", "type"};
  int (*parsers[9])(char*, void*) = {ld_pvalue, ld_pvalue, ld_pvalue, 
    ld_pvalue, ld_pvalue, ld_str, ld_multi, ld_str, ld_multi};
  void *data[9] = {&A, &C, &G, &T, &T, &file, &from_multi, &lastmod, &type_multi};
  BOOLEAN_T required[9] = {TRUE, TRUE, TRUE, FALSE, FALSE, FALSE, TRUE, FALSE, TRUE};
  BOOLEAN_T done[9];

  parse_attributes(dreme_attr_parse_error, ps, "background", attrs, 9, names, parsers, data, required, done);

  if (type == DREME_ALPH_DNA) {
    if (!done[3]) dreme_attr_parse_error(ps, PARSE_ATTR_MISSING, "background", "T", NULL);
    if (done[4]) error(ps, "Attribute background::U should not exist because alphabet is DNA.\n");
  } else {
    if (!done[4]) dreme_attr_parse_error(ps, PARSE_ATTR_MISSING, "background", "U", NULL);
    if (done[3]) error(ps, "Attribute background::T should not exist because alphabet is RNA.\n");
  }
  if (from == DREME_BG_FROM_FILE) {
    if (!done[5]) dreme_attr_parse_error(ps, PARSE_ATTR_MISSING, "background", "file", NULL);
    if (!done[7]) dreme_attr_parse_error(ps, PARSE_ATTR_MISSING, "background", "last_mod_date", NULL);
  }

  if (ps->state != PS_ERROR) {
    // check sums to 1
    double delta = A + C + G + T - 1;
    delta = (delta < 0 ? -delta : delta);
    if (delta > 0.004) {
      // dreme writes background probabilities to 3 decimal places so assuming 
      // the error on each is at maximum 0.001 then the total error for the 
      // sum must be less than or equal to 0.004
      error(ps, "Probabilities of background do not sum to 1, got %g .\n",
          A + C + G + T);
    }
  }

  if (ps->state != PS_ERROR) {
    ps->alphabet = (DREME_ALPH_EN)type;
  }

  if (ps->callbacks->handle_background && ps->state != PS_ERROR) {
    ps->callbacks->handle_background(ps->user_data, (DREME_ALPH_EN)type, A, C, G, T, from, file, lastmod);
  }
}

/*****************************************************************************
 * dreme > model > stop
 *
 *  evalue          the stopping evalue (returned as log10).
 *  count           the stopping count.
 *  time            the stopping time.
 ****************************************************************************/
static void start_ele_stop(PS_T *ps, const xmlChar **attrs) {
  int count, time;
  double log10evalue;

  char* names[3] = {"count", "evalue", "time"};
  int (*parsers[3])(char*, void*) = {ld_int, ld_log10_ev, ld_int};
  void *data[3] = {&count, &log10evalue, &time};
  BOOLEAN_T required[3] = {FALSE, FALSE, FALSE};
  BOOLEAN_T done[3];

  parse_attributes(dreme_attr_parse_error, ps, "stop", attrs, 3, names, parsers, data, required, done);

  if (ps->callbacks->handle_stop && ps->state != PS_ERROR) {
    ps->callbacks->handle_stop(ps->user_data, &log10evalue, &count, &time);
  }
}

/*****************************************************************************
 * dreme > model > /ngen
 * the number of generations to check (or something like that).
 ****************************************************************************/
static void end_ele_ngen(PS_T *ps) {
  int ngen;

  if (ld_int(ps->characters.buffer, &ngen)) {
    error(ps, "Bad value \"%s\" for ngen.\n", ps->characters.buffer);
  }

  if (ps->callbacks->handle_ngen && ps->state != PS_ERROR) {
    ps->callbacks->handle_ngen(ps->user_data, ngen);
  }
}

/*****************************************************************************
 * dreme > model > /add_pv_thresh
 * the p-value threshold (returned as log10) for adding a word to the list of 
 * possible motif seed values (this description is probably wrong).
 ****************************************************************************/
static void end_ele_add_pv_thresh(PS_T *ps) {
  double log10_add_pv_thresh;

  if (ld_log10_pv(ps->characters.buffer, &log10_add_pv_thresh)) {
    error(ps, "Bad value \"%s\" for add_pv_thresh.\n", ps->characters.buffer);
  }

  if (ps->callbacks->handle_add_pv_thresh && ps->state != PS_ERROR) {
    ps->callbacks->handle_add_pv_thresh(ps->user_data, log10_add_pv_thresh);
  }
}

/*****************************************************************************
 * dreme > model > /seed
 * the seed value used by the pseudo-random number generator.
 ****************************************************************************/
static void end_ele_seed(PS_T *ps) {
  long seed;

  if (ld_long(ps->characters.buffer, &seed)) {
    error(ps, "Bad value \"%s\" for seed.\n", ps->characters.buffer);
  }

  if (ps->callbacks->handle_seed && ps->state != PS_ERROR) {
    ps->callbacks->handle_seed(ps->user_data, seed);
  }
}

/*****************************************************************************
 * dreme > model > /host
 * the name of the computer which ran DREME.
 ****************************************************************************/
static void end_ele_host(PS_T *ps) {
  if (ps->callbacks->handle_host && ps->state != PS_ERROR) {
    ps->callbacks->handle_host(ps->user_data, ps->characters.buffer);
  }
}

/*****************************************************************************
 * dreme > model > /when
 * the time DREME was run. 
 ****************************************************************************/
static void end_ele_when(PS_T *ps) {
  if (ps->callbacks->handle_when && ps->state != PS_ERROR) {
    ps->callbacks->handle_when(ps->user_data, ps->characters.buffer);
  }
}

/*****************************************************************************
 * dreme > model > /description
 * an optional description of the experiment.
 ****************************************************************************/
static void end_ele_description(PS_T *ps) {
  if (ps->callbacks->handle_description && ps->state != PS_ERROR) {
    ps->callbacks->handle_description(ps->user_data, ps->characters.buffer);
  }
}

/*****************************************************************************
 * dreme > motifs > motif
 * contains: pos+, match*
 *
 *  id                the identifier used by DREME
 *  seq               the DNA iupac sequence representing the motif.
 *  length            the length of the motif
 *  nsites            the number of sites used to create the motif
 *  p                 the number of sequences in the positive set with the motif
 *  n                 the number of sequences in the negative set with the motif
 *  pvalue            the pvalue of the motif after erasing (returned as log10)
 *  evalue            the evalue of the motif after erasing (returned as log10)
 *  unerased_evalue   the evalue of the motif without erasing (returned as log10)
 ****************************************************************************/
static void start_ele_motif(PS_T *ps, const xmlChar **attrs) {
  char *id, *seq;
  int length;
  long nsites, p_hits, n_hits;
  double log10pvalue, log10evalue, log10uevalue;


  char* names[9] = {"evalue", "id", "length", "n", "nsites", "p", "pvalue", 
    "seq", "unerased_evalue"};
  int (*parsers[9])(char*, void*) = {ld_log10_ev, ld_str, ld_int, ld_long, 
    ld_long, ld_long, ld_log10_pv, ld_str, ld_log10_ev};
  void *data[9] = {&log10evalue, &id, &length, &n_hits, &nsites, &p_hits, 
    &log10pvalue, &seq, &log10uevalue};
  BOOLEAN_T required[9] = {TRUE, TRUE, TRUE, TRUE, TRUE, TRUE, TRUE, 
    TRUE, TRUE};
  BOOLEAN_T done[9];

  parse_attributes(dreme_attr_parse_error, ps, "motif", attrs, 9, names, 
      parsers, data, required, done);

  // copy the motif id so we can use it in any error messages
  if (ps->state != PS_ERROR) {
    int len = strlen(id);
    ps->motif_id = mm_malloc(sizeof(char) * (len + 1));
    strcpy(ps->motif_id, id);
    ps->last_pos = 0;
    ps->motif_len = length;
  }

  if (ps->callbacks->start_motif && ps->state != PS_ERROR) {
    ps->callbacks->start_motif(ps->user_data, id, seq, length, nsites, p_hits, 
        n_hits, log10pvalue, log10evalue, log10uevalue);
  }
  dreme_push_es(ps, PS_IN_MATCH, ES_ANY);
  dreme_push_es(ps, PS_IN_POS, ES_ONE_OR_MORE);
}

/*****************************************************************************
 * dreme > motifs > /motif
 * contains: pos+, match*
 ****************************************************************************/
static void end_ele_motif(PS_T *ps) {
  if (ps->state != PS_ERROR && ps->last_pos != ps->motif_len) {
    error(ps, "Motif %s has length %d which does not match the "
        "number of pos elements %d.", ps->motif_len, ps->last_pos);
  }

  if (ps->callbacks->end_motif && ps->state != PS_ERROR) {
    ps->callbacks->end_motif(ps->user_data);
  }
  // free stored motif id
  if (ps->motif_id) free(ps->motif_id);
  ps->motif_id = NULL;
}

/*****************************************************************************
 * dreme > motifs > motif > pos
 *
 *  i                 index of the motif position
 *  A                 frequency of A
 *  C                 frequency of C
 *  G                 frequency of G
 *  T/U               frequency of T or U depending on alphabet.
 ****************************************************************************/
static void start_ele_pos(PS_T *ps, const xmlChar **attrs) {
  double A, C, G, T;
  int i;

  char* names[6] = {"A", "C", "G", "T", "U", "i"};
  int (*parsers[6])(char*, void*) = {ld_pvalue, ld_pvalue, ld_pvalue, 
    ld_pvalue, ld_pvalue, ld_int};
  void *data[6] = {&A, &C, &G, &T, &T, &i};
  BOOLEAN_T required[6] = {TRUE, TRUE, TRUE, FALSE, FALSE, TRUE};
  BOOLEAN_T done[6];

  parse_attributes(dreme_attr_parse_error, ps, "pos", attrs, 6, names, 
      parsers, data, required, done);

  // check i makes sense
  if (ps->state != PS_ERROR && i != ps->last_pos + 1) {
    error(ps, "Motif %s did not have pos %d but instead "
        "has pos %d in its place.\n", ps->motif_id, ps->last_pos + 1, i);
  }
  ps->last_pos = i;
  // check alphabet is consistant
  if (ps->state != PS_ERROR) {
    if (ps->alphabet == DREME_ALPH_DNA) {
      if (!done[3]) 
        error(ps, "Motif %s pos %d is missing the attribute T "
            "which is required for a DNA motif.\n", ps->motif_id, i);
      if (done[4])
        error(ps, "Motif %s pos %d has the attribute U which should "
            "only be present on a RNA motif.\n", ps->motif_id, i);
    } else {
      if (!done[4])
        error(ps, "Motif %s pos %d is missing the attribute U "
            "which is required for a RNA motif.\n", ps->motif_id, i);
      if (done[3])
        error(ps, "Motif %s pos %d has the attribute T which should "
            "only be present on a DNA motif.\n", ps->motif_id, i);
    }
  }

  if (ps->state != PS_ERROR) {
    // check sums to 1
    double delta = A + C + G + T - 1;
    delta = (delta < 0 ? -delta : delta);
    if (delta > 0.000004) {
      // dreme writes probabilities to 6 decimal places so assuming the 
      // error on each is at maximum 0.000001 then the total error for the 
      // sum must be less than or equal to 0.000004
      error(ps, "Probabilities of pos %d do not sum to 1, got %g .\n", i, 
          A + C + G + T);
    }
  }

  if (ps->state != PS_ERROR && i > ps->motif_len) {
    error(ps, "Motif %s has a length smaller than the number "
        "of positions.\n", ps->motif_id);
  }

  if (ps->callbacks->handle_pos && ps->state != PS_ERROR) {
    ps->callbacks->handle_pos(ps->user_data, i, A, C, G, T);
  }
}

/*****************************************************************************
 * dreme > motifs > motif > match
 *
 *  seq               word which matches motif
 *  p                 number of positive sequences which contain this word
 *  n                 number of negative sequences which contain this word
 *  pvalue            the pvalue of this word (returned as log10)
 *  evalue            the evalue of this word (returned as log10)
 ****************************************************************************/
static void start_ele_match(PS_T *ps, const xmlChar **attrs) {
  char *seq;
  long p_hits, n_hits;
  double log10pvalue, log10evalue;

  char* names[5] = {"evalue", "n", "p", "pvalue", "seq"};
  int (*parsers[5])(char*, void*) = {ld_log10_ev, ld_long, ld_long, 
    ld_log10_pv, ld_str};
  void *data[5] = {&log10evalue, &n_hits, &p_hits, &log10pvalue, &seq};
  BOOLEAN_T required[5] = {TRUE, TRUE, TRUE, TRUE, TRUE};
  BOOLEAN_T done[5];

  parse_attributes(dreme_attr_parse_error, ps, "match", attrs, 5, names, 
      parsers, data, required, done);

  if (ps->callbacks->handle_match && ps->state != PS_ERROR) {
    ps->callbacks->handle_match(ps->user_data, seq, p_hits, n_hits, log10pvalue,
        log10evalue);
  }
}

/*****************************************************************************
 * Handle the document start
 ****************************************************************************/
void handle_dreme_start_doc(void *ctx) {
  PS_T *ps = (PS_T*)ctx;
  ps->state = PS_START;
  dreme_push_es(ps, PS_END, ES_ONCE);
  dreme_push_es(ps, PS_IN_DREME, ES_ONCE);
}

/*****************************************************************************
 * Handle the document end
 ****************************************************************************/
void handle_dreme_end_doc(void *ctx) {
  PS_T *ps = (PS_T*)ctx;
  ES_T *es;
  if (ps->state != PS_ERROR) dreme_update_es(ps, PS_END);
  while ((es = (ES_T*)linklst_pop(ps->expected_stack)) != NULL) {
    free(es);
  }
}

/*****************************************************************************
 * Handle the characters within elements. Only accepts characters
 * that pass the character filter. If a filter is not set then it doesn't
 * store anything.
 ****************************************************************************/
void handle_dreme_characters(void *ctx, const xmlChar *ch, int len) {
  PS_T *ps = (PS_T*)ctx;
  if (ps->state == PS_ERROR) return;
  if (ps->udepth) {//we don't know where we are!
  } else {
    store_xml_characters(&(ps->characters), (const char*)ch, len);
  }
}

/*****************************************************************************
 * Macros to greatly reduce the boilerplate code needed to keep track of the
 * parser state.
 *
 * DO_START_ELE compares the tag name to a possible option and if it matches
 * then validates the state stack to ensure that it really is a possible
 * option. If everything passes then it changes the state to the specified
 * transition state and calls the callback. It also changes the character
 * accept function.
 * CHECK_START_ELE does the same except it does not have a callback function.
 ****************************************************************************/
#define DO_START_ELE(_expected_,_call_,_transition_,_char_accept_) \
  if (strcasecmp((char*)name, #_expected_ ) == 0) { \
    if (dreme_update_es(ps, _transition_ )) { \
      ps->state = _transition_; \
      _call_ (ps, attrs); \
      ps->characters.accept = _char_accept_; \
    } \
    break; \
  }

#define CHECK_START_ELE(_expected_,_transition_,_char_accept_) \
  if (strcasecmp((char*)name, #_expected_ ) == 0) { \
    if (dreme_update_es(ps, _transition_ )) { \
      ps->state = _transition_; \
      ps->characters.accept = _char_accept_; \
    } \
    break; \
  }

/*****************************************************************************
 * Handle a starting element.
 ****************************************************************************/
void handle_dreme_start_ele(void *ctx, const xmlChar *name, const xmlChar **attrs) {
  PS_T *ps = (PS_T*)ctx;
  int known, allow_unknown;
  if (ps->state == PS_ERROR) return;
  if (ps->udepth) {//we don't know where we are!
    ps->udepth += 1;
  } else {
    //reset the character buffer to the begining
    ps->characters.pos = 0;
    ps->characters.buffer[0] = '\0';
    known = 1; //assume we can find it
    allow_unknown = 1; //are unknowns allowed in this state?
    switch (ps->state) {
      case PS_START:
        DO_START_ELE(dreme, start_ele_dreme, PS_IN_DREME, IGNORE);
        known = 0;
        break;
      case PS_IN_DREME:
        DO_START_ELE(model, start_ele_model, PS_IN_MODEL, IGNORE);
        DO_START_ELE(motifs, start_ele_motifs, PS_IN_MOTIFS, IGNORE);
        DO_START_ELE(run_time, start_ele_run_time, PS_IN_RUN_TIME, IGNORE);
        known = 0;
        break;
      case PS_IN_MODEL:
        CHECK_START_ELE(command_line, PS_IN_COMMAND_LINE, ALL_CHARS);
        DO_START_ELE(positives, start_ele_positives, PS_IN_POSITIVES, IGNORE);
        DO_START_ELE(negatives, start_ele_negatives, PS_IN_NEGATIVES, IGNORE);
        DO_START_ELE(background, start_ele_background, PS_IN_BACKGROUND, IGNORE);
        DO_START_ELE(stop, start_ele_stop, PS_IN_STOP, IGNORE);
        CHECK_START_ELE(ngen, PS_IN_NGEN, ALL_BUT_SPACE);
        CHECK_START_ELE(add_pv_thresh, PS_IN_ADD_PV_THRESH, ALL_BUT_SPACE);
        CHECK_START_ELE(seed, PS_IN_SEED, ALL_BUT_SPACE);
        CHECK_START_ELE(host, PS_IN_HOST, ALL_BUT_SPACE);
        CHECK_START_ELE(when, PS_IN_WHEN, ALL_CHARS);
        CHECK_START_ELE(description, PS_IN_DESCRIPTION, ALL_CHARS);
      case PS_IN_MOTIFS:
        DO_START_ELE(motif, start_ele_motif, PS_IN_MOTIF, IGNORE);
        known = 0;
        break;
      case PS_IN_MOTIF:
        DO_START_ELE(pos, start_ele_pos, PS_IN_POS, IGNORE);
        DO_START_ELE(match, start_ele_match, PS_IN_MATCH, IGNORE);
        known = 0;
        break;
      default:
        known = 0;
        break;
    }
    if (!known) {
      if (allow_unknown) {
        ps->udepth = 1;
      } else {
        error(ps, "Encountered illegal tag %s while in state %s\n",(char*)name, state_names[ps->state]);
      }
    }
  }
}

/*****************************************************************************
 * Macros to greatly reduce the boilerplate code needed to keep track of the
 * parser state.
 *
 * DO_END_ELE compares the state and if it matches then tries to match the
 * element name. If that passes then it calls the callback and if that 
 * succeeds it transitions to the parent state.
 * CHECK_END_ELE does the same except it does not have a callback function.
 ****************************************************************************/
#define DO_END_ELE(_expected_,_call_,_state_,_transition_) \
  case _state_: \
    if (strcasecmp((char*)name, #_expected_) == 0) { \
      known = 1; \
      _call_ (ps); \
      if (ps->state == _state_) ps->state = _transition_; \
    } \
    break
#define CHECK_END_ELE(_expected_,_state_,_transition_) \
  case _state_: \
    if (strcasecmp((char*)name, #_expected_) == 0) { \
      known = 1; \
      ps->state = _transition_; \
    } \
    break

/*****************************************************************************
 * Handle a closing element
 ****************************************************************************/
void handle_dreme_end_ele(void *ctx, const xmlChar *name) {
  PS_T *ps = (PS_T*)ctx;
  int known = 0;
  if (ps->state == PS_ERROR) return;
  if (ps->udepth) {
    ps->udepth -= 1; 
  } else {
    switch (ps->state) {
      DO_END_ELE(dreme, end_ele_dreme, PS_IN_DREME, PS_END);
      DO_END_ELE(model, end_ele_model, PS_IN_MODEL, PS_IN_DREME);
      DO_END_ELE(motifs, end_ele_motifs, PS_IN_MOTIFS, PS_IN_DREME);
      CHECK_END_ELE(run_time, PS_IN_RUN_TIME, PS_IN_DREME);
      DO_END_ELE(command_line, end_ele_command_line, PS_IN_COMMAND_LINE, PS_IN_MODEL);
      CHECK_END_ELE(positives, PS_IN_POSITIVES, PS_IN_MODEL);
      CHECK_END_ELE(negatives, PS_IN_NEGATIVES, PS_IN_MODEL);
      CHECK_END_ELE(background, PS_IN_BACKGROUND, PS_IN_MODEL);
      CHECK_END_ELE(stop, PS_IN_STOP, PS_IN_MODEL);
      DO_END_ELE(ngen, end_ele_ngen, PS_IN_NGEN, PS_IN_MODEL);
      DO_END_ELE(add_pv_thresh, end_ele_add_pv_thresh, PS_IN_ADD_PV_THRESH, PS_IN_MODEL);
      DO_END_ELE(seed, end_ele_seed, PS_IN_SEED, PS_IN_MODEL);
      DO_END_ELE(host, end_ele_host, PS_IN_HOST, PS_IN_MODEL);
      DO_END_ELE(when, end_ele_when, PS_IN_WHEN, PS_IN_MODEL);
      DO_END_ELE(description, end_ele_description, PS_IN_DESCRIPTION, PS_IN_MODEL);
      DO_END_ELE(motif, end_ele_motif, PS_IN_MOTIF, PS_IN_MOTIFS);
      CHECK_END_ELE(pos, PS_IN_POS, PS_IN_MOTIF);
      CHECK_END_ELE(match, PS_IN_MATCH, PS_IN_MOTIF);
    }
    if (!known) {
      error(ps, "Unexpected end tag %s in state %s\n", (char*)name, state_names[ps->state]);
    }
  }
}

/*****************************************************************************
 * Register handlers on the xmlSAXHandler structure
 ****************************************************************************/
void register_dreme_io_xml_sax_handlers(xmlSAXHandler *handler) {
  memset(handler, 0, sizeof(xmlSAXHandler));
  handler->startDocument = handle_dreme_start_doc;
  handler->endDocument = handle_dreme_end_doc;
  handler->characters = handle_dreme_characters;
  handler->startElement = handle_dreme_start_ele;
  handler->endElement = handle_dreme_end_ele;
}

/*****************************************************************************
 * Creates the data to be passed to the SAX parser
 ****************************************************************************/
void* create_dreme_io_xml_sax_context(void *user_data, DREME_IO_XML_CALLBACKS_T *callbacks) {
  PS_T *ps;
  CHARBUF_T *buf;
  ps = (PS_T*)mm_malloc(sizeof(PS_T));
  memset(ps, 0, sizeof(PS_T));
  ps->state = PS_START;
  ps->udepth = 0; 
  ps->callbacks = callbacks;
  ps->user_data = user_data;
  ps->motif_id = NULL;
  ps->last_pos = 0;
  //set up character buffer
  buf = &(ps->characters);
  buf->buffer = mm_malloc(sizeof(char)*10);
  buf->buffer[0] = '\0';
  buf->size = 10;
  buf->pos = 0;
  //set up expected queue
  ps->expected_stack = linklst_create();
  return ps;
}

/*****************************************************************************
 * Destroys the data used by the SAX parser
 ****************************************************************************/
void destroy_dreme_io_xml_sax_context(void *ctx) {
  PS_T *ps = (PS_T*)ctx;
  if (ps->motif_id) free(ps->motif_id);
  free(ps->characters.buffer);
  linklst_destroy_all(ps->expected_stack, free);
  free(ps);
}
