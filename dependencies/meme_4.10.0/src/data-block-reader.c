/******************************************************************************
 * FILE: data-block-reader.c
 * AUTHOR: Charles Grant, Bill Noble, Tim Bailey
 * CREATION DATE: 2010-09-16
 * COPYRIGHT: 2010 UW
 * 
 * This file contains the implementation for the data structures and functions used 
 * for the data_block_reader. This is the interface wrapper for reading position
 * specific priors and sequences from files. Concrete implemenations exist for 
 * reading priors from MEME PSP files (prior-reader-from-psp.c), 
 * WIG files (prior-reader-from-wig.c), and for reading sequence segments 
 * from FASTA files (seq-reader-from-fasta.c).
 *****************************************************************************/

#include <stdlib.h>
#include "data-block-reader.h"
#include "prior-reader-from-wig.h"
#include "fasta-io.h"


/******************************************************************************
 * This function creates an instance of the data block reader UDT.
 * It is only intended to be called by the actual implemenations of the data 
 * block reader interface.
 *
 * Returns a pointer to an instance of a data block reader UDT.
 *****************************************************************************/
DATA_BLOCK_READER_T *new_data_block_reader(
  void *data,
  void (*free)(DATA_BLOCK_READER_T *reader),
  BOOLEAN_T (*close)(DATA_BLOCK_READER_T *reader),
  BOOLEAN_T (*reset)(DATA_BLOCK_READER_T *reader),
  BOOLEAN_T (*is_eof)(DATA_BLOCK_READER_T *reader),
  BOOLEAN_T (*get_next_block)(
    DATA_BLOCK_READER_T *reader, 
    DATA_BLOCK_T *data_block 
  ),
  BOOLEAN_T (*unget_block)(DATA_BLOCK_READER_T *reader),
  BOOLEAN_T (*go_to_next_sequence)(DATA_BLOCK_READER_T *reader),
  BOOLEAN_T (*get_seq_name)(DATA_BLOCK_READER_T *reader, char **sequence)
) {
  DATA_BLOCK_READER_T *reader = mm_malloc(sizeof(DATA_BLOCK_READER_T) * 1);
  reader->data = data;
  reader->free = free;
  reader->close = close;
  reader->reset = reset;
  reader->is_eof = is_eof;
  reader->get_next_block = get_next_block;
  reader->unget_block = unget_block;
  reader->go_to_next_sequence = go_to_next_sequence;
  reader->get_seq_name = get_seq_name;
  return reader;
}

/******************************************************************************
 * This function frees an instanace of the data block reader UDT.
 *****************************************************************************/
void free_data_block_reader(DATA_BLOCK_READER_T *reader) {
  reader->free(reader);
  myfree(reader);
}

/******************************************************************************
 * This function gets the data member for the data block reader.
 * The data member contains information specific to the concrete implemenation
 * of the reader. The data should not be freed by the caller. It will be freed
 * by when close_data_block_reader() is called.
 *
 * Returns a void pointer to the data. The underlying implementation routines
 * will cast it to the appropriate type.
 *****************************************************************************/
void *get_data_block_reader_data(DATA_BLOCK_READER_T *reader) {
  return reader->data;
}

/****************************************************************************
 * Read priors until a new sequence is encountered or too many letters
 * are read.  The new priors are copied to the priors buffer of the given
 * sequence starting at buffer_offset.
 ****************************************************************************/
void read_one_priors_segment_from_reader(
   DATA_BLOCK_READER_T *prior_reader,
   size_t max_size,
   size_t buffer_offset,
   SEQ_T *sequence
) {

  assert(sequence != NULL);

  char *seq_name = get_seq_name(sequence);
  size_t seq_start = get_seq_offset(sequence);
  // Get the priors buffer from the SEQ_T
  double *priors = get_seq_priors(sequence);
  if (priors == NULL) {
    // Priors buffer not yet allocated
    priors = mm_malloc(max_size * sizeof(double));
  }

  get_prior_array_from_reader(
    prior_reader, 
    seq_name, 
    seq_start, 
    max_size, 
    buffer_offset,
    priors
  );
  set_seq_priors(priors, sequence);

}

/******************************************************************************
 * This function allocates and initializes a SEQ_T object from a FASTA and
 * a prior reader. The prior reader is optional and may be null.
 *
 * Returns a pointer to a new SEQ_T object.
 *****************************************************************************/
