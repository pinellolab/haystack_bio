/********************************************************************
 * FILE: ramen.c
 * AUTHOR: Robert McLeay
 * CREATE DATE: 19/08/2008
 * PROJECT: MEME suite
 * COPYRIGHT: 2008, Robert McLeay
 *
 * RAMEN seeks to assist in
 * determining whether a given transcription factor regulates a set
 * of genes that have been found by an experimental method that
 * provides ranks, such as microarray or ChIp-chip.
 *
 * See McLeay and Bailey, 2010. 
 *
 ********************************************************************/

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <float.h>
#include <math.h>
#include <ctype.h>
#include <time.h>
#include <sys/stat.h>
#include "matrix.h"
#include "alphabet.h"
#include "cisml.h"
#include "fasta-io.h"
#include "string-list.h"
#include "simple-getopt.h"
#include "ranksum_test.h"
#include "motif.h"
#include "motif-in.h"
#include "array.h"
#include "macros.h"
#include "motif-in.h"
#include "matrix.h"
#include "regress.h"

#include "ramen.h"
#include "ramen_scan.h"

#include "fisher_exact.h"

VERBOSE_T verbosity = NORMAL_VERBOSE;

/*
 * Global Variables
 */

ALPH_T ramen_alph = DNA_ALPH;
ramen_motifs_t motifs;

time_t  t0, t1; /* measuring time */
clock_t c0, c1; /* measuring cpu_time */

ramen_result_t** rsr; //Our results in global scope.

double** results;
STRING_LIST_T* seq_ids;
ARRAY_T* seq_fscores;
double* factorial_array;

/*
 *  Initialise parameters.
 */

ramen_arg_t args;
ramen_arg_t default_args;
void ramen_set_defaults() {
  default_args.bg_format = MOTIF_BG;
  default_args.motif_format = MEME_FORMAT;
  default_args.pvalue_cutoff = 0.05;
  default_args.pseudocount = 0.25;
  default_args.repeats = 10000;
  default_args.verbose = NORMAL_VERBOSE;
  default_args.log_pwmscores = FALSE;
  default_args.log_fscores = TRUE;
  default_args.linreg_normalise = TRUE;
  default_args.linreg_dump_dir = 0;
  default_args.linreg_switchxy = FALSE;
  //Now copy the defaults into the real args
  memcpy(&args, &default_args, sizeof(ramen_arg_t));
}

/*************************************************************************
 * Entry point for ramen
 *************************************************************************/
int main(int argc, char *argv[]) {

		/* Record the execution start and end times */
		t0 = time(NULL);
		c0 = clock();

		ramen_set_defaults(); //Set default cmd line args
		ramen_getopt(argc, argv); //Get command line args

		motifs.bg_freqs = ramen_load_background(); //Determine nucleotide freqs. to do odds
		ramen_load_motifs(); //Load the motifs from the file

		ramen_scan_sequences(); //Load each sequence, scanning with each motif to determine score
		ramen_get_scores(); //Do the significance tests to associate each motif with the set.

		ramen_print_results(); //Print out results

		ramen_terminate(0); //Successfully end.

		return 0; //shuts up a warning.
}


