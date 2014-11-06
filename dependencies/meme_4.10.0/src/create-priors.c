/********************************************************************
 * FILE: create-prior.c
 * AUTHOR: Charles E. Grant, William Stafford Noble, Timothy Bailey
 * PROJECT: MEME suite
 * COPYRIGHT: 2011 UW
 ********************************************************************/

#include <errno.h>
#include <float.h>
#include <stdio.h>

#include "../config.h"
#include "io.h"
#include "pssm.h"
#include "reservoir.h"
#include "simple-getopt.h"
#include "utils.h"
#include "wiggle-reader.h"

#define WIGGLE_MAX_CHROM_NAME 500

char* program_name = "create-priors";
VERBOSE_T verbosity = NORMAL_VERBOSE;

// create-prior command line parameters.
typedef struct options {

  BOOLEAN_T allow_clobber;
  double alpha;
  double beta;
  size_t seq_size;
  size_t num_bins;
  char *command_line;
  char *output_dirname;
  BOOLEAN_T output_bigwig;
  BOOLEAN_T output_psp;
  BOOLEAN_T output_version;
  char *usage;
  char *wiggle_filename;

} CREATEPRIORS_OPTIONS_T;

/***********************************************************************
  Process command line options
 ***********************************************************************/
static void process_command_line(
  int argc,
  char* argv[],
  CREATEPRIORS_OPTIONS_T *options
) {

  // Define command line options.
  cmdoption const createpriors_options[] = {
    {"alpha", REQUIRED_VALUE},
    {"beta", REQUIRED_VALUE},
    {"bigiwg", NO_VALUE},
    {"seq-size", REQUIRED_VALUE},
    {"num-bins", REQUIRED_VALUE},
    {"o", REQUIRED_VALUE},
    {"oc", REQUIRED_VALUE},
    {"psp", NO_VALUE},
    {"verbosity", REQUIRED_VALUE},
    {"version", NO_VALUE}
  };

  const int num_options = sizeof(createpriors_options) / sizeof(cmdoption);
  int option_index = 0;

  // Define the usage message.
  options->usage =
    "USAGE: create-priors [options] --seq-size <size> <wiggle file>\n"
    "\n"
    "   Options:\n"
    "     --alpha <double> (default 1.0)\n"
    "     --beta <double> (default 10000.0)\n"
    "     --bigwig\n"
    "     --num-bins <int> (default 100)\n"
    "     --o <output dir> (default=createpriors_out)\n"
    "     --oc <output dir> (default=createpriors_out)\n"
    "     --psp\n"
    "     --verbosity [1|2|3|4] (default 2)\n"
    "     --version\n"
    "\n";

  // Set options to NULL or defaults.
  options->allow_clobber = TRUE;
  options->alpha = 1.0;
  options->beta = 10000.0;
  options->seq_size = -1;
  options->num_bins = 100;
  options->output_dirname = "create-priors_out";
  options->output_bigwig = FALSE;
  options->output_psp = FALSE;
  options->output_version = FALSE;
  verbosity = NORMAL_VERBOSE;

  simple_setopt(argc, argv, num_options, createpriors_options);

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
    if (strcmp(option_name, "alpha") == 0) {
      options->alpha = strtod(option_value, NULL);
    }
    else if (strcmp(option_name, "beta") == 0) {
      options->beta = strtod(option_value, NULL);
    }
    else if (strcmp(option_name, "bigwig") == 0){
      options->output_bigwig = TRUE;
    }
    else if (strcmp(option_name, "seq-size") == 0){
      options->seq_size = strtol(option_value, NULL, 10);
    }
    else if (strcmp(option_name, "num-bins") == 0) {
      options->num_bins = strtol(option_value, NULL, 10);
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
    else if (strcmp(option_name, "psp") == 0){
      options->output_psp = TRUE;
    }
    else if (strcmp(option_name, "verbosity") == 0){
      verbosity = strtod(option_value, NULL);
    }
    else if (strcmp(option_name, "version") == 0){
      options->output_version = TRUE;
    }
  }

  if (options->seq_size <= 0) {
    die(
      "The estimated sequence size must be set to a\n"
      "value greater than 0 using the --seq-size flag\n"
    );
  }

  // Record the command line
  options->command_line = get_command_line(argc, argv);

  // Record the input file names
  options->wiggle_filename = argv[option_index];
  option_index++;

}

static double score_to_prior_transform(
  double y_i,
  double y_min,
  double y_max,
  size_t n,
  double beta
) {
  y_min = 0.0; // Deliberately overriding y_min
  double a = (beta - 1.0) / (n - 1);
  return (y_i - y_min) * (1.0 - a) / (y_max - y_min) + a;
}

