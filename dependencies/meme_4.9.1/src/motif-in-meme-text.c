#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "array-list.h"
#include "linked-list.h"
#include "motif-in-meme-text.h"
#include "motif-spec.h"
#include "red-black-tree.h"
#include "regex-utils.h"
#include "string-builder.h"
#include "string-match.h"
#include "utils.h"

enum mtext_state {
  MTEXT_FIND_VERSION, // also look for <html> tag
  MTEXT_PRE_MOTIF,
  MTEXT_IN_LETTER_FREQ,
  MTEXT_IN_BACKGROUND,
  MTEXT_IN_MOTIF,
  MTEXT_IN_PSPM,
  MTEXT_IN_PSSM,
  MTEXT_PRE_C_SITES,
  MTEXT_IN_C_SITES,
  MTEXT_IN_C_SITES_BLOCKS,
  MTEXT_DONE
};
typedef enum mtext_state MTEXT_STATE_EN;

struct minfo {
  int length;
};

struct fscope {
  int options_found;
  int options_returned;
  BOOLEAN_T is_html;
  int vmajor;
  int vminor;
  int vpatch;
  char *release;
  char *datafile;
  ALPH_T alphabet;
  int strands;
  char *bg_source;
  ARRAY_T *background;
  ARRAY_T *letter_freqs;
  int motif_count;
  struct minfo* motif_lookup;
  double scanned_sites_threshold;
  SCANNED_SITE_T *ssite_buffer;
  int ssite_allocated;
  int ssite_used;
  ARRAYLST_T *scanned_seqs;
};

struct mscope {
  int options_found;
  int options_returned;
  BOOLEAN_T has_width;
  BOOLEAN_T has_evalue;
  BOOLEAN_T has_sites;
  BOOLEAN_T started_with_bl_line;
  MOTIF_T *motif;
};

struct patterns {
  regex_t html;
  regex_t version;
  regex_t release;
  regex_t datafile;
  regex_t alphabet;
  regex_t strands;
  regex_t pos_strand;
  regex_t neg_strand;
  regex_t letter_freq;
  regex_t background;
  regex_t bg_source;
  regex_t freq_pair;
  regex_t whitespace;
  regex_t motif_intro;
  regex_t motif_id2;
  regex_t motif_kv_pair;
  regex_t motif_pspm;
  regex_t motif_pssm;
  regex_t motif_num;
  regex_t motif_url;
  regex_t s_sites_header;
  regex_t s_sites_dashes;
  regex_t s_sites_titles;
  regex_t s_sites_divider;
  regex_t s_sites_seq;
  regex_t s_sites_block;
  regex_t s_sites_offset;
  regex_t s_sites_end;
};

typedef struct mtext MTEXT_T;
struct mtext {
  int options;
  char *file_name;
  short format_match;
  MTEXT_STATE_EN state;
  LINKLST_T *errors;
  LINKLST_T *motif_queue;
  STR_T *line;
  int counter; // used for a few things, like keeping track of which position of the background I'm up to.
  double sum; // used to check sums of probabilities
  struct patterns re;
  struct fscope fscope;
  struct mscope mscope;
};

static const char * HTML_RE = "<html>";
static const char * VERSION_RE = "^[[:space:]]*MEME[[:space:]]+version"
    "[[:space:]]+([0-9]+)(\\.([0-9]+)(\\.([0-9]+))?)?([^0-9].*)?$";
static const char * RELEASE_RE ="[[:space:]]+\\(Release[[:space:]]+date:"
    "[[:space:]]+(.+)\\)[[:space:]]*$";
static const char * DATAFILE_RE = "^[[:space:]]*DATAFILE[[:space:]]*="
    "[[:space:]]*([^[:space:]].*)$";
static const char * ALPHABET_RE = "^[[:space:]]*ALPHABET[[:space:]]*="
    "[[:space:]]*([^[:space:]]*)[[:space:]]*$";
static const char * STRANDS_RE = "^[[:space:]]*strands[[:space:]]*:"
    "([-+[:space:]]*)$";
static const char * POS_STRAND_RE = "\\+";
static const char * NEG_STRAND_RE = "-";
static const char * LETTER_FREQ_RE = "^Letter frequencies in dataset:$";
static const char * BACKGROUND_RE = "^[[:space:]]*Background[[:space:]]+"
    "letter[[:space:]]+frequencies([[:space:]].*)?$";
static const char * BG_SOURCE_RE = "^[[:space:]]+\\(from[[:space:]]+"
    "(.*)\\):.*$";
static const char * FREQ_PAIR_RE = "^[[:space:]]*([A-Za-z])[[:space:]]+"
    POS_NUM_RE "([[:space:]].*)?$";
static const char * WHITESPACE_RE = "^[[:space:]]*$";
// Can't ignore case on this one as normal meme format repeatedly
// uses Motif as the first word on a line
static const char * MOTIF_INTRO_RE = "^[[:space:]]*(BL[[:space:]]+)?MOTIF"
    "[[:space:]]*([^[:space:]]+)([[:space:]].*)?$";
// don't allow the second id to contain a '=' or be directly followed by an '='
// as that probably means it's not an id
static const char * MOTIF_ID2_RE = "^[[:space:]]*([^[:space:]=]+)"
    "([[:space:]]+([^[:space:]=].*)?)?$";
static const char * MOTIF_KV_PAIR_RE = "^[[:space:]]*([^[:space:]=]+)"
    "[[:space:]]*=[[:space:]]*([^[:space:]=]+)([[:space:]].*)?$";
static const char * MOTIF_PSPM_RE = "^[[:space:]]*letter[[:space:]]*-"
    "[[:space:]]*probability[[:space:]]+matrix[[:space:]]*:(.*)$";
static const char * MOTIF_PSSM_RE = "^[[:space:]]*log[[:space:]]*-"
    "[[:space:]]*odds[[:space:]]+matrix[[:space:]]*:(.*)$";
static const char * MOTIF_NUM_RE = "^[[:space:]]*" NUM_RE "([[:space:]].*)?$";
static const char * MOTIF_URL_RE = "^[[:space:]]*URL[[:space:]]*[[:space:]]*"
    "(.*)[[:space:]]*$";
// as this only really comes up in MEME output then require an exact match
static const char * C_SITES_HEADER_RE = "^\tCombined block diagrams: "
    "non-overlapping sites with p-value < " POS_NUM_RE "$";
static const char * C_SITES_DASHES_RE = "^-+$";
static const char * C_SITES_TITLES_RE = "^SEQUENCE NAME[[:space:]]+COMBINED "
    "P-VALUE[[:space:]]+MOTIF DIAGRAM$";