void ramen_getopt(int argc, char *argv[]) {
		const int num_options = 12;
		cmdoption const motif_scan_options[] = {
				{"bgfile", REQUIRED_VALUE},
				{"bgformat", REQUIRED_VALUE},
				{"repeats", REQUIRED_VALUE},
				{"pseudocount", REQUIRED_VALUE},
				{"pvalue-cutoff", REQUIRED_VALUE},
				{"verbose", REQUIRED_VALUE},
				{"linreg-dumpdir", REQUIRED_VALUE},
				{"linreg-switchxy", REQUIRED_VALUE},
				{"log-fscores", REQUIRED_VALUE},
				{"log-pwmscores", REQUIRED_VALUE},
				{"normalise-motifs", REQUIRED_VALUE},
				{"help", NO_VALUE},
		};

		int option_index = 0;
		char* option_name = NULL;
		char* option_value = NULL;
		const char * message = NULL;
		BOOLEAN_T bad_options = FALSE;
		int i;

		if (simple_setopt(argc, argv, num_options, motif_scan_options) != NO_ERROR) {
				die("Error processing command line options: option name too long.\n");
		}

		/*
		 * Now parse the command line options
		 */
		//simple_getopt will return 0 when there are no more options to parse
		while(simple_getopt(&option_name, &option_value, &option_index) > 0) {
				if (strcmp(option_name, "bgfile") == 0) {
						args.bg_filename = option_value;
				} else if (strcmp(option_name, "bgformat") == 0) {
						if (atoi(option_value)==MOTIF_BG) {
								args.bg_format = MOTIF_BG;
						} else if (atoi(option_value)==FILE_BG) {
								args.bg_format = FILE_BG;
						} else if (atoi(option_value)==UNIFORM_BG) {
								args.bg_format = UNIFORM_BG;
						} else {
								ramen_usage();
								ramen_terminate(1);
						}
				} else if (strcmp(option_name, "pvalue-cutoff") == 0) {
						args.pvalue_cutoff = atof(option_value);
				} else if (strcmp(option_name, "repeats") == 0) {
						args.repeats = atof(option_value);
				} else if (strcmp(option_name, "pseudocount") == 0) {
						args.pseudocount = atof(option_value);
				} else if (strcmp(option_name, "verbose") == 0) {
						args.verbose = atoi(option_value);
						verbosity = args.verbose;
						if (args.verbose <= INVALID_VERBOSE || args.verbose > DUMP_VERBOSE) {
								ramen_usage();
								ramen_terminate(1);
						}
				} else if (strcmp(option_name, "log-fscores") == 0) {
						if (strcmp(option_value,"on")==0) {
								args.log_fscores = TRUE;
						} else if (strcmp(option_value,"off")==0){
								args.log_fscores = FALSE;
      } else {
        ramen_usage();
        ramen_terminate(1);
      }
    } else if (strcmp(option_name, "log-pwmscores") == 0) {
      if (strcmp(option_value,"on")==0) {
        args.log_pwmscores = TRUE;
      } else if (strcmp(option_value,"off")==0){
        args.log_pwmscores = FALSE;
      } else {
        ramen_usage();
        ramen_terminate(1);
      }
    } else if (strcmp(option_name, "normalise-motifs") == 0) {
      if (strcmp(option_value,"on")==0) {
        args.linreg_normalise = TRUE;
      } else if (strcmp(option_value,"off")==0){
        args.linreg_normalise = FALSE;
      } else {
        ramen_usage();
        ramen_terminate(1);
      }
    } else if (strcmp(option_name, "linreg-dumpdir") == 0) {
      args.linreg_dump_dir = option_value;
    } else if (strcmp(option_name, "linreg-switchxy") == 0) {
      if (strcmp(option_value,"on")==0) {
        args.linreg_switchxy = TRUE;
      } else if (strcmp(option_value,"off")==0){
        args.linreg_switchxy = FALSE;
      } else {
        ramen_usage();
        ramen_terminate(1);
      }
    } else if (strcmp(option_name, "help") == 0) {
      printf("%s",ramen_get_usage()); //not to stderr
      ramen_terminate(0);
    } else {
      printf("Error: %s is not a recognised switch.\n", option_name);
      ramen_usage();
      ramen_terminate(1);
    }

    option_index++;
  }

  args.sequence_filename = argv[option_index];
  args.motif_filenames = &argv[option_index+1]; // FIXME: must now iterate until argc getting all motif DB filenames
  args.number_motif_files = argc-option_index-1;

  /* Now validate the options.
   *
   * Illegal combinations are:
   *   - sequence bg and no sequence
   *   - no motif
   *   - no sequences
   *   - each file exists.
   */
  if (args.motif_filenames == NULL) {
    fprintf(stderr, "Error: Motif file not specified.\n");
    bad_options = TRUE;
  } else {
        int i;
        for (i = 0; i < args.number_motif_files; i++) {
            if (!file_exists(args.motif_filenames[i])) {
                fprintf(stderr, "Error: Specified motif '%s' file does not exist.\n", args.motif_filenames[i]);
                bad_options = TRUE;
            }
        }
  }
  if (args.sequence_filename == NULL) {
    fprintf(stderr, "Error: Sequence file not specified.\n");
    bad_options = TRUE;
  } else if (!file_exists(args.sequence_filename)) {
    fprintf(stderr, "Error: Specified sequence file does not exist.\n");
    bad_options = TRUE;
  }

  if (args.bg_format == MOTIF_BG) { //bgfile is the same as the motif file.
    //TODO: make more robust 
    args.bg_filename = argv[option_index];
    args.bg_filename = NULL;
  }


  if (bad_options) {
    ramen_usage();
    ramen_terminate(1);
  }
}

