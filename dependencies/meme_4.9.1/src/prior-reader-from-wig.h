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

/******************************************************************************
 * This function creates an instance of a data block reader UDT for reading
 * priors from a wiggle file.
 *****************************************************************************/
DATA_BLOCK_READER_T *new_prior_reader_from_wig(const char *filenae);

#endif
