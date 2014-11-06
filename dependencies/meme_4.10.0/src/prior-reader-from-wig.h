/******************************************************************************
 * FILE: prior-reader-from-wig.h
 * AUTHOR: Charles Grant, Bill Noble, Tim Bailey
 * CREATION DATE: 2012-09-12
 * COPYRIGHT: 2012 UW
 * 
 * This file contains the public declarations for a data block reader UDT for 
 * reading priors from wiggle files.
 *****************************************************************************/

#ifndef PRIOR_READER_FROM_WIG_H
#define PRIOR_READER_FROM_WIG_H

#include "data-block-reader.h"

typedef struct wig_prior_block_reader WIG_PRIOR_BLOCK_READER_T;

/******************************************************************************
 * This function creates an instance of a data block reader UDT for reading
 * priors from a wiggle file.
 *****************************************************************************/
DATA_BLOCK_READER_T *new_prior_reader_from_wig(
  const char *filenae, 
  double default_prior
);

/******************************************************************************
 * This function returns the value of the default prior for a WIGGLE_READER_T
 *****************************************************************************/
double get_default_prior_from_wig(WIG_PRIOR_BLOCK_READER_T *prior_reader);

#endif
