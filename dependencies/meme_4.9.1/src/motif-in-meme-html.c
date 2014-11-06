
#include <assert.h>
#include <errno.h>
#include <limits.h>
#include <stdarg.h>
#include <string.h>
#include <strings.h>
#include <sys/types.h>
#include <regex.h>

#include "array-list.h"
#include "html-data.h"
#include "linked-list.h"
#include "matrix.h"
#include "motif-in-meme-html.h"
#include "motif-spec.h"
#include "red-black-tree.h"
#include "regex-utils.h"
#include "utils.h"

struct fscope {
  int options_found;
  int options_returned;
  int vmajor;
  int vminor;
  int vpatch;
  ALPH_T alphabet;
  int strands;
  ARRAY_T *background;
  int nmotifs;
  ARRAYLST_T *scanned_sites;
};

struct mscope {
  int options_found;
  int options_returned;
  BOOLEAN_T has_name;
  BOOLEAN_T has_pssm;
  BOOLEAN_T has_pspm;
  BOOLEAN_T has_width;
  BOOLEAN_T has_sites;
  BOOLEAN_T has_evalue;
  MOTIF_T *motif;
};

struct patterns {
  regex_t hf_name;
  regex_t version;
  regex_t bg_letter;
  regex_t motif_name;
  regex_t scanned_seq;
  regex_t scanned_site;
  regex_t log_odds;
  regex_t letter_prob;
  regex_t kv_pair;
  regex_t whitespace;
};

typedef struct meme_ctx CTX_T;
struct meme_ctx {
  int options;
  char *filename;
  struct fscope fscope;
  struct mscope mscope;
  struct mscope *out;
  LINKLST_T *errors;
  LINKLST_T *scopes;
  int format_match;
  int current_num;
  struct patterns re;
};

typedef struct mhtml MHTML_T;
struct mhtml {
  CTX_T *data;
  HDATA_T *ctxt;
};

/* regular expressions {{{ */

static const char * HF_NAME_RE = "^([a-zA-Z]+)([0-9]+)$";

static const char * VERSION_RE = "^MEME version ([0-9]+)(\\.([0-9]+)(\\.([0-9]+))?)?$";

static const char * BG_LETTER_RE = "^([[:space:]]*([A-Za-z])[[:space:]]+"
    "([+]?[0-9]*\\.?[0-9]+([eE][-+]?[0-9]+)?))([[:space:]].*)?$";

static const char * MOTIF_NAME_RE = "^(.*[[:space:]])?MOTIF[[:space:]]+"
    "([^[:space:]]+)([[:space:]].*)?$";

static const char * SCANNED_SEQ_RE = "^[[:space:]]*(([^[:space:]]+)[[:space:]]+"
    "([+]?[0-9]*\\.?[0-9]+([eE][-+]?[0-9]+)?)[[:space:]]+"
    "([0-9]+)[[:space:]]+([0-9]+))([[:space:]].*)?$";

static const char * SCANNED_SITE_RE = "^[[:space:]]*([+-]?)([0-9]+)[[:space:]]+"
    "([0-9]+)[[:space:]]+([+]?[0-9]*\\.?[0-9]+([eE][-+]?[0-9]+)?)"
    "([[:space:]].*)?$";
static const char * LOG_ODDS_RE = "^[[:space:]]*log-odds matrix:"
    "([[:space:]].*)$";
static const char * LETTER_PROB_RE = "^[[:space:]]*letter-probability matrix:"
    "([[:space:]].*)$";

static const char * KV_PAIR_RE = "^[[:space:]]*([^[:space:]=]+)"
    "[[:space:]]*=[[:space:]]*([^[:space:]=]+)([[:space:]].*)?$";

static const char * WHITESPACE_RE = "^[[:space:]]*$";

/* }}} */

/* structure construction and destruction {{{ */

/*****************************************************************************
 * Compile all regular expressions
 ****************************************************************************/
static void compile_patterns(struct patterns *re) {
  regcomp_or_die("hidden field name", &(re->hf_name), HF_NAME_RE, REG_EXTENDED);
  regcomp_or_die("version", &(re->version), VERSION_RE, REG_EXTENDED | REG_ICASE);
  regcomp_or_die("background letter pair", &(re->bg_letter), BG_LETTER_RE, REG_EXTENDED);
  regcomp_or_die("motif info", &(re->motif_name), MOTIF_NAME_RE, REG_EXTENDED | REG_ICASE);
  regcomp_or_die("scanned sequence", &(re->scanned_seq), SCANNED_SEQ_RE, REG_EXTENDED | REG_NEWLINE);
  regcomp_or_die("scanned site", &(re->scanned_site), SCANNED_SITE_RE, REG_EXTENDED | REG_NEWLINE);
  regcomp_or_die("pssm", &(re->log_odds), LOG_ODDS_RE, REG_EXTENDED | REG_ICASE | REG_NEWLINE);
  regcomp_or_die("pspm", &(re->letter_prob), LETTER_PROB_RE, REG_EXTENDED | REG_ICASE | REG_NEWLINE);
  regcomp_or_die("kv pair", &(re->kv_pair), KV_PAIR_RE, REG_EXTENDED);
  regcomp_or_die("whitespace", &(re->whitespace), WHITESPACE_RE, REG_EXTENDED);
}