const char* ramen_get_usage() {
  // Define the usage message.
  //TODO: sprintf in default values
  return
    "USAGE: ramen [options] <sequence file> <motif file>\n"
    "\n"
    "   Linear Regression Options:\n"
    "     --log-fscores [on|off] Regression on the log_e of the fluorescence scores\n"
    "             on: (Default) Use the log_e(fluorescence) in the regression.\n"
    "            off: Use the score directly provided in the sequence file.\n"
    "     --log-pwmscores [on|off] Regression on the log_e of the PWM scores\n"
    "             on: Use the log_e(RMA or AMA Score) in the regression.\n"
    "            off: (Default) Use the RMA/AMA score directly.\n"
    "     --normalise-motifs [on|off] Normalise the motif scores so that the motifs are comparable\n"
    "             on: (Default) Normalise motifs for comparison (Use RMA score).\n"
    "            off: Use raw AMA score (Not recommended).\n"
    "     --linreg-switchxy [on|on] Switch the x and y axis for the linear regression\n"
          "              on: y-points are PWM scores, x-values are fluorescence scores.\n"
          "             off: (Default) y-points are fluorescence scores, x-points are PWM scores.\n"
    "     --linreg-dumpdir <existing_directory> Dump (R-format) TSV files of each regression.\n"
    "\n"
    "   P-Value Simulation Options:\n"
    "     --repeats <integer> (default=10,000) Number of times to sample for p-value determination.\n"
    "     --pvalue-cutoff <float> (default=0.05) Only show results with p-value <= this cutoff\n"
    "\n"
    "   File format options:\n"
    "     --bgformat [0|2|3] source used to determine background frequencies\n"
    "                        0: uniform background\n"
    "                        1: MEME motif file\n"
    "                        2: Background file\n"
    "     --bgfile <background> file containing background frequencies\n"
    "     --motif-format [meme|tamo|regexp] format of input motif file (default meme)\n"
    "\n"
    "   Miscellaneous Options:\n"
    "     --pseudocount <float, default = 0.25> Pseudocount for motif affinity scan\n"
    "     --verbose     <1...5>                 Integer describing verbosity. Best used as first argument in list.\n"
    "     --help                                Show this message again\n"
    "\n"
    "   Citing ramen (Regression Analysis of Motif ENrichment):\n"
    "     If ramen is of use to you in your research, please cite:\n"
    "\n"
    "          Robert C. McLeay, Timothy L. Bailey (2009).\n"
    "          \"Motif Enrichment Analysis: a unified framework and an evaluation on ChIP data.\"\n"
    "          BMC Bioinformatics 2010, 11:165, doi:10.1186/1471-2105-11-165.\n"
    "\n"
    "   Contact the authors:\n"
    "     You can contact the authors via email:\n"
    "\n"
    "         Robert McLeay <r.mcleay@imb.uq.edu.au>, and\n"
    "         Timothy Bailey <t.bailey@imb.uq.edu.au>.\n"
    "\n"
    "     Bug reports should be directed to Robert McLeay.\n"
    "\n";
}

void ramen_usage() {
  fprintf(stderr, "%s", ramen_get_usage());
}

ARRAY_T* ramen_load_background() {
        ALPH_T alph = DNA_ALPH;
        ARRAY_T* freqs;

        switch (args.bg_format) {
        case UNIFORM_BG:
                freqs = get_uniform_frequencies(alph, NULL);
                break;
        case MOTIF_BG:
                                                //We've previously read in the freqs when we read the motif list
                freqs = motifs.bg_freqs;        //so we're just copying a pointer address so that we use them.
                break;
        case FILE_BG:
                freqs = get_file_frequencies(&alph, args.bg_filename, NULL);
                break;
        default:
                die("Illegal background option");
                freqs = NULL; // make compiler happy
        }
        return freqs;
}

