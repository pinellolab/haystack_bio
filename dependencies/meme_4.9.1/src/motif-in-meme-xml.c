
#include <math.h>
#include <limits.h>

#define NO_SCORE 1000000000

#include "array-list.h"
#include "motif-in-meme-xml.h"
#include "motif-spec.h"
#include "meme-sax.h"
#include "linked-list.h"
#include "red-black-tree.h"
#include "string-match.h"
#include "utils.h"

struct fscope {
  int options_found;
  int options_returned;
  int vmajor;
  int vminor;
  int vpatch;
  int strands;
  char *release;
  ARRAY_T *background;
  ARRAYLST_T *scanned_sites;
};

struct mscope {
  int options_found;
  int options_returned;
  MOTIF_T *motif;
};

typedef struct meme_ctx CTX_T;
struct meme_ctx {
  char *filename;
  int options;
  struct fscope fscope;
  struct mscope mscope;
  int format_match;
  LINKLST_T *errors;
  LINKLST_T *motif_queue;
  ALPH_T alph;
  RBTREE_T *letter_lookup;
  RBTREE_T *sequence_lookup;
  RBTREE_T *motif_lookup;
  int current_site;
  int current_pos;
  int current_motif;
};

typedef struct mxml MXML_T;
struct mxml {
  CTX_T *data;
  MEME_IO_XML_CALLBACKS_T *callbacks;
  void *sax_context;
  xmlSAXHandler *handler;
  xmlParserCtxt *ctxt;
};

typedef struct letter LETTER_T;
struct letter {
  int order;
  char symbol;
  double freq;
  double score;
  double prob;
};

struct seqinfo {
  int length;
  char *name;
};

/*****************************************************************************
 * Create the datastructure for storing alphabet letters
 ****************************************************************************/
static LETTER_T* letter_create(int order) {
  LETTER_T *letter;

  letter = mm_malloc(sizeof(LETTER_T));
  memset(letter, 0, sizeof(LETTER_T));
  letter->order = order;
  letter->freq = -1;
  letter->symbol = '\0';
  return letter;
}

/*****************************************************************************
 * Create a seqinfo structure to keep track of the name and length of a 
 * sequence. Both are needed for the scanned sites output
 ****************************************************************************/
struct seqinfo* create_seqinfo(char *name, int length) {
  struct seqinfo *seq;

  seq = (struct seqinfo *)mm_malloc(sizeof(struct seqinfo));
  seq->name = strdup(name);
  seq->length = length;
  return seq;
}

/*****************************************************************************
 * Destroy a seqinfo structure.
 ****************************************************************************/
void destroy_seqinfo(void *p) {
  struct seqinfo *seq;
  seq = (struct seqinfo *)p;
  free(seq->name);
  memset(seq, 0, sizeof(struct seqinfo));
  free(seq);
}

/*****************************************************************************
 * Create the datastructure for storing motifs while their content is
 * still being parsed.
 ****************************************************************************/
static CTX_T* create_parser_data(int options, const char *optional_file_name) {
  CTX_T *data;
  data = (CTX_T*)mm_malloc(sizeof(CTX_T));
  memset(data, 0, sizeof(CTX_T));
  data->format_match = file_name_match("meme", "xml", optional_file_name);
  data->errors = linklst_create();
  data->motif_queue = linklst_create();
  data->options = options;
  data->letter_lookup = rbtree_create(rbtree_strcmp, rbtree_strcpy, free, NULL, free);
  if (options & SCANNED_SITES) {
    data->sequence_lookup = rbtree_create(rbtree_strcmp, rbtree_strcpy, free, NULL, destroy_seqinfo);
    data->motif_lookup = rbtree_create(rbtree_strcmp, rbtree_strcpy, free, rbtree_intcpy, free);
  }  
  return data;
}

/*****************************************************************************
 * Destroy the parser along with any motifs which weren't retrieved.
 ****************************************************************************/
