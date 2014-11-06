/****************************************************************************
 * FILE: seq.c
 * AUTHOR: William Stafford Noble
 * CREATE DATE: 06/24/2002
 * PROJECT: MHMM
 * DESCRIPTION: Biological sequences.
 * COPYRIGHT: 1998-2008, UCSD, UCSC, UW
 ****************************************************************************/
#include <assert.h>
#include <ctype.h>
#include <limits.h>
#include <string.h>
#include "utils.h"
#include "seq.h"

#define MAX_SEQ_NAME 100     // Longest sequence ID.
#define MAX_SEQ_COMMENT 128 // Longest comment.

// maximum count to keep track of when calculating a background
const static long BG_CALC_CHUNK = LONG_MAX;

// Instantiate the SEQ_T type.
struct seq {
  char  name[MAX_SEQ_NAME + 1];     // Sequence ID.
  char  desc[MAX_SEQ_COMMENT + 1];  // Sequence description.
  unsigned int length;              // Sequence length.
  unsigned int offset;              // Offset from the start of complete sequence.
  unsigned int starting_coord;      // Starting coord of the start of complete sequence.
  float weight;                 // Sequence weight.
  char* sequence;               // The actual sequence.
  int8_t* isequence;           // indexed sequence
  int*  intseq;                 // The sequence in integer format.
  unsigned int num_priors;      // Number of priors
  double* priors;               // The priors corresponding to the sequence.
  int*  gc;                     // Cumulative GC counts; note: 2Gb size limit.
  double total_gc;              // Total frequency of G and C in sequence; DNA only
  BOOLEAN_T is_complete;        // Is this the end of the sequence?
};

// For calculating the background
// of a truly huge dataset keep
// track of the result up to size
// long max and then store weights
// depending on how many chunks
// have been normalized into the
// final result
struct bgcalc {
  // the alphabet
  ALPH_T alph;
  // the number of bases seen so far
  // in this chunk
  long chunk_seen;
  // the counts of the individual
  // bases seen so far
  long *chunk_counts;
  // the background seen so far
  double *bg;
  // the weight of the background
  // seen so far. Note that 1 chunk
  // has the weight of 1.
  long weight;
};


/****************************************************************************
 * Allocate one sequence object.
 ****************************************************************************/
SEQ_T* allocate_seq
  (char* name,
   char* description,
   unsigned int offset,
   char* sequence)
{
  SEQ_T* new_sequence;

  // Allocate the sequence object.
  new_sequence = (SEQ_T*)mm_malloc(sizeof(SEQ_T));

  // Fill in the fields.
  new_sequence->priors = NULL;
  new_sequence->length = 0;
  new_sequence->offset = offset;
  new_sequence->starting_coord = offset;
  new_sequence->sequence = NULL;
  new_sequence->isequence = NULL;
  new_sequence->weight = 1.0;
  new_sequence->intseq = NULL;
  new_sequence->gc = NULL;
  new_sequence->num_priors = 0;
  new_sequence->is_complete = FALSE;

  // Store the name, truncating if necessary.
  new_sequence->name[0] = 0;
  if (name != NULL) {
    strncpy(new_sequence->name, name, MAX_SEQ_NAME);
    new_sequence->name[MAX_SEQ_NAME] = '\0';
    if (strlen(new_sequence->name) != strlen(name)) {
      fprintf(stderr, "Warning: truncating sequence ID %s to %s.\n",
          name, new_sequence->name);
    }
  }

  // Store the description, truncating if necessary.
  new_sequence->desc[0] = 0;
  if (description != NULL) {
    new_sequence->desc[0] = '\0';
    if (description) {
      strncpy(new_sequence->desc, description, MAX_SEQ_COMMENT);
      new_sequence->desc[MAX_SEQ_COMMENT] = '\0';
    }
  }

  // Store the sequence.
  if (sequence != NULL) {
    copy_string(&(new_sequence->sequence), sequence);
    new_sequence->length = strlen(sequence);
  }

  return(new_sequence);
}

/****************************************************************************
 * Get and set various fields.
 ****************************************************************************/