void ramen_load_motifs() {
  BOOLEAN_T read_file = FALSE;
  MREAD_T *mread;
  ARRAYLST_T* read_motifs;
  int num_motifs_before_rc;
  int i;
  int j;

  memset(&motifs, 0, sizeof(ramen_motifs_t));
	read_motifs = arraylst_create();
  for (i = 0; i < args.number_motif_files; i++) {
      mread = mread_create(args.motif_filenames[i], OPEN_MFILE);
      if (args.bg_format == FILE_BG) {
		mread_set_bg_source(mread, args.bg_filename);
      } else {
		mread_set_background(mread, motifs.bg_freqs);
      }
      mread_set_pseudocount(mread, args.pseudocount);

      mread_load(mread, read_motifs);
      if (!(motifs.bg_freqs)) motifs.bg_freqs = mread_get_background(mread);

      mread_destroy(mread);
  }

  // reverse complement the originals adding to the original read in list
  num_motifs_before_rc = arraylst_size(read_motifs);
  add_reverse_complements(read_motifs);        
  motifs.num = arraylst_size(read_motifs);
  //Allocate array for the motifs
  motif_list_to_array(read_motifs, &(motifs.motifs), &(motifs.num));
  //free the list of motifs
  free_motifs(read_motifs);
  

  // check reverse complements.
  assert(motifs.num / 2 == num_motifs_before_rc);
  // reset motif count to before rev comp
  motifs.num = num_motifs_before_rc;

  //Now, we need to convert the motifs into odds matrices if we're doing that kind of scoring
  for (i=0;i<2*motifs.num;i++) {
	  convert_to_odds_matrix(motif_at(motifs.motifs, i), motifs.bg_freqs);
  }
}

void ramen_scan_sequences() {
		FILE* seq_file = NULL;
		MOTIF_T* motif = NULL;
		MOTIF_T* rev_motif = NULL;
		SEQ_T* sequence = NULL;
		SCANNED_SEQUENCE_T* scanned_seq = NULL;
		PATTERN_T* pattern;
		int i;
		int j;
		SEQ_T** seq_list;
		int num_seqs;
		int seq_len;
		//For the bdb_bg mode:
		ARRAY_T* seq_bg_freqs;
		double atcontent;
		double roundatcontent;
		double avg_seq_length = 0;

		//Open the file.
		if (open_file(args.sequence_filename, "r", FALSE, "FASTA", "sequences", &seq_file) == 0) {
				fprintf(stderr, "Couldn't open the file %s.\n", args.sequence_filename);
				ramen_terminate(1);
		}

		//Start reading in the sequences
		read_many_fastas(ramen_alph, seq_file, MAX_SEQ_LENGTH, &num_seqs, &seq_list);


		seq_ids = new_string_list();
		seq_fscores = allocate_array(num_seqs);

		//Allocate the required space for results
		results = malloc(sizeof(double*) * motifs.num);
		for (i=0;i<motifs.num;i++) {
				results[i] = malloc(sizeof(double)*num_seqs);
		}

		for (j=0;j<num_seqs;j++) {

				fprintf(stderr, "\rScanning %i of %i sequences...", j+1, num_seqs);

				//copy the pointer into our current object for clarity
				sequence = seq_list[j];

				//Read the fluorescence data from the description field.
				add_string(get_seq_name(sequence),seq_ids);
				seq_len = get_seq_length(sequence);
				set_array_item(j,atof(get_seq_description(sequence)),seq_fscores);

				//Scan with each motif.
				for (i=0;i<motifs.num;i++) {
						int motifindex = i*2;

						results[i][j] = ramen_sequence_scan(sequence, motif_at(motifs.motifs, motifindex), 
											      motif_at(motifs.motifs, motifindex+1),
											      NULL, NULL, //No need to pass PSSM.
											      get_motif_freqs(motif_at(motifs.motifs, motifindex)),
											      get_motif_freqs(motif_at(motifs.motifs, motifindex+1)),
										              AVG_ODDS, 0, TRUE, 0, motifs.bg_freqs);

						if (TRUE == args.linreg_normalise) {
								int k;
								double maxscore = 1;
								motif = motif_at(motifs.motifs,motifindex); 
								for (k=0;k<get_motif_length(motif);k++) {
										double maxprob = 0;
										if (maxprob < get_matrix_cell(k, alph_index(ramen_alph, 'A'), get_motif_freqs(motif)))
												maxprob = get_matrix_cell(k, alph_index(ramen_alph, 'A'), get_motif_freqs(motif));
										if (maxprob < get_matrix_cell(k, alph_index(ramen_alph, 'C'), get_motif_freqs(motif)))
												maxprob = get_matrix_cell(k, alph_index(ramen_alph, 'C'), get_motif_freqs(motif));
										if (maxprob < get_matrix_cell(k, alph_index(ramen_alph, 'G'), get_motif_freqs(motif)))
												maxprob = get_matrix_cell(k, alph_index(ramen_alph, 'G'), get_motif_freqs(motif));
										if (maxprob < get_matrix_cell(k, alph_index(ramen_alph, 'T'), get_motif_freqs(motif)))
												maxprob = get_matrix_cell(k, alph_index(ramen_alph, 'T'), get_motif_freqs(motif));
										maxscore *= maxprob;
								}
								results[i][j] /= maxscore;
						}
				}
		}

}

