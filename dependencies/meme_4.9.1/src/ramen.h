/********************************************************************
 * FILE: ramen.h
 * AUTHOR: Robert McLeay
 * CREATE DATE: 19/08/2008
 * PROJECT: MEME suite
 * COPYRIGHT: 2008, Robert McLeay
 *
 * rsClover is a yet unpublished algorithm that seeks to assist in
 * determining whether a given transcription factor regulates a set
 * of genes that have been found by an experimental method that
 * provides ranks, such as microarray or ChIp-chip.
 *
 *
 * rsClover is a code name. The name will change prior to publication.
 ********************************************************************/
#ifndef __RAMEN_H__
#define __RAMEN_H__

/*
 * ramen-specific macros
 */

#define min(a,b)      (a<b)?a:b
#define max(a,b)      (a>b)?a:b

#define MAX_SEQ_LENGTH 1e6

/*
 * Define ramen constants
 */

#define MEME_FORMAT 1 	// input format is meme
#define TAMO_FORMAT 2 	// input format is tamo
#define REGEXP_FORMAT 3 //read in regular expressions

#define UNIFORM_BG 0    //uniform.
#define MOTIF_BG 1 		// background frequencies taken from motif file
#define FILE_BG 2		// background frequencies taken from specified file

/*
 * Struct definitions
 */

typedef struct  {
	char* bg_filename; //file to get base freqs from
	int bg_format; //whether it's fasta, meme, etc.
	int motif_format; //is the motif file meme or tamo
	char** motif_filenames; //filename of the motif library
	char* sequence_filename; //input sequences in fasta format
	float pseudocount; //add to the motif frequency counts
    int repeats; //how many times to sample for p-value.
	int verbose;
    double pvalue_cutoff;
	BOOLEAN_T log_fscores;
	BOOLEAN_T log_pwmscores;
	BOOLEAN_T linreg_normalise;
	BOOLEAN_T linreg_switchxy;
	char* linreg_dump_dir;
	int number_motif_files;
} ramen_arg_t;

typedef struct {
	char* filename;
	MOTIF_T* motifs;
	int num;
	ARRAY_T* bg_freqs;
	BOOLEAN_T has_reverse_strand;
} ramen_motifs_t;

typedef struct {
	char* motif_id;
	double mse;
	double m;
	double b;
	double p;
} ramen_result_t;

typedef struct {
	int f_rank;
	int pwm_rank;
	double pwm_score;
	double f_score;
} ramen_rank_t;



/*
 *  Function definitions
 */

void ramen_getopt(int argc, char *argv[]);
const char* ramen_get_usage();
ARRAY_T* ramen_load_background();
ARRAY_T* ramen_get_bg_freqs(SEQ_T* seq);
void ramen_load_motifs();
void ramen_scan_sequences();
void ramen_get_scores();
void ramen_get_pvalues();
void ramen_print_results();
void ramen_usage();
void ramen_terminate(int status);
int ramen_compare_doubles (const double *a, const double *b);
int ramen_compare_mse (const void *a, const void *b);
int ramen_compare_ranks_f_rank (const void *a, const void *b);
int ramen_compare_ranks_pwm_score (const void *a, const void *b);
void ramen_dump_motif(MOTIF_T* m);
ramen_result_t* ramen_do_linreg_test(int motif_num);
unsigned long long choose(unsigned n, unsigned k);
long double logchoose(unsigned n, unsigned k);
void shuffle(double* array, size_t n);
double ramen_bonferroni_correction(double, double);

#else
#endif