/*****************************************************************************
 * Free all regular expressions
 ****************************************************************************/
static void free_patterns(struct patterns *re) {
  regfree(&(re->hf_name));
  regfree(&(re->version));
  regfree(&(re->bg_letter));
  regfree(&(re->motif_name));
  regfree(&(re->scanned_seq));
  regfree(&(re->scanned_site));
  regfree(&(re->log_odds));
  regfree(&(re->letter_prob));
  regfree(&(re->kv_pair));
  regfree(&(re->whitespace));
}

/*****************************************************************************
 * Create a structure for storing data for the parser
 ****************************************************************************/
static CTX_T* create_parser_data(int options, const char *optional_filename) {
  CTX_T* data;
  int i;

  data = (CTX_T*)mm_malloc(sizeof(CTX_T));
  memset(data, 0, sizeof(CTX_T));
  data->format_match = file_name_match("meme", "html", optional_filename);
  data->options = options;
  data->errors = linklst_create();
  data->scopes = linklst_create();
  compile_patterns(&(data->re));
  return data;
}

/*****************************************************************************
 * Destroy the motif scope structure
 ****************************************************************************/
void destroy_motif_scope(void *p) {
  struct mscope *data; 
  data = (struct mscope *)p;
  if (data->motif) destroy_motif(data->motif);
  memset(data, 0, sizeof(struct mscope));
  free(data);
}

/*****************************************************************************
 * Destroy the parser data structure
 ****************************************************************************/
static void destroy_parser_data(CTX_T *data) {
  linklst_destroy_all(data->errors, free);
  linklst_destroy_all(data->scopes, destroy_motif_scope);
  if (data->out) {
    data->out->motif = NULL; // don't destroy the motif as it has been returned
    destroy_motif_scope(data->out);
  }
  if (data->fscope.background) {
    free_array(data->fscope.background);
  }
  if (data->filename) free(data->filename);
  free_patterns(&(data->re));
  memset(data, 0, sizeof(CTX_T));
  free(data);
}

/*****************************************************************************
 * Store an error message
 ****************************************************************************/
void html_error(void *ctx, const char *format, ...) {
  CTX_T *data;
  va_list  argp;
  int len;
  char dummy[1], *msg;
  data = (CTX_T*)ctx;
  va_start(argp, format);
  len = vsnprintf(dummy, 1, format, argp);
  va_end(argp);
  msg = mm_malloc(sizeof(char) * (len + 1));
  va_start(argp, format);
  vsnprintf(msg, len + 1, format, argp);
  va_end(argp);
  linklst_add(msg, data->errors);
}

/* }}} */


/* Other utility methods {{{ */
/*****************************************************************************
 * Read a single number, checking if a newline character was skipped.
 ****************************************************************************/
BOOLEAN_T read_number(const char *str, int *offset, BOOLEAN_T *newline, double *num) {
  char *endptr;
  BOOLEAN_T valid;
  // consume space, checking for new line
  *newline = (*offset == 0);// the inital read is always a new line
  while (TRUE) {
    if (str[*offset] == '\n') {
      *newline = TRUE;
    } else if (!isspace(str[*offset])) {
      break;
    }
    *offset = (*offset) + 1;
  }
  // read a number
  *num = strtod(str+(*offset), &endptr);
  valid = (endptr != (str+(*offset)));
  *offset = (endptr - str) / sizeof(char);
  return valid;
}

/*****************************************************************************
 * Read a grid of numbers into PSPM or PSSM
 ****************************************************************************/