void ramen_get_scores() {
  int i;
  int seq_num;

  seq_num = get_num_strings(seq_ids); //number of sequences
  //allocate space for final one result per motif array.
  rsr = malloc(sizeof(ramen_result_t*)*motifs.num);

  for(i=0;i<motifs.num;i++) {
    fprintf(stderr, "\rScoring %i of %i motifs...", i+1, motifs.num);
        rsr[i] = ramen_do_linreg_test(i);
  }

  //Order by MSE.
  qsort(rsr, motifs.num, sizeof(ramen_result_t*), ramen_compare_mse);

  fprintf(stderr, "\n");

}



void ramen_print_results() {

  int i,count;
  ramen_result_t* result;

  printf("ramen (Regression Analysis of Motif ENrichment): Compiled on " __DATE__ "\n"
         "------------------------------\n"
         "Copyright (C) Robert McLeay <r.mcleay@imb.uq.edu.au> & Timothy Bailey <t.bailey@imb.uq.edu.au>, 2009.\n\n");

  printf("Options Invoked:\n"
         "----------------\n\n");

  printf("Background Format: ");
  switch(args.bg_format) {
    case UNIFORM_BG:
      printf("Uniform\n");
      break;
    case MOTIF_BG:
      printf("MEME Motif File\n");
      break;
    case FILE_BG:
      printf("Background File\n");
      printf("Background File: %s\n", args.bg_filename);
      break;
  }

  if (args.linreg_switchxy) {
    printf("y-axis:%s PWM Scores\n", args.log_pwmscores ? " Log_e of" : ""); 
    printf("x-axis:%s Fluorescence Scores\n", args.log_fscores ? " Log_e of" : ""); 
  } else {
    printf("y-axis:%s Fluorescence Scores\n", args.log_fscores ? " Log_e of" : ""); 
    printf("x-axis:%s PWM Scores\n", args.log_pwmscores ? " Log_e of" : "");
  }
  printf("Motif Scoring Function: %s\n", args.linreg_normalise ? "RMA (normalised motif scores)" : "AMA (Raw motif scores)");
  printf("Sampling Repetitions for p-values: %d\n", args.repeats);
  printf("Pseudocount: %g\n", args.pseudocount);

  if (args.linreg_dump_dir > 0) {
    printf("R-Format Regression Dump Directory: %s\n", args.linreg_dump_dir > 0 ? args.linreg_dump_dir : "Disabled.");
  }

  printf("\nMotif File: %s\n", args.motif_filenames[0]);
  printf("Sequence File: %s\n", args.sequence_filename);

  printf("\n\n");

  printf("Results:\n"
         "========\n\n");
  printf("Showing all motifs with p-value <= %g\n", args.pvalue_cutoff);
  printf("Fitting motifs to y: = mx + b\n\n");


  printf("Over-represented Motifs:\n"
         "------------------------\n\n");

  printf("%-10s%-10s%-15s%-15s%-15s%-15s%-15s\n","Rank","Motif","MSE","p-value (adj)","p-value (raw)","m","b");
  printf("%-10s%-10s%-15s%-15s%-15s%-15s%-15s\n","----","-----","---","-------------","-------------","-","-");

  count=0;
  i=0; //yes, this is an unusual way to use a for loop.
  for (result = rsr[i]; i < motifs.num; result = rsr[++i]) {
    if (result->m < 0 && ramen_bonferroni_correction(result->p,motifs.num) <= args.pvalue_cutoff) {
      if (0 == result->p) { //We show the p-value as being less than the lowest possible
        printf("%-10i%-10s%-13g< %-13g< %-15g%-15g%-15g\n", //"%i\t%s\t%g\t%g\t%g\t%g\t%g\n",
               ++count, result->motif_id, result->mse, ramen_bonferroni_correction(1.0/(args.repeats+1),motifs.num), 1.0/(args.repeats+1), result->m, result->b);
        //The motif_id + 1 is to remove the strand sign from the motif's name.
      } else {
        printf("%-10i%-10s%-15g%-15g%-15g%-15g%-15g\n", //"%i\t%s\t%g\t%g\t%g\t%g\t%g\n",
               ++count, result->motif_id, result->mse, ramen_bonferroni_correction(result->p,motifs.num), result->p, result->m, result->b);
        //The motif_id + 1 is to remove the strand sign from the motif's name.
      }
    }
  }

  printf("\n\nUnder-represented Motifs:\n"
         "-------------------------\n\n");

  printf("%-10s%-10s%-15s%-15s%-15s%-15s%-15s\n","Rank","Motif","MSE","p-value (adj)","p-value (raw)","m","b");
  printf("%-10s%-10s%-15s%-15s%-15s%-15s%-15s\n","----","-----","---","-------------","-------------","-","-");

  count=0;
  i=0; //yes, this is an unusual way to use a for loop.
  for (result = rsr[i]; i < motifs.num; result = rsr[++i]) {
    if (result->m >= 0  && ramen_bonferroni_correction(result->p,motifs.num) <= args.pvalue_cutoff) {
      if (0 == result->p) { //We show the p-value as being less than the lowest possible
        printf("%-10i%-10s%-13g< %-13g< %-15g%-15g%-15g\n", //"%i\t%s\t%g\t%g\t%g\t%g\t%g\n",
               ++count, result->motif_id, result->mse, ramen_bonferroni_correction(1.0/(args.repeats+1),motifs.num), 1.0/(args.repeats+1), result->m, result->b);
        //The motif_id + 1 is to remove the strand sign from the motif's name.
      } else {
        printf("%-10i%-10s%-15g%-15g%-15g%-15g%-15g\n", //"%i\t%s\t%g\t%g\t%g\t%g\t%g\n",
               ++count, result->motif_id, result->mse, ramen_bonferroni_correction(result->p,motifs.num), result->p, result->m, result->b);
        //The motif_id + 1 is to remove the strand sign from the motif's name.
      }
    }
  }

  /* Display time of execution */
  t1 = time(NULL);
  c1 = clock();
  printf("\n\n---\n");
  printf("Elapsed wall clock time: %ld seconds\n", (long) (t1 - t0));
  printf("Elapsed CPU time:        %f seconds\n", (float) (c1 - c0)/CLOCKS_PER_SEC);
}


