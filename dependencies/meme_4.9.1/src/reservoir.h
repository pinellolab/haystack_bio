/******************************************************************************
 * FILE: reservoir.h
 * AUTHOR: Charles Grant, Bill Noble, Tim Bailey
 * CREATION DATE: 2010-10-22
 * COPYRIGHT: 2010 UW
 * 
 * This file contains the declarations for data structures and functions used 
 * for reservoir sampling.  Based on Knuth's "Semi-Numerical Algorithms", 
 * Algorithm R. pgs. 138-139
 *****************************************************************************/

#ifndef RESERVOIR_H
#define RESERVOIR_H

#include <stdlib.h>

typedef struct reservoir_sampler RESERVOIR_SAMPLER_T;

/***********************************************************************
  Creates a reservoir sampler.
  If the free_sample function is NULL, the reservoir can only be used 
  to store doubles. If the function pointer free_sample is provided, 
  it can store pointers to an abitrary data structure, though all data
  structrers store in the reservoir must be of the same type.
  The free_sample function will be used to free the structures as they 
  are dropped from the reservoir.
 ***********************************************************************/
RESERVOIR_SAMPLER_T *new_reservoir_sampler(
  size_t size, 
  void (*free_sample)(void *)
);

/***********************************************************************
  Frees memory associated with a reservoir sampler.
 ***********************************************************************/
void free_reservoir(RESERVOIR_SAMPLER_T *reservoir);

/***********************************************************************
  Resets the values in the reservoir sampler.
 ***********************************************************************/
void clear_reservoir(RESERVOIR_SAMPLER_T *reservoir);

/***********************************************************************
  Get the size of the reservoir sampler.
 ***********************************************************************/
size_t get_reservoir_size(RESERVOIR_SAMPLER_T *reservoir);

/***********************************************************************
  Get the number of samples seen by the reservoir.
 ***********************************************************************/
size_t get_reservoir_num_samples_seen(RESERVOIR_SAMPLER_T *reservoir);

/***********************************************************************
  Get the number of samples retained in the reservoir sampler.
 ***********************************************************************/
size_t get_reservoir_num_samples_retained(RESERVOIR_SAMPLER_T *reservoir);

/***********************************************************************
  Retreive the samples values form the reservoir sampler.
  The user should not free these values.
  They will be freed when the reservoir sampler is freed.
 ***********************************************************************/
double *get_reservoir_samples(RESERVOIR_SAMPLER_T *reservoir);

/***********************************************************************
  Submit one sample to the reservoir sampler.
 ***********************************************************************/
void reservoir_sample(RESERVOIR_SAMPLER_T *reservoir, double value);

/***********************************************************************
  Submit one sample pointer to the reservoir sampler.
 ***********************************************************************/
void reservoir_sample_pointer(RESERVOIR_SAMPLER_T *reservoir, void *sample);

#endif
