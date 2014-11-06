

#ifndef EPS2PNG_H
#define EPS2PNG_H

#include "utils.h"

/**************************************************************************
 * Check if converting an eps into a png is possible
 **************************************************************************/
BOOLEAN_T eps_to_png_possible();

/**************************************************************************
 * Convert an EPS to a PNG. Assumes you have an EPS file at path.eps and
 * want a png file at path.png
 **************************************************************************/
void eps_to_png(char *path);

/**************************************************************************
 * Convert a eps file to a png file.
 **************************************************************************/
void eps_to_png2(const char *eps_path, const char *png_path);

#endif
