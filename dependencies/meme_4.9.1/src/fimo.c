/********************************************************************
 * FILE: fimo.c
 * AUTHOR: William Stafford Noble, Charles E. Grant, Timothy Bailey
 * CREATE DATE: 12/17/2004
 * PROJECT: MEME suite
 * COPYRIGHT: 2004-2007, UW
 ********************************************************************/

#define DEFINE_GLOBALS

#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include "matrix.h"
#include "alphabet.h"
#include "cisml.h"
#include "dir.h"
#include "fasta-io.h"
#include "hash_alph.h"
#include "io.h"
#include "motif-in.h"
#include "projrel.h"
#include "pssm.h"
#include "prior-reader-from-psp.h"
#include "prior-reader-from-wig.h"
#include "reservoir.h"
#include "seq-reader-from-fasta.h"
#include "simple-getopt.h"
#include "string-list.h"
#include "utils.h"
#include "wiggle-reader.h"
#include "xml-util.h"

char* program_name = "fimo";
VERBOSE_T verbosity = NORMAL_VERBOSE;
typedef enum {INVALID_THRESH, PV_THRESH, QV_THRESH} THRESHOLD_TYPE;

const char *threshold_type_to_string(THRESHOLD_TYPE type) {
  switch(type) {
    case PV_THRESH:
      return "p-value";
      break;
    case QV_THRESH:
      return "q-value";
      break;
    default:
      return "invalid";
      break;
  }
}

// Structure for recording FIMO motif matches
typedef struct fimo_match {
  char *motif_name;
  char *seq_name;
  char *raw_seq;
  int start;
  int stop;
  char strand;
  double pvalue;
  double score;
} FIMO_MATCH_T;

// Structure for tracking fimo command line parameters.
typedef struct options {

  BOOLEAN_T allow_clobber;      // Allow overwritting of files in output directory.
  BOOLEAN_T compute_qvalues;    // Compute q-values
  BOOLEAN_T parse_genomic_coord;// Parse genomic coord. from seq. headers.
  BOOLEAN_T text_only;          // Generate only plain text output
  BOOLEAN_T max_strand;         // When scores available for both strands
                                // print only the max of the two strands.
  BOOLEAN_T scan_both_strands;  // Scan forward and reverse strands

  char* bg_filename;            // Name of file file containg background freq.
  char* command_line;           // Full command line
  char* meme_filename;          // Name of file containg motifs.
  char* output_dirname;         // Name of the output directory
  char* seq_filename;           // Name of file containg sequences.
  char* seq_name;               // Use this sequence name in the output.

  int max_seq_length;    // Maximum allowed sequence length.
  int max_stored_scores; // Maximum number of matches to store per pattern.

  double alpha;       // Non-motif specific scale factor.
  double pseudocount; // Pseudocount added to Motif PSFM.
  double output_threshold; // Maximum p-value/q-value to report.

  ALPH_T alphabet;    // Alphabet specified by MEME file.
  THRESHOLD_TYPE threshold_type;  // Type of output threshold.
  STRING_LIST_T* selected_motifs; // Indices of requested motifs.

  char *psp_filename; // Path to file containing position specific priors (PSP)
  char *prior_distribution_filename; // Path to file containing prior distribution
  char *pval_lookup_filename;   // Print p-value lookup table.
  char *html_stylesheet_path; // Path to master copy of HTML stlesheet for CisML
  char *html_stylesheet_local_path; // Path to working copy of HTML stylesheet for CisML
  char *css_stylesheet_path; // Path to master copy of CSS style sheet for CisML
  char *css_stylesheet_local_path; // Path to working copy of CSS stylesheet for CisML
  char *text_stylesheet_path; // Path to plain-text stylesheet for CisML
  char *gff_stylesheet_path; // Path to GFF stylesheeet for CisML
  char *html_path; // Path to FIMO HTML output file
  char *text_path; // Path to FIMO plain-text output file
  char *gff_path; // Path to FIMO GFF output file
  char *fimo_path; // Pathe to FIMO XML output file
  char *cisml_path; // Path to CisML XML output file

  const char* HTML_STYLESHEET;  // Name of HTML XSLT stylesheet.
  const char* CSS_STYLESHEET;   // Name of CSS stylesheet.
  const char* GFF_STYLESHEET;   // Name of GFF XSLT stylesheet.
  const char* TEXT_STYLESHEET;  // Name of plain-text XSLT stylesheet.
  const char* HTML_FILENAME;    // Name of HTML output file.
  const char* TEXT_FILENAME;    // Name of plain-text output file.
  const char* GFF_FILENAME;     // Name of GFF output file.
  const char* FIMO_FILENAME;    // Name of FIMO XML output file.
  const char* CISML_FILENAME;   // Name of CisML XML output file.

  const char* usage; // Usage statment

} FIMO_OPTIONS_T;

/***********************************************************************
  Print plain text record for motif site to standard output.
 ***********************************************************************/
static void print_site_as_text(FIMO_MATCH_T *match);

/***********************************************************************
  Free memory allocated in options processing
 ***********************************************************************/
static void cleanup_options(FIMO_OPTIONS_T *options) {
  myfree(options->command_line);
  myfree(options->text_stylesheet_path);
  myfree(options->gff_stylesheet_path);
  myfree(options->html_path);
  myfree(options->text_path);
  myfree(options->gff_path);
  myfree(options->html_stylesheet_path);
  myfree(options->html_stylesheet_local_path);
  myfree(options->css_stylesheet_path);
  myfree(options->css_stylesheet_local_path);
  myfree(options->fimo_path);
  myfree(options->cisml_path);
}

/***********************************************************************
  Process command line options
 ***********************************************************************/