/*
 * Using the linreg test,
 *
 * this method returns the lowest scoring subdivision of a set of sequences for a given motif.
 * It's not self-contained, as it requires to hook into the global variables results, motifs, seq_ids.
 */
ramen_result_t* ramen_do_linreg_test(int motif_num) {
  //Assorted vars
  int seq_num;
  int j,k;
  int motif_index = motif_num * 2; //This is a workaround to the change in the motif datastructure where it now
                      // goes +MOTIFA -MOTIFA +MOTIFB etc. rather than all + then all - motifs.

  //Vars for the regression
  double* x;
  double* y;
  double m = 0;
  double b = 0;
  double mse = 0;

  //Vars for scoring
  ramen_result_t* r;

  //Allocate memory or set initial values
  seq_num = get_num_strings(seq_ids); //number of sequences
  r = malloc(sizeof(ramen_result_t)); //allocate space, as a ptr to this will go in the array later
                    //that's why we don't free it in this loop.
  x = malloc(sizeof(double)*seq_num);
  y = malloc(sizeof(double)*seq_num);

  //Now we need to copy the scores into two double arrays
  //Use LOG macro so that log(0) 'works'
  for (j=0; j < seq_num; j++) {
    if (args.log_fscores == TRUE) {
      y[j] = LOG(get_array_item(j, seq_fscores));
    } else {
      y[j] = get_array_item(j, seq_fscores);
    }

    if (args.log_pwmscores == TRUE) {
      x[j] = LOG(results[motif_num][j]);
    } else {
      x[j] = results[motif_num][j];
    }
  }

    //Switch x&y if they're to be switched
    if (args.linreg_switchxy) {
        SWAP(double*, x, y);
    }

  // TODO: Tidy and/or remove this for production
  if(args.linreg_dump_dir > 0) {
    FILE *fh;
    char* filename;
    filename = malloc(sizeof(char)*(strlen(args.linreg_dump_dir) + 50));
    sprintf(filename, "%s/%s.tsv", args.linreg_dump_dir, get_motif_id(motif_at(motifs.motifs, motif_index)));

    fh = fopen(filename, "w");
    fputs("PWM_Score\tFluorescence_Score\n", fh);
    for (j=0; j < seq_num; j++) {
      fprintf(fh, "%.10e %.10e\n", x[j], y[j]);
    }
    fclose(fh);
    free(filename);
  }


  /*extern double regress(
    int n,                        / number of points /
    double *x,                    / x values /
    double *y,                    / y values /
    double *m,                    / slope /
    double *b                     / y intercept /
    );*/
    mse = regress(seq_num, x, y, &m, &b);

  if (args.verbose >= 3) {
    printf("LinReg MSE of motif %s on %i seqs: %.4g (m: %.4g b: %.4g)\n",
           get_motif_id(motif_at(motifs.motifs, motif_index)), seq_num, mse, m, b);
  }

  //Add to our motif list if lowest MSE
  r->motif_id = strdup(get_motif_id(motif_at(motifs.motifs, motif_index)));
  r->m = m; //Not p-values, but they'll do when we re-use this structure...
  r->b = b;
  r->mse = mse;
  r->p = -1;

    //Do stochastic sampling if required.
    if (args.repeats > 0) {
        int repeat_wins = 0;
        for (j=0;j<args.repeats;j++) {
            double repeat_mse = 0;
            shuffle(x,seq_num); //Shuffle and break the associations between x and y
            repeat_mse = regress(seq_num, x, y, &m, &b);
            //fprintf(stderr, "Motif %d Repeat %d RMSE: %g MSE: %g\n",motif_index,j,repeat_mse,mse);
            if (repeat_mse <= mse) {
                repeat_wins++;
            }
        }
        r->p = repeat_wins*1.0/ args.repeats*1.0;
    }
    free(x);
    free(y);

  return r;
}


