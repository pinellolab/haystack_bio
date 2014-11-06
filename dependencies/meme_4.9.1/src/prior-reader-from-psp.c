/******************************************************************************
 * FILE: prior-reader-from-psp.c
 * AUTHOR: Charles Grant, Bill Noble, Tim Bailey
 * CREATION DATE: 2010-09-17
 * COPYRIGHT: 2010 UW
 * 
 * This file contains the concrete implementation for a data block reader UDT
 * for reading priors from plain text PSP files originally support by MEME.
 * CONSIDER: This and the SEQ_READER_FROM_FASTA_T data structure are almost 
 * identical. Can they be refactored to share more of the implementation?
 *****************************************************************************/

#include <errno.h>
#include <regex.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "prior-reader-from-psp.h"

typedef struct psp_data_block_reader {
  BOOLEAN_T at_start_of_line;
  BOOLEAN_T parse_genomic_coord;
  char* filename;
  size_t filename_len; // Includes trailing '\0'
  size_t filename_buffer_len;
  char* sequence_header;
  size_t sequence_header_len; // Includes trailing '\0'
  size_t sequence_buffer_len;
  size_t sequence_name_len;
  FILE *psp_file;
  size_t current_position;
  char* sequence_name;
} PSP_DATA_BLOCK_READER_T;

// Forward declarations

// "Virtual" functions for DATA_BLOCK_READER
BOOLEAN_T get_next_data_block_from_psp(DATA_BLOCK_READER_T *reader, DATA_BLOCK_T *data_block);
BOOLEAN_T go_to_next_sequence_in_psp(DATA_BLOCK_READER_T *reader);
BOOLEAN_T get_seq_name_from_psp(DATA_BLOCK_READER_T *reader, char **name /*OUT*/);
BOOLEAN_T prior_reader_from_psp_is_eof(DATA_BLOCK_READER_T *reader);
BOOLEAN_T reset_prior_reader_from_psp(DATA_BLOCK_READER_T *reader);
BOOLEAN_T close_prior_reader_from_psp(DATA_BLOCK_READER_T *reader);
void free_prior_reader_from_psp(DATA_BLOCK_READER_T *reader);

// Functions peculiar to PSP reader
BOOLEAN_T read_seq_header_from_psp(PSP_DATA_BLOCK_READER_T *psp_reader);
static BOOLEAN_T parse_genomic_coordinates(PSP_DATA_BLOCK_READER_T *psp_reader);
static BOOLEAN_T parse_seq_name(PSP_DATA_BLOCK_READER_T *psp_reader);

/******************************************************************************
 * This function creates an instance of a data block reader UDT for reading
 * priors from a MEME PSP file.
 *****************************************************************************/
DATA_BLOCK_READER_T *new_prior_reader_from_psp(BOOLEAN_T parse_genomic_coord, const char *filename) {
  PSP_DATA_BLOCK_READER_T *psp_reader = mm_malloc(sizeof(PSP_DATA_BLOCK_READER_T) * 1);
  psp_reader->at_start_of_line = TRUE;
  psp_reader->parse_genomic_coord = parse_genomic_coord;
  int filename_len = strlen(filename) + 1;
  psp_reader->filename = mm_malloc(sizeof(char)* filename_len);
  psp_reader->filename_len = filename_len;
  strncpy(psp_reader->filename, filename, filename_len);
  psp_reader->current_position = 0;
  psp_reader->sequence_header = NULL;
  psp_reader->sequence_header_len = 0;
  psp_reader->psp_file = fopen(psp_reader->filename, "r");
  if (psp_reader->psp_file == NULL) {
    die(
      "Unable to open file: %s.\nError message: %s.\n", 
      psp_reader->filename, 
      strerror(errno)
    );
  }

  // Setup "Virtual" function table
  DATA_BLOCK_READER_T *reader = new_data_block_reader(
    (void *) psp_reader,
    free_prior_reader_from_psp,
    close_prior_reader_from_psp,
    reset_prior_reader_from_psp,
    prior_reader_from_psp_is_eof,
    get_next_data_block_from_psp,
    go_to_next_sequence_in_psp,
    get_seq_name_from_psp
  );
  return reader;
}