static void process_command_line(
  int argc,
  char* argv[],
  FIMO_OPTIONS_T *options
) {

  // Define command line options.
  cmdoption const fimo_options[] = {
    {"alpha", REQUIRED_VALUE},
    {"bgfile", REQUIRED_VALUE},
    {"max-seq-length", REQUIRED_VALUE},
    {"max-stored-scores", REQUIRED_VALUE},
    {"max-strand", NO_VALUE},
    {"motif", REQUIRED_VALUE},
    {"motif-pseudo", REQUIRED_VALUE},
    {"norc", NO_VALUE},
    {"o", REQUIRED_VALUE},
    {"oc", REQUIRED_VALUE},
    {"no-qvalue", NO_VALUE},
    {"parse-genomic-coord", NO_VALUE},
    {"psp", REQUIRED_VALUE},
    {"prior-dist", REQUIRED_VALUE},
    {"qv-thresh", NO_VALUE},
    {"text", NO_VALUE},
    {"thresh", REQUIRED_VALUE},
    {"verbosity", REQUIRED_VALUE},
    {"version", NO_VALUE},
    {"pval-lookup", REQUIRED_VALUE} // This option is hidden from users.
  };
  const int num_options = sizeof(fimo_options) / sizeof(cmdoption);

  // Define the usage message.
  options->usage =
    "USAGE: fimo [options] <motif file> <sequence file>\n"
    "\n"
    "   Options:\n"
    "     --alpha <double> (default 1.0)\n"
    "     --bgfile <background> (default from NR sequence database)\n"
    "     --max-seq-length <int> (default=2.5e8)\n"
    "     --max-stored-scores <int> (default=100000)\n"
    "     --max-strand\n"
    "     --motif <id> (default=all)\n"
    "     --motif-pseudo <float> (default=0.1)\n"
    "     --no-qvalue\n"
    "     --norc\n"
    "     --o <output dir> (default=fimo_out)\n"
    "     --oc <output dir> (default=fimo_out)\n"
    "     --parse-genomic-coord\n"
    "     --psp <PSP filename> (default none)\n"
    "     --prior-dist <PSP distribution filename> (default none)\n"
    "     --qv-thresh\n"
    "     --text\n"
    "     --thresh <float> (default = 1e-4)\n"
    "     --verbosity [1|2|3|4] (default 2)\n"
    "     --version (print the version and exit)\n"
    "\n"
    "   Use \'-\' for <sequence file> to read the database from standard input.\n"
    "   Use \'--bgfile motif-file\' to read the background from the motif file.\n"
    "\n";

  int option_index = 0;

  /* Make sure various options are set to NULL or defaults. */
  options->allow_clobber = TRUE;
  options->compute_qvalues = TRUE;
  options->max_strand = FALSE;
  options->parse_genomic_coord = FALSE;
  options->threshold_type = PV_THRESH;
  options->text_only = FALSE;
  options->scan_both_strands = TRUE;

  options->bg_filename = NULL;
  options->command_line = NULL;
  options->meme_filename = NULL;
  options->output_dirname = "fimo_out";
  options->psp_filename = NULL;
  options->prior_distribution_filename = NULL;
  options->seq_filename = NULL;

  options->max_seq_length = MAX_SEQ;
  options->max_stored_scores = 100000;


  options->alpha = 1.0;
  options->pseudocount = 0.1;
  options->pseudocount = 0.1;
  options->output_threshold = 1e-4;

  options->selected_motifs = new_string_list();
  options->pval_lookup_filename = NULL;
  verbosity = 2;

  simple_setopt(argc, argv, num_options, fimo_options);

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
      die("Error processing command line options (%s)\n", message);
    }
    if (strcmp(option_name, "bgfile") == 0){
      options->bg_filename = option_value;
    }
    else if (strcmp(option_name, "psp") == 0){
      options->psp_filename = option_value;
    }
    else if (strcmp(option_name, "prior-dist") == 0){
      options->prior_distribution_filename = option_value;
    }
    else if (strcmp(option_name, "alpha") == 0) {
      options->alpha = atof(option_value);
    }
    else if (strcmp(option_name, "max-seq-length") == 0) {
      // Use atof and cast to be able to read things like 1e8.
      options->max_seq_length = (int)atof(option_value);
    }
    else if (strcmp(option_name, "max-stored-scores") == 0) {
      // Use atof and cast to be able to read things like 1e8.
      options->max_stored_scores = (int)atof(option_value);
    }
    else if (strcmp(option_name, "max-strand") == 0) {
      // Turn on the max-strand option
      options->max_strand = TRUE;
    }
    else if (strcmp(option_name, "motif") == 0){
      if (options->selected_motifs == NULL) {
        options->selected_motifs = new_string_list();
      }
      add_string(option_value, options->selected_motifs);
    }
    else if (strcmp(option_name, "motif-pseudo") == 0){
      options->pseudocount = atof(option_value);
    }
    else if (strcmp(option_name, "norc") == 0){
      options->scan_both_strands = FALSE;
    }
    else if (strcmp(option_name, "o") == 0){
      // Set output directory with no clobber
      options->output_dirname = option_value;
      options->allow_clobber = FALSE;
    }
    else if (strcmp(option_name, "oc") == 0){
      // Set output directory with clobber
      options->output_dirname = option_value;
      options->allow_clobber = TRUE;
    }
    else if (strcmp(option_name, "parse-genomic-coord") == 0){
      options->parse_genomic_coord = TRUE;
    }
    else if (strcmp(option_name, "thresh") == 0){
      options->output_threshold = atof(option_value);
    }
    else if (strcmp(option_name, "qv-thresh") == 0){
      options->threshold_type = QV_THRESH;
    }
    else if (strcmp(option_name, "no-qvalue") == 0){
      options->compute_qvalues = FALSE;
    }
    else if (strcmp(option_name, "text") == 0){
      options->text_only = TRUE;
    }
    else if (strcmp(option_name, "verbosity") == 0){
      verbosity = atoi(option_value);
    }
    else if (strcmp(option_name, "version") == 0) {
      fprintf(stdout, VERSION "\n");
      exit(EXIT_SUCCESS);
    }
    else if (strcmp(option_name, "pval-lookup") == 0) {
      options->pval_lookup_filename = option_value;
    }
  }

  // Check that positiion specific priors options are consistent
  if (options->psp_filename != NULL 
      && options->prior_distribution_filename == NULL) {
    die(
      "Setting the --psp option requires that the"
      " --prior-dist option be set as well.\n"
    );
  }
  if (options->psp_filename == NULL 
      && options->prior_distribution_filename != NULL) {
    die(
      "Setting the --prior-dist option requires that the"
      " --psp option be set as well.\n");
  }

  // Check that qvalue options are consistent
  if (options->compute_qvalues == FALSE && options->threshold_type == QV_THRESH) {
    die("The --no-qvalue option cannot be used with the --qv-thresh option");
  }

  // Turn off q-values if text only.
  if (options->text_only == TRUE) {
    if (options->threshold_type == QV_THRESH) {
      die("The --text option cannot be used with the --qv-thresh option");
    }
    if (options->compute_qvalues) {
      fprintf(stderr, "Warning: text mode turns off computation of q-values\n");
    }
    options->compute_qvalues = FALSE;
  }

  // Must have sequence and motif file names
  if (argc != option_index + 2) {
    fprintf(stderr, "%s", options->usage);
    exit(EXIT_FAILURE);
  }
  // Record the command line
  options->command_line = get_command_line(argc, argv);

  // Record the input file names
  options->meme_filename = argv[option_index];
  option_index++;
  options->seq_filename = argv[option_index];
  option_index++;

  // Set up path values for needed stylesheets and output files.
  options->HTML_STYLESHEET = "fimo-to-html.xsl";
  options->CSS_STYLESHEET = "cisml.css";
  options->GFF_STYLESHEET = "fimo-to-gff3.xsl";
  options->TEXT_STYLESHEET = "fimo-to-text.xsl";
  options->HTML_FILENAME = "fimo.html";
  options->TEXT_FILENAME = "fimo.txt";
  options->GFF_FILENAME = "fimo.gff";
  options->FIMO_FILENAME = "fimo.xml";
  options->CISML_FILENAME = "cisml.xml";
  options->text_stylesheet_path = make_path_to_file(get_meme_etc_dir(), options->TEXT_STYLESHEET);
  options->gff_stylesheet_path = make_path_to_file(get_meme_etc_dir(), options->GFF_STYLESHEET);
  options->html_path = make_path_to_file(options->output_dirname, options->HTML_FILENAME);
  options->text_path = make_path_to_file(options->output_dirname, options->TEXT_FILENAME);
  options->gff_path = make_path_to_file(options->output_dirname, options->GFF_FILENAME);
  options->html_stylesheet_path = make_path_to_file(get_meme_etc_dir(), options->HTML_STYLESHEET);
  options->html_stylesheet_local_path
    = make_path_to_file(options->output_dirname, options->HTML_STYLESHEET);
  options->css_stylesheet_path = make_path_to_file(get_meme_etc_dir(), options->CSS_STYLESHEET);
  options->css_stylesheet_local_path
    = make_path_to_file(options->output_dirname, options->CSS_STYLESHEET);
  options->fimo_path = make_path_to_file(options->output_dirname, options->FIMO_FILENAME);
  options->cisml_path = make_path_to_file(options->output_dirname, options->CISML_FILENAME);

}

