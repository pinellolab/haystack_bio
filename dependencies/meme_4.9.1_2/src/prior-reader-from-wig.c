/******************************************************************************
 * FILE: prior-reader-from-wig.c
 * AUTHOR: Charles Grant, Bill Noble, Tim Bailey
 * CREATION DATE: 2012-09-12
 * COPYRIGHT: 2012 UW
 * 
 * This file contains the concrete implementation for a data block reader UDT
 * for reading priors from wiggle files.
 *****************************************************************************/

#include <errno.h>
#include <regex.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "prior-reader-from-wig.h"
#include "wiggle-reader.h"

typedef struct wig_data_block_reader {
  WIGGLE_READER_T *raw_reader;
  size_t current_position;
  char *sequence_name;
} WIG_DATA_BLOCK_READER_T;

// Forward declarations

// "Virtual" functions for DATA_BLOCK_READER
DATA_BLOCK_READER_T *new_prior_reader_from_wig(const char *filename);
BOOLEAN_T get_next_data_block_from_wig(DATA_BLOCK_READER_T *reader, DATA_BLOCK_T *data_block);
BOOLEAN_T go_to_next_sequence_in_wiggle_reader(DATA_BLOCK_READER_T *reader);
BOOLEAN_T get_seq_name_from_wig(DATA_BLOCK_READER_T *reader, char **name /*OUT*/);
BOOLEAN_T prior_reader_from_wig_is_eof(DATA_BLOCK_READER_T *reader);
BOOLEAN_T reset_prior_reader_from_wig(DATA_BLOCK_READER_T *reader);
BOOLEAN_T close_prior_reader_from_wig(DATA_BLOCK_READER_T *reader);
void free_prior_reader_from_wig(DATA_BLOCK_READER_T *reader);

/******************************************************************************
 * This function creates an instance of a data block reader UDT for reading
 * priors from a wiggle file.
 *****************************************************************************/
DATA_BLOCK_READER_T *new_prior_reader_from_wig(const char *filename) {

  WIGGLE_READER_T *raw_reader = new_wiggle_reader(filename);
  WIG_DATA_BLOCK_READER_T *wig_reader = mm_malloc(sizeof(WIG_DATA_BLOCK_READER_T) * 1);
  wig_reader->raw_reader = raw_reader;
  wig_reader->current_position = 0;
  wig_reader->sequence_name = NULL;

  // Setup "Virtual" function table
  DATA_BLOCK_READER_T *reader = new_data_block_reader(
    (void *) wig_reader,
    free_prior_reader_from_wig,
    close_prior_reader_from_wig,
    reset_prior_reader_from_wig,
    prior_reader_from_wig_is_eof,
    get_next_data_block_from_wig,
    go_to_next_sequence_in_wiggle_reader,
    get_seq_name_from_wig
  );

  return reader;
}

/******************************************************************************
 * This function frees an instance of the wiggle prior block reader UDT.
 *****************************************************************************/
void free_prior_reader_from_wig(DATA_BLOCK_READER_T *reader) {
  WIG_DATA_BLOCK_READER_T *wig_reader 
    = (WIG_DATA_BLOCK_READER_T *) get_data_block_reader_data(reader);
  myfree(wig_reader->sequence_name);
  myfree(wig_reader);
}

/******************************************************************************
 * This function closes a wiggle prior block reader UDT.
 *****************************************************************************/
BOOLEAN_T close_prior_reader_from_wig(DATA_BLOCK_READER_T *reader) {
  WIG_DATA_BLOCK_READER_T *wig_reader 
    = (WIG_DATA_BLOCK_READER_T *) get_data_block_reader_data(reader);
  wig_reader->current_position = 0;
  if (wig_reader->raw_reader) {
    free_wiggle_reader(wig_reader->raw_reader);
    wig_reader->raw_reader = NULL;
  }
  return TRUE;
}

/******************************************************************************
 * This function resets a wiggle prior block reader UDT.
 *****************************************************************************/
BOOLEAN_T reset_prior_reader_from_wig(DATA_BLOCK_READER_T *reader) {
  WIG_DATA_BLOCK_READER_T *wig_reader 
    = (WIG_DATA_BLOCK_READER_T *) get_data_block_reader_data(reader);
  reset_wiggle_reader(wig_reader->raw_reader);
  myfree(wig_reader->sequence_name);
  wig_reader->current_position = -1;
  return TRUE;
}

/******************************************************************************
 * This function reports on whether a prior reader has reached EOF
 * Returns TRUE if the reader is at EOF
 *****************************************************************************/
BOOLEAN_T prior_reader_from_wig_is_eof(DATA_BLOCK_READER_T *reader) {
  WIG_DATA_BLOCK_READER_T *wig_reader 
    = (WIG_DATA_BLOCK_READER_T *) get_data_block_reader_data(reader);
  return get_wiggle_eof(wig_reader->raw_reader) ? TRUE : FALSE;
}


/******************************************************************************
 * This function gets the name of the current sequence from the data block
 * reader. The name of the sequence is passed using the name parameter.
 * The caller is responsible for freeing the memory for the sequence name.
 *
 * Returns TRUE if successful, FALSE if there is no current sequence, as 
 * at the start of the file.
 *****************************************************************************/
BOOLEAN_T get_seq_name_from_wig(
  DATA_BLOCK_READER_T *reader, 
  char **name // OUT
) {

  BOOLEAN_T result = FALSE;
  WIG_DATA_BLOCK_READER_T *wig_reader 
    = (WIG_DATA_BLOCK_READER_T *) get_data_block_reader_data(reader);

  if (wig_reader->sequence_name == NULL) {
    result = FALSE;
  }
  else {
    *name = strdup(wig_reader->sequence_name);
    result = TRUE;
  }

  return result;
}

/******************************************************************************
 * Read from the current position in the file to the first prior after the
 * start of the next sequence. Set the value of the current sequence.
 *
 * Returns TRUE if it was able to advance to the next sequence, FALSE if 
 * EOF reached before the next sequence was found. Dies if other errors
 * encountered.
 *****************************************************************************/
BOOLEAN_T go_to_next_sequence_in_wiggle_reader(
  DATA_BLOCK_READER_T *reader
) {

  WIG_DATA_BLOCK_READER_T *wig_reader 
    = (WIG_DATA_BLOCK_READER_T *) get_data_block_reader_data(reader);
  BOOLEAN_T result = go_to_next_sequence_in_wiggle(wig_reader->raw_reader);
  wig_reader->sequence_name = get_wiggle_seq_name(wig_reader->raw_reader);
  wig_reader->current_position = 0;
  return result;
}

BOOLEAN_T get_next_data_block_from_wig(
  DATA_BLOCK_READER_T *reader, 
  DATA_BLOCK_T *data_block
) {

  BOOLEAN_T result = FALSE;
  int num_read = 0;

  WIG_DATA_BLOCK_READER_T *wig_reader 
    = (WIG_DATA_BLOCK_READER_T *) get_data_block_reader_data(reader);

  size_t step;
  size_t span;
  double value;
  result = get_next_data_block_from_wiggle(
    wig_reader->raw_reader,
    &(wig_reader->sequence_name),
    &(wig_reader->current_position),
    &step,
    &span,
    &value
  );
  set_start_pos_for_data_block(data_block, wig_reader->current_position);
  set_num_read_into_data_block(data_block, span);
  set_prior_in_data_block(data_block, value);

  return result;
}

