/*
 * motif_regexp.h
 *
 *  Created on: 22/09/2008
 *      Author: rob
 */

#ifndef MOTIF_REGEXP_H_
#define MOTIF_REGEXP_H_

#include "motif.h"
#include "array-list.h"

ARRAYLST_T* read_regexp_file(
   char *filename, // IN Name of file containing regular expressions
   ARRAYLST_T *motifs // IN/OUT The retrieved motifs, allocated and returned if NULL
); 

#endif /* MOTIF_REGEXP_H_ */