/********************************************************
* Read priors from the priors file.
*
* If *prior_block is NULL, a prior block will be allocated,
* and the first block of priors will be read into it.
*
* If the seq. position is less than the current prior
* position we leave the prior block in the current position
* and set the prior to NaN.
*
* If the seq. position is within the extent of the current
* prior block we set the prior to the value from the block.
*
* If the seq. position is past the extent of the current
* prior block we read blocks until reach the seq. position
* or we reach the end of the sequence.
* 
********************************************************/
void get_prior_from_reader(
  DATA_BLOCK_READER_T *psp_reader,
  const char *seq_name,
  size_t seq_position,
  DATA_BLOCK_T **prior_block, // Out variable
  double *prior // Out variable
) {

  *prior = NaN();

  if (*prior_block == NULL) {
    // Allocate prior block if not we've not already done so
    *prior_block = new_prior_block();
    // Fill in first data for block 
    BOOLEAN_T result = psp_reader->get_next_block(psp_reader, *prior_block);
    if (result == FALSE) {
      die("Failed to read first prior from sequence %s.", seq_name);
    }
  }

  // Get prior for sequence postion
  size_t block_position = get_start_pos_for_data_block(*prior_block);
  size_t block_extent = get_num_read_into_data_block(*prior_block);
  if (block_position > seq_position) {
    // Sequence position is before current prior position
    return;
  }
  else if (block_position <= seq_position && seq_position <= (block_position + block_extent - 1)) {
    // Sequence position is contained in current prior block
    *prior = get_prior_from_data_block(*prior_block);
  }
  else {
    // Sequence position is after current prior position.
    // Try reading the next prior block.
    BOOLEAN_T priors_remaining = FALSE;
    while ((priors_remaining = psp_reader->get_next_block(psp_reader, *prior_block)) != FALSE) {
      block_position = get_start_pos_for_data_block(*prior_block);
      block_extent = get_num_read_into_data_block(*prior_block);
      if (block_position > seq_position) {
        // Sequence position is before current prior position
        return;
      }
      else if (block_position <= seq_position && seq_position <= (block_position + block_extent - 1)) {
        // Sequence position is contained in current prior block
        *prior = get_prior_from_data_block(*prior_block);
        break;
      }
    }
    if (priors_remaining == FALSE && verbosity > NORMAL_VERBOSE) {
      fprintf(stderr, "Warning: reached end of priors for sequence %s.\n", seq_name);
    }
  }
}

