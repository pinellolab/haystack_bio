/******************************************************************************
 * FILE: seq-reader-from-fasta.h
 * AUTHOR: Charles Grant, Bill Noble, Tim Bailey
 * CREATION DATE: 2010-09-17
 * COPYRIGHT: 2010 UW
 * 
 * This file contains the public declarations for the concrete implementation of a 
 * data-block-reader UDT for reading sequence segments from a FASTA file.
 *****************************************************************************/

#ifndef SEQ_READER_FROM_FASTA_H
#define SEQ_READER_FROM_FASTA_H

#include "alphabet.h"
#include "data-block-reader.h"

/******************************************************************************
 * This function creates an instance of a data block reader UDT for reading
 * sequence segments from a FASTA file.
 *****************************************************************************/
DATA_BLOCK_READER_T *new_seq_reader_from_fasta(
  BOOLEAN_T parse_genomic_coord, 
  ALPH_T alph, 
  const char *filename
);

/******************************************************************************
 * This function parses a FASTA sequence header and returns the
 * the chromosome name and starting position if the header has the format
 *	"chrXX:start-end", where start and end are positive integers.
 *****************************************************************************/
BOOLEAN_T parse_genomic_coordinates_helper(
  char* header,           // FASTA sequence header
  char** chr_name,  	  // chromosome name ((chr[^:]))
  size_t * chr_name_len_ptr, // number of characters in chromosome name
  int * start_ptr,        // start position of sequence (chr:(\d+)-)
  int * end_ptr           // end position of sequence (chr:\d+-(\d+))
);

#endif