static void destroy_parser_data(CTX_T *data) {
  if (data->fscope.scanned_sites) {
    if (!(data->fscope.options_returned & SCANNED_SITES)) {
      arraylst_destroy(sseq_destroy, data->fscope.scanned_sites);
    }
  }
  if (data->fscope.background) {
    free_array(data->fscope.background);
  }
  if (data->fscope.release) {
    free(data->fscope.release);
  }
  if (data->sequence_lookup) rbtree_destroy(data->sequence_lookup);
  linklst_destroy_all(data->errors, free);
  linklst_destroy_all(data->motif_queue, destroy_motif);
  rbtree_destroy(data->letter_lookup);
  memset(data, 0, sizeof(CTX_T));
  free(data);
}

/*****************************************************************************
 * Convert an error format and arguments into a string and store it
 * in the error list.
 ****************************************************************************/
void mxml_error(void *ctx, char *format, va_list ap) {
  CTX_T *data = (CTX_T*)ctx;
  int len;
  char dummy[1], *msg;
  va_list ap_copy;
  va_copy(ap_copy, ap);
  len = vsnprintf(dummy, 1, format, ap_copy);
  va_end(ap_copy);
  msg = mm_malloc(sizeof(char) * (len + 1));
  vsnprintf(msg, len + 1, format, ap);
  linklst_add(msg, data->errors);
}

/*****************************************************************************
 * As the other error function takes va_list this is a simply to
 * enable calling the real error function
 ****************************************************************************/
static void local_error(CTX_T *data, char *format, ...) {
  va_list  argp;
  va_start(argp, format);
  mxml_error(data, format, argp);
  va_end(argp);
}

/*****************************************************************************
 * MEME
 * Copy the version and release date of the meme file. 
 * At this point I'm not sure if this information will be used.
 ****************************************************************************/
void mxml_start_meme(void *ctx, int major, int minor, int patch, char *release) {
  CTX_T *data = (CTX_T*)ctx;
  data->format_match = 3;
  data->fscope.vmajor = major;
  data->fscope.vminor = minor;
  data->fscope.vpatch = patch;
  data->fscope.release = strdup(release);
}

/*****************************************************************************
 * MEME > alphabet
 * Read in the  alphabet type and setup the letter lookup.
 ****************************************************************************/
void mxml_start_alphabet(void *ctx, int is_nucleotide, int alength) {
  CTX_T *data = (CTX_T*)ctx;
  data->alph = (is_nucleotide ? DNA_ALPH : PROTEIN_ALPH);
  switch (data->alph) {
    case DNA_ALPH:
      rbtree_put(data->letter_lookup, "letter_A", letter_create(0));
      rbtree_put(data->letter_lookup, "letter_C", letter_create(1));
      rbtree_put(data->letter_lookup, "letter_G", letter_create(2));
      rbtree_put(data->letter_lookup, "letter_T", letter_create(3));
      break;
    case PROTEIN_ALPH:
      rbtree_put(data->letter_lookup, "letter_A", letter_create(0));
      rbtree_put(data->letter_lookup, "letter_C", letter_create(1));
      rbtree_put(data->letter_lookup, "letter_D", letter_create(2));
      rbtree_put(data->letter_lookup, "letter_E", letter_create(3));
      rbtree_put(data->letter_lookup, "letter_F", letter_create(4));
      rbtree_put(data->letter_lookup, "letter_G", letter_create(5));
      rbtree_put(data->letter_lookup, "letter_H", letter_create(6));
      rbtree_put(data->letter_lookup, "letter_I", letter_create(7));
      rbtree_put(data->letter_lookup, "letter_K", letter_create(8));
      rbtree_put(data->letter_lookup, "letter_L", letter_create(9));
      rbtree_put(data->letter_lookup, "letter_M", letter_create(10));
      rbtree_put(data->letter_lookup, "letter_N", letter_create(11));
      rbtree_put(data->letter_lookup, "letter_P", letter_create(12));
      rbtree_put(data->letter_lookup, "letter_Q", letter_create(13));
      rbtree_put(data->letter_lookup, "letter_R", letter_create(14));
      rbtree_put(data->letter_lookup, "letter_S", letter_create(15));
      rbtree_put(data->letter_lookup, "letter_T", letter_create(16));
      rbtree_put(data->letter_lookup, "letter_V", letter_create(17));
      rbtree_put(data->letter_lookup, "letter_W", letter_create(18));
      rbtree_put(data->letter_lookup, "letter_Y", letter_create(19));
      break;
    default:
      die("Impossible state");
  }
}