/**********************************************
* Read the motifs from the motif file.
**********************************************/
static void fimo_read_motifs(
  FIMO_OPTIONS_T *options, 
  int *num_motif_names,
  ARRAYLST_T **motifs, 
  ARRAY_T **bg_freqs) {

  MREAD_T *mread;

  mread = mread_create(options->meme_filename, OPEN_MFILE);
  mread_set_bg_source(mread, options->bg_filename);
  mread_set_pseudocount(mread, options->pseudocount);

  *motifs = mread_load(mread, NULL);
  options->alphabet = mread_get_alphabet(mread);
  *bg_freqs = mread_get_background(mread);
  mread_destroy(mread);

  // Check that we actuall got back some motifs
  *num_motif_names = arraylst_size(*motifs);
  if (*num_motif_names == 0) {
    die("No motifs could be read.\n");
  }

  // If motifs use protein alphabet we will not scan both strands
  if (options->alphabet == PROTEIN_ALPH) {
    options->scan_both_strands = FALSE;
  }

  if (options->scan_both_strands == TRUE) {
    // Set up hash tables for computing reverse complement
    setup_hash_alph(DNAB);
    setalph(0);
    // Correct background by averaging on freq. for both strands.
    average_freq_with_complement(DNA_ALPH, *bg_freqs);
    int asize = alph_size(options->alphabet, ALPH_SIZE);
    normalize_subarray(0, asize, 0.0, *bg_freqs);
    // Make reverse complement motifs.
    add_reverse_complements(*motifs);
  }

}

/*************************************************************************
 * Write a motif match to the appropriate output files.
 *************************************************************************/
void fimo_record_score(
  FIMO_OPTIONS_T *options,
  PATTERN_T *pattern,
  SCANNED_SEQUENCE_T *scanned_seq,
  RESERVOIR_SAMPLER_T *reservoir,
  FIMO_MATCH_T *match
) {

  if (options->text_only != TRUE) {
    // Keep count of how may positions we've scanned.
    add_scanned_sequence_scanned_position(scanned_seq);
    reservoir_sample(reservoir, match->pvalue);
  }

  if (match->pvalue <= options->output_threshold) {
    if (options->text_only == TRUE) {
        print_site_as_text(match);
    } else {
      MATCHED_ELEMENT_T *element = allocate_matched_element_with_score(
        match->start,
        match->stop,
        match->score,
        match->pvalue,
        scanned_seq
      );
      BOOLEAN_T added = add_pattern_matched_element(pattern, element);
      // If already have dropped elements of greater p-value
      // we won't be keeping this one.
      if (added == TRUE) {
        set_matched_element_sequence(element, match->raw_seq);
        set_matched_element_strand(element, match->strand);
      }
      else {
        free_matched_element(element);
      }
    }
  }
}

/*************************************************************************
 * Calculate the log odds score for a single motif-sized window.
 *************************************************************************/
static inline BOOLEAN_T score_motif_site(
  FIMO_OPTIONS_T *options,
  char *seq,
  double prior,
  PSSM_T *pssm,
  double *pvalue, // OUT
  double *score // OUT
) {

  ARRAY_T* pv_lookup = pssm->pv;
  MATRIX_T* pssm_matrix = pssm->matrix;
  int asize = alph_size(options->alphabet, ALPH_SIZE);
  BOOLEAN_T scorable_site = TRUE;
  double scaled_log_odds = 0.0;

  // For each position in the site
  int motif_position;
  for (motif_position = 0; motif_position < pssm->w; motif_position++) {

    char c = seq[motif_position];
    int aindex = alph_index(options->alphabet, c);

    // Check for gaps and ambiguity codes at this site
    if(aindex == -1 || aindex >= asize) {
        scorable_site = FALSE;
        break;
    }

    scaled_log_odds += get_matrix_cell(motif_position, aindex, pssm_matrix);
  }

  if (scorable_site == TRUE) {

    int w = pssm->w;
    *score = get_unscaled_pssm_score(scaled_log_odds, pssm);

    if (!isnan(prior)) {
      // Use the prior to adjust the motif site score.
      // Using the log-odds prior increases the width
      // of the scaled score table by 1.
      ++w;
      double prior_log_odds = (options->alpha) * prior;
      prior_log_odds = my_log2(prior_log_odds / (1.0L - prior_log_odds));
      *score += prior_log_odds;
      scaled_log_odds = raw_to_scaled(*score, w, pssm->scale, pssm->offset);
    }

    // Handle scores that are out of range
    if ((int) scaled_log_odds >= get_array_length(pv_lookup)) {
      scaled_log_odds = (float)(get_array_length(pv_lookup) - 1);
      *score = scaled_to_raw(scaled_log_odds, w, pssm->scale, pssm->offset);
    }
    *pvalue = get_array_item((int) scaled_log_odds, pv_lookup);

  }

  return scorable_site;

}

/*************************************************************************
 * Calculate and record the log-odds score and p-value for each 
 * possible motif site in the sequence.
 *
 * Returns the length of the sequence.
 *************************************************************************/