char* get_seq_name
  (SEQ_T* a_sequence)
{
  assert(a_sequence != NULL);
  return(a_sequence->name);
}

char* get_seq_description
  (SEQ_T* a_sequence)
{
  assert(a_sequence != NULL);
  return(a_sequence->desc);
}

unsigned int get_seq_length
  (SEQ_T* a_sequence)
{
  assert(a_sequence != NULL);
  return(a_sequence->length);
}

unsigned int get_seq_offset
  (SEQ_T* a_sequence)
{
  assert(a_sequence != NULL);
  return(a_sequence->offset);
}

void set_seq_offset
  (unsigned int offset, 
   SEQ_T* a_sequence)
{
  assert(a_sequence != NULL);
  a_sequence->offset = offset;
}

unsigned int get_seq_starting_coord
  (SEQ_T* a_sequence)
{
  return a_sequence->starting_coord;
}

void set_seq_starting_coord
  (unsigned int start,
  SEQ_T* a_sequence)
{
  a_sequence->starting_coord = start;
}

float get_seq_weight
  (SEQ_T* a_sequence)
{
  assert(a_sequence != NULL);
  return(a_sequence->weight);
}

void set_seq_weight
  (float  weight,
   SEQ_T* a_sequence)
{
  assert(a_sequence != NULL);
  a_sequence->weight = weight;
}

unsigned char get_seq_char
  (int index,
   SEQ_T* a_sequence)
{
  assert(a_sequence != NULL);
  assert(a_sequence->sequence != NULL);
  assert(index >= 0);
  assert(index <= a_sequence->length);
  return(a_sequence->sequence[index]);
}

void set_seq_char
  (int    index,
   char   a_char,
   SEQ_T* a_sequence)
{
  assert(a_sequence != NULL);
  assert(a_sequence->sequence != NULL);
  assert(index >= 0);
  assert(index <= a_sequence->length);
  a_sequence->sequence[index] = a_char;
}

int get_seq_int
  (int index,
   SEQ_T* a_sequence)
{
  assert(a_sequence != NULL);
  assert(a_sequence->sequence != NULL);
  assert(index >= 0);
  assert(index < a_sequence->length);
  return(a_sequence->intseq[index]);
}

int get_seq_gc
  (int index,
   SEQ_T* a_sequence)
{
  assert(a_sequence != NULL);
  assert(a_sequence->sequence != NULL);
  assert(index >= 0);
  assert(index < a_sequence->length);
  return(a_sequence->gc[index]);
}

/**************************************************************************
 * Return a pointer to the raw sequence data in a SEQ_T object.
 **************************************************************************/
char* get_raw_sequence
  (SEQ_T* a_sequence)
{
  assert(a_sequence != NULL);
  return(a_sequence->sequence);
}

char* get_raw_subsequence
  (int start, int stop, SEQ_T* a_sequence)
{
  assert(a_sequence != NULL);
  assert((stop - start) >= 0);
  char *sequence = get_raw_sequence(a_sequence);
  char *subsequence = mm_malloc((stop - start + 2) * sizeof(char));
  strncpy(subsequence, sequence + start, stop - start + 1);
  subsequence[stop - start + 1] = 0;
  return(subsequence);
}

void set_raw_sequence(
  char *raw_sequence,
  BOOLEAN_T is_complete,
  SEQ_T* a_sequence
) {
  assert(a_sequence != NULL);
  a_sequence->sequence = raw_sequence;
  a_sequence->length = strlen(raw_sequence);
  a_sequence->is_complete = is_complete;

}

int8_t* get_isequence(SEQ_T* a_sequence)
{
  assert(a_sequence != NULL);
  assert(a_sequence->isequence != NULL);
  return(a_sequence->isequence);
}

int* get_int_sequence
  (SEQ_T* a_sequence)
{
  assert(a_sequence != NULL);
  return(a_sequence->intseq);
}


int* get_gc_sequence
  (SEQ_T* a_sequence)
{
  assert(a_sequence != NULL);
  return(a_sequence->gc);
}