/*****************************************************************************
 * MEME > alphabet > letter
 * Read in a identifier and symbol pair for a letter.
 ****************************************************************************/
void mxml_letter(void *ctx, char *id, char symbol) {
  LETTER_T *letter;
  CTX_T *data;

  data = (CTX_T*)ctx;
  letter = (LETTER_T*)rbtree_get(data->letter_lookup, id);
  if (letter == NULL) { 
    local_error(data, "\"%s\" is not a recognised "
        "letter identifier for the %s alphabet! "
        "The associated symbol can't be stored.\n", id, 
         alph_name(data->alph));
    return;
  }
  letter->symbol = symbol;
}

/*****************************************************************************
 * MEME > alphabet
 * Read in the number of symbols in the alphabet and if it is nucleotide or 
 * amino-acid (RNA is apparently classed as nucleotide).
 ****************************************************************************/
void mxml_end_alphabet(void *ctx) {
  CTX_T *data;
  RBNODE_T *node;
  LETTER_T *letter;
  char *id;

  data = (CTX_T*)ctx;
  for (node = rbtree_first(data->letter_lookup); node != NULL; node = rbtree_next(node)) {
    id = (char*)rbtree_key(node);
    letter = (LETTER_T*)rbtree_value(node);
    if (letter->symbol == '\0') {
      local_error(data, "No alphabet letter entry for \"%s\".\n", id);
    }
  }

}

/*****************************************************************************
 * MEME > model > strands
 ****************************************************************************/
void mxml_handle_strands(void *ctx, int strands) {
  CTX_T *data;

  data = (CTX_T*)ctx;
  data->fscope.strands = strands;
}

/*****************************************************************************
 * MEME > model > background_frequencies > alphabet_array > value
 ****************************************************************************/
void mxml_background_value(void *ctx, char *id, double freq) {
  LETTER_T *letter;
  CTX_T *data;

  data = (CTX_T*)ctx;
  letter = (LETTER_T*)rbtree_get(data->letter_lookup, id);
  if (letter == NULL) { 
    local_error(data, "\"%s\" is not a recognised "
        "letter identifier for the %s alphabet! "
        "The associated background frequency can't be stored.\n", id, 
        alph_name(data->alph));
    return;
  }
  letter->freq = freq;
}

/*****************************************************************************
 * MEME > model > /background_frequencies
 ****************************************************************************/
void mxml_end_background(void *ctx) {
  RBNODE_T *node;
  CTX_T *data;
  ARRAY_T *bg;
  BOOLEAN_T missing;
  char *id;
  LETTER_T *letter;

  data = (CTX_T*)ctx;
  missing = FALSE;
  bg = allocate_array(rbtree_size(data->letter_lookup));
  for (node = rbtree_first(data->letter_lookup); node != NULL; node = rbtree_next(node)) {
    id = rbtree_key(node);
    letter = rbtree_value(node);
    if (letter->freq == -1) {
      local_error(data, "No alphabet letter frequency for \"%s\".\n", id);
      missing = TRUE;
    }
    set_array_item(letter->order, letter->freq, bg);
  }
  if (missing) {
    free_array(bg);
  } else {
    data->fscope.background = bg;
  }
}

/*****************************************************************************
 * MEME > motifs > motif
 * Construct the skeleton of a motif.
 ****************************************************************************/