static long fimo_score_sequence(
  FIMO_OPTIONS_T *options,
  RESERVOIR_SAMPLER_T *reservoir,
  char strand,
  DATA_BLOCK_READER_T *fasta_reader,
  DATA_BLOCK_READER_T *psp_reader,
  MOTIF_T* motif,
  MOTIF_T* rev_motif,
  ARRAY_T* bg_freqs,
  PSSM_T*  pssm,
  PSSM_T*  rev_pssm,
  PATTERN_T* pattern
)
{
  assert(motif != NULL);
  assert(bg_freqs != NULL);
  assert(pssm != NULL);

  long num_positions = 0L;

  char *fasta_seq_name = NULL;
  BOOLEAN_T fasta_result 
    = fasta_reader->get_seq_name(fasta_reader, &fasta_seq_name);

  if (psp_reader != NULL) {
    // Go to what should be the corresponding sequence in the PSP
    BOOLEAN_T psp_result = psp_reader->go_to_next_sequence(psp_reader);
    if (psp_result == FALSE) {
      die(
        "Reached end of sequences in prior file %s before reaching end of\n"
        "sequences in fasta file %s.\n",
        options->psp_filename,
        options->seq_filename
      );
    }
    char *psp_seq_name = NULL;
    psp_result 
      = psp_reader->get_seq_name(psp_reader, &psp_seq_name);

    // Sequences must be in the same order in the FASTA file and PSP file.
    if (strcmp(fasta_seq_name, psp_seq_name) != 0) {
      die(
        "Sequence name %s from PSP file %s doesn't match\n"
        "sequence name %s from FASTA file %s.\n",
        psp_seq_name,
        options->psp_filename,
        fasta_seq_name,
        options->seq_filename
      );
    }
    myfree(psp_seq_name);

  }

  // Get at least the motif name.
  char* bare_motif_id = get_motif_id(motif);

  // Create a scanned_sequence record and record it in pattern.
  SCANNED_SEQUENCE_T* scanned_seq = NULL;
  if (!options->text_only) {
    scanned_seq =
      allocate_scanned_sequence(fasta_seq_name, fasta_seq_name, pattern);
  }

  // Score and record each possible motif site in the sequence

  // Reserve memory for incoming sequence and prior data
  DATA_BLOCK_T *seq_block = new_sequence_block(pssm->w);
  DATA_BLOCK_T  *prior_block = NULL;

  while (fasta_reader->get_next_block(fasta_reader, seq_block) != FALSE) {

    FIMO_MATCH_T fwd_match;
    FIMO_MATCH_T rev_match;
    double prior = NaN();

    // Track number of positions in sequence
    ++num_positions;

    // Get sequence data
    char *raw_seq = get_sequence_from_data_block(seq_block);
    char *invcomp_seq = NULL;
    if (rev_pssm != NULL) {
      // Since we're using the reverse complemment motif
      // convert sequence to reverse complment for output.
      invcomp_seq = strdup(raw_seq);
      if (invcomp_seq == NULL) {
        die("Unable to allocate memory for RC sequence string while printing\n");
      }
      invcomp_dna(invcomp_seq, get_motif_length(motif));
    }

    fwd_match.start = get_start_pos_for_data_block(seq_block);
    rev_match.stop = fwd_match.start;
    fwd_match.stop = fwd_match.start + get_motif_length(motif) - 1;
    rev_match.start = fwd_match.stop;
    fwd_match.motif_name = get_motif_id(motif);
    rev_match.motif_name = fwd_match.motif_name;
    fwd_match.seq_name = fasta_seq_name;
    rev_match.seq_name = fasta_seq_name;
    fwd_match.raw_seq = raw_seq;
    rev_match.raw_seq = invcomp_seq;
    fwd_match.strand = '+';
    rev_match.strand = '-';
    fwd_match.score = NaN();
    rev_match.score = NaN();
    fwd_match.pvalue = NaN();
    rev_match.pvalue = NaN();

    // Get corresponding prior.
    if (psp_reader != NULL) { 
      get_prior_from_reader(psp_reader, fasta_seq_name, fwd_match.start, &prior_block, &prior); 
    }

    BOOLEAN_T scoreable_site = FALSE;
    // Always score forward strand
    scoreable_site = score_motif_site(options, raw_seq, prior, pssm, &fwd_match.pvalue, &fwd_match.score);
    if (scoreable_site == TRUE && rev_pssm != NULL) {
      // Score reverse strand if reverse PSSM available.
      scoreable_site = score_motif_site(options, raw_seq, prior, rev_pssm, &rev_match.pvalue, &rev_match.score);
    }

    if (scoreable_site == TRUE) {
      if (rev_pssm != NULL && options->max_strand == TRUE) {
        // Report the larger of the two scores for this site.
        if (fwd_match.score < rev_match.score) {
          fimo_record_score(options, pattern, scanned_seq, reservoir, &rev_match);
        }
        else {
          fimo_record_score(options, pattern, scanned_seq, reservoir, &fwd_match);
        }
      }
      else {
        // Certainly record the forward score
        fimo_record_score(options, pattern, scanned_seq, reservoir, &fwd_match);
        if (rev_pssm != NULL) {
          // Record the rev. score too
          fimo_record_score(options, pattern, scanned_seq, reservoir, &rev_match);
        }
      }
    }

    if (rev_pssm != NULL) {
      myfree(invcomp_seq);
    }
  }

  // Count reminaing positions in the sequence.
  num_positions += get_num_read_into_data_block(seq_block);

  free_data_block(seq_block);
  if (prior_block) free_data_block(prior_block);
  myfree(fasta_seq_name);

  return  num_positions;

}

/**************************************************************
 * Score each of the sites for each of the selected motifs.
 **************************************************************/