double get_total_gc_sequence
  (SEQ_T* a_sequence)
{
  assert(a_sequence != NULL);
  return(a_sequence->total_gc);
}

void set_total_gc_sequence
  (SEQ_T* a_sequence, double gc)
{
  assert(a_sequence != NULL);
  a_sequence->total_gc = gc;
}

/**************************************************************************
 * Return the number of priors currently stored in the SEQ_T object
 **************************************************************************/
unsigned int get_seq_num_priors
  (SEQ_T* a_sequence)
{
  assert(a_sequence != NULL);
  return(a_sequence->num_priors);
}

/**************************************************************************
 * Return a pointer to the priors associated with a SEQ_T object.
 * It may be a NULL pointer. If it is not NULL the number of priors
 * is the length member of the SEQ_T object.
 **************************************************************************/
double* get_seq_priors
  (SEQ_T* a_sequence)
{
  assert(a_sequence != NULL);
  return(a_sequence->priors);
}

/**************************************************************************
 * Set the priors data for a SEQ_T object.
 **************************************************************************/
void set_seq_priors
  (double *priors,
  SEQ_T* a_sequence) 
{
  assert(a_sequence != NULL);
  a_sequence->priors = priors;
}


/**************************************************************************
 * Copy a sequence object.  Memory must be freed by caller.
 **************************************************************************/
SEQ_T* copy_sequence
  (SEQ_T* source_sequence)
{
  // Allocate the sequence object.
  SEQ_T* new_sequence;

  new_sequence = (SEQ_T*)mm_malloc(sizeof(SEQ_T));

  // Store the name
  // moving from one buffer to another of identical size so no truncation
  strncpy(new_sequence->name, source_sequence->name, MAX_SEQ_NAME);
  new_sequence->name[MAX_SEQ_NAME] = '\0';

  // Store the description
  // moving from one buffer to another of identical size so no truncation
  strncpy(new_sequence->desc, source_sequence->desc, MAX_SEQ_COMMENT);
  new_sequence->desc[MAX_SEQ_COMMENT] = '\0';

  // Fill in the always set fields
  new_sequence->starting_coord = source_sequence->starting_coord;
  new_sequence->length = source_sequence->length;
  new_sequence->offset = source_sequence->offset;
  new_sequence->weight = source_sequence->weight;
  new_sequence->is_complete = source_sequence->is_complete;
  new_sequence->num_priors = source_sequence->num_priors;

  // copy over the sequence (note that either this or the isequence will be present)
  if (source_sequence->sequence != NULL) {
    new_sequence->sequence = mm_malloc(sizeof(char) * (source_sequence->length + 1));
    memcpy(new_sequence->sequence, source_sequence->sequence, (sizeof(char) * source_sequence->length));
    new_sequence->sequence[source_sequence->length] = '\0';
  } else {
    new_sequence->sequence = NULL;
  }
  // copy over the isequence (note that either this or the sequence will be present)
  if (source_sequence->isequence != NULL) {
    new_sequence->isequence = mm_malloc(sizeof(int8_t) * source_sequence->length);
    memcpy(new_sequence->isequence, source_sequence->isequence, (sizeof(int8_t) * source_sequence->length));
  } else {
    new_sequence->isequence = NULL;
  }
  // copy over the int sequence (this may be present, used by mhmm code, 
  // required to be int so that hashing of multiple characters can be done)
  if (source_sequence->intseq != NULL) {
    new_sequence->intseq = (int*)mm_malloc(sizeof(int) * source_sequence->length);
    memcpy(new_sequence->intseq, source_sequence->intseq, (sizeof(int) * source_sequence->length));
  } else {
    new_sequence->intseq = NULL;
  }
  // copy over the gc count (this may be present, used by mhmm code)
  if (source_sequence->gc != NULL) {
    new_sequence->gc = (int*)mm_malloc(sizeof(int) * source_sequence->length);
    memcpy(new_sequence->gc, source_sequence->gc, (sizeof(int) * source_sequence->length));
  } else {
    source_sequence->gc = NULL;
  }
  // copy over the priors
  if (source_sequence->priors && source_sequence->num_priors > 0) {
    new_sequence->priors = mm_malloc(sizeof(double) * source_sequence->num_priors);
    memcpy(new_sequence->priors, source_sequence->priors, (sizeof(double) * source_sequence->num_priors));
  } else {
    new_sequence->priors = NULL;
  }
  return(new_sequence);
}