void mxml_start_motif(void *ctx, char *id, char *name, int width, double sites, 
    double llr, double ic, double re, double bayes_threshold,
    double log10_evalue, double elapsed_time, char * url) {
  CTX_T *data;
  MOTIF_T *motif;
  
  data = (CTX_T*)ctx;
  data->mscope.motif = mm_malloc(sizeof(MOTIF_T));
  motif = data->mscope.motif;
  memset(motif, 0, sizeof(MOTIF_T));
  set_motif_id(name, strlen(name), motif);
  set_motif_id2("", 0, motif);
  set_motif_strand('+', motif);
  motif->length = width;
  motif->num_sites = sites;
  motif->url = strdup(url);
  motif->log_evalue = log10_evalue;
  motif->evalue = exp10(log10_evalue);
  // calculate alphabet size
  motif->alph = data->alph;
  motif->flags = (data->fscope.strands == 2 ? MOTIF_BOTH_STRANDS : 0);
  // allocate matricies
  motif->freqs = allocate_matrix(motif->length, alph_size(motif->alph, ALPH_SIZE));
  motif->scores = allocate_matrix(motif->length, alph_size(motif->alph, ALPH_SIZE));
  // should be set by a post processing method
  motif->complexity = -1;
  motif->trim_left = 0;
  motif->trim_right = 0;
  // cache motif position
  if (data->options & SCANNED_SITES) {
    rbtree_put(data->motif_lookup, id, &(data->current_motif));
  }
}

/*****************************************************************************
 * MEME > motifs > /motif
 * Make the motif available to the user.
 ****************************************************************************/
void mxml_end_motif(void *ctx) {
  CTX_T *data;
  MOTIF_T *motif;

  data = (CTX_T*)ctx;
  motif = data->mscope.motif;
  linklst_add(motif, data->motif_queue);
  data->mscope.motif = NULL;
  data->current_motif++;
}

/*****************************************************************************
 * MEME > motifs > motif > scores
 * Setup the score matrix. Note it does not have ambigous columns as they 
 * are not in the file and I don't know how to go about calculating them.
 ****************************************************************************/
void mxml_start_scores(void *ctx) {
  CTX_T *data;
  
  data = (CTX_T*)ctx;
  data->current_pos = 0;
}

/*****************************************************************************
 * MEME > motifs > motif > scores > alphabet_matrix > alphabet_array
 * Set all the scores to infinity which should be an impossible score under
 * normal circumstances. 
 ****************************************************************************/
void mxml_start_score_pos(void *ctx) {
  CTX_T *data;
  RBNODE_T *node;
  LETTER_T *letter;

  data = (CTX_T*)ctx;
  for (node = rbtree_first(data->letter_lookup); node != NULL; node = rbtree_next(node)) {
    letter = (LETTER_T*)rbtree_value(node);
    letter->score = NO_SCORE;
  }
}

/*****************************************************************************
 * MEME > motifs > motif > scores > alphabet_matrix > alphabet_array > /value
 * Lookup a letter and check it exists and does not have a score. 
 * Set the letter's score to the passed value.
 ****************************************************************************/
void mxml_score_value(void *ctx, char *letter_id, double score) {
  CTX_T *data;
  LETTER_T *letter;

  data = (CTX_T*)ctx;
  letter = (LETTER_T*)rbtree_get(data->letter_lookup, letter_id);
  if (letter == NULL) {
    local_error(data, "Score for unknown letter identifier \"%s\".\n", letter_id);
    return;
  }
  if (letter->score != NO_SCORE) {
    local_error(data, "Score for \"%s\" has already been set.\n", letter_id);
    return;
  }
  letter->score = score;
}

/*****************************************************************************
 * MEME > motifs > motif > scores > alphabet_matrix > /alphabet_array
 * Check that all letters have a score and update the current matrix row.
 ****************************************************************************/
void mxml_end_score_pos(void *ctx) {
  CTX_T *data;
  RBNODE_T *node;
  LETTER_T *letter;
  ARRAY_T *pos;
  char *id;

  data = (CTX_T*)ctx;
  pos = get_matrix_row(data->current_pos, data->mscope.motif->scores);
  for (node = rbtree_first(data->letter_lookup); node != NULL; node = rbtree_next(node)) {
    letter = (LETTER_T*)rbtree_value(node);
    letter = rbtree_value(node);
    id = rbtree_key(node);
    if (letter->score == NO_SCORE) {
      local_error(data, "Missing score for \"%s\".\n", id);
    }
    set_array_item(letter->order, letter->score, pos);
  }
  data->current_pos++;
}