static double sum_priors(
  WIGGLE_READER_T *reader,
  double y_min,
  double y_max,
  double y_median,
  size_t covered_sequence,
  size_t seq_size,
  double beta
) {

  char *chrom = NULL;
  size_t start = 0;
  size_t step = 0;
  size_t span = 0;
  size_t chrom_max_position = 0;
  double value = NaN();
  double prior_sum = 0.0;

  reset_wiggle_reader(reader);
  INPUT_FMT_T format = INVALID_FMT;

  // Loop over all sequences
  while (go_to_next_sequence_in_wiggle(reader) == TRUE) {

    // Loop over all positions in the sequence
    while (
      get_next_data_line_from_wiggle(
        reader,
        &chrom,
        &start,
        &step,
        &span,
        &value
      ) == TRUE
    ) {
      double prior = score_to_prior_transform(value, y_min, y_max, seq_size, beta);
      prior_sum += (span * prior);
    }
  }

  // Adjust for positions with no data
  double median_prior = score_to_prior_transform(
    y_median, 
    y_min,
    y_max,
    seq_size,
    beta
  );
  prior_sum += (seq_size - covered_sequence) * median_prior;

  return prior_sum;
}

static double score_sum_to_prior_sum(
  double score_sum,
  double y_min,
  double y_max,
  double y_median,
  size_t covered_sequence,
  size_t seq_size,
  double beta
) {

  y_min = 0.0; // Deliberately overriding y_min
  double a = (beta - 1.0) / (seq_size - 1);

  double prior_sum = score_to_prior_transform(
    score_sum, 
    y_min, 
    y_max, 
    seq_size,
    beta
  );
  // The score_to_prior_transform() is actually affine
  // not linear, so we need to add (n - 1) copies
  // of the constant a and subtract (n - 1) copies of
  // the y_min terms. 
  prior_sum += ((covered_sequence - 1) * a);
  prior_sum -= ((covered_sequence - 1) * (1 - a) * y_min) / (y_max - y_min);

  // Adjust for positions with no data
  double median_prior = score_to_prior_transform(
    y_median, 
    y_min,
    y_max,
    seq_size,
    beta
  );
  prior_sum += (seq_size - covered_sequence) * median_prior;

  return prior_sum;
}

static void get_score_stats(
  WIGGLE_READER_T *reader,
  double* min_value,   // Smallest value in wiggle file, OUT
  double* max_value,   // Largest value in wiggile file, OUT
  double* median_value,// Median of values in wiggle file, OUT
  double* score_sum, // Sum of scores
  size_t* seq_covered  // Number of postions covered by wiggle file, OUT
) {

  *min_value = DBL_MAX;
  *max_value = -DBL_MAX;
  *seq_covered = 0;

  char *chrom = NULL;
  size_t start = 0;
  size_t step = 0;
  size_t span = 0;
  size_t chrom_max_position = 0;
  double value = NaN();

  const size_t MAX_SAMPLES = 10000;

  RESERVOIR_SAMPLER_T *sampler = new_reservoir_sampler(MAX_SAMPLES, NULL);
  
  // Loop over all sequences
  while (go_to_next_sequence_in_wiggle(reader) == TRUE) {

    // Loop over all positions in the sequence
    while (
      get_next_data_line_from_wiggle(
        reader,
        &chrom,
        &start,
        &step,
        &span,
        &value
      ) == TRUE
    ) {

      // Track min and max wiggle values
      if (value <= *min_value) {
        *min_value = value;
      }
      if (value >= *max_value) {
        *max_value = value;
      }
      *seq_covered += span;
      *score_sum += (span * value);
      int i;
      for (i = 0; i < span; ++i) {
        // Sample the score at each position in the span
        reservoir_sample(sampler, value);
      }

    }
  }

  size_t num_samples = get_reservoir_num_samples_retained(sampler);
  double *sampled_scores = get_reservoir_samples(sampler);
  ARRAY_T *sampled_score_array = allocate_array(num_samples);
  fill_array(sampled_scores, sampled_score_array);
  *median_value = compute_median(sampled_score_array);
  free_array(sampled_score_array);
  free_reservoir(sampler);

}

