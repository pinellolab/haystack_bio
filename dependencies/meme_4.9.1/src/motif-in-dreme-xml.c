
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>

#include "dreme-sax.h"
#include "linked-list.h"
#include "motif-in-dreme-xml.h"
#include "motif-spec.h"
#include "string-match.h"
#include "utils.h"
#include "alphabet.h"


struct fscope {
  int options_found;
  int options_returned;
  ALPH_T alphabet;
  ARRAY_T *background;
};

typedef struct dreme_ctx CTX_T;
struct dreme_ctx {
  int options;
  struct fscope fscope;
  MOTIF_T *motif;
  LINKLST_T *motif_queue;
  LINKLST_T *errors;
  short file_type_match;
};

typedef struct dxml DXML_T;
struct dxml {
  CTX_T *data;
  DREME_IO_XML_CALLBACKS_T *callbacks;
  void *sax_context;
  xmlSAXHandler *handler;
  xmlParserCtxt *ctxt;
};

static CTX_T* create_parser_data(int options, const char *optional_filename) {
  CTX_T *data;
  data = mm_malloc(sizeof(CTX_T));
  memset(data, 0, sizeof(CTX_T));
  data->options = options;
  data->motif_queue = linklst_create();
  data->errors = linklst_create();
  data->file_type_match = file_name_match("dreme", "xml", optional_filename);
  return data;
}

static void destroy_parser_data(CTX_T *data) {
  linklst_destroy_all(data->motif_queue, destroy_motif);
  linklst_destroy_all(data->errors, free);
  if (data->fscope.background) {
    free_array(data->fscope.background);
  }
  if (data->motif) destroy_motif(data->motif);
  memset(data, 0, sizeof(CTX_T));
  free(data);
}
/*
 * Callbacks
 */
void dxml_error(void *ctx, const char *fmt, va_list ap) {
  CTX_T *data = (CTX_T*)ctx;
  int len;
  char dummy[1], *msg;
  va_list ap_copy;
  va_copy(ap_copy, ap);
  len = vsnprintf(dummy, 1, fmt, ap_copy);
  va_end(ap_copy);
  msg = mm_malloc(sizeof(char) * (len + 1));
  vsnprintf(msg, len + 1, fmt, ap);
  linklst_add(msg, data->errors);
}

void dxml_start_dreme(void *ctx, int major, int minor, int patch, char *release_date) {
  CTX_T *data;
  data = (CTX_T*)ctx;
  data->file_type_match = 3; // probably a dreme xml file
}

void dxml_handle_background(void *ctx, DREME_ALPH_EN alpha, double A, double C, 
    double G, double T, DREME_BG_EN source, char *file, char *last_mod_date) {
  CTX_T *data;
  MOTIF_T *motif;
  ARRAY_T *bg;

  data = (CTX_T*)ctx;
  data->file_type_match = 4; // it must be a dreme xml file!
  data->fscope.alphabet = DNA_ALPH;
  data->fscope.background = allocate_array(4);
  bg = data->fscope.background;
  set_array_item(0, A, bg);
  set_array_item(1, C, bg);
  set_array_item(2, G, bg);
  set_array_item(3, T, bg);
}

void dxml_start_motif(void *ctx, char *id, char *seq, int length, 
    long num_sites, long p_hits, long n_hits, 
    double log10pvalue, double log10evalue, double log10uevalue) {
  CTX_T *data;
  MOTIF_T *motif;

  data = (CTX_T*)ctx;
  data->motif = (MOTIF_T*)mm_malloc(sizeof(MOTIF_T));
  motif = data->motif;
  memset(motif, 0, sizeof(MOTIF_T));
  set_motif_id(seq, strlen(seq), motif);
  set_motif_id2("", 0, motif);
  set_motif_strand('+', motif);
  motif->length = length;
  motif->num_sites = num_sites;
  motif->log_evalue = log10evalue;
  motif->evalue = exp10(log10evalue);
  // both DNA and RNA have 4 letters
  motif->alph = data->fscope.alphabet;
  motif->flags = MOTIF_BOTH_STRANDS; // DREME does not support the concept of single strand scanning (yet)
  // allocate the matrix
  motif->freqs = allocate_matrix(motif->length, alph_size(motif->alph, ALPH_SIZE));
  motif->scores = NULL; // no scores in DREME xml
  // no url in DREME
  motif->url = strdup("");
  // set by postprocessing
  motif->complexity = -1;
  motif->trim_left = 0;
  motif->trim_right = 0;
}

void dxml_end_motif(void *ctx) {
  CTX_T *data;
  MOTIF_T *motif;

  data = (CTX_T*)ctx;
  motif = data->motif;

  linklst_add(motif, data->motif_queue);
  data->motif = NULL;
}

