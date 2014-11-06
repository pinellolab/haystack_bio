/******************************************************************************
 * FILE: data-block-reader.h
 * AUTHOR: Charles Grant, Bill Noble, Tim Bailey
 * CREATION DATE: 2010-09-16
 * COPYRIGHT: 2010 UW
 * 
 * This file contains the declarations for the data structures and functions used 
 * for the data_block_reader. This is the interface wrapper for reading position
 * specific priors and sequences from files. Concrete implemenations exist for 
 * reading priors from MEME PSP files (prior-reader-from-psp.c), 
 * WIG files (prior-reader-from-wig.c), and for reading sequence segments 
 * from FASTA files (seq-reader-from-fasta.c).
 *****************************************************************************/

#ifndef DATA_BLOCK_READER_H
#define DATA_BLOCK_READER_H

#include "data-block.h"
#include "utils.h"
#include "seq.h"

typedef struct data_block_reader DATA_BLOCK_READER_T;

/******************************************************************************
 * This structure is the UDT for the data block reader interface.
 * It contains pointers to the actual data and the functions that provide
 * the concrete implementations.
 *****************************************************************************/
struct data_block_reader {

  void *data; // Implementation specific data

  // Pointers to implementation specific functions
  void (*free)(DATA_BLOCK_READER_T *reader);

  BOOLEAN_T (*close)(DATA_BLOCK_READER_T *reader);

  BOOLEAN_T (*reset)(DATA_BLOCK_READER_T *reader);

  BOOLEAN_T (*is_eof)(DATA_BLOCK_READER_T *reader);

  BOOLEAN_T (*get_next_block)(
    DATA_BLOCK_READER_T *reader,
    DATA_BLOCK_T *data_block
  );
  
  BOOLEAN_T (*unget_block)(DATA_BLOCK_READER_T *reader);
  
  BOOLEAN_T (*go_to_next_sequence)(DATA_BLOCK_READER_T *reader);

  BOOLEAN_T (*get_seq_name)( DATA_BLOCK_READER_T *reader, char **name);

};


/******************************************************************************
 * This function creates an instanace of the data block reader UDT.
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
  BOOLEAN_T (*get_seq_name)(
    DATA_BLOCK_READER_T *reader, 
    char **sequence
  )
);

/******************************************************************************
 * This function frees an instance of the data block reader UDT.
 *****************************************************************************/
void free_data_block_reader(DATA_BLOCK_READER_T *reader);

/******************************************************************************
 * This function gets the data member for the data block reader.
 * The data member contains information specific to the concrete implementation
 * of the reader. The data should not be freed by the caller. It will be freed
 * by when close_data_block_reader() is called.
 *
 * Returns a void pointer to the data. The underlying implementation routines
 * will cast it to the appropriate type.
 *****************************************************************************/
void *get_data_block_reader_data(DATA_BLOCK_READER_T *reader);

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
);

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
);

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
);

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
  double *priors // Out variable
);

/********************************************************
* Get the default prior from the prior reader
********************************************************/
double get_default_prior_from_reader(DATA_BLOCK_READER_T *prior_reader);

#endif
