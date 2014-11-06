/**************************************************************************
 * FILE: json-reader.h
 * AUTHOR: James Johnson 
 * CREATE DATE: 26 February 2012
 * PROJECT: shared
 * COPYRIGHT: UQ, 2013
 * VERSION: $Revision: 1.0 $
 * DESCRIPTION: Provides a interface for parsing JSON data out of a HTML file
 * provided that the data has been landmarked with "@JSON_VAR "
 **************************************************************************/

#ifndef JSON_READER_H
#define JSON_READER_H

#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>

typedef struct json_reader JSONRD_T;

/*****************************************************************************
 * Callbacks for use with the JSON reader. If you have a more rigidly structured
 * JSON document then it probably makes more sense to use the higher level
 * interface that allows defining a document structure.
 ****************************************************************************/
typedef struct json_reader_callbacks JSONRD_CALLBACKS_T;
struct json_reader_callbacks {
  void (*error)(void *, char *, va_list);
  void (*start_data)(void *, char *, size_t); // data-name, strlen(data-name)
  void (*end_data)(void *);
  void (*start_property)(void *, char *, int); // property-name, strlen(property-name)
  void (*end_property)(void *);
  void (*start_object)(void *);
  void (*end_object)(void *);
  void (*start_list)(void *);
  void (*end_list)(void *);
  void (*atom_null)(void *);
  void (*atom_bool)(void *, bool); // boolean
  void (*atom_number)(void *, long double); // number
  void (*atom_string)(void *, char *, size_t); // string, strlen(string)
};

// Creates a JSON reader that can extract data sections marked with the
// landmark @JSON_VAR
JSONRD_T* jsonrd_create(JSONRD_CALLBACKS_T *callbacks, void *user_data);

// Destroys a JSON reader.
void jsonrd_destroy(JSONRD_T *ctxt);

// Process a chunk of data with the JSON reader.
void jsonrd_update(JSONRD_T *ctxt, const char *chunk, size_t size, bool end);


#endif