static const char * C_SITES_DIVIDER_RE = "^-+[[:space:]]+-+[[:space:]]+-+$";
static const char * C_SITES_SEQ_RE = "^[[:space:]]*([^[:space:]]+)[[:space:]]+"
    POS_NUM_RE "[[:space:]]+(.*)$";
static const char * C_SITES_BLOCK_RE = "^([[<])([-+]?)([0-9]+)([abc]?)\\("
    POS_NUM_RE "\\)([]>])";
static const char * C_SITES_OFFSET_RE = "^([0-9]+)";
static const char * C_SITES_END_RE = "^\\\\$";


/*
 * Compiles regular expressions
 */
static void compile_patterns(struct patterns *re) {
  regcomp_or_die("html", &(re->html), HTML_RE, REG_ICASE);
  regcomp_or_die("version", &(re->version), VERSION_RE, REG_EXTENDED | REG_ICASE);
  regcomp_or_die("release date", &(re->release), RELEASE_RE, REG_EXTENDED | REG_ICASE);
  regcomp_or_die("data file", &(re->datafile), DATAFILE_RE, REG_EXTENDED | REG_ICASE);
  regcomp_or_die("alphabet", &(re->alphabet), ALPHABET_RE, REG_EXTENDED | REG_ICASE);
  regcomp_or_die("strands", &(re->strands), STRANDS_RE, REG_EXTENDED | REG_ICASE);
  regcomp_or_die("positive strand", &(re->pos_strand), POS_STRAND_RE, 0);
  regcomp_or_die("negative strand", &(re->neg_strand), NEG_STRAND_RE, 0);
  regcomp_or_die("letter frequency", &(re->letter_freq), LETTER_FREQ_RE, REG_EXTENDED | REG_ICASE);
  regcomp_or_die("background", &(re->background), BACKGROUND_RE, REG_EXTENDED | REG_ICASE); // note ICASE is off for HTML
  regcomp_or_die("bg source", &(re->bg_source), BG_SOURCE_RE, REG_EXTENDED | REG_ICASE);
  regcomp_or_die("freq pair", &(re->freq_pair), FREQ_PAIR_RE, REG_EXTENDED);
  regcomp_or_die("whitespace", &(re->whitespace), WHITESPACE_RE, REG_NOSUB);
  regcomp_or_die("motif intro", &(re->motif_intro), MOTIF_INTRO_RE, REG_EXTENDED);
  regcomp_or_die("motif id 2", &(re->motif_id2), MOTIF_ID2_RE, REG_EXTENDED);
  regcomp_or_die("motif kv pair", &(re->motif_kv_pair), MOTIF_KV_PAIR_RE, REG_EXTENDED);
  regcomp_or_die("motif pspm", &(re->motif_pspm), MOTIF_PSPM_RE, REG_EXTENDED | REG_ICASE);
  regcomp_or_die("motif pssm", &(re->motif_pssm), MOTIF_PSSM_RE, REG_EXTENDED | REG_ICASE);
  regcomp_or_die("motif num", &(re->motif_num), MOTIF_NUM_RE, REG_EXTENDED);
  regcomp_or_die("motif url", &(re->motif_url), MOTIF_URL_RE, REG_EXTENDED | REG_ICASE);
  regcomp_or_die("c sites header", &(re->s_sites_header), C_SITES_HEADER_RE, REG_EXTENDED);
  regcomp_or_die("c sites dashes", &(re->s_sites_dashes), C_SITES_DASHES_RE, REG_EXTENDED);
  regcomp_or_die("c sites titles", &(re->s_sites_titles), C_SITES_TITLES_RE, REG_EXTENDED);
  regcomp_or_die("c sites divider", &(re->s_sites_divider), C_SITES_DIVIDER_RE, REG_EXTENDED);
  regcomp_or_die("c sites seq", &(re->s_sites_seq), C_SITES_SEQ_RE, REG_EXTENDED);
  regcomp_or_die("c sites block", &(re->s_sites_block), C_SITES_BLOCK_RE, REG_EXTENDED);
  regcomp_or_die("c sites offset", &(re->s_sites_offset), C_SITES_OFFSET_RE, REG_EXTENDED);
  regcomp_or_die("c sites offset", &(re->s_sites_end), C_SITES_END_RE, REG_EXTENDED);
}

/*
 * frees compiled regular expressions
 */
static void free_patterns(struct patterns *re) {
  regfree(&(re->html));
  regfree(&(re->version));
  regfree(&(re->release));
  regfree(&(re->datafile));
  regfree(&(re->alphabet));
  regfree(&(re->strands));
  regfree(&(re->pos_strand));
  regfree(&(re->neg_strand));
  regfree(&(re->letter_freq));
  regfree(&(re->background));
  regfree(&(re->bg_source));
  regfree(&(re->freq_pair));
  regfree(&(re->whitespace));
  regfree(&(re->motif_intro));
  regfree(&(re->motif_id2));
  regfree(&(re->motif_kv_pair));
  regfree(&(re->motif_pspm));
  regfree(&(re->motif_pssm));
  regfree(&(re->motif_num));
  regfree(&(re->motif_url));
  regfree(&(re->s_sites_header));
  regfree(&(re->s_sites_dashes));
  regfree(&(re->s_sites_titles));
  regfree(&(re->s_sites_divider));
  regfree(&(re->s_sites_seq));
  regfree(&(re->s_sites_block));
  regfree(&(re->s_sites_offset));
  regfree(&(re->s_sites_end));
}

/*
 * Initilizes file scoped data.
 * Assumes data is already zeroed.
 */
static void init_fscope(struct fscope *fscope) {
  fscope->alphabet = INVALID_ALPH;
}

/*
 * Frees file scoped data.
 */
static void free_fscope(struct fscope *fscope) {
  int i;
  if (fscope->release) free(fscope->release);
  if (fscope->datafile) free(fscope->datafile);
  if (fscope->bg_source) free(fscope->bg_source);
  if (fscope->background) {
    free_array(fscope->background);
  }
  if (fscope->letter_freqs) free_array(fscope->letter_freqs);
  if (fscope->motif_lookup) free(fscope->motif_lookup);
  if (fscope->ssite_buffer) free(fscope->ssite_buffer);
}

/*
 * Adds a motif in the mscope to the queue and resets the mscope.
 * Does not validate the motif.
 */
