/********************************************************************
 * FILE: wiggle-reader.c
 * AUTHOR: Charles Grant, Bill Noble, Tim Bailey, James Johnson
 * CREATION DATE: 2012-05-09
 * COPYRIGHT: 2012 UW
 *
 * This file contains the public interface for the data structures and
 * functions used to read wiggle files.
 ********************************************************************/

#ifndef WIGGLE_READER_H
#define WIGGLE_READER_H

#include "utils.h"

typedef enum { FIXED_WIGGLE_FMT, VAR_WIGGLE_FMT, BIGWIG_FMT, INVALID_FMT } INPUT_FMT_T;

#define WIGGLE_DEFAULT_SPAN 1

typedef struct wiggle_reader WIGGLE_READER_T;

WIGGLE_READER_T *new_wiggle_reader(const char *filename);

void free_wiggle_reader(WIGGLE_READER_T *reader);

void reset_wiggle_reader(WIGGLE_READER_T *reader);

/******************************************************************************
 * Read from the current position in the file to the first format line 
 * containing a new sequence.  Update the value of the current sequence.
 *
 * Returns TRUE if it was able to advance to the next sequence, FALSE if 
 * EOF reached before the next sequence was found. Dies if other errors
 * encountered.
 *****************************************************************************/
BOOLEAN_T go_to_next_sequence_in_wiggle(WIGGLE_READER_T *reader);


/******************************************************************************
 * Reads the next data line from the wiggle file
 * 
 * Returns TRUE if it was able to read the line, FALSE if it reaches
 * a new sequence sequence or EOF.  Dies if other errors encountered.
 *****************************************************************************/
BOOLEAN_T get_next_data_line_from_wiggle(
  WIGGLE_READER_T *reader,
  char **chrom,
  size_t *start,
  size_t *step,
  size_t *span,
  double *value
);

/******************************************************************************
 * Sets the state of the reader to the positon before the last data block was
 * read
 * 
 * Returns TRUE if it was able to rewind the reader, FALSE otherwise.
 * Dies if other errors encountered.
 *****************************************************************************/
BOOLEAN_T unget_data_line_from_wiggle(WIGGLE_READER_T *reader);

int get_wiggle_eof(WIGGLE_READER_T *reader);

const char* get_wiggle_filename(WIGGLE_READER_T *reader);

INPUT_FMT_T get_wiggle_format(WIGGLE_READER_T *reader);

size_t get_wiggle_span(WIGGLE_READER_T *reader);
size_t get_wiggle_step(WIGGLE_READER_T *reader);
char *get_wiggle_seq_name(WIGGLE_READER_T* reader);

#endif