/******************************************************************************
 * This function frees an instance of the MEME PSP prior block reader UDT.
 *****************************************************************************/
void free_prior_reader_from_psp(DATA_BLOCK_READER_T *reader) {
  PSP_DATA_BLOCK_READER_T *psp_reader 
    = (PSP_DATA_BLOCK_READER_T *) get_data_block_reader_data(reader);
  myfree(psp_reader->filename);
  psp_reader->filename_len = 0;
  psp_reader->filename_buffer_len = 0;
  myfree(psp_reader->sequence_header);
  psp_reader->sequence_header_len = 0;
  psp_reader->sequence_buffer_len = 0;
  myfree(psp_reader->sequence_name);
  psp_reader->sequence_name_len = 0;
  myfree(psp_reader);
}

/******************************************************************************
 * This function closes a MEME PSP prior block reader UDT.
 *****************************************************************************/
BOOLEAN_T close_prior_reader_from_psp(DATA_BLOCK_READER_T *reader) {
  BOOLEAN_T result = FALSE;
  PSP_DATA_BLOCK_READER_T *psp_reader 
    = (PSP_DATA_BLOCK_READER_T *) get_data_block_reader_data(reader);
  psp_reader->current_position = 0;
  if (psp_reader->psp_file != NULL) {
    if (fclose(psp_reader->psp_file) == EOF) {
      die(
        "Error closing file: %s.\nError message: %s\n", 
        psp_reader->filename, 
        strerror(errno)
      );
    }
    else {
      result = TRUE;
    }
  }
  return result;
}

/******************************************************************************
 * This function resets a MEME PSP prior block reader UDT.
 *****************************************************************************/
BOOLEAN_T reset_prior_reader_from_psp(DATA_BLOCK_READER_T *reader) {
  PSP_DATA_BLOCK_READER_T *psp_reader 
    = (PSP_DATA_BLOCK_READER_T *) get_data_block_reader_data(reader);
  rewind(psp_reader->psp_file);
  psp_reader->current_position = -1;
  psp_reader->at_start_of_line = TRUE;
  return TRUE;
}

/******************************************************************************
 * This function reports on whether a prior reader has reached EOF
 * Returns TRUE if the reader is at EOF
 *****************************************************************************/
BOOLEAN_T prior_reader_from_psp_is_eof(DATA_BLOCK_READER_T *reader) {
  PSP_DATA_BLOCK_READER_T *psp_reader 
    = (PSP_DATA_BLOCK_READER_T *) get_data_block_reader_data(reader);
  return feof(psp_reader->psp_file) ? TRUE : FALSE;
}

/******************************************************************************
 * Read from the current position to the end of the current line.
 * Current position is assumed to be start of a new sequence.
 *
 * Returns TRUE if it was able to read the sequence text, FALSE if 
 * EOF reached before the terminal newline was found. Dies if other errors
 * are encountered.
 *****************************************************************************/
BOOLEAN_T read_seq_header_from_psp(
  PSP_DATA_BLOCK_READER_T *psp_reader
) {

  int result = FALSE;

  // Initial allocation of sequence buffer
  const size_t initial_buffer_len = 100;
  if (psp_reader->sequence_header == NULL) {
     psp_reader->sequence_header = mm_malloc(sizeof(char) * initial_buffer_len);
     psp_reader->sequence_buffer_len = initial_buffer_len;
  }

  // Look for EOL
  int c = 0;
  int seq_index = 0;
  while((c = fgetc(psp_reader->psp_file)) != EOF) {

    if (seq_index >= psp_reader->sequence_buffer_len) {
      // Need to grow buffer
      psp_reader->sequence_header 
        = mm_realloc(
          psp_reader->sequence_header, 
          2 * psp_reader->sequence_buffer_len
        );
      psp_reader->sequence_buffer_len = 2 * psp_reader->sequence_buffer_len;
    }

    if (c == '\n') {
      psp_reader->sequence_header[seq_index] = '\0';
      psp_reader->sequence_header_len = seq_index + 1;
      psp_reader->at_start_of_line = TRUE;
      result = TRUE;
      break;
    }
    else {
      psp_reader->sequence_header[seq_index] = c;
      ++seq_index;
    }
  }

  // At this point c is EOL or EOF
  if (c == EOF) {
    if (ferror(psp_reader->psp_file)) {
      // EOF could actually indicate an error.
      die(
        "Error reading file:%s.\nError message: %s\n", 
        psp_reader->filename,
        strerror(ferror(psp_reader->psp_file))
      );
    }
    else if (feof(psp_reader->psp_file)) {
      // Reached EOF before reaching EOL for the sequence.
      psp_reader->sequence_header[0] = '\0';
      psp_reader->sequence_header_len = 0;
    }
  }

  return result;

}