SEQ_T *get_next_seq_from_readers(
  DATA_BLOCK_READER_T *fasta_reader, 
  DATA_BLOCK_READER_T *prior_reader,
  size_t max_size
) {

  // Move to the next sequence in the fasta file.
  BOOLEAN_T got_seq = fasta_reader->go_to_next_sequence(fasta_reader);
  if (got_seq == FALSE) {
    // Reached EOF
    return NULL;
  }
  char *fasta_seq_name = NULL;
  fasta_reader->get_seq_name(fasta_reader, &fasta_seq_name);
  size_t seq_offset = get_current_pos_from_seq_reader_from_fasta(fasta_reader);
  SEQ_T *sequence = allocate_seq(
    fasta_seq_name, 
    NULL, // description
    seq_offset,
    NULL // raw sequence
  );
  // Read the first raw sequence segment into the sequence.
  read_one_fasta_segment_from_reader(
    fasta_reader,
    max_size, 
    0, // No buffer offset on first segment
    sequence
  );

  // Move to the next sequence in the priors file.
  if (prior_reader) {
    BOOLEAN_T got_priors_seq = prior_reader->go_to_next_sequence(prior_reader);
    if (got_priors_seq == FALSE) {
      die("Unable to read sequence from priors file.");
    }
    // Check that the sequence name from the FASTA reader matches
    // the sequence name from the prior reader.
    char *prior_seq_name = NULL;
    prior_reader->get_seq_name(prior_reader, &prior_seq_name);
    if (strcmp(fasta_seq_name, prior_seq_name) != 0) {
      die(
        "Sequence named %s from prior reader did not "
        "match sequence name %s from fasta reader\n",
        prior_seq_name,
        fasta_seq_name
      );
    }
    // Read the first segment of priors data into the sequence.
    read_one_priors_segment_from_reader(
      prior_reader,
      max_size,
      0, // No buffer offset on first segment
      sequence 
    );
  }

  myfree(fasta_seq_name);
  return sequence;

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
  DATA_BLOCK_READER_T *prior_reader,
  const char *seq_name,
  size_t seq_position,
  DATA_BLOCK_T **prior_block, // Out variable
  double *prior // Out variable
) {

  double default_prior = get_default_prior_from_reader(prior_reader);
  *prior = default_prior;

  if (*prior_block == NULL) {
    // Allocate prior block if not we've not already done so
    *prior_block = new_prior_block();
    // Fill in first data for block 
    BOOLEAN_T result = prior_reader->get_next_block(prior_reader, *prior_block);
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
  else if (block_position <= seq_position 
           && seq_position <= (block_position + block_extent - 1)) {
    // Sequence position is contained in current prior block
    *prior = get_prior_from_data_block(*prior_block);
  }
  else {
    // Sequence position is after current prior position.
    // Try reading the next prior block.
    BOOLEAN_T priors_remaining = FALSE;
    while ((priors_remaining = prior_reader->get_next_block(prior_reader, *prior_block)) != FALSE) {
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

/********************************************************
* Read an array of priors from the priors file.
*
* If no prior for in the sequence and coordinate given 
* is found in the prior file, the priors will be set to 0.5
*
* If the seq. position is within the extent of the current
* prior block we set the prior to the value from the block.
*
* If the seq. position is past the extent of the current
* prior block we read blocks until reach the seq. position
* or we reach the end of the sequence.
*
* Sequences must occur in the same order as the FASTA file
* Positions in sequence must be in increasing order
********************************************************/
void get_prior_array_from_reader(
  DATA_BLOCK_READER_T *prior_reader,
  const char *seq_name,
  size_t seq_start,
  size_t num_priors,
  size_t buffer_offset,
  double *priors
) {

  size_t seq_end = seq_start + num_priors;
  char *prior_seq_name = NULL;
  prior_reader->get_seq_name(prior_reader, &prior_seq_name);

  assert(strcmp(seq_name, prior_seq_name) == 0);

  // Fill the array with the default prior 
  // starting at the buffer offset
  double default_prior = get_default_prior_from_reader(prior_reader);
  size_t i;
  for (i = buffer_offset; i < num_priors; ++i) {
    priors[i] = default_prior;
  }

  BOOLEAN_T result;
  size_t prior_start;
  size_t prior_length;
  size_t prior_end;
  DATA_BLOCK_T *prior_block = new_prior_block();

  // Read and copy prior blocks until we've filled in the array
  while (TRUE) {

    result = prior_reader->get_next_block(prior_reader, prior_block);
    if (result == FALSE) {
      // Reached the end of priors for this sequence
      break;
    }
    prior_start = get_start_pos_for_data_block(prior_block) - 1;
    prior_length = get_num_read_into_data_block(prior_block);
    prior_end = prior_start + prior_length - 1;

    if (prior_end < seq_start) {
      // Skip prior blocks before region of interest
      continue;
    }

    // Copy the priors into the array
    size_t start_intersect = MAX(prior_start, seq_start);
    size_t end_intersect = MIN(prior_end, seq_end);
    BOOLEAN_T overlap = (end_intersect >= start_intersect);
    if (overlap == TRUE) {
      // FIXEME CEG
      // size_t num_to_copy = end_intersect - start_intersect + 1;
      size_t num_to_copy = end_intersect - start_intersect;
      size_t intersect_offset =  start_intersect - seq_start;
      for (i = 0; i < num_to_copy; ++i) {
        priors[intersect_offset + i] = get_prior_from_data_block(prior_block);
      }
    }

    if (prior_end > seq_end) {
      // We're done filling the array, but have some priors left over
      // Rewind the reader to before the last block read
      prior_reader->unget_block(prior_reader);
      break;
    }

  }

  free_data_block(prior_block);
  return;
}

/********************************************************
* Get the default prior from the prior reader
********************************************************/
double get_default_prior_from_reader(DATA_BLOCK_READER_T *prior_reader) {

    WIG_PRIOR_BLOCK_READER_T* wig_reader = (WIG_PRIOR_BLOCK_READER_T *) prior_reader->data;
    return get_default_prior_from_wig(wig_reader);
}