static void fimo_score_each_motif(
  FIMO_OPTIONS_T *options,
  ARRAY_T *bg_freqs,
  ARRAYLST_T *motifs,
  CISML_T *cisml,
  FILE *cisml_file,
  PRIOR_DIST_T *prior_dist,
  DATA_BLOCK_READER_T *fasta_reader,
  DATA_BLOCK_READER_T *psp_reader,
  FILE *pval_lookup_file,
  STRING_LIST_T *incomplete_motifs,
  int *num_scanned_sequences,
  long *num_scanned_positions
) {

  // Create p-value sampling reservoir
  RESERVOIR_SAMPLER_T *reservoir = NULL;
  if (!options->text_only) {
    reservoir = new_reservoir_sampler(10000, NULL);
  }

  // Count of all motifs including rev. comp. motifs
  int num_motifs = arraylst_size(motifs);
  int motif_index = 0;
  for (motif_index = 0; motif_index < num_motifs; motif_index++) {

    MOTIF_T* motif = (MOTIF_T *) arraylst_get(motif_index, motifs);
    char* motif_id = get_motif_st_id(motif);
    char* bare_motif_id = get_motif_id(motif);

    if ((get_num_strings(options->selected_motifs) == 0)
      || (have_string(bare_motif_id, options->selected_motifs) == TRUE)) {

      if (verbosity >= NORMAL_VERBOSE) {
        fprintf(
          stderr,
          "Using motif %s of width %d.\n",
          motif_id,
          get_motif_length(motif)
        );
      }

      // Build PSSM for motif and tables for p-value calculation.
      // FIXME: the non-averaged freqs should be used for p-values
      PSSM_T* pos_pssm 
        = build_motif_pssm(
            motif, 
            bg_freqs, 
            bg_freqs, 
            prior_dist, 
            options->alpha,
            PSSM_RANGE, 
            0,    // no GC bins
            FALSE // make log-likelihood pssm
          );

      if (options->pval_lookup_filename != NULL) {
        // Print the cumulative lookup table.
        print_array(pos_pssm->pv, 8, 6, TRUE, pval_lookup_file);
        // Also print the non-cumulative version.
        int i;
        for (i = 1; i < get_array_length(pos_pssm->pv); i++) {
          fprintf(pval_lookup_file, "%g ",
          get_array_item(i-1, pos_pssm->pv) - get_array_item(i, pos_pssm->pv));
        }
        fprintf(pval_lookup_file, "\n");
      }

      // If required, do the same for the reverse complement motif.
      MOTIF_T* rev_motif = NULL;
      PSSM_T* rev_pssm = NULL;
      if (options->scan_both_strands) {
        ++motif_index;
        rev_motif = (MOTIF_T *) arraylst_get(motif_index, motifs);
        motif_id = get_motif_st_id(rev_motif);
        // FIXME: the non-averaged freqs should be used for p-values
        rev_pssm 
          = build_motif_pssm(
              rev_motif, 
              bg_freqs, 
              bg_freqs, 
              prior_dist, 
              options->alpha,
              PSSM_RANGE, 
              0, // GC bins
              FALSE
            );
        if (verbosity >= NORMAL_VERBOSE) {
          fprintf(
            stderr,
            "Using motif %s of width %d.\n",
            motif_id,
            get_motif_length(motif)
          );
        }
      }

      char strand = (options->alphabet == PROTEIN_ALPH ? '.' : '+');
      *num_scanned_sequences = 0;

      // Create cisml pattern for this motif.
      PATTERN_T* pattern = NULL;
      if (!options->text_only) {
        pattern = allocate_pattern(bare_motif_id, bare_motif_id);
        set_pattern_max_stored_matches(pattern, options->max_stored_scores);
        set_pattern_max_pvalue_retained(pattern, options->output_threshold);
      }

      // Read the FASTA file one sequence at a time.
      while (
        fasta_reader->go_to_next_sequence(fasta_reader) != FALSE
      ) {

        *num_scanned_positions += fimo_score_sequence(
          options,
          reservoir,
          strand,
          fasta_reader,
          psp_reader,
          motif,
          rev_motif,
          bg_freqs,
          pos_pssm,
          rev_pssm,
          pattern
        );
        ++(*num_scanned_sequences);

      }  // All sequences parsed

      // The pattern is complete.
      if (!options->text_only) {
        set_pattern_is_complete(pattern);
      }

      // Compute q-values, if requested.
      if (options->compute_qvalues) {
        int num_samples = get_reservoir_num_samples_retained(reservoir);
        ARRAY_T *sampled_pvalues = allocate_array(num_samples);
        fill_array(get_reservoir_samples(reservoir), sampled_pvalues);
        pattern_calculate_qvalues(pattern, sampled_pvalues);
        free_array(sampled_pvalues);
      }

      if (!options->text_only) {
        print_cisml_start_pattern(cisml, cisml_file, pattern);
        SCANNED_SEQUENCE_T **scanned_sequences
          = get_pattern_scanned_sequences(pattern);
        print_cisml_scanned_sequences(
          cisml,
          cisml_file,
          *num_scanned_sequences,
          scanned_sequences
        );
        print_cisml_end_pattern(cisml_file);
        double max_pvalue = get_pattern_max_pvalue_retained(pattern);
        if (max_pvalue < options->output_threshold) {
          // Record info for patterns that have dropped matches
          add_string_with_score(bare_motif_id, incomplete_motifs, max_pvalue);
        }
        free_pattern(pattern);
      }

      // Reset to start of sequence data.
      fasta_reader->reset(fasta_reader);
      if (psp_reader) {
         psp_reader->reset(psp_reader);
      }

      if (!options->text_only) {
        clear_reservoir(reservoir);
      }

      // Free memory associated with this motif.
      free_pssm(pos_pssm);
      free_pssm(rev_pssm);

    }
    else {
      if (verbosity >= NORMAL_VERBOSE) {
        fprintf(stderr, "Skipping motif %s.\n", motif_id);
      }
    }

  } // All motifs parsed

  if (reservoir != NULL) {
    free_reservoir(reservoir);
  }

}

static void print_site_as_text(FIMO_MATCH_T *match) {
  fprintf(
    stdout, 
    "%s\t%s\t%d\t%d\t%c\t%g\t%.3g\t\t%s\n", 
    match->motif_name,
    match->seq_name,
    match->stop > match->start ? match->start : match->stop,
    match->stop > match->start ? match->stop : match->start,
    match->strand,
    match->score,
    match->pvalue,
    match->raw_seq
  );
}

/***********************************************************************
 * Print FIMO settings information to an XML file
 ***********************************************************************/