static void enqueue_motif(MTEXT_T *parser) {
  int num;
  struct minfo **lookup_ref, *item;
  if (parser->mscope.motif) {
    // set the alphabet
    parser->mscope.motif->alph = parser->fscope.alphabet;
    // set the flags
    parser->mscope.motif->flags = (parser->fscope.strands == 2 ? MOTIF_BOTH_STRANDS : 0);
    // give the url a default value of empty string if it is not present
    if (!parser->mscope.motif->url) parser->mscope.motif->url = strdup("");
    // get the motif index and increment the motif count
    num = parser->fscope.motif_count++;
    // cache motif information for the scanned sites
    if (parser->options & SCANNED_SITES) {
      // to read the scanned sites I need to cache the length
      lookup_ref = &(parser->fscope.motif_lookup);
      *lookup_ref = mm_realloc(*lookup_ref, sizeof(struct minfo) * (num + 1));
      item = (*lookup_ref)+num; 
      item->length = parser->mscope.motif->length;
    }
    // make the motif avaliable
    linklst_add(parser->mscope.motif, parser->motif_queue);
  }
  memset(&(parser->mscope), 0, sizeof(struct mscope));
}

/*
 * Creates the parser state object
 */
void* mtext_create(const char *optional_filename, int options) {
  MTEXT_T *parser;
  parser = mm_malloc(sizeof(MTEXT_T));
  memset(parser, 0, sizeof(MTEXT_T));
  parser->options = options;
  parser->file_name = (optional_filename ? strdup(optional_filename) : NULL);
  parser->format_match = file_name_match("meme", "txt", optional_filename);
  parser->state = MTEXT_FIND_VERSION;
  parser->errors = linklst_create();
  parser->motif_queue = linklst_create();
  parser->line = str_create(250);
  compile_patterns(&(parser->re));
  init_fscope(&(parser->fscope));
  return parser;
}

/*
 * Destroys the parser state object
 */
void mtext_destroy(void *data) {
  MTEXT_T *parser;
  parser = (MTEXT_T*)data;
  if (parser->file_name) free(parser->file_name);
  linklst_destroy_all(parser->errors, free);
  linklst_destroy_all(parser->motif_queue, destroy_motif);
  str_destroy(parser->line, FALSE);
  free_patterns(&(parser->re));
  free_fscope(&(parser->fscope));
  memset(parser, 0, sizeof(MTEXT_T));
  free(parser);
}

/*
 * Stores a parser error message for return
 */
static void error(MTEXT_T *parser, const char *format, ...) {
  va_list  argp;
  int len;
  char dummy[1], *msg;
  va_start(argp, format);
  len = vsnprintf(dummy, 1, format, argp);
  va_end(argp);
  msg = mm_malloc(sizeof(char) * (len + 1));
  va_start(argp, format);
  vsnprintf(msg, len + 1, format, argp);
  va_end(argp);
  linklst_add(msg, parser->errors);
}

/*
 * Allocate an alphabet array. If the alphabet is known then use the exact
 * size required, otherwise expand the array to fit.
 */
static void size_freqs_array(ARRAY_T **array, ALPH_T alpha, int index) {
  // reallocate the array to the correct size
  if (alpha != INVALID_ALPH) {
    if (*array == NULL || get_array_length(*array) < alph_size(alpha, ALPH_SIZE)) {
      *array = resize_array(*array, alph_size(alpha, ALPH_SIZE));
    }
  } else {
    *array = resize_array(*array, index + 1);
  }
}

/*
 * Parse space delimited key=value pairs and store them in a red-black tree.
 * If there is anything left over that is not whitespace then report an error.
 * Return the red-black tree.
 */
static RBTREE_T* parse_keyvals(MTEXT_T *parser, const char *line) {
  regmatch_t matches[3];
  const char *substr;
  RBTREE_T *kvpairs;
  char *key, *value;

  kvpairs = rbtree_create(rbtree_strcasecmp, NULL, free, NULL, free);
  substr = line;
  while(regexec_or_die("key/value", &(parser->re.motif_kv_pair), substr, 3, matches, 0)) {
    key = regex_str(matches+1, substr);
    value = regex_str(matches+2, substr);
    rbtree_put(kvpairs, key, value);
    substr = substr+(matches[2].rm_eo);
  }
  if (!regexec_or_die("whitespace", &(parser->re.whitespace), substr, 0, matches, 0)) {
    error(parser, "Couldn't parse \"%s\" as key = value pairs.", substr);
  }
  return kvpairs;
}

/*
 * Check if a given red-black tree with strings for keys and values
 * contains a passed key. If it does then convert the associated value
 * which should contain the length of the alphabet. Given that DNA has
 * 4 letters and protein has 20, use this value to set the alphabet.
 * If the alphabet has already been set then validate the value.
 */
static void set_alphabet_from_params(MTEXT_T *parser, RBTREE_T *kv_pairs, 
    const char *type, const char *name, const char *alpha_key) {
  RBNODE_T *node;
  char *value, *end;
  int alen;

  if ((node = rbtree_find(kv_pairs, alpha_key)) != NULL) {
    value = (char*)rbtree_value(node);
    errno = 0;
    alen = strtol(value, &end, 10);
    if (errno != 0 || *end != '\0') {
      error(parser, "The %s of motif %s has an alphabet length value \"%s\" "
          "which is not an integer.\n", type, name, value);
    } else {
      if (parser->fscope.alphabet != INVALID_ALPH) {
        if (alen != alph_size(parser->fscope.alphabet, ALPH_SIZE)) {
          error(parser, "The %s of motif %s has an alphabet length value %d "
              "which does not match the %s alphabet length of %d.\n", 
              type, name, alen, alph_name(parser->fscope.alphabet), 
              alph_size(parser->fscope.alphabet, ALPH_SIZE));
        }
      } else {
        switch (alen) {
          case DNA_ASIZE:
            parser->fscope.alphabet = DNA_ALPH;
            break;
          case PROTEIN_ASIZE:
            parser->fscope.alphabet = PROTEIN_ALPH;
            break;
          default:
            error(parser, "The %s of motif %s has an alphabet length value %d "
                "which does not match the lengths of any supported alphabets.\n", 
                type, name, alen);
        }
      }
    }
  }
}

/*
 * Check if a given red-black tree with strings for keys and values
 * contains a passed key. If it does then convert the associated value
 * which should contain the width of the current motif. If the width
 * of the motif is already known then validate this value against it,
 * otherwise check this value is sane and use it to set the motif width.
 */
static void set_motif_width_from_params(MTEXT_T *parser, RBTREE_T *kv_pairs, 
    const char *type, const char *name, const char *width_key) {
  RBNODE_T *node;
  char *value, *end;
  int width;
  if ((node = rbtree_find(kv_pairs, width_key)) != NULL) {
    value = (char*)rbtree_value(node);
    errno = 0;
    width = strtol(value, &end, 10);
    if (errno != 0 || *end != '\0') {
      error(parser, "The %s of motif %s has an width value \"%s\" "
          "which is not an integer.\n", type, name, value);
    } else {
      if (parser->mscope.has_width) {
        if (width != get_motif_length(parser->mscope.motif)) {
          error(parser, "The %s of motif %s has a width value %d "
              "which does not match the existing width value of %d.\n",
              type, name, width, get_motif_length(parser->mscope.motif));
        }
      } else {
        if (width <= 0) {
          error(parser, "The %s of motif %s has a width value %d "
              "which is invalid as it is not larger than zero.\n", 
              type, name, width);
        } else {
          parser->mscope.has_width = TRUE;
          parser->mscope.motif->length =  width;
        }
      }
    }
  }
}