/******************************************************************************
 * This function gets the name of the current sequence from the data block
 * reader. The name of the sequence is passed using the name parameter.
 * The caller is responsible for freeing the memory for the sequence name.
 *
 * Returns TRUE if successful, FALSE if there is no current sequence, as 
 * at the start of the file.
 *****************************************************************************/
BOOLEAN_T get_seq_name_from_psp(
  DATA_BLOCK_READER_T *reader, 
  char **name // OUT
) {
  BOOLEAN_T result = FALSE;
  PSP_DATA_BLOCK_READER_T *psp_reader 
    = (PSP_DATA_BLOCK_READER_T *) get_data_block_reader_data(reader);
  if (psp_reader->sequence_name == NULL || psp_reader->sequence_name_len <= 0) {
    result = FALSE;
  }
  else {
    *name = strdup(psp_reader->sequence_name);
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
BOOLEAN_T go_to_next_sequence_in_psp(
  DATA_BLOCK_READER_T *reader
) {
  BOOLEAN_T result = FALSE;
  PSP_DATA_BLOCK_READER_T *psp_reader 
    = (PSP_DATA_BLOCK_READER_T *) get_data_block_reader_data(reader);
  psp_reader->current_position = 0;
  int c = 0;
  while((c = fgetc(psp_reader->psp_file)) != EOF) {
    if (psp_reader->at_start_of_line == TRUE && c == '>') {
      break;
    }
    else if (c == '\n') {
      psp_reader->at_start_of_line = TRUE;
    }
    else {
      psp_reader->at_start_of_line = FALSE;
    }
  }
  // At this point c is '>' or EOF
  if (c == '>') {
    BOOLEAN_T found_genomic_coordinates = FALSE;
    result = read_seq_header_from_psp(psp_reader);
    if (result == TRUE && psp_reader->parse_genomic_coord) {
      // Look for genomic coordinates in header
      found_genomic_coordinates = parse_genomic_coordinates(psp_reader);
    }
    if (found_genomic_coordinates == FALSE) {
      //  Look for whitespace in header
      //  The sequence name is the string before the white space.
      BOOLEAN_T found_name = FALSE;
      found_name = parse_seq_name(psp_reader);
      if (found_name == FALSE) {
        die(
            "Unable to find sequence name in header %s.\n",
            psp_reader->sequence_header
        );
      }
    }
  }
  else {
    if (ferror(psp_reader->psp_file)) {
      die(
        "Error reading file:%s.\nError message: %s\n", 
        psp_reader->filename,
        strerror(ferror(psp_reader->psp_file))
      );
    }
    else if (feof(psp_reader->psp_file)) {
        // Reached EOF before reaching the start of the sequence
        result = FALSE;
    }
  }
  return result;
}

/******************************************************************************
 * Reads in the next data block for the priors. 
 * 
 * Returns TRUE if it was able to read the block, FALSE if it reaches
 * the next sequence or if the EOF.
 * Dies if other errors encountered.
 *****************************************************************************/
BOOLEAN_T get_next_data_block_from_psp(
  DATA_BLOCK_READER_T *reader, 
  DATA_BLOCK_T *data_block
) {

  BOOLEAN_T result = FALSE;
  const int buffer_size = 100;
  char buffer[buffer_size];
  int num_read = 0;

  PSP_DATA_BLOCK_READER_T *psp_reader 
    = (PSP_DATA_BLOCK_READER_T *) get_data_block_reader_data(reader);

  int c = 0;

  // Skip over leading white space
  while((c = fgetc(psp_reader->psp_file)) != EOF) {

    if (isspace(c)) {
      if (c == '\n') {
        psp_reader->at_start_of_line = TRUE;
      }
      else {
        psp_reader->at_start_of_line = FALSE;
      }
      continue;
    }
    else {
      break;
    }
  }

  if (c == '>' && psp_reader->at_start_of_line == TRUE) {
    // We found the start of a new sequence while trying
    // to find a prior.
    c = ungetc(c, psp_reader->psp_file);
    if (ferror(psp_reader->psp_file)) {
      die(
        "Error reading file:%s.\nError message: %s\n", 
        psp_reader->filename,
        strerror(ferror(psp_reader->psp_file))
      );
    }
  }
  else {
    // We are at start of a prior.
    // Read prior string until next space or EOF.
    int buffer_index = 0;
    while(c != EOF && !isspace(c)) {
      buffer[buffer_index] = c;
      ++buffer_index;
      if (buffer_index >= (buffer_size - 1)) {
        // No prior string should be this long
        buffer[buffer_size - 1] = 0;
        die("File %s contains invalid prior value: %s\n", psp_reader->filename, buffer);
      }
      c = fgetc(psp_reader->psp_file);
    }

    if (c == '\n') {
      psp_reader->at_start_of_line = TRUE;
    }
    else {
      psp_reader->at_start_of_line = FALSE;
    }

    buffer[buffer_index] = '\0';

    // If the buffer is not empty, it should contain a string
    // representing the prior. Convert it to a double.
    if (buffer_index != 0) {
      char *end_ptr = NULL;
      double prior = strtod(buffer, &end_ptr);
      if (end_ptr == buffer 
          || *end_ptr != '\0' 
          || prior < 0.0L 
          || prior > 1.0L
      ) {
        die("File %s contains invalid prior value: %s\n", psp_reader->filename, buffer);
      }
      set_prior_in_data_block(data_block, prior);
      num_read = 1;
      ++psp_reader->current_position;
      result = TRUE;
    }

  }

  if (c == EOF && ferror(psp_reader->psp_file)) {
    die(
      "Error while reading file:%s.\nError message: %s\n", 
      psp_reader->filename,
      strerror(ferror(psp_reader->psp_file))
    );
  }

  set_start_pos_for_data_block(data_block, psp_reader->current_position);
  set_num_read_into_data_block(data_block, num_read);
  return result;
}

/******************************************************************************
 * This function attempts to parse genomic coordinates from the
 * current sequence header. If successful it will set the sequence
 * name to the chromosome name, and set the starting sequence position.
 *
 * Returns TRUE if it was able to find genomic coordinates, FALSE otherwise.
 *****************************************************************************/
static BOOLEAN_T parse_genomic_coordinates(
  PSP_DATA_BLOCK_READER_T *psp_reader
) {

  #define NUM_SUBMATCHES 4
  #define ERROR_MESSAGE_SIZE 100
  #define BUFFER_SIZE 512

  static BOOLEAN_T first_time = TRUE;
  static regex_t fasta_header_regex;
  static regmatch_t matches[NUM_SUBMATCHES];

  char error_message[ERROR_MESSAGE_SIZE];
  int status = 0;

  if (first_time == TRUE) {
    // Initialize regular express for extracting chromsome coordinates;
    // Expected format is name:start-stop
    status = regcomp(
      &fasta_header_regex, 
      "[[:space:]]*([^[:space:]:]+):([[:digit:]]+)-([[:digit:]]+)", 
      REG_EXTENDED | REG_ICASE | REG_NEWLINE
    );

    if (status != 0) {
      regerror(status, &fasta_header_regex, error_message, ERROR_MESSAGE_SIZE);
      die(
        "Error while intitializing regular expression\n"
        "for parsing genome coordinates from FASTA header: %s\n", 
        error_message
      );
    }

    first_time = FALSE;
  }

  BOOLEAN_T found_coordinates = FALSE;
  char* header = psp_reader->sequence_header;
  status = regexec(
    &fasta_header_regex, 
    header,
    NUM_SUBMATCHES, 
    matches, 
    0
  );
  if(!status) {
    // The sequence header contains genomic coordinates
    found_coordinates = TRUE;

    // Copy chromosome name and position to reader
    char buffer[BUFFER_SIZE];
    int name_len = matches[1].rm_eo - matches[1].rm_so;
    strncpy(buffer, header + matches[1].rm_so, name_len);
    buffer[name_len] = 0;
    psp_reader->sequence_name = strdup(buffer);
    if (psp_reader->sequence_name == NULL) {
      die("Unable to allocate memory while parsing sequence header.\n");
    }
    psp_reader->sequence_name_len = name_len;
    size_t pos_len = matches[2].rm_eo - matches[2].rm_so;
    strncpy(buffer, header + matches[2].rm_so, pos_len);
    buffer[pos_len] = 0;
    psp_reader->current_position = atol(buffer) - 1;
  }
  else if(status != REG_NOMATCH ){
    regerror(status, &fasta_header_regex, error_message, 100);
    die("Error trying to parse genome coordinates from sequence header: %s\n"
        "error message is %s\n",
        header, 
        error_message
    );
  }
  else {
    found_coordinates = FALSE;
  }

  return found_coordinates;

  #undef NUM_SUBMATCHES
  #undef ERROR_MESSAGE_SIZE
  #undef BUFFER_SIZE
}

/******************************************************************************
 * This function attempts to parse the sequence name from the
 * current sequence header. The sequence name is the string up to the first
 * white space in the sequence header.
 *
 * Returns TRUE if it was able to parse genomic coordinates, FALSE otherwise.
 *****************************************************************************/
static BOOLEAN_T parse_seq_name(
  PSP_DATA_BLOCK_READER_T *psp_reader
) {

  #define NUM_SUBMATCHES 2
  #define ERROR_MESSAGE_SIZE 100
  #define BUFFER_SIZE 512

  static BOOLEAN_T first_time = TRUE;
  static regex_t fasta_header_regex;
  static regmatch_t matches[NUM_SUBMATCHES];

  char error_message[ERROR_MESSAGE_SIZE];
  int status = 0;

  if (first_time == TRUE) {
    // Initialize regular express for extracting chromsome coordinates;
    // Expected format is name:start-stop
    status = regcomp(
      &fasta_header_regex, 
      "([^[:space:]:]+)[[:space:]]*", 
      REG_EXTENDED | REG_ICASE | REG_NEWLINE
    );

    if (status != 0) {
      regerror(status, &fasta_header_regex, error_message, ERROR_MESSAGE_SIZE);
      die(
        "Error while intitializing regular expression\n"
        "for parsing sequence name from FASTA header: %s\n", 
        error_message
      );
    }

    first_time = FALSE;
  }

  BOOLEAN_T found_name = FALSE;
  char* header = psp_reader->sequence_header;
  status = regexec(
    &fasta_header_regex, 
    header,
    NUM_SUBMATCHES, 
    matches, 
    0
  );
  if(!status) {
    // The sequence header contains genomic coordinates
    found_name = TRUE;

    // Copy sequence name to reader
    char buffer[BUFFER_SIZE];
    int name_len = matches[1].rm_eo - matches[1].rm_so;
    strncpy(buffer, header + matches[1].rm_so, name_len);
    buffer[name_len] = 0;
    psp_reader->sequence_name = strdup(buffer);
    if (psp_reader->sequence_name == NULL) {
      die("Unable to allocate memory while parsing sequence header.\n");
    }
    psp_reader->sequence_name_len = name_len;
  }
  else if(status != REG_NOMATCH ){
    regerror(status, &fasta_header_regex, error_message, 100);
    die("Error trying to parse name from sequence header: %s\n"
        "error message is %s\n",
        header, 
        error_message
    );
  }
  else {
    found_name = FALSE;
  }

  return found_name;

  #undef NUM_SUBMATCHES
  #undef ERROR_MESSAGE_SIZE
  #undef BUFFER_SIZE
}