static  void print_settings_xml(
  FILE *out,
  FIMO_OPTIONS_T *options
) {

  fputs("<settings>\n",  out);
  fprintf(out, "<setting name=\"%s\">%s</setting>\n", "output directory", options->output_dirname);
  fprintf(out, "<setting name=\"%s\">%s</setting>\n", "MEME file name", options->meme_filename);
  fprintf(out, "<setting name=\"%s\">%s</setting>\n", "sequence file name", options->seq_filename);
  if (options->bg_filename) {
    fprintf(out, "<setting name=\"%s\">%s</setting>\n", "background file name", options->bg_filename);
  }
  fprintf(
    out,
    "<setting name=\"%s\">%s</setting>\n",
    "allow clobber",
    boolean_to_string(options->allow_clobber)
  );
  fprintf(
    out,
    "<setting name=\"%s\">%s</setting>\n",
    "compute q-values",
    boolean_to_string(options->compute_qvalues)
  );
  fprintf(
    out,
    "<setting name=\"%s\">%s</setting>\n",
    "parse genomic coord.",
    boolean_to_string(options->parse_genomic_coord)
  );
  fprintf(
    out,
    "<setting name=\"%s\">%s</setting>\n",
    "text only",
    boolean_to_string(options->text_only)
  );
  fprintf(
    out,
    "<setting name=\"%s\">%s</setting>\n",
    "scan both strands",
    boolean_to_string(options->scan_both_strands)
  );
  fprintf(
    out,
    "<setting name=\"%s\">%d</setting>\n",
    "max sequence length",
    options->max_seq_length
  );
  fprintf(
    out,
    "<setting name=\"%s\">%3.2g</setting>\n",
    "output threshold",
    options->output_threshold);
  fprintf(
    out,
    "<setting name=\"%s\">%s</setting>\n",
    "threshold type",
    threshold_type_to_string(options->threshold_type)
  );
  fprintf(
    out,
    "<setting name=\"%s\">%d</setting>\n",
    "max stored scores",
    options->max_stored_scores
  );
  fprintf(
    out,
    "<setting name=\"%s\">%3.2g</setting>\n",
    "pseudocount",
    options->pseudocount
  );
  fprintf(
    out,
    "<setting name=\"%s\">%d</setting>\n",
    "verbosity",
    verbosity
  );
  int i = 0;
  int num_strings = get_num_strings(options->selected_motifs);
  for(i = 0; i < num_strings; i++) {
    fprintf(
      out,
      "<setting name=\"%s\">%s</setting>\n", "selected motif",
      get_nth_string(i, options->selected_motifs)
    );
  }

  fputs("</settings>\n",  out);

}
/***********************************************************************
 * Print FIMO specific information to an XML file
 ***********************************************************************/
static void print_fimo_xml_file(
  FILE *out,
  FIMO_OPTIONS_T *options,
  ARRAYLST_T *motifs,
  STRING_LIST_T *incomplete_motifs,
  ARRAY_T *bgfreq,
  char  *stylesheet,
  int num_seqs,
  long num_residues
) {

  fputs("<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"no\"?>\n", out);
  if (stylesheet != NULL) {
    fprintf(out, "<?xml-stylesheet type=\"text/xsl\" href=\"%s\"?>\n", stylesheet);
  }
  fputs("<!-- Begin document body -->\n", out);
  fputs("<fimo version=\"" VERSION "\" release=\"" ARCHIVE_DATE "\">\n", out);
  fputs("  xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\"", out);
  fputs("\n", out);
  fputs("  xsi:schemaLocation=", out);
  fputs("  xmlns:fimo=\"http://noble.gs.washington.edu/schema/fimo\"\n>\n", out);
  fprintf(out, "<command-line>%s</command-line>\n", options->command_line);
  print_settings_xml(out, options);
  fprintf(
    out, 
    "<sequence-data num-sequences=\"%d\" num-residues=\"%ld\" />\n", 
    num_seqs,
    num_residues
  );
  fprintf(
    out,
    "<alphabet>%s</alphabet>\n",
    options->alphabet == DNA_ALPH ? "nucleotide" : "protein"
  );
  int i;
  int num_motifs = arraylst_size(motifs);
  for (i = 0; i < num_motifs; i++) {
    MOTIF_T *motif = arraylst_get(i, motifs);
    char *bare_motif_id = get_motif_id(motif);
    char *best_possible_match = get_best_possible_match(motif);
    fprintf(
      out,
      "<motif name=\"%s\" width=\"%d\" best-possible-match=\"%s\"/>\n",
      bare_motif_id,
      get_motif_length(motif),
      best_possible_match
    );
    if (options->scan_both_strands == TRUE) {
      // Skip RC motif
      ++i;
    }
    myfree(best_possible_match);
  }
  fprintf(
    out,
    "<background source=\"%s\">\n",
    options->bg_filename ? options->bg_filename : "non-redundant database"
  );
  int asize = alph_size(options->alphabet, ALPH_SIZE);
  for (i = 0; i < asize; i++) {
    fprintf(
      out,
      "<value letter=\"%c\">%1.3f</value>\n",
      alph_char(options->alphabet, i),
      get_array_item(i, bgfreq)
    );
  }
  fputs("</background>\n", out);
  int num_incomplete_motifs = get_num_strings(incomplete_motifs);
    if (num_incomplete_motifs > 0) {
    fputs("<incomplete-motifs>\n", out);
    int motif_idx;
    for(motif_idx = 0; motif_idx < num_incomplete_motifs; ++motif_idx) {
      char *name = get_nth_string(motif_idx, incomplete_motifs);
      double max_pvalue = get_nth_score(motif_idx, incomplete_motifs);
      fprintf(out, "<incomplete-motif name=\"%s\" max-pvalue=\"%3.2g\"/>\n", name, max_pvalue);
    }
    fputs("</incomplete-motifs>\n", out);
  }
  fputs("<cisml-file>cisml.xml</cisml-file>\n", out);
  fputs("</fimo>\n", out);
}

/**********************************************************************
 * This function saves CisML results as a set of files in a
 * directory. The file names are provided by the input parameters:
 *   html_filename will be the name of the HTML output
 *   text_filename will be the name of the plain text output
 *   gff_filename will be the name of the GFF output
 *   allow_clobber will determine whether or not existing files will
 *                 be overwritten.
 *********************************************************************/