static void write_prior_dist(
  const char *output_dir,
  size_t seq_length,
  double min_prior,
  double max_prior,
  double median_prior,
  double *prior_dist,
  size_t num_bins
  ) {

  char* prior_dist_path = make_path_to_file(output_dir, "priors.dist");
  errno = 0;
  FILE *dist_out = fopen(prior_dist_path, "wb");
  if (!dist_out) {
    die("Can't open %s to write: %s\n", prior_dist_path, strerror(errno));
  }
  fprintf(dist_out, "%g\n", min_prior);
  fprintf(dist_out, "%g\n", max_prior);
  fprintf(dist_out, "%g\n", median_prior);
  size_t dist_index = 0;
  double sum = 0.0;
  for (dist_index = 0; dist_index < num_bins; ++dist_index) {
    // Print normalized bin counts
    prior_dist[dist_index] /= seq_length;
    sum += prior_dist[dist_index];
    fprintf(dist_out, "%g\n", prior_dist[dist_index]);
  }
  fprintf(stderr, "Sum of prior distribution = %g\n", sum);
  fclose(dist_out);
  myfree(prior_dist_path);
}

static void create_priors(
  WIGGLE_READER_T *reader,
  char* output_dir, 
  double min_value, 
  double max_value, 
  double median_value,
  size_t seq_length, 
  size_t num_bins,
  double sum_priors,
  double alpha,
  double beta,
  BOOLEAN_T produce_psp
) {

  int pos; //position in the chromosome
  double raw_value = 0.0;
  double prior = 0.0;
  double normalized_prior = 0.0;

  // Setup for recording prior distribution
  double *prior_dist = mm_calloc(num_bins, sizeof(double));
  double min_prior = score_to_prior_transform(min_value, min_value, max_value, seq_length, beta);
  double min_normalized_prior = (min_prior * beta) / sum_priors;
  double max_prior = score_to_prior_transform(max_value, min_value, max_value, seq_length, beta);
  double max_normalized_prior = (max_prior * beta) / sum_priors;
  double median_prior = score_to_prior_transform(median_value, min_value, max_value, seq_length, beta);
  double median_normalized_prior = (median_prior * beta) / sum_priors;
  double scale = (num_bins - 1) / (max_normalized_prior - min_normalized_prior);
  double offset = min_normalized_prior;

  // Open wiggle output file
  char* wiggle_path = make_path_to_file(output_dir, "priors.wig");
  errno = 0;
  FILE *wig_out = fopen(wiggle_path, "wb");
  if (!wig_out) {
    die("Can't open %s to write: %s\n", wiggle_path, strerror(errno));
  }

  // Open PSP wiggle output file
  char* psp_path = make_path_to_file(output_dir, "create-priors.psp");
  FILE *psp_out = NULL;
  if (produce_psp == TRUE) {
    errno = 0;
    psp_out = fopen(psp_path, "wb");
    if (!psp_out) {
      die("Can't open %s to write: %s\n", psp_path, strerror(errno));
    }
  }

  char *chrom = NULL;
  char *prev_chrom = NULL;
  size_t num_priors = 0;
  size_t start = 0;
  size_t step = 0;
  size_t span = 0;
  double value = NaN();
  double sum_normalized_priors = 0.0;
  
  reset_wiggle_reader(reader);
  INPUT_FMT_T format = INVALID_FMT;

  // Loop over all sequences
  while (go_to_next_sequence_in_wiggle(reader) == TRUE) {

    chrom = get_wiggle_seq_name(reader);
    span = get_wiggle_span(reader);
    step = get_wiggle_step(reader);

    // Print wiggle format line
    format = get_wiggle_format(reader);
    if (format == VAR_WIGGLE_FMT) {
      fprintf(wig_out, "variableStep chrom=%s", chrom);
      if (span != WIGGLE_DEFAULT_SPAN) {
        fprintf(wig_out, " span=%zd", span);
      }
      fprintf(wig_out, "\n");
    }
    else if (get_wiggle_format(reader) == FIXED_WIGGLE_FMT) {
      fprintf(wig_out, "fixedStep chrom=%s start=%zd step=%zd", chrom, start, step);
      if (span != WIGGLE_DEFAULT_SPAN) {
        fprintf(wig_out, " span=%zd", span);
      }
      fprintf(wig_out, "\n");
    }
    else {
      die("Found unrecongized wiggle format when reading priors\n");
    }

    if (produce_psp == TRUE) {
      // PSP requires a prior for each position so
      // if step given it has to equal the span have
      if (step && step != span) {
        die(
          "Creating PSP output requires that if step is specified in the input file\n"
          "it must equal the span, but found step = %zd and span = %zd\n",
          step,
          span
        );
      }
      fprintf(psp_out, ">%s\n", chrom);
    }

    // Loop over all positions in the sequence
    while (
      get_next_data_line_from_wiggle(
        reader,
        &chrom,
        &start,
        &step,
        &span,
        &value
      ) == TRUE
    ) {

      prior = score_to_prior_transform(value, min_value, max_value, seq_length, beta);
      normalized_prior = (prior * beta) / sum_priors;
      sum_normalized_priors += (span * normalized_prior);

      // Update prior distribution
      size_t dist_index = raw_to_scaled(normalized_prior, 1, scale, offset);
      prior_dist[dist_index] += span;
      num_priors += span;

      if (format == VAR_WIGGLE_FMT) {
        fprintf(wig_out, "%zd %g\n", start, normalized_prior);
      }
      else if (get_wiggle_format(reader) == FIXED_WIGGLE_FMT) {
        fprintf(wig_out, "%g\n", normalized_prior);
      }
      else {
        die("Found unrecongized wiggle format when writing priors\n");
      }

      if (produce_psp == TRUE) {
        // PSP requires a prior for each position so repeatedly
        // print the prior for each position in the span
        int span_idx;
        for (span_idx = 0; span_idx < span; ++span_idx) {
          fprintf(psp_out, "%0.10lf ", normalized_prior);
        }
        fputc('\n', psp_out);
      }

    }

  }

  // Update prior distribution for missing data
  // assign median prior to all missing positions
  size_t dist_index = raw_to_scaled(median_normalized_prior, 1, scale, offset);
  size_t num_missing = seq_length - num_priors;
  if (num_missing) {
    prior_dist[dist_index] += num_missing;
    num_priors += num_missing;
    sum_normalized_priors += (num_missing * median_normalized_prior);
  }

  fprintf(stderr, "Sum of normalized priors = %g\n", sum_normalized_priors);


  fclose(wig_out);
  if (psp_out) {
    fclose(psp_out);
  }
  myfree(wiggle_path);
  myfree(psp_path);
  
  // Write out prior distribution
  write_prior_dist(
    output_dir,
    seq_length,
    min_normalized_prior,
    max_normalized_prior,
    median_normalized_prior,
    prior_dist,
    num_bins
  );

}