void dxml_handle_pos(void *ctx, int pos, double A, double C, double G, double T) {
  CTX_T *data;
  MOTIF_T *motif;
  ARRAY_T *row;

  data = (CTX_T*)ctx;
  motif = data->motif;
  row = get_matrix_row(pos - 1, motif->freqs);
  set_array_item(0, A, row);
  set_array_item(1, C, row);
  set_array_item(2, G, row);
  set_array_item(3, T, row);
}

void* dxml_create(const char *optional_filename, int options) {
  DXML_T *parser;
  // allocate the structure to hold the parser information
  parser = (DXML_T*)mm_malloc(sizeof(DXML_T));
  // setup data store
  parser->data = create_parser_data(options, optional_filename);
  // setup meme xml callbacks
  parser->callbacks = (DREME_IO_XML_CALLBACKS_T*)mm_malloc(sizeof(DREME_IO_XML_CALLBACKS_T));
  memset(parser->callbacks, 0, sizeof(DREME_IO_XML_CALLBACKS_T));
  //start callbacks
  parser->callbacks->error = dxml_error;
  parser->callbacks->start_dreme = dxml_start_dreme;
  // start dreme
  // start model
  parser->callbacks->handle_background = dxml_handle_background;
  // end model
  // start motifs
  parser->callbacks->start_motif = dxml_start_motif;
  parser->callbacks->end_motif = dxml_end_motif;
  // start motif
  parser->callbacks->handle_pos = dxml_handle_pos;
  // end motif
  // end motifs
  // end dreme
  // end callbacks
  // setup context for SAX parser
  parser->sax_context = create_dreme_io_xml_sax_context(parser->data, parser->callbacks);
  // setup SAX handler
  parser->handler = (xmlSAXHandler*)mm_malloc(sizeof(xmlSAXHandler));
  register_dreme_io_xml_sax_handlers(parser->handler);
  // create the push parser context
  parser->ctxt = xmlCreatePushParserCtxt(parser->handler, parser->sax_context, NULL, 0, optional_filename); 
  return parser;
}

void dxml_destroy(void *data) {
  DXML_T *parser;
  parser = (DXML_T*)data;
  // destroy the push parser context
  xmlFreeParserCtxt(parser->ctxt);
  // destroy the push parser callbacks
  free(parser->handler);
  // destroy the sax parser context
  destroy_dreme_io_xml_sax_context(parser->sax_context);
  // destroy the sax parser callbacks
  free(parser->callbacks);
  // destroy the data store
  destroy_parser_data(parser->data);
  // destroy the parser information
  free(parser);
}

void dxml_update(void *data, const char *chunk, size_t size, short end) {
  DXML_T *parser;
  parser = (DXML_T*)data;
  xmlParseChunk(parser->ctxt, chunk, size, end);
}

short dxml_has_motif(void *data) {
  DXML_T *parser;
  parser = (DXML_T*)data;
  return linklst_size(parser->data->motif_queue) > 0;
}

short dxml_has_format_match(void *data) {
  DXML_T *parser;
  parser = (DXML_T*)data;
  return parser->data->file_type_match;
}

short dxml_has_error(void *data) {
  DXML_T *parser;
  parser = (DXML_T*)data;
  return linklst_size(parser->data->errors) > 0;
}

char* dxml_next_error(void *data) {
  DXML_T *parser;
  parser = (DXML_T*)data;
  return linklst_pop(parser->data->errors);
}

MOTIF_T* dxml_next_motif(void *data) {
  DXML_T *parser;
  parser = (DXML_T*)data;
  return linklst_pop(parser->data->motif_queue);
}

ALPH_T dxml_get_alphabet(void *data) {
  DXML_T *parser;
  parser = (DXML_T*)data;
  return parser->data->fscope.alphabet;
}

int dxml_get_strands(void *data) {
  // DREME doesn't support single strand modes currently.
  return 2;
}

BOOLEAN_T dxml_get_bg(void *data, ARRAY_T **bg) {
  DXML_T *parser;
  parser = (DXML_T*)data;
  if (parser->data->fscope.background == NULL) return FALSE;
  *bg = resize_array(*bg, get_array_length(parser->data->fscope.background));
  copy_array(parser->data->fscope.background, *bg);
  return TRUE;
}

void* dxml_motif_optional(void *data, int option) {
  DXML_T *parser;
  parser = (DXML_T*)data;
  if (!(parser->data->options & option)) {
    die("Requested value of optional component which was not requested "
        "during construction.\n");
    return NULL;
  }
  // DREME xml does not currently support optional components
  return NULL;
}

void* dxml_file_optional(void *data, int option) {
  DXML_T *parser;
  parser = (DXML_T*)data;
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
    default:
      die("Option code %d does not match any single option. "
          "This means that it must contain multiple options "
          "and only one is allowed at a time\n.", option);
  }
  return NULL; //unreachable! make compiler happy
}