void create_output_files(
  ARRAY_T *bgfreqs,
  FIMO_OPTIONS_T *options,
  ARRAYLST_T *motifs,
  STRING_LIST_T *incomplete_motifs,
  int num_seqs,
  long num_residues
) {

  // Copy XML to HTML and CSS stylesheets to output directory
  copy_file(options->html_stylesheet_path, options->html_stylesheet_local_path);
  copy_file(options->css_stylesheet_path, options->css_stylesheet_local_path);


  FILE *fimo_file = fopen(options->fimo_path, "w");
  if (!fimo_file) {
    die("Couldn't open file %s for output.\n", options->fimo_path);
  }
  print_fimo_xml_file(
    fimo_file,
    options,
    motifs,
    incomplete_motifs,
    bgfreqs,
    "fimo-to-html.xsl",
    num_seqs,
    num_residues
  );
  fclose(fimo_file);

  // Output HTML
  print_xml_filename_to_filename_using_stylesheet(
    options->fimo_path,
    options->html_stylesheet_local_path,
    options->html_path
  );

  // Output text
  print_xml_filename_to_filename_using_stylesheet(
    options->fimo_path,
    options->text_stylesheet_path,
    options->text_path
  );

  // Output GFF
  print_xml_filename_to_filename_using_stylesheet(
    options->fimo_path,
    options->gff_stylesheet_path,
    options->gff_path
  );

}

/*************************************************************************
 * Entry point for fimo
 *************************************************************************/
int main(int argc, char *argv[]) {

  FIMO_OPTIONS_T options;

  // Get command line arguments
  process_command_line(argc, argv, &options);

  // Create cisml data structure for recording results
  CISML_T *cisml = NULL;
  if (!options.text_only) {
    cisml = allocate_cisml("fimo", options.meme_filename, options.seq_filename);
    if (options.threshold_type == PV_THRESH) {
      set_cisml_site_pvalue_cutoff(cisml, options.output_threshold);
    }
    else if (options.threshold_type == QV_THRESH) {
      set_cisml_site_qvalue_cutoff(cisml, options.output_threshold);
    }
  }

  // Set up motif input
  ARRAYLST_T *motifs = NULL;
  ARRAY_T *bg_freqs = NULL;
  int num_motif_names = 0;
  fimo_read_motifs(&options, &num_motif_names, &motifs, &bg_freqs);

  // Set up sequence input
  DATA_BLOCK_READER_T *fasta_reader = new_seq_reader_from_fasta(
        options.parse_genomic_coord,
        options.alphabet, 
        options.seq_filename
      );

  // Set up priors input
  DATA_BLOCK_READER_T *psp_reader = NULL;
  PRIOR_DIST_T *prior_dist = NULL;
  if (options.psp_filename != NULL && options.prior_distribution_filename) {
    // If suffix for file is '.wig' assume it's a wiggle file
    size_t len = strlen(options.psp_filename);
    if (strcmp(options.psp_filename + len - 4, ".wig") == 0) {
      psp_reader = new_prior_reader_from_wig(options.psp_filename);
    }
    else {
      psp_reader = new_prior_reader_from_psp(
          options.parse_genomic_coord,
          options.psp_filename
        );
    }
    prior_dist = new_prior_dist(options.prior_distribution_filename);
  }

  // Set up output files
  FILE *cisml_file = NULL;

  // Open the output file for the p-value lookup table, if requested.
  FILE* pval_lookup_file = NULL;
  if (options.pval_lookup_filename != NULL) {
    if (open_file(options.pval_lookup_filename, "w", FALSE,
      "p-value lookup table",
      "p-value lookup table", &pval_lookup_file) == 0) {
      exit(EXIT_FAILURE);
    }
  }

  // Print the text-only header line.
  if (options.text_only) {
    fprintf(stdout, "#pattern name");
    fprintf(stdout, "\tsequence name");
    fprintf(stdout, "\tstart");
    fprintf(stdout, "\tstop");
    fprintf(stdout, "\tstrand");
    fprintf(stdout, "\tscore");
    fprintf(stdout, "\tp-value");
    fprintf(stdout, "\tq-value");
    fprintf(stdout, "\tmatched sequence\n");
  }
  else {
    // Create output directory
    if (create_output_directory(
         options.output_dirname,
         options.allow_clobber,
         FALSE /* Don't print warning messages */
        )
      ) {
      // Failed to create output directory.
      die("Couldn't create output directory %s.\n", options.output_dirname);
    }
    // Open the CisML XML file for output
    cisml_file = fopen(options.cisml_path, "w");
    if (!cisml_file) {
      die("Couldn't open file %s for output.\n", options.cisml_path);
    }

    // Print the opening section of the CisML XML
    print_cisml_start(
      cisml_file,
      program_name,
      TRUE, // print header
      options.HTML_STYLESHEET, // Name of HTML XSLT stylesheet
      TRUE // print namespace in XML header
    );

    // Print the parameters section of the CisML XML
    print_cisml_parameters(cisml_file, cisml);
  }

  int num_scanned_sequences = 0;
  long num_scanned_positions = 0;

  // We record a list of  motifs for which we've dropped matches.
  STRING_LIST_T *incomplete_motifs = new_string_list();

  fimo_score_each_motif(
    &options, 
    bg_freqs, 
    motifs, 
    cisml,
    cisml_file,
    prior_dist,
    fasta_reader,
    psp_reader,
    pval_lookup_file,
    incomplete_motifs,
    &num_scanned_sequences,
    &num_scanned_positions
  );

  if (options.text_only == FALSE) {
    // print cisml end
    print_cisml_end(cisml_file);
    fclose(cisml_file);
    // Output other file formats
    create_output_files(
      bg_freqs,
      &options,
      motifs,
      incomplete_motifs,
      num_scanned_sequences,
      num_scanned_positions
    );
    free_cisml(cisml);
  }

  // Clean up.

  // Close input readers
  fasta_reader->close(fasta_reader);
  free_data_block_reader(fasta_reader);
  if (psp_reader != NULL) {
    psp_reader->close(psp_reader);
    free_data_block_reader(psp_reader);
  }

  if (prior_dist != NULL) {
    free_prior_dist(prior_dist);
  }

  free_motifs(motifs);
  free_array(bg_freqs);
  free_string_list(incomplete_motifs);
  free_string_list(options.selected_motifs);
  cleanup_options(&options);

  return 0;

}