/*
 * Check if a given red-black tree with strings for keys and values
 * contains a passed key. If it does then convert the associated value
 * which should contain the nsites of the current motif. If the nsites
 * of the motif is already known then validate this value against it,
 * otherwise check this value is sane and use it to set the motif nsites.
 */
static void set_motif_sites_from_params(MTEXT_T *parser, RBTREE_T *kv_pairs, 
    const char *type, const char *name, const char *sites_key) {
  RBNODE_T *node;
  char *value, *end;
  double sites;
  if ((node = rbtree_find(kv_pairs, sites_key)) != NULL) {
    value = (char*)rbtree_value(node);
    errno = 0;
    sites = strtod(value, &end);
    if (*end != '\0') {
      error(parser, "The %s of motif %s has a number of sites value \"%s\" "
          "which is not a number.\n", type, name, value);
    } else if (errno == ERANGE) {
      if (sites == 0) {
        error(parser, "The %s of motif %s has a number of sites value \"%s\" "
            "which is too small to represent.\n", type, name, value);
      } else {
        error(parser, "The %s of motif %s has a number of sites value \"%s\" "
            "which is too large to represent.\n", type, name, value);
      }
    } else {
      if (parser->mscope.has_sites) {
        if (sites != get_motif_nsites(parser->mscope.motif)) {
          error(parser, "The %s of motif %s has a number of sites value %g "
              "which does not match the existing value of %g.\n", type, name, 
              sites, get_motif_nsites(parser->mscope.motif));
        }
      } else {
        if (sites < 0) {
          error(parser, "The %s of motif %s has a number of sites value %g "
              "which is invalid as it is smaller than zero.\n", type, 
              name, sites);
        } else {
          parser->mscope.has_sites = TRUE;
          parser->mscope.motif->num_sites = sites;
        }
      }
    }
  }
}

/*
 * Check if a given red-black tree with strings for keys and values
 * contains a passed key. If it does then convert the associated value
 * which should contain the evalue of the current motif. If the evalue
 * of the motif is already known then validate this value against it,
 * otherwise check this value is sane and use it to set the motif evalue.
 */
static void set_motif_evalue_from_params(MTEXT_T *parser, RBTREE_T *kv_pairs, 
    const char *type, const char *name, const char *evalue_key) {
  RBNODE_T *node;
  char *value, *end;
  double log10_evalue;
  if ((node = rbtree_find(kv_pairs, evalue_key)) != NULL) {
    value = (char*)rbtree_value(node);
    errno = 0;
    log10_evalue = log10_evalue_from_string(value);
    if (errno == EINVAL) {
      error(parser, "The %s of motif %s has an evalue value \"%s\" "
          "which is not a number.\n", type, name, value);
    } else {
      if (parser->mscope.has_evalue) {
        if (log10_evalue != get_motif_log_evalue(parser->mscope.motif)) {
          error(parser, "The %s of motif %s has an evalue value %g "
              "which does not match the existing value of %g.\n", 
              type, name, exp10(log10_evalue), get_motif_evalue(parser->mscope.motif));
        }
      } else {
        parser->mscope.has_evalue = TRUE;
        parser->mscope.motif->log_evalue = log10_evalue;
        parser->mscope.motif->evalue = exp10(log10_evalue);
      }
    }
  }
}

/*
 * Given a red-black tree with strings as keys and values
 * check the alphabet, motif-width, motif-sites and motif-evalue.
 */
static void set_motif_params(MTEXT_T *parser, RBTREE_T *kv_pairs, 
    const char *type, const char *alpha_key, const char *width_key, 
    const char *sites_key, const char *evalue_key) {
  char *name;
  name = get_motif_id(parser->mscope.motif);
  set_alphabet_from_params(parser, kv_pairs, type, name, alpha_key);
  set_motif_width_from_params(parser, kv_pairs, type, name, width_key);
  set_motif_sites_from_params(parser, kv_pairs, type, name, sites_key);
  set_motif_evalue_from_params(parser, kv_pairs, type, name, evalue_key);
}

/*
 * Read an array of numbers which make up a row of either a PSPM or a PSSM
 */
static ARRAY_T* parse_nums(MTEXT_T *parser, BOOLEAN_T probabilities, BOOLEAN_T allow_bad, const char *line) {
  regmatch_t matches[3];
  int col, row;
  double num, sum, delta;
  const char *substr;
  char *name, *type;
  ARRAY_T *array;
  ALPH_T alpha;
  BOOLEAN_T problem;

  problem = FALSE;
  name = get_motif_id(parser->mscope.motif);
  type = (probabilities ? "PSPM" : "PSSM");
  row = parser->counter;
  alpha = parser->fscope.alphabet;
  if (alpha != INVALID_ALPH) array = allocate_array(alph_size(alpha, ALPH_SIZE));
  else array = NULL;
  substr = line;
  col = 0;
  sum = 0;
  while (regexec_or_die("motif num", &(parser->re.motif_num), substr, 3, matches, 0)) {
    if (alpha == INVALID_ALPH || col >= alph_size(alpha, ALPH_SIZE)) 
      array = resize_array(array, col + 1);
    num = regex_dbl(matches+1, substr);
    if (probabilities) {
      if (num < 0 || num > 1) {
        error(parser, "The PSPM of motif %s has a number which isn't a "
            "probability on row %d column %d. The number should be in "
            "the range 0 to 1 but it was %g.\n", name, row+1, col+1, num);
        problem = TRUE;
      }
      sum += num;
    }
    set_array_item(col, num, array);
    substr = substr+(matches[1].rm_eo);
    col++;
  }
  if (!regexec_or_die("whitespace", &(parser->re.whitespace), substr, 0, matches, 0) && 
      !(allow_bad && col == 0)) {
    error(parser, "The %s of motif %s has unparsable characters \"%s\" on "
        "the end of row %d. Only spaces or tabs are allowed on the same "
        "line as the numbers.\n", type, name, substr, row+1);
    problem = TRUE;
  }
  if (col == 0) return NULL; // empty row
  // try to guess the alphabet if it is unknown
  if (alpha == INVALID_ALPH) {
    switch(col) {
      case DNA_ASIZE: // assume DNA (could be RNA but DNA is more likely)
        parser->fscope.alphabet = DNA_ALPH;
        break;
      case PROTEIN_ASIZE: // AA is the only supported alphabet with 20 items
        parser->fscope.alphabet = PROTEIN_ALPH;
        break;
      default:
        error(parser, "The %s of motif %s has %d numbers in the %d row but "
            "this does not match with any alphabet.\n", type, name, 
            col, row+1);
        problem = TRUE;
    }
  } else if (col != alph_size(alpha, ALPH_SIZE)) {
    error(parser, "The %s of motif %s has %d numbers in the %d row but "
        "this does not match with the %s alphabet which requires %d "
        "numbers.\n", type, name, col+1, row, alph_name(alpha), 
        alph_size(alpha, ALPH_SIZE));
    problem = TRUE;
  }
  if (probabilities) {
    delta = sum - 1;
    if (delta < 0) delta = -delta;
    if (delta > 0.1) {
      error(parser, "The PSPM of motif %s has probabilities which don't "
          "sum to 1 on row %d.\n", name, row+1);
      problem = TRUE;
    }
  }
  if (!problem) return array;
  else {
    if (array) free_array(array);
    return NULL;
  }
}

