/*
 * FILE: json-writer.h
 * AUTHOR: James Johnson 
 * CREATE DATE: 30-November-2011
 * PROJECT: shared
 * COPYRIGHT: UQ, 2011
 * VERSION: $Revision: 1.0 $
 * DESCRIPTION: Writes Javascript Object Notation.
 */
#include <stdio.h>

typedef struct json_writer JSONWR_T;

/*
 * jsonwr_open
 * Opens a JSON writer which aims to make human readable JSON
 * indented min_cols with each sub-section indented another tab_cols
 * aiming for a maximum line length of line_cols.
 */
JSONWR_T* jsonwr_open(FILE *dest, int min_cols, int tab_cols, int line_cols);

/*
 * jsonwr_close
 * Closes a JSON writer.
 */
void jsonwr_close(JSONWR_T* jsonwr);

/*
 * jsonwr_property
 * Write a property. The next call must be a value.
 */
void jsonwr_property(JSONWR_T* jsonwr, const char* property);

/*
 * jsonwr_start_object_value
 */
void jsonwr_start_object_value(JSONWR_T* jsonwr);

/*
 * jsonwr_end_object_value
 */
void jsonwr_end_object_value(JSONWR_T* jsonwr);

/*
 * jsonwr_start_array_value
 */
void jsonwr_start_array_value(JSONWR_T* jsonwr);

/*
 * jsonwr_end_array_value
 */
void jsonwr_end_array_value(JSONWR_T* jsonwr);

/*
 * jsonwr_null_value
 * Write a null value.
 */
void jsonwr_null_value(JSONWR_T* jsonwr);

/*
 * jsonwr_str_value
 * Write a string value.
 */
void jsonwr_str_value(JSONWR_T* jsonwr, const char* value);

/*
 * jsonwr_nstr_value
 * Write a string or a null value.
 */
void jsonwr_nstr_value(JSONWR_T* jsonwr, const char* value);

/*
 * jsonwr_lng_value
 * Write a number
 */
void jsonwr_lng_value(JSONWR_T* jsonwr, long value);

/*
 * jsonwr_dbl_value
 * Write a number.
 */
void jsonwr_dbl_value(JSONWR_T* jsonwr, double value);

/*
 * jsonwr_dbl_value
 * Write a number.
 */
void jsonwr_log10num_value(JSONWR_T* jsonwr, double value, int prec);

/*
 * jsonwr_bool_value
 * Write a boolean value
 */
void jsonwr_bool_value(JSONWR_T* jsonwr, int value);

/****************************************************************************
 * Convenience methods
 ****************************************************************************/
/*
 * jsonwr_str_array_value
 * Writes an array of strings.
 */
void jsonwr_str_array_value(JSONWR_T* jsonwr, char** values, int count);

/*
 * jsonwr_nstr_array_value
 * Writes an array of strings which can be null.
 */
void jsonwr_nstr_array_value(JSONWR_T* jsonwr, char** values, int count);

/*
 * jsonwr_lng_array_value
 * Writes an array of long integers.
 */
void jsonwr_lng_array_value(JSONWR_T* jsonwr, long* values, int count);

/*
 * jsonwr_dbl_array_value
 * Writes an array of doubles.
 */
void jsonwr_dbl_array_value(JSONWR_T* jsonwr, double* values, int count);

/*
 * jsonwr_log10num_array_value
 * Writes an array of doubles.
 */
void jsonwr_log10num_array_value(JSONWR_T* jsonwr, double* values, int prec,
    int count);

/*
 * jsonwr_bool_array_value
 * Writes an array of booleans.
 */
void jsonwr_bool_array_value(JSONWR_T* jsonwr, int* values, int count);

/*
 * jsonwr_null_prop
 */
void jsonwr_null_prop(JSONWR_T* jsonwr, const char* property);

/*
 * jsonwr_str_prop
 */
void jsonwr_str_prop(JSONWR_T* jsonwr, const char* property, const char* value);

/*
 * jsonwr_nstr_prop
 */
void jsonwr_nstr_prop(JSONWR_T* jsonwr, const char* property, const char* value);

/*
 * jsonwr_lng_prop
 */
void jsonwr_lng_prop(JSONWR_T* jsonwr, const char* property, long value);

/*
 * jsonwr_dbl_prop
 */
void jsonwr_dbl_prop(JSONWR_T* jsonwr, const char* property, double value);

/*
 * jsonwr_log10num_prop
 */
void jsonwr_log10num_prop(JSONWR_T* jsonwr, const char* property, double value,
    int prec);

/*
 * jsonwr_bool_prop
 */
void jsonwr_bool_prop(JSONWR_T* jsonwr, const char* property, int value);

/*
 * jsonwr_str_array_prop
 */
void jsonwr_str_array_prop(JSONWR_T* jsonwr, const char* property, char** values, int count);

/*
 * jsonwr_nstr_array_prop
 */
void jsonwr_nstr_array_prop(JSONWR_T* jsonwr, const char* property, char** values, int count);

/*
 * jsonwr_lng_array_prop
 */
void jsonwr_lng_array_prop(JSONWR_T* jsonwr, const char* property, long* values, int count);

/*
 * jsonwr_dbl_array_prop
 */
void jsonwr_dbl_array_prop(JSONWR_T* jsonwr, const char* property, double* values, int count);

/*
 * jsonwr_log10num_array_prop
 */
void jsonwr_log10num_array_prop(JSONWR_T* jsonwr, const char* property, 
    double* values, int prec, int count);

/*
 * jsonwr_bool_array_prop
 */
void jsonwr_bool_array_prop(JSONWR_T* jsonwr, const char* property, int* values, int count);

/*
 * jsonwr_arg_prop
 *
 * Outputs an argv array while removing the path from the program name
 */
void jsonwr_args_prop(JSONWR_T* jsonwr, const char* property, int argc, char** argv);

/*
 * jsonwr_arg_prop
 *
 * Outputs a job description.
 */
void jsonwr_desc_prop(JSONWR_T* jsonwr, const char* property, 
    const char* desc_file, const char* desc_message);