void ramen_terminate(int status) {
  /* Display time of execution */
  if (verbosity >= HIGH_VERBOSE) {
    t1 = time(NULL);
    c1 = clock();
    fprintf (stderr,"Elapsed wall clock time: %ld seconds\n", (long) (t1 - t0));
    fprintf (stderr,"Elapsed CPU time:        %f seconds\n", (float) (c1 - c0)/CLOCKS_PER_SEC);
  }
  exit(status);
}

/*
 * COMPARISON METHODS FOLLOW
 */


int ramen_compare_mse (const void *a, const void *b)
{
  ramen_result_t r1 = **(ramen_result_t**)a;
  ramen_result_t r2 = **(ramen_result_t**)b;
  if (r2.mse- r1.mse < 0.0) {
    return 1;
  } else if (r2.mse - r1.mse > 0.0){
    return -1;
  } else {
    return 0;
  }
}

int ramen_compare_doubles (const double *a, const double *b)
{
  //This does it in reverse order
  return (int) (*b - *a);
}

int ramen_compare_ranks_f_rank (const void *a, const void *b) {
  ramen_rank_t r1 = **(ramen_rank_t**)a;
  ramen_rank_t r2 = **(ramen_rank_t**)b;

  //fprintf(stderr, "Comparing: %i to %i\n", r1.f_rank, r2.f_rank);

  if (r2.f_rank - r1.f_rank < 0.0) {
    return 1;
  } else if (r2.f_rank - r1.f_rank > 0.0){
    return -1;
  } else {
    return 0;
  }
  //This compares it smallest first

}

int ramen_compare_ranks_pwm_score (const void *a, const void *b) {
  ramen_rank_t r1 = **(ramen_rank_t**)a;
  ramen_rank_t r2 = **(ramen_rank_t**)b;

  //fprintf(stderr, "\nComparing: %g to %g", r1.pwm_score, r2.pwm_score);

  if (r1.pwm_score - r2.pwm_score < 0.0) {
    return 1;
  } else if (r1.pwm_score - r2.pwm_score > 0.0){
    return -1;
  } else {
    return 0;
  }
  //This compares it largest first
}

/* Arrange the N elements of ARRAY in random order.
   Only effective if N is much smaller than RAND_MAX;
   if this may not be the case, use a better random
   number generator. */
void shuffle(double* array, size_t n)
{
  double t;
  if (n > 1) {
    size_t i;
    for (i = 0; i < n - 1; i++) {
      size_t j = i + rand() / (RAND_MAX / (n - i) + 1);
      t = array[j];
      array[j] = array[i];
      array[i] = t;
    }
  }
}

double ramen_bonferroni_correction(double p, double numtests) {
  return 1 - pow(1-p, numtests);
}