/*
 * Searches lines for the version declaration. The version declaration is
 * required for MEME text format. Also attempts to bail out if the file
 * looks like the new HTML format.
 */
static BOOLEAN_T state_find_version(MTEXT_T *parser, const char *line) {
  regmatch_t matches[7];
  memset(matches, 0, sizeof(regmatch_t)*7);
  if (regexec_or_die("html", &(parser->re.html), line, 0, matches, 0)) {
    parser->fscope.is_html = TRUE; 
  }
  memset(matches, 0, sizeof(regmatch_t)*7);
  if (regexec_or_die("version", &(parser->re.version), line, 7, matches, 0)) {
    parser->fscope.vmajor = regex_int(matches+1, line, 0);
    parser->fscope.vminor = regex_int(matches+3, line, 0);
    parser->fscope.vpatch = regex_int(matches+5, line, 0);
    // see if the release date is avaliable as well
    if (regexec_or_die("release date", &(parser->re.release), line, 2, matches, 0)) {
      parser->fscope.release = regex_str(matches+1, line);
    }
    // check if this is a more modern html file to avoid 
    // conflict with the preferred parser
    if (parser->fscope.is_html) {
      if (parser->fscope.vmajor > 4 || 
          (parser->fscope.vmajor == 4 && parser->fscope.vminor > 3) || 
          (parser->fscope.vmajor == 4 && parser->fscope.vminor == 3 && 
           parser->fscope.vpatch > 2)) { 
        error(parser, "Newer html format detected. Stopping parse to avoid "
            "conflicts with the preferred parser.\n");
      } else {
        // Turn off ignore case for the background pattern
        regfree(&(parser->re.background));
        regcomp_or_die("background", &(parser->re.background), BACKGROUND_RE, REG_EXTENDED);
      }
    }
    parser->state = MTEXT_PRE_MOTIF;
  }
  return TRUE;
}

/*
 * Parses things that come before the motif but after the version line.
 * These include the alphabet, strands, letter frequencies and background.
 * The alphabet and strands it parses in full but for the letter frequencies
 * and background it reads the first line and changes state to parse the others.
 * When it spots the first motif it changes the state and rejects the line
 * so the motif parsing code can read it in full.
 */
static BOOLEAN_T state_pre_motif(MTEXT_T *parser, const char *line) {
  regmatch_t matches[2];
  BOOLEAN_T pos, neg;
  const char *substr;
  char *temp;
  int len;
  if (regexec_or_die("data file", &(parser->re.datafile), line, 2, matches, 0)) {
    parser->fscope.datafile = regex_str(matches+1, line);
  } else if (regexec_or_die("alphabet", &(parser->re.alphabet), line, 2, matches, 0)) {
    substr = line+(matches[1].rm_so);
    len = matches[1].rm_eo - matches[1].rm_so;
    if ((parser->fscope.alphabet = alph_type(substr, len)) == INVALID_ALPH) {
      temp = regex_str(matches+1, line);
      error(parser, "Unrecognised alphabet %s\n", temp);
      free(temp);
    }
  } else if (regexec_or_die("strands", &(parser->re.strands), line, 2, matches, 0)) {
    substr = line+(matches[1].rm_so);
    pos = regexec_or_die("positive strand", &(parser->re.pos_strand), substr, 0, matches, 0);
    neg = regexec_or_die("negative strand", &(parser->re.neg_strand), substr, 0, matches, 0);
    if (pos && neg) {
      parser->fscope.strands = 2;
    } else if (pos) {
      parser->fscope.strands = 1;
    }
  } else if (regexec_or_die("letter frequencies", &(parser->re.letter_freq), line, 0, matches, 0)) {
    parser->counter = 0;
    parser->sum = 0;
    parser->state = MTEXT_IN_LETTER_FREQ;
  } else if (regexec_or_die("background", &(parser->re.background), line, 2, matches, 0)) {
    substr = line+(matches[1].rm_so);
    if (regexec_or_die("background source", &(parser->re.bg_source), substr, 2, matches, 0)) {
      parser->fscope.bg_source = regex_str(matches+1, substr);
    }
    parser->counter = 0;
    parser->sum = 0;
    parser->state = MTEXT_IN_BACKGROUND;
  } else if (regexec_or_die("motif intro", &(parser->re.motif_intro), line, 0, matches, 0)) {
    parser->state = MTEXT_IN_MOTIF;
    return FALSE;// need to reparse this line
  } else if (regexec_or_die("combined sites header", &(parser->re.s_sites_header), line, 0, matches, 0)) {
    parser->state = MTEXT_PRE_C_SITES;
    parser->counter = 0;
    return FALSE;
  }
  return TRUE;
}

