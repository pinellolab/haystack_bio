
#ifndef MEME_SAX_H
#define MEME_SAX_H

#include <libxml/parser.h>

#include <stdarg.h>

typedef struct meme_io_xml_callbacks MEME_IO_XML_CALLBACKS_T;

struct meme_io_xml_callbacks {
  void (*error)(void *, char *, va_list);
  void (*start_meme)(void *, int, int, int, char *); //major version, minor version, patch version, release date
  void (*end_meme)(void *);
  //start meme
  void (*start_training_set)(void *, char *, int); // datafile, length
  void (*end_training_set)(void *);
  //start training_set
  void (*start_alphabet)(void *, int, int); // is nucleotide, length
  void (*end_alphabet)(void *);
  //start alphabet
  void (*handle_alphabet_letter)(void *, char *, char); // id, symbol
  //end alphabet
  void (*start_ambigs)(void *);
  void (*end_ambigs)(void *);
  //start ambigs
  void (*handle_ambigs_letter)(void *, char *, char); // id, symbol
  //end ambigs
  void (*handle_sequence)(void *, char *, char *, int, double); // id, name, length, weight
  void (*start_letter_frequencies)(void *);
  void (*end_letter_frequencies)(void *); 
  //start letter_frequencies
  void (*start_lf_alphabet_array)(void *);
  void (*end_lf_alphabet_array)(void *);
  //start alphabet_array
  void (*handle_lf_aa_value)(void *, char *, double);
  //end alphabet_array
  //end letter_frequencies
  //end training_set
  void (*start_model)(void *);
  void (*end_model)(void *);
  //start model
  void (*handle_command_line)(void *, char *); //command line
  void (*handle_host)(void *, char *); //host
  void (*handle_type)(void *, int); //type - TODO investigate
  void (*handle_nmotifs)(void *, int); //nmotifs
  void (*handle_evalue_threshold)(void *, double); //evalue threshold (in log base 10)
  void (*handle_object_function)(void *, char *); // object function - TODO investigate
  void (*handle_min_width)(void *, int); // min width of motifs
  void (*handle_max_width)(void *, int); // max width of motifs
  void (*handle_minic)(void *, double); // min information content of motifs
  void (*handle_wg)(void *, double); // wg - TODO investigate (guessing some kind of weight?)
  void (*handle_ws)(void *, double); // ws - TODO investigate (guessing some kind of weight?)
  void (*handle_endgaps)(void *, int); //allow gaps on end (boolean) 
  void (*handle_minsites)(void *, int); // minimum sites for motif
  void (*handle_maxsites)(void *, int); // maximum sites for motif
  void (*handle_wnsites)(void *, int); // wnsites - TODO
  void (*handle_prob)(void *, double); // prob - TODO
  void (*handle_spmap)(void *, int); // type of spmap
  void (*handle_spfuzz)(void *, double); // spfuzz (weight?) TODO
  void (*handle_prior)(void *, int); // type of prior
  void (*handle_beta)(void *, double); // beta (weight?) TODO
  void (*handle_maxiter)(void *, int); // maxiter TODO
  void (*handle_distance)(void *, double); // distance (weight?) TODO
  void (*handle_num_sequences)(void *, int); // num sequences TODO
  void (*handle_num_positions)(void *, int); // num positions TODO
  void (*handle_seed)(void *, long); // seed
  void (*handle_seqfrac)(void *, double); //seqfrac TODO
  void (*handle_strands)(void *, int); // strands 0 = NA, 1 = Forward strand only, 2 = Both strands
  void (*handle_priors_file)(void *, char *); // priors file
  void (*handle_reason_for_stopping)(void *, char *); // reason for stopping
  void (*start_background_frequencies)(void *, char *); // source of data
  void (*end_background_frequencies)(void *);
  //start background frequencies
  void (*start_bf_alphabet_array)(void *);
  void (*end_bf_alphabet_array)(void *);
  //start alphabet_array
  void (*handle_bf_aa_value)(void *, char *, double);
  //end alphabet_array
  //end background frequencies
  //end model
  void (*start_motifs)(void *);
  void (*end_motifs)(void *);
  //start motifs
  void (*start_motif)(void *, char *, char *, int, double, double, //id, name, width, sites, llr
      double, double, double, double, double, char *); //ic, re TODO, bayes_threshold, e_value (in log10), elpased_time, url
  void (*end_motif)(void *);
  //start motif
  void (*start_scores)(void *);
  void (*end_scores)(void *);
  //start scores
  void (*start_sc_alphabet_matrix)(void *);
  void (*end_sc_alphabet_matrix)(void *);
  //start alphabet matrix
  void (*start_sc_am_alphabet_array)(void *);
  void (*end_sc_am_alphabet_array)(void *);
  //start alphabet array
  void (*handle_sc_am_aa_value)(void *, char *, double);
  //end alphabet array
  //end alphabet matrix
  //end scores
  void (*start_probabilities)(void *);
  void (*end_probabilities)(void *);
  //start probabilities
  void (*start_pr_alphabet_matrix)(void *);
  void (*end_pr_alphabet_matrix)(void *);
  //start alphabet matrix
  void (*start_pr_am_alphabet_array)(void *);
  void (*end_pr_am_alphabet_array)(void *);
  //start alphabet array
  void (*handle_pr_am_aa_value)(void *, char *, double);
  //end alphabet array
  //end alphabet matrix
  //end probabilities
  void (*handle_regular_expression)(void *, char *);
  void (*start_contributing_sites)(void *);
  void (*end_contributing_sites)(void *);
  //start contributing sites
  void (*start_contributing_site)(void *, char *, int, char, double); // sequence id, position, strand, pvalue (in log 10)
  void (*end_contributing_site)(void *);
  //start contributing site
  void (*handle_left_flank)(void *, char *);
  void (*start_site)(void *);
  void (*end_site)(void *);
  //start site
  void (*handle_letter_ref)(void *, char *);
  //end site
  void (*handle_right_flank)(void *, char *);
  //end contributing site
  //end contributing sites
  //end motif
  //end motifs
  void (*start_scanned_sites_summary)(void *, double); // pvalue threshold for scanned sites
  void (*end_scanned_sites_summary)(void *);
  //start scanned_sites_summary
  void (*start_scanned_sites)(void *, char *, double, int); // sequence id, pvalue (in log 10), number of sites
  void (*end_scanned_sites)(void *);
  //start scanned_sites
  void (*handle_scanned_site)(void *, char *, char, int, double); // motif id, strand, position, pvalue (in log 10)
  //end scanned_sites
  //end scanned_sites_summary
  //end meme
};

/*****************************************************************************
 * Register handlers on the xmlSAXHandler structure
 ****************************************************************************/
void register_meme_io_xml_sax_handlers(xmlSAXHandler *handler);

/*****************************************************************************
 * Creates the data to be passed to the SAX parser
 ****************************************************************************/
void* create_meme_io_xml_sax_context(void *user_data, MEME_IO_XML_CALLBACKS_T *callbacks);

/*****************************************************************************
 * Destroys the data used by the SAX parser
 ****************************************************************************/
void destroy_meme_io_xml_sax_context(void *context);

#endif