static void read_grid(CTX_T *ctx, BOOLEAN_T is_pspm, const char *grid) {
  ARRAY_T *line;
  MATRIX_T *matrix;
  BOOLEAN_T newline, read_num;
  int offset, line_count, row, col;
  double num;
  char *name, *type;
  ALPH_T *alph;

  name = (ctx->mscope.has_name ? get_motif_id(ctx->mscope.motif) : "<name not set>");
  type = (is_pspm ? "PSPM" : "PSSM");
  alph = &(ctx->fscope.alphabet);

  // count the entries per line
  line_count = 0;
  // read the first number
  offset = 0;
  if (!read_number(grid, &offset, &newline, &num)) {
    html_error(ctx, "The %s of motif %s has no matrix.\n", type, name);
    return;
  }
  // now read until we find a new line or run out of numbers
  do {
    line_count++;
    read_num = read_number(grid, &offset, &newline, &num);
  } while (read_num && !newline);

  if (*alph != INVALID_ALPH && alph_size(*alph, ALPH_SIZE) != line_count) {
    html_error(ctx, "The %s of motif %s has %d numbers in the first row "
        "but this does not match the %s alphabet which requires %d "
        "numbers.\n", type, name, line_count, alph_name(*alph), 
        alph_size(*alph, ALPH_SIZE));
    return;
  }

  offset = 0;
  line = allocate_array(line_count);
  matrix = NULL;
  row = 0;
  while (TRUE) {
    for (col = 0; col < line_count; ++col) {
      read_num = read_number(grid, &offset, &newline, &num);
      if (is_pspm && (num < 0 || num > 1)) {
        html_error(ctx, "The %s of motif %s has a number which isn't a "
            "probability on row %d column %d. The number should be in "
            "the range 0 to 1 but it was %g.\n", type, name, row+1, col+1, num);
        goto cleanup;
      }
      if (!read_num) {
        if (col != 0) {
          html_error(ctx, "The %s of motif %s has too few numbers on row %d.\n",
              type, name, row+1);
          goto cleanup;
        }
        goto all_read; // escape both loops
      }
      if (newline) {
        if (col != 0) { // expected a few more numbers before the new line!
          html_error(ctx, "The %s of motif %s has too few numbers on row %d.\n",
              type, name, row+1);
          goto cleanup;
        }
      } else {
        if (col == 0) { // there should have been a newline!
          html_error(ctx, "The %s of motif %s has too many numbers on row %d.\n",
              type, name, row+1);
          goto cleanup;
        }
      }
      set_array_item(col, num, line);
    }
    if (matrix == NULL) {
      matrix = allocate_matrix(1, line_count);
      set_matrix_row(0, line, matrix);
    } else {
      grow_matrix(line, matrix);
    }
    row++;
  }
all_read:
  if (ctx->mscope.has_width) {
    if (ctx->mscope.motif->length != row) {
      html_error(ctx, "The %s of motif %s has %d rows but %d rows were "
          "expected.\n", type, name, row, ctx->mscope.motif->length);
      goto cleanup;
    }
  } else {
    ctx->mscope.motif->length = row;
  }
  if (is_pspm) {
    ctx->mscope.motif->freqs = matrix;
  } else {
    ctx->mscope.motif->scores = matrix;
  }
cleanup:
  free_array(line);
}


/*****************************************************************************
 * Read a set of key/number pairs from a specified part of the string.
 ****************************************************************************/
static RBTREE_T* parse_keyvals(CTX_T *parser, const char *line) {
  regmatch_t matches[3];
  const char *substr;
  RBTREE_T *kvpairs;
  char *key, *value;

  kvpairs = rbtree_create(rbtree_strcasecmp, NULL, free, NULL, free);
  substr = line;
  while(regexec_or_die("key/value", &(parser->re.kv_pair), substr, 3, matches, 0)) {
    key = regex_str(matches+1, substr);
    value = regex_str(matches+2, substr);
    rbtree_put(kvpairs, key, value);
    substr = substr+(matches[2].rm_eo);
  }
  if (!regexec_or_die("whitespace", &(parser->re.whitespace), substr, 0, matches, 0)) {
    html_error(parser, "Couldn't parse \"%s\" as key = value pairs.\n", substr);
  }
  return kvpairs;
}

/*****************************************************************************
 * Convert or test an alphabet parameter of a PSSM or PSPM
 ****************************************************************************/
