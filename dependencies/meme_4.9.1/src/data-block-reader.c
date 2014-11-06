/******************************************************************************
 * FILE: data-block-reader.c
 * AUTHOR: Charles Grant, Bill Noble, Tim Bailey
 * CREATION DATE: 2010-09-16
 * COPYRIGHT: 2010 UW
 * 
 * This file contains the implementation for the data structures and functions used 
 * for the data_block_reader. This is the interface wrapper for reading position
 * specific priors and sequences from files. Concrete implemenations exist for 
 * reading priors from MEME PSP files (prior-reader-from-psp.c), and for
 * reading sequence segments from FASTA files (seq-reader-from-fasta.c).
 *****************************************************************************/

#include <stdlib.h>
#include "data-block-reader.h"

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