static BOOLEAN_T state_in_freqs(MTEXT_T *parser, const char *line) {
  regmatch_t matches[5];
  const char *substr;
  char letter;
  double freq, delta;
  ARRAY_T **array;
  
  if (parser->state == MTEXT_IN_LETTER_FREQ) {
    array = &(parser->fscope.letter_freqs);
  } else {
    array = &(parser->fscope.background);
  }
  substr = line;
  while (regexec_or_die("freq pair", &(parser->re.freq_pair), substr, 5, matches, 0)) {
    letter = regex_chr(matches+1, substr);
    freq = regex_dbl(matches+2, substr);
    substr = substr+(matches[2].rm_eo);
    // check the letter and determine alphabet if it is unknown
    if (!alph_test(&(parser->fscope.alphabet), parser->counter, letter)) {
      error(parser, "The frequency letter %c at position %d is invalid "
            "for the %s alphabet.\n", letter, parser->counter + 1, 
            alph_name(parser->fscope.alphabet));
      return TRUE;
    }
    // check the freq is valid
    if (freq < 0 || freq > 1) {
      error(parser, "The frequency %f associated with the letter %c at "
          "position %d is not valid as it is not in the range 0 to 1.\n", 
          freq, letter, parser->counter + 1);
      return TRUE;
    }
    // the letter is correct so now process it
    // resize the array if required
    size_freqs_array(array, parser->fscope.alphabet, parser->counter);
    set_array_item(parser->counter, freq, *array);
    parser->sum += freq;
    // move on to the next
    parser->counter++;
  }
  // see if we're finished
  if (parser->fscope.alphabet != INVALID_ALPH && 
      parser->counter >= alph_size(parser->fscope.alphabet, ALPH_SIZE)) {
    // check that the frequencies sum to 1
    delta = parser->sum - 1;
    if (delta < 0) delta = -delta;
    if (delta > 0.1) {
      error(parser, "The letter frequencies do not sum to 1.\n");
      return TRUE;
    }
    if (parser->format_match < 4) parser->format_match = 4;
    // continue searching for pre motif information
    parser->state = MTEXT_PRE_MOTIF;
  } 
  if (!regexec_or_die("whitespace", &(parser->re.whitespace), substr, 0, matches, 0)) {
    error(parser, "Expected only space after the letter "
        "frequencies but found \"%s\".\n", substr);
  }
  return TRUE;
}

static BOOLEAN_T state_in_motif(MTEXT_T *parser, const char *line) {
  regmatch_t matches[4];
  const char *substr;
  MOTIF_T *motif;
  RBTREE_T *kvpairs;
  BOOLEAN_T bl_line;
  if (regexec_or_die("motif intro", &(parser->re.motif_intro), line, 4, matches, 0)) {
    bl_line = ((matches[1].rm_eo - matches[1].rm_so) >= 2);
    if (parser->mscope.motif && regex_cmp(matches+2, line, get_motif_id(parser->mscope.motif)) == 0) {
      // check if we've seen this motif intro before as the old html format 
      // has a nasty habit of repeating the intro
      if (parser->fscope.is_html) return TRUE;
      // Additionally it's possible to have one or both of:
      // MOTIF 1 width=18 seqs=18
      // BL    MOTIF 1 width=18 seqs=18
      // So only conclude this is a new motif if we see the same type twice.
      if (parser->mscope.started_with_bl_line != bl_line) return TRUE;
    }
    enqueue_motif(parser);// enqueue existing motifs
    // create a new motif
    parser->mscope.started_with_bl_line = bl_line;
    parser->mscope.motif = (MOTIF_T*)mm_malloc(sizeof(MOTIF_T));
    motif = parser->mscope.motif;
    memset(motif, 0, sizeof(MOTIF_T));
    // set defaults
    motif->complexity = -1;
    set_motif_strand('+', motif);
    // copy over the id
    set_motif_id(line+(matches[2].rm_so), matches[2].rm_eo - matches[2].rm_so, motif);
    // try to find a second id
    substr = line+(matches[2].rm_eo);
    if ((!parser->fscope.is_html) && 
        regexec_or_die("motif id 2", &(parser->re.motif_id2), substr, 4, matches, 0)) {
      // copy of the second id
      set_motif_id2(substr+(matches[1].rm_so), matches[1].rm_eo - matches[1].rm_so, motif);
      substr = substr+(matches[1].rm_eo);
    }
    // should I bother with the key/value arguments?
  } else if (regexec_or_die("motif pspm", &(parser->re.motif_pspm), line, 2, matches, 0)) {
    assert(parser->mscope.motif != NULL);
    if (parser->mscope.motif->freqs != NULL) {
      error(parser, "Repeated \"letter-probability matrix\" section in motif %s.\n", 
          get_motif_id(parser->mscope.motif));
      return TRUE;
    }
    substr = line+(matches[1].rm_so);
    // parse key / value pairs
    kvpairs = parse_keyvals(parser, substr);
    set_motif_params(parser, kvpairs, "pspm", "alength", "w", "nsites", "E");
    rbtree_destroy(kvpairs);
    parser->counter = 0;
    parser->state = MTEXT_IN_PSPM;
  } else if (regexec_or_die("motif pssm", &(parser->re.motif_pssm), line, 2, matches, 0)) {
    assert(parser->mscope.motif != NULL);
    if (parser->mscope.motif->scores != NULL) {
      error(parser, "Repeated \"log-odds matrix\" section in motif %s.\n", 
          get_motif_id(parser->mscope.motif));
      return TRUE;
    }
    substr = line+(matches[1].rm_so);
    kvpairs = parse_keyvals(parser, substr);
    // n is not equalivent to nsites, n is weighted possible sites
    set_motif_params(parser, kvpairs, "pssm", "alength", "w", "", "E");
    rbtree_destroy(kvpairs);
    parser->counter = 0;
    parser->state = MTEXT_IN_PSSM;
  } else if (regexec_or_die("motif url", &(parser->re.motif_url), line, 2, matches, 0)) {
    parser->mscope.motif->url = regex_str(matches+1, line);
  } else if (regexec_or_die("combined sites header", &(parser->re.s_sites_header), line, 0, matches, 0)) {
    parser->state = MTEXT_PRE_C_SITES;
    parser->counter = 0;
    return FALSE;
  }
  return TRUE;
}

static BOOLEAN_T state_in_pspm(MTEXT_T *parser, const char *line) {
  ARRAY_T *row;
  MOTIF_T *motif;

  motif = parser->mscope.motif;
  // sanity check
  if (motif->freqs) {
    assert(parser->counter == get_num_rows(motif->freqs));
  } else {
    assert(parser->counter == 0);
  }
  // parse the line
  row = parse_nums(parser, TRUE, !parser->mscope.has_width, line);
  if (row) {
    if (motif->freqs) {
      grow_matrix(row, motif->freqs);
    } else {
      motif->freqs = array_to_matrix(TRUE, row);
    }
    free_array(row);
    parser->counter++;
  } else if (!parser->mscope.has_width) {
    // assume the first blank line or non-number is the end of the numbers
    motif->length = parser->counter;
    parser->mscope.has_width = TRUE;
    parser->state = MTEXT_IN_MOTIF;
    return FALSE;
  }
  if (parser->mscope.has_width && 
      parser->counter >= get_motif_length(parser->mscope.motif)) {
    parser->state = MTEXT_IN_MOTIF;
  }
  return TRUE;
}