/*****************************************************************************
 * MEME > motifs > motif > probabilities
 * Setup the probability matrix. 
 ****************************************************************************/
void mxml_start_probabilities(void *ctx) {
  CTX_T *data;
  
  data = (CTX_T*)ctx;
  data->current_pos = 0;
}

/*****************************************************************************
 * MEME > motifs > motif > probabilities > alphabet_matrix > alphabet_array
 * Set all the probabilities to -1 to flag them as unset.
 ****************************************************************************/
void mxml_start_probability_pos(void *ctx) {
  CTX_T *data;
  RBNODE_T *node;
  LETTER_T *letter;

  data = (CTX_T*)ctx;
  for (node = rbtree_first(data->letter_lookup); node != NULL; node = rbtree_next(node)) {
    letter = (LETTER_T*)rbtree_value(node);
    letter->prob = -1;
  }
}

/*****************************************************************************
 * MEME > motifs > motif > probabilities > alphabet_matrix > alphabet_array > /value
 * Lookup a letter and check it exists and does not have a probability. 
 * Set the letter's score to the passed value.
 ****************************************************************************/
void mxml_probability_value(void *ctx, char *letter_id, double probability) {
  CTX_T *data;
  LETTER_T *letter;

  data = (CTX_T*)ctx;
  letter = (LETTER_T*)rbtree_get(data->letter_lookup, letter_id);
  if (letter == NULL) {
    local_error(data, "Probability for unknown letter identifier \"%s\".\n", letter_id);
    return;
  }
  if (letter->prob != -1) {
    local_error(data, "Probability for \"%s\" has already been set.\n", letter_id);
    return;
  }
  letter->prob = probability;
}

/*****************************************************************************
 * MEME > motifs > motif > probabilities > alphabet_matrix > /alphabet_array
 * Check that all letters have a probability and update the current matrix row.
 ****************************************************************************/
void mxml_end_probability_pos(void *ctx) {
  CTX_T *data;
  RBNODE_T *node;
  LETTER_T *letter;
  ARRAY_T *pos;
  char *id;

  data = (CTX_T*)ctx;
  pos = get_matrix_row(data->current_pos, data->mscope.motif->freqs);
  for (node = rbtree_first(data->letter_lookup); node != NULL; node = rbtree_next(node)) {
    letter = (LETTER_T*)rbtree_value(node);
    letter = rbtree_value(node);
    id = rbtree_key(node);
    if (letter->prob == -1) {
      local_error(data, "Missing probability for \"%s\".\n", id);
    }
    set_array_item(letter->order, letter->prob, pos);
  }
  data->current_pos++;
}

/*****************************************************************************
 * MEME > training_set > sequence
 ****************************************************************************/
void mxml_sequence(void *ctx, char *id, char *name, int length, double weight) {
  CTX_T *data;

  data = (CTX_T*)ctx;
  if (data->options & SCANNED_SITES) {
    rbtree_put(data->sequence_lookup, id, create_seqinfo(name, length));
  }
}

/*****************************************************************************
 * MEME > scanned_sites_summary
 ****************************************************************************/
void mxml_start_scanned_sites(void *ctx, double pv_threshold) {
  CTX_T *data;

  data = (CTX_T*)ctx;
  if (data->options & SCANNED_SITES) {
    data->fscope.scanned_sites = arraylst_create();
  }
}

/*****************************************************************************
 * MEME > /scanned_sites_summary
 ****************************************************************************/
void mxml_end_scanned_sites(void *ctx) {
  CTX_T *data;
  data = (CTX_T*)ctx;
  if (data->options & SCANNED_SITES) {
    data->fscope.options_found |= SCANNED_SITES;
  }
}

/*****************************************************************************
 * MEME > scanned_sites_summary > scanned_sites
 ****************************************************************************/
void mxml_start_scanned_seq(void *ctx, char *seq_id, double log10pvalue, int site_count) {
  CTX_T *data;
  int *length;
  struct seqinfo *seq;
  data = (CTX_T*)ctx;
  if (data->options & SCANNED_SITES) {
    data->current_site = 0;
    seq = (struct seqinfo *)rbtree_get(data->sequence_lookup, seq_id);
    if (seq == NULL) {
      local_error(data, "Scanned sites references unknown sequence \"%s\".\n", seq_id);
      return;
    }
    arraylst_add(sseq_create(seq->name, seq->length, log10pvalue, site_count), data->fscope.scanned_sites);
  }
}

