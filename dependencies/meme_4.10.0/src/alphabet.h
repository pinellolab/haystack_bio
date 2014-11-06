/********************************************************************
 * FILE: alphabet.h
 * AUTHOR: William Stafford Noble
 * CREATE DATE: 4-17-97
 * PROJECT: MHMM
 * COPYRIGHT: 1997-2008, WSN
 * DESCRIPTION: Define the amino acid and nucleotide alphabets.
 ********************************************************************/
#ifndef ALPHABET_H
#define ALPHABET_H

#include "array.h"
#include "utils.h"

#include <assert.h>

/* Define a type and a global variable for the alphabet type. */
typedef enum {INVALID_ALPH, PROTEIN_ALPH, DNA_ALPH} ALPH_T;

typedef enum {INVALID_SIZE, ALPH_SIZE, AMBIG_SIZE, ALL_SIZE} ALPH_SIZE_T;

typedef const short* ALPH_INDEX_T;

#define PROTEIN_ASIZE 20
#define DNA_ASIZE 4

extern const ALPH_INDEX_T ALPH_INDEX[];

/*
 * Get the name of the alphabet - useful for error messages
 */
const char* alph_name(ALPH_T alph);

/*
 * Get the size of the alphabet
 */
int alph_size(ALPH_T alph, ALPH_SIZE_T which_size);

/*
 * Get the alphabet index of a letter. 
 *
 * Assumes that the alphabet is valid and that the letter is
 * represented by one byte and does not exceed 255 (if treated as unsigned)
 */
#define alph_index(alph, letter) (ALPH_INDEX[(alph)][(letter) - '\0'])

/*
 * Get the ALPH_INDEX_T for this alphabet (it's an array but who knows it might change)
 */
#define alph_indexer(alph) (ALPH_INDEX[(alph)])

/*
 * Lookup the index of a letter in the passed ALPH_INDEX_T
 *
 * Assumes that the letter is represented by one byte and hence does not exceed
 * 255 when treated as unsigned.
 */
#define alph_index2(index, letter) (index)[(letter) - '\0'];

/*
 * Get the letter at an index of the alphabet
 */
char alph_char(ALPH_T alph, int index);

/*
 * Get the alphabet string (no ambiguous characters)
 */
const char* alph_string(ALPH_T alph);

/*
 * Get the wildcard letter of the alphabet
 */
char alph_wildcard(ALPH_T alph);

/*
 * Determine if a letter is known in this alphabet
 */
BOOLEAN_T alph_is_known(ALPH_T alph, char letter);

/*
 * Determine if a letter is a concrete representation
 */
BOOLEAN_T alph_is_concrete(ALPH_T alph, char letter);

/*
 * Determine if a letter is ambiguous
 */
BOOLEAN_T alph_is_ambiguous(ALPH_T alph, char letter);

/*
 *  Tests the letter against the alphabet. If the alphabet is unknown
 *  it attempts to work it out and set it from the letter.
 *  For simplicy this assumes you will pass indexes in asscending order.
 *  Returns false if the letter is unacceptable
 */
BOOLEAN_T alph_test(ALPH_T *alpha, int index, char letter);

/*
 *  Tests the alphabet string and attempts to return the ALPH_T.
 *  If the alphabet is from some buffer a max size can be set
 *  however it will still only test until the first null byte.
 *  If the string is null terminated just set a max > 20 
 */
ALPH_T alph_type(const char *alphabet, int max);

/*
 * Calculate the values of the ambiguous letters from sums of
 * the normal letter values.
 */
void calc_ambigs(ALPH_T alph, BOOLEAN_T log_space, ARRAY_T* array);

/*
 * Replace the elements an array of frequences with the average
 * over complementary bases.
 */
void average_freq_with_complement(ALPH_T alph, ARRAY_T *freqs);

/*
 * Load the non-redundant database frequencies into the array.
 */
ARRAY_T* get_nrdb_frequencies(ALPH_T alph, ARRAY_T* freqs);

/*
 * Load uniform frequencies into the array.
 */
ARRAY_T* get_uniform_frequencies(ALPH_T alph, ARRAY_T* freqs);

/*
 * Load background file frequencies into the array.
 */
ARRAY_T* get_file_frequencies(ALPH_T *alph, char *bg_filename, ARRAY_T *freqs);

/*
 * Get a background distribution
 *  - by reading values from a file if filename is given, or
 *  - equal to the NRDB frequencies if filename is NULL.
 */
ARRAY_T* get_background(ALPH_T alph, char* bg_filename);

/*
 * Take the counts from each ambiguous character and evenly distribute
 * them among the corresponding concrete characters.
 *
 * This function operates in log space.
 */
void dist_ambigs(ALPH_T alph, ARRAY_T* freqs);

#endif