static BOOLEAN_T state_in_pssm(MTEXT_T *parser, const char *line) {
  ARRAY_T *row;
  MOTIF_T *motif;

  motif = parser->mscope.motif;
  // sanity check
  if (motif->scores) {
    assert(parser->counter == get_num_rows(motif->scores));
  } else {
    assert(parser->counter == 0);
  }
  // parse the line
  row = parse_nums(parser, FALSE, !parser->mscope.has_width, line);
  if (row) {
    if (motif->scores) {
      grow_matrix(row, motif->scores);
    } else {
      motif->scores = array_to_matrix(TRUE, row);
    }
    free_array(row);
    parser->counter++;
  } else if (!parser->mscope.has_width) {
    // assume the first blank line is the end of the numbers
    motif->length = parser->counter;
    parser->mscope.has_width = TRUE;
    parser->state = MTEXT_IN_MOTIF;
    return FALSE;
  }
  if (parser->mscope.has_width && 
      parser->counter >= get_motif_length(parser->mscope.motif)) {
    parser->state = MTEXT_IN_MOTIF;
  }
  return TRUE;
}

static void cleanup_in_pspm_or_pssm(MTEXT_T *parser, const char *type) {
  MOTIF_T *motif;
  motif = parser->mscope.motif;
  if (parser->mscope.has_width && parser->counter < get_motif_length(motif)) {
    error(parser, "The %s of motif %s has a detected width value %d "
        "which does not match the existing width value of %d.\n",
        type, get_motif_id(motif), parser->counter, get_motif_length(motif));
    // note that we return the motif anyway...
  }
  motif->length = parser->counter;
  parser->mscope.has_width = TRUE;
  parser->state = MTEXT_IN_MOTIF;
}

static BOOLEAN_T state_pre_s_sites(MTEXT_T *parser, const char *line) {
  regmatch_t matches[3];
  regex_t *patterns[4] = {
    &(parser->re.s_sites_header), 
    &(parser->re.s_sites_dashes), 
    &(parser->re.s_sites_titles), 
    &(parser->re.s_sites_divider)};

  if (regexec_or_die("c sites headers", patterns[parser->counter], line, 3, matches, 0)) {
    if (parser->counter == 0) {
      parser->fscope.scanned_sites_threshold = regex_dbl(matches+1, line);
    }
    parser->counter++;
    if (parser->counter >= 4) {
      enqueue_motif(parser);
      parser->state = MTEXT_IN_C_SITES;
      parser->counter = 0;
    }
    return TRUE;
  } else {
    parser->state = MTEXT_DONE;
    return FALSE;
  }
}

static BOOLEAN_T state_in_s_sites_blocks(MTEXT_T *parser, const char *line) {
  SCANNED_SEQ_T *sseq;
  int i;
  BOOLEAN_T continued;
  const char *substr;
  regmatch_t matches[8];
  
  continued = FALSE;
  substr = line;
  // skip leading whitespace
  while (isspace(*substr)) substr++;
  // get the scanned sequence object to store this information
  if (parser->options & SCANNED_SITES) {
    sseq = arraylst_get(arraylst_size(parser->fscope.scanned_seqs) - 1, 
        parser->fscope.scanned_seqs);
  } else {
    sseq = NULL;
  }
  // tally up the components and check the structure
  while (*substr != '\0') {
    if (regexec_or_die("c sites block", &(parser->re.s_sites_block), substr, 8, matches, 0)) {
      int motif_i;
      // found a motif, check the brackets surrounding it match
      // left bracket + 2 = right bracket in ascii
      if ((regex_chr(matches+1, substr) + 2) != regex_chr(matches+7, substr)) {
        error(parser, "Brackets don't match!\n");
        return TRUE;
      }
      // get the index of the motif
      motif_i = regex_int(matches+3, substr, 0) - 1; // turn into index
      if (motif_i < 0 || motif_i >= parser->fscope.motif_count) {
        error(parser, "Bad motif number %d in scanned sites.\n", motif_i +1);
        return TRUE;
      }
      if (parser->options & SCANNED_SITES) {
        BOOLEAN_T isstrong;
        char strand;
        double log10pvalue;
        int motif_len;
        motif_len = parser->fscope.motif_lookup[motif_i].length;
        strand = regex_chr(matches+2, substr);
        log10pvalue = regex_log10_dbl(matches+5, substr); 
        sseq_add_site(sseq, motif_i + 1, motif_len, strand, log10pvalue);
      }
      // move over the motif
      substr += matches[0].rm_eo;
    } else if (regexec_or_die("c sites offset", &(parser->re.s_sites_offset), substr, 2, matches, 0)) {
      // found a part of the sequence with no motifs
      if (parser->options & SCANNED_SITES) {
        sseq_add_spacer(sseq, regex_int(matches+1, substr, 0));
      }
      substr += matches[0].rm_eo;
    } else if (regexec_or_die("c sites end", &(parser->re.s_sites_end), substr, 0, matches, 0)) {
      // found a line continuation indicator
      continued = TRUE;
      substr++;
      break;
    } else {
      error(parser, "Unexpected text in combined sites: %s\n", substr);
      return TRUE;
    }
    if (*substr == '_') {
      substr++;
    } else if (*substr != '\0') {
      error(parser, "Expected underscore!\n");
      return TRUE;
    }
  }
  if (continued) {
    parser->state = MTEXT_IN_C_SITES_BLOCKS;
  } else {
    parser->state = MTEXT_IN_C_SITES;
  }
  return TRUE;
}

static BOOLEAN_T state_in_s_sites(MTEXT_T *parser, const char *line) {
  regmatch_t matches[8];
  const char *blocks, *substr;
  char *seq_id;
  double seq_log10_pvalue;
  int seq_site_count, seq_length;
  SCANNED_SEQ_T *sseq;

  if (parser->options & SCANNED_SITES) {
    if (!parser->fscope.scanned_seqs) 
      parser->fscope.scanned_seqs = arraylst_create();
  }

  if (regexec_or_die("c sites seq", &(parser->re.s_sites_seq), line, 5, matches, 0)) {
    blocks = line+(matches[4].rm_so);
    if (parser->options & SCANNED_SITES)  {
      seq_id = regex_str(matches+1, line);  
      seq_log10_pvalue = regex_log10_dbl(matches+2, line);
      sseq = sseq_create2(seq_id, seq_log10_pvalue);
      arraylst_add(sseq, parser->fscope.scanned_seqs);
    }
    return state_in_s_sites_blocks(parser, blocks);
  } else if (regexec_or_die("dashes", &(parser->re.s_sites_dashes), line, 0, matches, 0)) {
    if (parser->options & SCANNED_SITES) parser->fscope.options_found |= SCANNED_SITES;
    parser->state = MTEXT_DONE;
  } else {
    error(parser, "Unrecognised line \"%s\" in scanned sites.\n", line);
    parser->state = MTEXT_DONE;
  }
  return TRUE;
}

/*
 * When given a line of text from the file consult the current state of the 
 * parser and send the line to the approprate parsing method. That parsing
 * method may either consume the line or change the parser state and reject
 * the line. A rejected line will be sent on to another parsing method dependent
 * on the new state.
 */