/**************************************************************************
 * Convert a sequence to an indexed version. This is a one-way conversion
 * but you can choose to keep the source sequence if you need it
 **************************************************************************/
void index_sequence(SEQ_T* seq, ALPH_T alpha, int options) {
  int asize, index;
  char *in;
  int8_t *out;
  ALPH_INDEX_T indexer;
  in = seq->sequence;
  if ((options & SEQ_KEEP) != 0 || sizeof(char) != sizeof(int8_t)) {
    out = mm_malloc(sizeof(int8_t) * seq->length);
    seq->isequence = out;
  } else {
    // use the same memory and overwrite the sequence with the indexed version
    out = (int8_t*)seq->sequence;
    seq->isequence = out;
    seq->sequence = NULL;
  }
  indexer = alph_indexer(alpha);
  if ((options & SEQ_NOAMBIG) != 0) {
    asize = alph_size(alpha, ALPH_SIZE);
    for (; *in != '\0'; in++, out++) {
      *out = alph_index2(indexer, *in);
      // mark ambiguous characters as unknown
      if (*out >= asize) *out = -1;
    }
  } else {
    for (; *in != '\0'; in++, out++) {
      *out = alph_index2(indexer, *in);
    }
  }
}


/**************************************************************************
 * Sometimes a sequence object contains only a portion of the actual
 * sequence.  This function tells whether or not the end of this
 * sequence object corresponds to the end of the actual sequence.
 **************************************************************************/
void set_complete
  (BOOLEAN_T is_complete,
   SEQ_T* a_sequence)
{
  assert(a_sequence != NULL);
  a_sequence->is_complete = is_complete;
}

BOOLEAN_T is_complete
  (SEQ_T* a_sequence)
{
  assert(a_sequence != NULL);
  return(a_sequence->is_complete);
}


/***************************************************************************
 * Add or remove Xs from either side of the sequence.
 ***************************************************************************/
static void add_flanking_xs
  (SEQ_T* sequence, ALPH_T alph)
{
  char*  new_seq = NULL;         // Pointer to copy of the sequence.

  new_seq = (char*)mm_calloc(sequence->length + 3, sizeof(char));
  strcpy(&(new_seq[1]), sequence->sequence);

  new_seq[0] = alph_wildcard(alph);
  new_seq[sequence->length + 1] = alph_wildcard(alph);
  new_seq[sequence->length + 2] = '\0';

  myfree(sequence->sequence);
  sequence->sequence = new_seq;
  sequence->length += 2;
}

void remove_flanking_xs
  (SEQ_T* sequence)
{
  char*  new_seq;         // Copy of the sequence.

  new_seq = (char*)mm_calloc(sequence->length - 1, sizeof(char));
  strncpy(new_seq, &(sequence->sequence[1]), sequence->length - 2);
  new_seq[sequence->length - 2] = '\0';

  myfree(sequence->sequence);
  sequence->sequence = new_seq;
  sequence->length -= 2;
}

/**************************************************************************
 * Prepare a sequence for recognition by
 *  - making sure it is uppercase,
 *  - making sure it doesn't contain illegal characters,
 *  - adding flanking Xs to match START/END states, and
 *  - converting it to an integer format
 *  - computing cumulative GC counts
 *
 * In the integer form, each character in the sequence is replaced by
 * the index of that character in the alphabet array.  Thus, if the
 * alphabet is 'ACGT', every occurence of the letter 'G' in the
 * sequence will be represented by the index 2.
 **************************************************************************/