/*****************************************************************************
 * MEME > scanned_sites_summary > scanned_sites > scanned_site
 ****************************************************************************/
void mxml_scanned_site(void *ctx, char *motif_id, char strand, int position, double log10pvalue) {
  CTX_T *data;
  SCANNED_SEQ_T *sseq;
  int *mindex;
  data = (CTX_T*)ctx;
  if (data->options & SCANNED_SITES) {
    sseq = (SCANNED_SEQ_T*)arraylst_get(
        arraylst_size(data->fscope.scanned_sites)-1, data->fscope.scanned_sites);
    mindex = (int*)rbtree_get(data->motif_lookup, motif_id);
    if (mindex == NULL) {
      local_error(data, "Scanned site references unknown motif \"%s\".\n", motif_id);
      return;
    }
    sseq_set(sseq, data->current_site++, (*mindex) + 1, strand, position, log10pvalue);
  }
}

/*****************************************************************************
 * Create a parser for MEME's XML format.
 ****************************************************************************/
void* mxml_create(const char *optional_filename, int options) {
  MXML_T *parser;
  // allocate the structure to hold the parser information
  parser = (MXML_T*)mm_malloc(sizeof(MXML_T));
  // setup data store
  parser->data = create_parser_data(options, optional_filename);
  // setup meme xml callbacks
  parser->callbacks = (MEME_IO_XML_CALLBACKS_T*)mm_malloc(sizeof(MEME_IO_XML_CALLBACKS_T));
  memset(parser->callbacks, 0, sizeof(MEME_IO_XML_CALLBACKS_T));
  parser->callbacks->error = mxml_error;
  parser->callbacks->start_meme = mxml_start_meme;
  parser->callbacks->start_alphabet = mxml_start_alphabet;
  parser->callbacks->handle_alphabet_letter = mxml_letter;
  parser->callbacks->end_alphabet = mxml_end_alphabet;
  parser->callbacks->handle_strands = mxml_handle_strands;
  parser->callbacks->handle_bf_aa_value = mxml_background_value;
  parser->callbacks->end_background_frequencies = mxml_end_background;
  parser->callbacks->start_motif = mxml_start_motif;
  parser->callbacks->end_motif = mxml_end_motif;
  parser->callbacks->start_scores = mxml_start_scores;
  parser->callbacks->start_sc_am_alphabet_array = mxml_start_score_pos;
  parser->callbacks->end_sc_am_alphabet_array = mxml_end_score_pos;
  parser->callbacks->handle_sc_am_aa_value = mxml_score_value;
  parser->callbacks->start_probabilities = mxml_start_probabilities;
  parser->callbacks->start_pr_am_alphabet_array = mxml_start_probability_pos;
  parser->callbacks->end_pr_am_alphabet_array = mxml_end_probability_pos;
  parser->callbacks->handle_pr_am_aa_value = mxml_probability_value;
  if (options & SCANNED_SITES) {
    parser->callbacks->handle_sequence = mxml_sequence;
    parser->callbacks->start_scanned_sites_summary = mxml_start_scanned_sites;
    parser->callbacks->end_scanned_sites_summary = mxml_end_scanned_sites;
    parser->callbacks->start_scanned_sites = mxml_start_scanned_seq;
    parser->callbacks->handle_scanned_site = mxml_scanned_site;
  }
  // setup context for SAX parser
  parser->sax_context = create_meme_io_xml_sax_context(parser->data, parser->callbacks);
  // setup SAX handler
  parser->handler = (xmlSAXHandler*)mm_malloc(sizeof(xmlSAXHandler));
  register_meme_io_xml_sax_handlers(parser->handler);
  // create the push parser context
  parser->ctxt = xmlCreatePushParserCtxt(parser->handler, parser->sax_context, NULL, 0, optional_filename); 
  return parser;
}