static void dispatch_line(MTEXT_T *parser, const char *line, size_t size, BOOLEAN_T end) {
  BOOLEAN_T consumed;
  MTEXT_STATE_EN before;
  consumed = FALSE;
  while (!consumed) {
    before = parser->state;
    switch (parser->state) {
      case MTEXT_FIND_VERSION:
        consumed = state_find_version(parser, line);
        break;
      case MTEXT_PRE_MOTIF:
        consumed = state_pre_motif(parser, line);
        break;
      case MTEXT_IN_LETTER_FREQ:
        consumed = state_in_freqs(parser, line);
        break;
      case MTEXT_IN_BACKGROUND:
        consumed = state_in_freqs(parser, line);
        break;
      case MTEXT_IN_MOTIF:
        consumed = state_in_motif(parser, line);
        break;
      case MTEXT_IN_PSPM:
        consumed = state_in_pspm(parser, line);
        break;
      case MTEXT_IN_PSSM:
        consumed = state_in_pssm(parser, line);
        break;
      case MTEXT_PRE_C_SITES:
        consumed = state_pre_s_sites(parser, line);
        break;
      case MTEXT_IN_C_SITES:
        consumed = state_in_s_sites(parser, line);
        break;
      case MTEXT_IN_C_SITES_BLOCKS:
        consumed = state_in_s_sites_blocks(parser, line);
        break;
      case MTEXT_DONE:
        consumed = TRUE;
        break;//do nothing
      default:
        die("Unknown state");
    }
    if (!consumed && parser->state == before) {
      die("Infinite loop");
    }
  }
  if (end) {
    while (parser->state != MTEXT_DONE) {
      before = parser->state;
      switch (parser->state) {
        case MTEXT_FIND_VERSION:
        case MTEXT_PRE_MOTIF:
        case MTEXT_IN_LETTER_FREQ:
        case MTEXT_IN_BACKGROUND:
        case MTEXT_IN_C_SITES:
          parser->state = MTEXT_DONE;
          break;
        case MTEXT_IN_MOTIF:
        case MTEXT_PRE_C_SITES:
          enqueue_motif(parser);
          parser->state = MTEXT_DONE;
          break;
        case MTEXT_IN_PSPM:
          cleanup_in_pspm_or_pssm(parser, "PSPM");
          break;
        case MTEXT_IN_PSSM:
          cleanup_in_pspm_or_pssm(parser, "PSSM");
          break;
        default:
          die("Unknown state");
      }
      if (parser->state == before) {
        die("Infinite loop");
      }
    }
  }
}

/*
 * Receives chunks of the file and caches or splits as necessary to get lines 
 * that can be parsed. Complete lines are sent to the dispatch_line method.
 */
#define MAX_LINE_LEN 1000
void mtext_update(void *data, const char *chunk, size_t size, short end) {
  assert(data != NULL);
  assert(size >= 0);

  MTEXT_T *parser;
  int offset, i;

  parser = (MTEXT_T*)data;
  i = 0;
  while (i < size) {
    // handle old Mac style newlines (\n\r)
    if (str_len(parser->line) == 0 && chunk[i] == '\r') i++;
    offset = i;
    for (; i < size; ++i) {
      if (chunk[i] == '\n') break;
    }
    if (str_len(parser->line) + i > MAX_LINE_LEN) {
      error(parser, "Maximum line length exceeded.\n");
      return;
    }
    str_append(parser->line, chunk+offset, (i - offset));
    if (i < size || end) {
      // handle DOS style newlines (\r\n)
      if (str_len(parser->line) > 0 && str_char(parser->line, -1) == '\r') {
        str_truncate(parser->line, -1);
      }
      dispatch_line(parser, str_internal(parser->line), str_len(parser->line), 
          (end && i >= size - 1));
      // don't allow it to shrink the buffer
      str_ensure(parser->line, str_len(parser->line));
      str_clear(parser->line);
    }
    i++; // step over the newline
  }
  if (size == 0 && end) {
    dispatch_line(parser, str_internal(parser->line), str_len(parser->line), 1);
  }
}

short mtext_has_format_match(void *data) {
  return ((MTEXT_T*)data)->format_match;
}

short mtext_has_error(void *data) {
  return linklst_size(((MTEXT_T*)data)->errors) > 0;
}

char* mtext_next_error(void *data) {
  return (char*)linklst_pop(((MTEXT_T*)data)->errors);
}

short mtext_has_motif(void *data) {
  return linklst_size(((MTEXT_T*)data)->motif_queue) > 0;
}

MOTIF_T* mtext_next_motif(void *data) {
  return (MOTIF_T*)linklst_pop(((MTEXT_T*)data)->motif_queue);
}

ALPH_T mtext_get_alphabet(void *data) {
  MTEXT_T *parser;
  parser = (MTEXT_T*)data;
  return parser->fscope.alphabet;
}

int mtext_get_strands(void *data) {
  MTEXT_T *parser;
  parser = (MTEXT_T*)data;
  return parser->fscope.strands;
}

BOOLEAN_T mtext_get_bg(void *data, ARRAY_T **bg) {
  MTEXT_T *parser;
  parser = (MTEXT_T*)data;
  if (parser->fscope.background == NULL) return FALSE;
  *bg = resize_array(*bg, get_array_length(parser->fscope.background));
  copy_array(parser->fscope.background, *bg);
  return TRUE;
}

void* mtext_motif_optional(void *data, int option) {
  MTEXT_T *parser;
  parser = (MTEXT_T*)data;
  if (!(parser->options & option)) {
    die("Requested value of optional component which was not requested "
        "during construction.\n");
    return NULL;
  }
  //TODO
  return NULL;
}

void* mtext_file_optional(void *data, int option) {
  MTEXT_T *parser;
  parser = (MTEXT_T*)data;
  if (!(parser->options & option)) {
    die("Requested value of optional component which was not requested "
        "during construction.\n");
    return NULL;
  }
  if (parser->fscope.options_found & option) {
    if (parser->fscope.options_returned & option) {
      die("Sorry, optional values are only returned once. "
          "This is because we can not guarantee that the "
          "previous caller did not deallocate the memory. "
          "Hence this is a feature to avoid memory bugs.\n");
      return NULL;
    }
    parser->fscope.options_returned |= option;
  } else {
    // Not yet found or unsupported
    return NULL;
  }
  switch (option) {
    case SCANNED_SITES:
      return parser->fscope.scanned_seqs;
    default:
      die("Option code %d does not match any single option. "
          "This means that it must contain multiple options "
          "and only one is allowed at a time\n.", option);
  }
  return NULL; //unreachable! make compiler happy
}