void prepare_sequence
  (SEQ_T* sequence, ALPH_T alph)
{
  int i_seq;        // Index in the sequence.
  int badchar;      // Number of characters converted.
  char wildcard;    // Wildcard character

  wildcard = alph_wildcard(alph);
  badchar = 0;

  for (i_seq = 0; i_seq < get_seq_length(sequence); i_seq++) {
    // Make sure the sequence is uppercase.
    if (islower((int)(sequence->sequence)[i_seq])) {
      (sequence->sequence)[i_seq] = toupper((int)(sequence->sequence)[i_seq]);
    }

    // Convert non-alphabetic characters to ambiguous.
    if (alph_index(alph, (sequence->sequence)[i_seq]) == -1) {
      fprintf(stderr, "%c -> %c\n", (sequence->sequence)[i_seq], wildcard);
      (sequence->sequence)[i_seq] = wildcard;
      badchar++;
    }
  }

  // Tell the user about the conversions.
  if (badchar) {
    fprintf(stderr, "Warning: converted %d non-alphabetic ", badchar);
    fprintf(stderr, "characters to %c in sequence %s.\n", wildcard, 
        get_seq_name(sequence));
  }

  // Add flanking X's.
  add_flanking_xs(sequence, alph);

  // Make the integer sequence.
  sequence->intseq = (int *)mm_malloc(sizeof(int) * get_seq_length(sequence));
  for (i_seq = 0; i_seq < get_seq_length(sequence); i_seq++) {
    (sequence->intseq)[i_seq]
      = alph_index(alph, (sequence->sequence)[i_seq]);
  }

  //
  // Get cumulative GC counts.
  //
  if (alph == DNA_ALPH) {
    int len = get_seq_length(sequence);
    char c = (sequence->sequence)[0];		// first character

    sequence->gc = (int *)mm_malloc(sizeof(int) * get_seq_length(sequence));

    // set count at first position
    (sequence->gc)[0] = (c == 'G' || c == 'C') ? 1 : 0;
    // set cumulative counts at rest of postitions
    for (i_seq = 1; i_seq < len; i_seq++) {
      c = (sequence->sequence)[i_seq];
      (sequence->gc)[i_seq] = (c == 'G' || c == 'C') ?
        (sequence->gc)[i_seq-1] + 1 : (sequence->gc)[i_seq-1];
    }
  }
}

/***************************************************************************
 * Remove the first N bases of a given sequence.
 ***************************************************************************/
void shift_sequence
  (int    offset,
   SEQ_T* sequence)
{
  char *new_sequence = NULL;
  double *new_priors = NULL;

  // Make a copy of the raw sequence for the overlap.
  assert(offset > 0);
  assert(offset <= sequence->length);

  memmove(
    sequence->sequence,
    sequence->sequence + offset,
    sequence->length - offset + 1
  );

  // Shift priors if needed.
  if (sequence->priors) {
    memmove(
      sequence->priors,
      sequence->priors + offset,
      (sequence->length - offset) * sizeof(double)
    );
  }

  sequence->offset += offset;

  // Free the integer version.
  myfree(sequence->intseq);
  sequence->intseq = NULL;

  // Free the GC counts.
  myfree(sequence->gc);
  sequence->gc = NULL;
}

/***************************************************************************
 * Get the maximum sequence length from a set of sequences.
 ***************************************************************************/
int get_max_seq_length
  (int     num_seqs,
   SEQ_T** sequences)
{
  int max_length;
  int this_length;
  int i_seq;

  max_length = 0;
  for (i_seq = 0; i_seq < num_seqs; i_seq++) {
    this_length = get_seq_length(sequences[i_seq]);
    if (this_length > max_length) {
      max_length = this_length;
    }
  }
  return(max_length);
}

/***************************************************************************
 * Get the maximum sequence ID length from a set of sequences.
 ***************************************************************************/
int get_max_seq_name
  (int     num_seqs,
   SEQ_T** sequences)
{
  int max_length;
  int this_length;
  int i_seq;

  max_length = 0;
  for (i_seq = 0; i_seq < num_seqs; i_seq++) {
    this_length = strlen(get_seq_name(sequences[i_seq]));
    if (this_length > max_length) {
      max_length = this_length;
    }
  }
  return(max_length);
}

/***************************************************************************
 * Set the sequence weights according to an external file.
 *
 * If the filename is "none," "internal," or NULL, then the weights are
 * set uniformly.
 ***************************************************************************/