/*****************************************************************************
 * Destroy a MXML_T parser 
 ****************************************************************************/
void mxml_destroy(void *data) {
  MXML_T *parser;
  parser = (MXML_T*)data;
  // destroy the push parser context
  xmlFreeParserCtxt(parser->ctxt);
  // destroy the push parser callbacks
  free(parser->handler);
  // destroy the sax parser context
  destroy_meme_io_xml_sax_context(parser->sax_context);
  // destroy the sax parser callbacks
  free(parser->callbacks);
  // destroy the data store
  destroy_parser_data(parser->data);
  // destroy the parser information
  free(parser);
}

/*****************************************************************************
 * Update the parser with more file content.
 ****************************************************************************/
void mxml_update(void *data, const char *chunk, size_t size, short end) {
  MXML_T *parser;
  int result;
  parser = (MXML_T*)data;
  if ((result = xmlParseChunk(parser->ctxt, chunk, size, end)) != 0) {
    local_error(parser->data, "MEME XML parser returned error code %d.\n", result);
  }
}

/*****************************************************************************
 * Check if the parser has a motif ready to return.
 ****************************************************************************/
short mxml_has_motif(void *data) {
  MXML_T *parser;
  parser = (MXML_T*)data;
  return linklst_size(parser->data->motif_queue) > 0;
}

/*****************************************************************************
 * Check if the parser thinks that this file was really meant to be a 
 * MEME XML file.
 ****************************************************************************/
short mxml_has_format_match(void *data) {
  MXML_T *parser;
  parser = (MXML_T*)data;
  return parser->data->format_match;
}

/*****************************************************************************
 * Check if the parser has found a problem with the file that has not already
 * been returned.
 ****************************************************************************/
short mxml_has_error(void *data) {
  MXML_T *parser;
  parser = (MXML_T*)data;
  return linklst_size(parser->data->errors) > 0;
}

/*****************************************************************************
 * Get the next human readable error message detailing the 
 * problem with the file. The caller is responsible for freeing the memory.
 ****************************************************************************/
char* mxml_next_error(void *data) {
  MXML_T *parser;
  parser = (MXML_T*)data;
  return (char*)linklst_pop(parser->data->errors);
}

/*****************************************************************************
 * Get the next motif. The caller is responsible for freeing the memory.
 ****************************************************************************/
MOTIF_T* mxml_next_motif(void *data) {
  MXML_T *parser;
  parser = (MXML_T*)data;
  return (MOTIF_T*)linklst_pop(parser->data->motif_queue);
}

/*****************************************************************************
 * Get the motif alphabet
 ****************************************************************************/
ALPH_T mxml_get_alphabet(void *data) {
  MXML_T *parser;
  parser = (MXML_T*)data;
  return parser->data->alph;
}

/*****************************************************************************
 * Get the motif strands
 ****************************************************************************/
int mxml_get_strands(void *data) {
  MXML_T *parser;
  parser = (MXML_T*)data;
  return parser->data->fscope.strands;
}

/*****************************************************************************
 * Get the motif background
 ****************************************************************************/
BOOLEAN_T mxml_get_bg(void *data, ARRAY_T **bg) {
  MXML_T *parser;
  parser = (MXML_T*)data;
  if (parser->data->fscope.background == NULL) return FALSE;
  *bg = resize_array(*bg, get_array_length(parser->data->fscope.background));
  copy_array(parser->data->fscope.background, *bg);
  return TRUE;
}

/*****************************************************************************
 * Get an optional motif scoped object, currently none are supported
 ****************************************************************************/
void* mxml_motif_optional(void *data, int option) {
  MXML_T *parser;
  parser = (MXML_T*)data;
  if (!(parser->data->options & option)) {
    die("Requested value of optional component which was not requested "
        "during construction.\n");
    return NULL;
  }
  //TODO
  return NULL;
}

/*****************************************************************************
 * Get an optional file scoped object.
 * This supports scanned sites and background though the background may
 * be moved to its own method.
 ****************************************************************************/
void* mxml_file_optional(void *data, int option) {
  MXML_T *parser;
  parser = (MXML_T*)data;
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
