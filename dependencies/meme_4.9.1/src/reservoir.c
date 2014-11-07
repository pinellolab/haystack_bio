/******************************************************************************
 * FILE: reservoir.c
 * AUTHOR: Charles Grant and Bill Noble
 * CREATION DATE: 2010-10-22
 * COPYRIGHT: 2010 UW
 * 
 * This file contains the implementation for data structures and functions used 
 * for reservoir sampling.  Based on Knuth's "Semi-Numerical Algorithms", 
 * Algorithm R. pgs. 138-139
 *****************************************************************************/

#include "reservoir.h"
#include "utils.h"

/***********************************************************************
  Data structure for reservoir sampling.
  It can accomodate doubles or pointers to more complex data structures.
 ***********************************************************************/
struct reservoir_sampler {
  size_t size;
  double *samples;
  void **sample_pointers;
  void (*free_sample)(void *);
  size_t num_samples_seen;
  size_t num_samples_retained;
  size_t num_samples_swapped;
};

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
) {

  RESERVOIR_SAMPLER_T *reservoir = mm_malloc(sizeof(RESERVOIR_SAMPLER_T) * 1);
  reservoir->size = size;
  reservoir->samples = NULL;
  reservoir->sample_pointers = NULL;
  if (free_sample == NULL) {
    reservoir->samples = mm_malloc(sizeof(double) * size);
  }
  else {
    reservoir->sample_pointers = mm_malloc(sizeof(double *) * size);
  }
  reservoir->num_samples_seen = 0;
  reservoir->num_samples_retained = 0;
  reservoir->num_samples_swapped = 0;
  reservoir->free_sample = free_sample;

  return reservoir;

}

/***********************************************************************
  Frees memory associated with a reservoir sampler
 ***********************************************************************/
void free_reservoir(RESERVOIR_SAMPLER_T *reservoir) {
  if (reservoir->free_sample != NULL) {
    int i_sample = 0;
    for (i_sample = 0; i_sample < reservoir->num_samples_retained; ++i_sample) {
      reservoir->free_sample(reservoir->sample_pointers[i_sample]);
    }
  }
  myfree(reservoir->samples);
  myfree(reservoir->sample_pointers);
  myfree(reservoir);
}

/***********************************************************************
  Resets the values in the reservoir sampler.
 ***********************************************************************/
void clear_reservoir(RESERVOIR_SAMPLER_T *reservoir) {
  if (reservoir->free_sample != NULL) {
    int i_sample = 0;
    for (i_sample = 0; i_sample < reservoir->num_samples_retained; ++i_sample) {
      reservoir->free_sample(reservoir->sample_pointers[i_sample]);
      reservoir->sample_pointers[i_sample] = NULL;
    }
  }
  reservoir->num_samples_seen = 0;
  reservoir->num_samples_retained = 0;
  reservoir->num_samples_swapped = 0;
}

/***********************************************************************
  Get the size of the reservoir sampler.
 ***********************************************************************/
size_t get_reservoir_size(RESERVOIR_SAMPLER_T *reservoir) {
  return reservoir->size;
}

/***********************************************************************
  Get the number of samples seen by the reservoir.
 ***********************************************************************/
size_t get_reservoir_num_samples_seen(RESERVOIR_SAMPLER_T *reservoir) {
  return reservoir->num_samples_seen;
}

/***********************************************************************
  Get the number of samples retained in the reservoir sampler.
 ***********************************************************************/
size_t get_reservoir_num_samples_retained(RESERVOIR_SAMPLER_T *reservoir) {
  return reservoir->num_samples_retained;
}

/***********************************************************************
  Retrieve the samples values form the reservoir sampler.
  The user should not free these values.
  They will be freed when the reservoir sampler is freed.
 ***********************************************************************/
double *get_reservoir_samples(RESERVOIR_SAMPLER_T *reservoir) {
  return reservoir->samples;
}

/***********************************************************************
  Submit one sample to the reservoir sampler.
 ***********************************************************************/
void reservoir_sample(RESERVOIR_SAMPLER_T *reservoir, double sample) {
  if (reservoir->samples == NULL) {
    die("Attempt to add a double to a reservoir initalized for pointers");
  }
  if (reservoir->num_samples_retained < reservoir->size) {
    // The first samples go directory into the reservoir
    // until it is filled.
    (reservoir->samples)[reservoir->num_samples_retained] = sample;
    ++reservoir->num_samples_seen;
    ++reservoir->num_samples_retained;
  }
  else {
    ++reservoir->num_samples_seen;
    int r = reservoir->num_samples_seen * my_drand();
    if (r < reservoir->size) {
      (reservoir->samples)[r] = sample;
      ++reservoir->num_samples_swapped;
    }
  }
}

/***********************************************************************
  Submit one sample pointer to the reservoir sampler.
 ***********************************************************************/
void reservoir_sample_pointer(RESERVOIR_SAMPLER_T *reservoir, void *sample) {
  if (reservoir->sample_pointers == NULL) {
    die("Attempt to add a double to a sample reservoir initalized for pointers");
  }
  if (reservoir->num_samples_retained < reservoir->size) {
    // The first samples go directory into the reservoir
    // until it is filled.
    (reservoir->sample_pointers)[reservoir->num_samples_retained] = sample;
    ++reservoir->num_samples_seen;
    ++reservoir->num_samples_retained;
  }
  else {
    ++reservoir->num_samples_seen;
    int r = reservoir->num_samples_seen * my_drand();
    if (r < reservoir->size) {
      reservoir->free_sample(reservoir->sample_pointers[r]);
      (reservoir->sample_pointers)[r] = sample;
      ++reservoir->num_samples_swapped;
    }
  }
}