void set_sequence_weights
  (char*    weight_filename, // Name of file containing sequence weights.
   int      num_seqs,        // Number of sequences.
   SEQ_T**  sequences)       // The sequences.
{
  ARRAY_T* weights;
  FILE *   weight_file;
  int      i_seq;

  /* Allocate the weights. */
  weights = allocate_array(num_seqs);

  /* Set uniform weights if no file was supplied. */
  if ((weight_filename == NULL) || (strcmp(weight_filename, "none") == 0) ||
      (strcmp(weight_filename, "internal") == 0)) {
    init_array(1.0, weights);
  }

  /* Read the weights from a file. */
  else {
    if (open_file(weight_filename, "r", FALSE, "weight", "weights",
		  &weight_file) == 0)
      exit(1);
    read_array(weight_file, weights);
    fclose(weight_file);

    /* Normalize the weights so they add to the total number of sequences. */
    normalize(0.0, weights);
    scalar_mult(num_seqs, weights);
  }

  /* Assign the weights to the sequences. */
  for (i_seq = 0; i_seq < num_seqs; i_seq++) {
    (sequences[i_seq])->weight = get_array_item(i_seq, weights);
  }

  /* Free the weights. */
  free_array(weights);
}


/****************************************************************************
 *  Return an array containing the frequencies in the sequence for each
 *  character of the alphabet. Characters not in the alphabet are not
 *  counted.
 ****************************************************************************/
ARRAY_T* get_sequence_freqs
  (SEQ_T* seq, ALPH_T alph)
{
  int a_size, a_index, i;
  int total_bases;
  int *num_bases;
  int8_t *iseq;
  ARRAY_T* freqs;

  // Initialize counts for each character in the alphabet
  a_size = alph_size(alph, ALPH_SIZE);
  num_bases = mm_malloc(a_size * sizeof(int));
  for (i = 0; i < a_size; i++) {
    num_bases[i] = 0;
  }
  total_bases = 0;

  if (seq->isequence) {
    for (i = 0, iseq = seq->isequence; i < seq->length; i++, iseq++) {
      if (*iseq == -1 || *iseq >= a_size) continue;
      num_bases[*iseq]++;
      total_bases++;
    }
  } else {
    for (i = 0; i < seq->length; i++) {
      a_index = alph_index(alph, seq->sequence[i]);
      if (a_index == -1 || a_index >= a_size) continue;
      num_bases[a_index]++;
      total_bases++;
    }
  }

  freqs = allocate_array(a_size);
  for (i = 0; i < a_size; i++) {
    set_array_item(i, (double) num_bases[i] / (double) total_bases, freqs);
  }

  // Clean up the count of characters
  myfree(num_bases);

  return freqs;
}

/****************************************************************************
 *  Return an array containing the frequencies in the sequences for each
 *  character of the alphabet. Characters not in the alphabet are not
 *  counted.
 *
 *  When seq is provided it returns null, otherwise it converts the accumulated 
 *  result in bgcalc into a background.
 *
 *
 *  Pseudocode example:
 *    ALPH_T alph = ...
 *    BGCALC_T *bgcalc = NULL;
 *    for each seq:
 *      calculate_background(alph, seq, &bgcalc);
 *    ARRAY_T *bg = calculate_background(NULL, &bgcalc);
 ****************************************************************************/