/*************************************************************************
 * Entry point for create-priors
 *************************************************************************/
int main(int argc, char *argv[]) {

  CREATEPRIORS_OPTIONS_T options;

  /**********************************************
   * COMMAND LINE PROCESSING
   **********************************************/
  process_command_line(argc, argv, &options);

  if (options.output_version) {
    fprintf(stderr, "create-priors %s\n", PACKAGE_VERSION);
    return 0;
  }

  // Must have wiggle file name
  if (!options.wiggle_filename) {
    fprintf(stderr, "%s", options.usage);
    return EXIT_FAILURE;
  }

  fprintf(
    stderr, 
    "Creating priors with alpha=%3.1f and beta=%.0f\n", 
    options.alpha,
    options.beta
  );

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

  // Open the wiggle file as a wiggle file reader
  WIGGLE_READER_T *reader = new_wiggle_reader(options.wiggle_filename);

  double min_value = NaN();
  double max_value = NaN();
  double median_value = NaN();
  double score_sum = 0.0;
  size_t seq_covered = 0;
  size_t step_size = 0;

  // To calculate the prior_sum we need range information and an estimate of
  // the size of the full sequences.
  get_score_stats(
    reader, 
    &min_value, 
    &max_value,
    &median_value,
    &score_sum,
    &seq_covered
  );

  if (max_value <= 0.0) {
    die("The maximum observed score was 0.0. Unable to compute priors\n");
  }

  // Convert the sum of scores into the sum of priors
  double prior_sum = 
    score_sum_to_prior_sum(
      score_sum,
      min_value,
      max_value,
      median_value,
      seq_covered,
      options.seq_size,
      options.beta
    );

  fprintf(
    stderr,
    "Sequence size is %zd bases.\n"
    "Scores were observed for %zd bases.\n"
    "Minimum score is %g\n"
    "Median score is %g\n"
    "Maximum score is %g\n"
    "Sum of scores is %g\n"
    "Sum of un-normalized priors is %g\n",
    options.seq_size,
    seq_covered,
    min_value,
    median_value,
    max_value,
    score_sum,
    prior_sum
  );

  create_priors(
    reader, 
    options.output_dirname, 
    min_value, 
    max_value, 
    median_value,
    options.seq_size, 
    options.num_bins,
    prior_sum,
    options.alpha, 
    options.beta, 
    options.output_psp
  );

  free_wiggle_reader(reader);
  myfree(options.command_line);

  return 0;
}