static void set_alphabet_from_params(CTX_T *parser, RBTREE_T *kv_pairs, 
    const char *type, const char *name, const char *alpha_key) {
  RBNODE_T *node;
  char *value, *end;
  int alen;

  if ((node = rbtree_find(kv_pairs, alpha_key)) != NULL) {
    value = (char*)rbtree_value(node);
    errno = 0;
    alen = strtol(value, &end, 10);
    if (errno != 0 || *end != '\0') {
      html_error(parser, "The %s of motif %s has an alphabet length value \"%s\" "
          "which is not an integer.\n", type, name, value);
    } else {
      if (parser->fscope.alphabet != INVALID_ALPH) {
        if (alen != alph_size(parser->fscope.alphabet, ALPH_SIZE)) {
          html_error(parser, "The %s of motif %s has an alphabet length value %d "
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
            html_error(parser, "The %s of motif %s has an alphabet length value %d "
                "which does not match the lengths of any supported alphabets.\n", 
                type, name, alen);
        }
      }
    }
  }
}

/*****************************************************************************
 * Convert or test a width parameter from a PSSM or PSPM
 ****************************************************************************/
static void set_motif_width_from_params(CTX_T *parser, RBTREE_T *kv_pairs, 
    const char *type, const char *name, const char *width_key) {
  RBNODE_T *node;
  char *value, *end;
  int width;
  if ((node = rbtree_find(kv_pairs, width_key)) != NULL) {
    value = (char*)rbtree_value(node);
    errno = 0;
    width = strtol(value, &end, 10);
    if (errno != 0 || *end != '\0') {
      html_error(parser, "The %s of motif %s has an width value \"%s\" "
          "which is not an integer.\n", type, name, value);
    } else {
      if (parser->mscope.has_width) {
        if (width != get_motif_length(parser->mscope.motif)) {
          html_error(parser, "The %s of motif %s has a width value %d "
              "which does not match the existing width value of %d.\n",
              type, name, width, get_motif_length(parser->mscope.motif));
        }
      } else {
        if (width <= 0) {
          html_error(parser, "The %s of motif %s has a width value %d "
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

/*****************************************************************************
 * Convert or test a sites parameter from a PSSM or PSPM
 ****************************************************************************/
static void set_motif_sites_from_params(CTX_T *parser, RBTREE_T *kv_pairs, 
    const char *type, const char *name, const char *sites_key) {
  RBNODE_T *node;
  char *value, *end;
  double sites;
  if ((node = rbtree_find(kv_pairs, sites_key)) != NULL) {
    value = (char*)rbtree_value(node);
    errno = 0;
    sites = strtod(value, &end);
    if (*end != '\0') {
      html_error(parser, "The %s of motif %s has a number of sites value \"%s\" "
          "which is not a number.\n", type, name, value);
    } else if (errno == ERANGE) {
      if (sites == 0) {
        html_error(parser, "The %s of motif %s has a number of sites value \"%s\" "
            "which is too small to represent.\n", type, name, value);
      } else {
        html_error(parser, "The %s of motif %s has a number of sites value \"%s\" "
            "which is too large to represent.\n", type, name, value);
      }
    } else {
      if (parser->mscope.has_sites) {
        if (sites != get_motif_nsites(parser->mscope.motif)) {
          html_error(parser, "The %s of motif %s has a number of sites value %g "
              "which does not match the existing value of %g.\n", type, name, 
              sites, get_motif_nsites(parser->mscope.motif));
        }
      } else {
        if (sites < 0) {
          html_error(parser, "The %s of motif %s has a number of sites value %g "
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

/*****************************************************************************
 * Convert or test an evalue parameter from a PSSM or PSPM
 ****************************************************************************/
static void set_motif_evalue_from_params(CTX_T *parser, RBTREE_T *kv_pairs, 
    const char *type, const char *name, const char *evalue_key) {
  RBNODE_T *node;
  char *value, *end;
  double log10_evalue;
  if ((node = rbtree_find(kv_pairs, evalue_key)) != NULL) {
    value = (char*)rbtree_value(node);
    errno = 0;
    log10_evalue = log10_evalue_from_string(value);
    if (errno == EINVAL) {
      html_error(parser, "The %s of motif %s has an evalue value \"%s\" "
          "which is not a number.\n", type, name, value);
    } else {
      if (parser->mscope.has_evalue) {
        if (log10_evalue != get_motif_log_evalue(parser->mscope.motif)) {
          html_error(parser, "The %s of motif %s has an evalue value %g "
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

/*****************************************************************************
 * Convert or test various parameters from a PSSM or PSPM
 ****************************************************************************/
static void set_motif_params(CTX_T *parser, RBTREE_T *kv_pairs, 
    const char *type, const char *alpha_key, const char *width_key, 
    const char *sites_key, const char *evalue_key) {
  char *name;
  name = get_motif_id(parser->mscope.motif);
  set_alphabet_from_params(parser, kv_pairs, type, name, alpha_key);
  set_motif_width_from_params(parser, kv_pairs, type, name, width_key);
  set_motif_sites_from_params(parser, kv_pairs, type, name, sites_key);
  set_motif_evalue_from_params(parser, kv_pairs, type, name, evalue_key);
}

/* }}} */

/* Callbacks for all motifs {{{ */

/*****************************************************************************
 * Read the version.
 ****************************************************************************/
void meme_version(CTX_T *ctx, const char *version) {
  regmatch_t matches[6];
  // execute regular expression
  if (regexec_or_die("version", &(ctx->re.version), version, 6, matches, 0)) {
    // match!
    ctx->fscope.vmajor = regex_int(matches+1, version, 0);
    ctx->fscope.vminor = regex_int(matches+3, version, 0);
    ctx->fscope.vpatch = regex_int(matches+5, version, 0);
    if (ctx->fscope.vmajor < 4 || 
        (ctx->fscope.vmajor == 4 && ctx->fscope.vminor < 3) ||
        (ctx->fscope.vmajor == 4 && ctx->fscope.vminor == 3 && 
         ctx->fscope.vmajor < 2)) {
      html_error(ctx, "MEME version is too old to be parsed by this parser.\n");
    } else {
      ctx->format_match = 3;
    }
  } else {
    html_error(ctx, "Could not parse version string.\n");
  }
}

/*****************************************************************************
 * Read the alphabet.
 ****************************************************************************/
void meme_alphabet(CTX_T *ctx, const char *alphabet) {
  if ((ctx->fscope.alphabet = alph_type(alphabet, 30)) == INVALID_ALPH) {
    html_error(ctx, "Unknown alphabet string %s\n", alphabet);
  }
}

/*****************************************************************************
 * Read the type of the strands.
 ****************************************************************************/
void meme_strands(CTX_T *ctx, const char *strands) {
  if (strcasecmp(strands, "both") == 0 || strcmp(strands, "+ -") == 0) {
    ctx->fscope.strands = 2;
  } else if (strcasecmp(strands, "forward") == 0 || strcmp(strands, "+") == 0) {
    ctx->fscope.strands = 1;
  } else if (strcasecmp(strands, "none") == 0) { // not applicable for DREME
    ctx->fscope.strands = 0;
  } else {
    html_error(ctx, "Could not parse strands string.\n");
  }
}

/*****************************************************************************
 * Read the background frequencies. TODO
 ****************************************************************************/
void meme_bgfreq(CTX_T *ctx, const char *bgfreqs) {
  regmatch_t matches[4];
  const char *substr;
  char letter;
  double prob, sum, delta;
  ALPH_T *alph;
  ARRAY_T *bg;
  int i;

  if (ctx->fscope.background) {
    html_error(ctx, "Background field found but background is already set!");
    return;
  }
  alph = &(ctx->fscope.alphabet);
  bg = NULL;
  substr = bgfreqs;
  i = 0;
  sum = 0;
  while (regexec_or_die("bg letter", &(ctx->re.bg_letter), substr, 4, matches, 0)) {
    letter = regex_chr(matches+2, substr);
    if (!alph_test(alph, i, letter)) {
      html_error(ctx, "The background frequency letter %c at position %d is "
          "invalid for the %s alphabet.\n", letter, i + 1, alph_name(*alph));
      goto cleanup;
    }
    prob = regex_dbl(matches+3, substr);
    // test for probability
    if (prob < 0 || prob > 1) {
      html_error(ctx, "The background frequency %f associated with the "
          "letter %c at position %d is not valid as it is not in the "
          "range 0 to 1.\n", prob, letter, i + 1);
      goto cleanup;
    }
    sum += prob;
    // expand array if required
    if (*alph != INVALID_ALPH) {
      if (bg == NULL || get_array_length(bg) != alph_size(*alph, ALPH_SIZE)) {
        bg = resize_array(bg, alph_size(*alph, ALPH_SIZE));
      }
    } else {
      bg = resize_array(bg, i + 1);
    }
    assert(i < get_array_length(bg));
    // store the probability
    set_array_item(i, prob, bg);
    // move the search forward
    substr += matches[1].rm_eo;
    i++;
  }
  // test for whitespace
  if (!regexec_or_die("whitespace", &(ctx->re.whitespace), substr, 0, matches, 0)) {
    html_error(ctx, "Expected only space after the background letter "
        "frequencies but found \"%s\".\n", substr);
    goto cleanup;
  }
  // test for element count
  if (i != alph_size(*alph, ALPH_SIZE)) {
    html_error(ctx, "The background does not have entries for all the "
        "letters in the %s alphabet.\n", alph_name(*alph));
    goto cleanup;
  }
  // test for summing to 1
  delta = sum - 1;
  if (delta < 0) delta = -delta;
  if (delta > 0.1) {
    html_error(ctx, "The background frequencies do not sum to 1 but %g (delta = %g)\n", sum, delta);
    goto cleanup;
  }

  if (ctx->format_match < 4) ctx->format_match = 4;
  ctx->fscope.background = bg;
  return;
cleanup:
  if (bg) free_array(bg);
}

/*****************************************************************************
 * Read the name of the file.
 ****************************************************************************/
void meme_name(CTX_T *ctx, const char *name) {
}

/*****************************************************************************
 * Read the combined scanned sites.
 ****************************************************************************/
void meme_combinedblock(CTX_T *ctx, const char *combinedblock) {
  regmatch_t matches[8];
  const char *scanned_seq, *scanned_sites;
  char *seq_name, site_strand;
  double seq_log10_pvalue, site_log10_pvalue;
  int i, seq_length, seq_count, site_motif_num, site_position;
  SCANNED_SEQ_T *sseq;
  BOOLEAN_T sites_ok;

  if (ctx->fscope.scanned_sites) {
    html_error(ctx, "Duplicate scanned sites.\n");
    return;
  }

  if (ctx->options & SCANNED_SITES) {
    ctx->fscope.scanned_sites = arraylst_create();
  }

  sites_ok = TRUE;
  scanned_seq = combinedblock;
  while (regexec_or_die("scanned seq", &(ctx->re.scanned_seq), scanned_seq, 8, matches, 0)) {
    seq_name = regex_str(matches+2, scanned_seq);
    seq_log10_pvalue = regex_log10_dbl(matches+3, scanned_seq);
    seq_count = regex_int(matches+5, scanned_seq, 0);
    seq_length = regex_int(matches+6, scanned_seq, 0);
    sseq = NULL;
    if (ctx->options & SCANNED_SITES) {
      // make scanned sequence record
      sseq = sseq_create(seq_name, seq_length, seq_log10_pvalue, seq_count);
    }
    //must do this before we overwrite the matches
    scanned_sites = scanned_seq+(matches[1].rm_eo);
    scanned_seq = scanned_seq+(matches[0].rm_eo);
    for (i = 0; i < seq_count; ++i) {
      // match each strand, motif, position and pvalue for each site
      if (regexec_or_die("", &(ctx->re.scanned_site), scanned_sites, 7, matches, 0)) {
        site_strand = regex_chr(matches+1, scanned_sites);
        site_motif_num = regex_int(matches+2, scanned_sites, 0); //convert to index
        site_position = regex_int(matches+3, scanned_sites, 0);
        site_log10_pvalue = regex_log10_dbl(matches+4, scanned_sites);
        // set scanned site record
        if (ctx->options & SCANNED_SITES) {
          sseq_set(sseq, i, site_motif_num, site_strand, site_position, site_log10_pvalue);
        }
        scanned_sites += matches[4].rm_eo;
      } else { // site missing!
        html_error(ctx, "Too few scanned sequence sites for sequence %s. "
            "Expected %d but only found %d.\n", seq_name, seq_count, i);
        sites_ok = FALSE;
        break;
      }
    }
    if (ctx->options & SCANNED_SITES) {
      arraylst_add(sseq, ctx->fscope.scanned_sites);
    }
    free(seq_name);
  }
  if (sites_ok) {
    ctx->fscope.options_found |= SCANNED_SITES;
  }
}

/*****************************************************************************
 * Read the number of motifs.
 ****************************************************************************/
void meme_nmotifs(CTX_T *ctx, const char *nmotifs) {
  char *endptr;
  ctx->fscope.nmotifs = strtol(nmotifs, &endptr, 10);
  if (*endptr != '\0' || nmotifs == endptr) {
    html_error(ctx, "Failed to correctly parse nmotifs \"%s\".\n", nmotifs);
  }
}

/* }}} */

/* Callbacks for specific motifs {{{ */
/*****************************************************************************
 * Read the motif intro.
 ****************************************************************************/
void meme_motif_intro(CTX_T *ctx, const char *intro) {
  regmatch_t matches[4];

  if (regexec_or_die("motif intro", &(ctx->re.motif_name), intro, 4, matches, 0)) {
    set_motif_id(intro+(matches[2].rm_so), 
        matches[2].rm_eo - matches[2].rm_so, ctx->mscope.motif);
    set_motif_id2("", 0, ctx->mscope.motif);
    set_motif_strand('+', ctx->mscope.motif);
    ctx->mscope.has_name = TRUE;
  } else {
    html_error(ctx, "Could not parse motif intro \"%s\".\n", intro);
  }
}

/*****************************************************************************
 * Read the motif intro.
 ****************************************************************************/
void meme_motif_name(CTX_T *ctx, const char *motif_name) {
  set_motif_id(motif_name, strlen(motif_name), ctx->mscope.motif);
  set_motif_id2("", 0, ctx->mscope.motif);
  set_motif_strand('+', ctx->mscope.motif);
  ctx->mscope.has_name = TRUE;
}


/*****************************************************************************
 * Process the motif's position specific score matrix.
 ****************************************************************************/
void meme_motif_pssm(CTX_T *ctx, const char *pssm) {
  regmatch_t matches[2];
  RBTREE_T *kvpairs;
  char *name, *parameters;

  name = (ctx->mscope.has_name ? get_motif_id(ctx->mscope.motif) : "<name not set>");

  if (ctx->mscope.has_pssm) {
    html_error(ctx, "Duplicate PSSM of motif %s\n", name);
    return;
  }
  ctx->mscope.has_pssm = TRUE;
   
  if (!regexec_or_die("PSSM", &(ctx->re.log_odds), pssm, 2, matches, 0)) {
    html_error(ctx, "Couldn't parse PSSM of motif %s.\n", name);
    return;
  }

  parameters = regex_str(matches+1, pssm);
  kvpairs = parse_keyvals(ctx, parameters);
  free(parameters);
  set_motif_params(ctx, kvpairs, "pssm", "alength", "w", "nsites", "E");
  rbtree_destroy(kvpairs);

  read_grid(ctx, FALSE, pssm+(matches[1].rm_eo));
}

/*****************************************************************************
 * Process the motif's position specific probability matrix.
 ****************************************************************************/
void meme_motif_pspm(CTX_T *ctx, const char *pspm) {
  regmatch_t matches[2];
  RBTREE_T *kvpairs;
  char *name, *parameters;

  name = (ctx->mscope.has_name ? get_motif_id(ctx->mscope.motif) : "<name not set>");

  if (ctx->mscope.has_pspm) {
    html_error(ctx, "Duplicate PSPM of motif %s\n", name);
    return;
  }
  ctx->mscope.has_pspm = TRUE;
   
  if (!regexec_or_die("PSPM", &(ctx->re.letter_prob), pspm, 2, matches, 0)) {
    html_error(ctx, "Couldn't parse PSPM of motif %s.\n", name);
    return;
  }

  parameters = regex_str(matches+1, pspm);
  kvpairs = parse_keyvals(ctx, parameters);
  free(parameters);
  set_motif_params(ctx, kvpairs, "pssm", "alength", "w", "nsites", "E");
  rbtree_destroy(kvpairs);

  read_grid(ctx, TRUE, pspm+(matches[1].rm_eo));
}

/*****************************************************************************
 * Process the motif's blocks (the sequence sites that went to making the
 * motif).
 ****************************************************************************/
void meme_motif_blocks(CTX_T *ctx, const char *blocks) {
}
/* }}} */

/* Current motif utility methods {{{ */

/*****************************************************************************
 * Add the current motif to the completed queue and remove it from editing
 ****************************************************************************/
void enqueue_motif(CTX_T *ctx) {
  struct mscope *mdata;
  if (ctx->mscope.motif) {
    ctx->mscope.motif->alph = ctx->fscope.alphabet;
    ctx->mscope.motif->flags = (ctx->fscope.strands == 2 ? MOTIF_BOTH_STRANDS : 0);
    mdata = mm_malloc(sizeof(struct mscope));
    *mdata = ctx->mscope;
    linklst_add(mdata, ctx->scopes);
  }
  memset(&(ctx->mscope), 0, sizeof(struct mscope));
}

/*****************************************************************************
 * Allocate a motif structure for the current motif.
 ****************************************************************************/
void init_motif(CTX_T *ctx, int num) {
  ctx->mscope.motif = (MOTIF_T*)mm_malloc(sizeof(MOTIF_T));
  memset(ctx->mscope.motif, 0, sizeof(MOTIF_T));
  ctx->mscope.motif->complexity = -1;
  ctx->mscope.motif->url = strdup("");
  ctx->current_num = num;
}
/* }}} */

/* Callbacks for HTML {{{ */

/*****************************************************************************
 * Check the name of the hidden input field and passes the value to the
 * approprate function. If the name contains additional information like
 * the motif number it selects the correct motif structure to pass as well.
 ****************************************************************************/
void hidden_input(HDATA_T *info, void *data, 
    const char *name, long name_overflow, 
    const char *value, long value_overflow) {
  CTX_T *ctx;
  regmatch_t matches[3];
  int num;
  ctx = (CTX_T*)data;
  if (strcasecmp(name, "version") == 0) {
    meme_version(ctx, value);
  } else if (strcasecmp(name, "alphabet") == 0) {
    meme_alphabet(ctx, value);
  } else if (strcasecmp(name, "strands") == 0) {
    meme_strands(ctx, value);
  } else if (strcasecmp(name, "bgfreq") == 0) {
    meme_bgfreq(ctx, value);
  } else if (strcasecmp(name, "name") == 0) {
    meme_name(ctx, value);
  } else if (strcasecmp(name, "combinedblock") == 0) {
    meme_combinedblock(ctx, value);
  } else if (strcasecmp(name, "nmotifs") == 0) {
    meme_nmotifs(ctx, value);
  } else if (regexec_or_die("hidden field name", &(ctx->re.hf_name), name, 3, matches, 0)) {
    num = regex_int(matches+2, name, 0);
    if (num != ctx->current_num) {
      if (ctx->mscope.motif) {
        //TODO check complete
        enqueue_motif(ctx);
      }
      init_motif(ctx, num);
    }
    if (ctx->mscope.motif) {
      if (regex_casecmp(matches+1, name, "motifname") == 0) {
        meme_motif_name(ctx, value);
      } else if (regex_casecmp(matches+1, name, "motifblock") == 0) {
        meme_motif_intro(ctx, value);
      } else if (regex_casecmp(matches+1, name, "pssm") == 0) {
        meme_motif_pssm(ctx, value);
      } else if (regex_casecmp(matches+1, name, "pspm") == 0) {
        meme_motif_pspm(ctx, value);
      } else if (regex_casecmp(matches+1, name, "BLOCKS") == 0) {
        meme_motif_blocks(ctx, value);
      } 
    }
  }
}



/* }}} */


void* mhtml_create(const char *optional_filename, int options) {
  MHTML_T *parser;
  parser = mm_malloc(sizeof(MHTML_T));
  memset(parser, 0, sizeof(MHTML_T));
  parser->data = create_parser_data(options, optional_filename);
  parser->ctxt = hdata_create(hidden_input, parser->data, 1000000);
  return parser;
}

void mhtml_destroy(void *data) {
  MHTML_T *parser;
  parser = (MHTML_T*)data;
  hdata_destroy(parser->ctxt);
  destroy_parser_data(parser->data);
  memset(parser, 0, sizeof(MHTML_T));
  free(parser);
}

void mhtml_update(void *data, const char *chunk, size_t size, short end) {
  MHTML_T *parser;
  parser = (MHTML_T*)data;
  hdata_update(parser->ctxt, chunk, size, end);
  if (end && parser->data->mscope.motif) {
    //TODO check complete
    enqueue_motif(parser->data);
  }
}

short mhtml_has_format_match(void *data) {
  MHTML_T *parser;
  parser = (MHTML_T*)data;
  return parser->data->format_match;
}

short mhtml_has_error(void *data) {
  MHTML_T *parser;
  parser = (MHTML_T*)data;
  return linklst_size(parser->data->errors) > 0;
}

char* mhtml_next_error(void *data) {
  MHTML_T *parser;
  parser = (MHTML_T*)data;
  return linklst_pop(parser->data->errors);
}

short mhtml_has_motif(void *data) {
  MHTML_T *parser;
  parser = (MHTML_T*)data;
  return linklst_size(parser->data->scopes) > 0;
}

MOTIF_T* mhtml_next_motif(void *data) {
  MHTML_T *parser;
  parser = (MHTML_T*)data;
  if (parser->data->out) {
    parser->data->out->motif = NULL;
    destroy_motif_scope(parser->data->out);
  }
  parser->data->out = (struct mscope*)linklst_pop(parser->data->scopes);
  return parser->data->out->motif;
}

ALPH_T mhtml_get_alphabet(void *data) {
  MHTML_T *parser;
  parser = (MHTML_T*)data;
  return parser->data->fscope.alphabet;
}

int mhtml_get_strands(void *data) {
  MHTML_T *parser;
  parser = (MHTML_T*)data;
  return parser->data->fscope.strands;
}

BOOLEAN_T mhtml_get_bg(void *data, ARRAY_T **bg) {
  MHTML_T *parser;
  parser = (MHTML_T*)data;
  if (parser->data->fscope.background == NULL) return FALSE;
  *bg = resize_array(*bg, get_array_length(parser->data->fscope.background));
  copy_array(parser->data->fscope.background, *bg);
  return TRUE;
}

void* mhtml_motif_optional(void *data, int option) {
  MHTML_T *parser;
  parser = (MHTML_T*)data;
  if (!(parser->data->options & option)) {
    die("Requested value of optional component which was not requested "
        "during construction.\n");
    return NULL;
  }
  if (parser->data->out->options_found & option) {
    if (parser->data->out->options_returned & option) {
      die("Sorry, optional values are only returned once. "
          "This is because we can not guarantee that the "
          "previous caller did not deallocate the memory. "
          "Hence this is a feature to avoid memory bugs.\n");
      return NULL;
    }
    parser->data->out->options_returned |= option;
  } else {
    // Not yet found or unsupported
    return NULL;
  }
  switch (option) {
    default:
      die("Option code %d does not match any single option. "
          "This means that it must contain multiple options "
          "and only one is allowed at a time\n.", option);
  }
  return NULL; // unreachable! make compiler happy
}

void* mhtml_file_optional(void *data, int option) {
  MHTML_T *parser;
  parser = (MHTML_T*)data;
  if (!(parser->data->options & option)) {
    die("Requested value of optional component which was not requested "
        "during construction.\n");
    return NULL;
  }
  if (parser->data->fscope.options_found & option) {
    if (parser->data->fscope.options_returned & option) {
      die("Sorry, optional values are only returned once. "
          "This is because we can not guarantee that the "
          "previous caller did not deallocate the memory. "
          "Hence this is a feature to avoid memory bugs.\n");
      return NULL;
    }
    parser->data->fscope.options_returned |= option;
  } else {
    // Not yet found or unsupported
    return NULL;
  }
  switch (option) {
    case SCANNED_SITES:
      return parser->data->fscope.scanned_sites;
    default:
      die("Option code %d does not match any single option. "
          "This means that it must contain multiple options "
          "and only one is allowed at a time\n.", option);
  }
  return NULL; //unreachable! make compiler happy
}
   