ARRAY_T* calculate_background(
  ALPH_T alph,
  SEQ_T* seq,
  BGCALC_T** bgcalc
){
  BGCALC_T *calc;
  int a_size, i, a_index;
  char c;
  double freq, chunk_part, chunk_freq;
  ARRAY_T *background;
  assert(bgcalc != NULL);
  assert(seq != NULL || *bgcalc != NULL);
  // get the alphabet
  // get the alphabet size
  a_size = alph_size(alph, ALPH_SIZE);
  if (*bgcalc == NULL) {
    //allocate and initialize calc
    calc = mm_malloc(sizeof(BGCALC_T));
    calc->alph = alph;
    calc->chunk_seen = 0;
    calc->weight = 0;
    calc->chunk_counts = mm_malloc(a_size * sizeof(long));
    calc->bg = mm_malloc(a_size * sizeof(double));
    for (i = 0; i < a_size; ++i) {
      calc->chunk_counts[i] = 0;
      calc->bg[i] = 0;
    }
    *bgcalc = calc;
  } else {
    calc = *bgcalc;
    assert(alph == calc->alph);
    if (calc->weight == LONG_MAX) return NULL;
  }
  if (seq == NULL) {
    // no sequence so calculate the final result
    background = allocate_array(alph_size(alph, ALL_SIZE));
    if (calc->weight == 0) {
      if (calc->chunk_seen > 0) {
        // when we haven't had to approximate yet
        // just do a normal background calculation
        for (i = 0; i < a_size; i++) {
          freq = (double) calc->chunk_counts[i] / (double) calc->chunk_seen;
          set_array_item(i, freq, background);
        }
      } else {
        fputs("Uniform\n", stdout);
        // when there are no counts then return uniform
        freq = (double) 1 / (double) a_size;
        for (i = 0; i < a_size; i++) {
          set_array_item(i, freq, background);
        }
      }
    } else {
      if (calc->chunk_seen > 0) {
        // combine the frequencies for the existing chunks with the counts
        // for the partially completed chunk
        chunk_part = (double) calc->chunk_seen / (double) BG_CALC_CHUNK;
        for (i = 0; i < a_size; i++) {
          chunk_freq = (double) calc->chunk_counts[i] / 
              (double) calc->chunk_seen;
          freq = ((calc->bg[i] * calc->weight) + (chunk_freq * chunk_part)) / 
              (calc->weight + chunk_part);
          set_array_item(i, freq, background);
        }
      } else {
        // in the odd case we get to an integer number of chunks
        for (i = 0; i < a_size; i++) {
          set_array_item(i, calc->bg[i], background);
        }
      }
    }
    calc_ambigs(alph, FALSE, background);
    // free bgcalc structure
    free(calc->bg);
    free(calc->chunk_counts);
    free(calc);
    *bgcalc = NULL;
    return background;
  }
  // we have a sequence to add to the background calculation
  for (i = 0; i < seq->length; i++) {
    // check if the sequence has been indexed
    if (seq->isequence) {
      a_index = seq->isequence[i];
    } else {
      a_index = alph_index(alph, seq->sequence[i]);
    }
    if (a_index == -1 || a_index >= a_size) continue;
    calc->chunk_counts[a_index]++;
    calc->chunk_seen++;
    if (calc->chunk_seen == BG_CALC_CHUNK) {
      if (calc->weight == 0) {
        for (i = 0; i < a_size; i++) {
          calc->bg[i] = (double) calc->chunk_counts[i] / (double) BG_CALC_CHUNK;
        }
      } else {
        for (i = 0; i < a_size; i++) {
          chunk_freq = (double) calc->chunk_counts[i] / (double) BG_CALC_CHUNK;
          calc->bg[i] = (calc->bg[i] * calc->weight + chunk_freq) / 
              (calc->weight + 1);
        }
      }
      calc->weight++;
      // reset the counts for the next chunk
      for (i = 0; i < a_size; i++) {
        calc->chunk_counts[i] = 0;
      }
      calc->chunk_seen = 0;
      // I don't think it is feasible to reach this limit
      // but I guess I'd better check anyway
      if (calc->weight == LONG_MAX) {
        fprintf(stderr, "Sequence data set is so large that even the "
            "approximation designed for large datasets can't handle it!");
        return NULL;
      }
    }
  }
  return NULL;
}

/****************************************************************************
 * Free one sequence object.
 ****************************************************************************/
void free_seq
  (SEQ_T* a_sequence)
{
  if (a_sequence == NULL) {
    return;
  }
  myfree(a_sequence->sequence);
  myfree(a_sequence->isequence);
  myfree(a_sequence->intseq);
  myfree(a_sequence->gc);
  myfree(a_sequence->priors);
  myfree(a_sequence);
}

