/*
 * FILE: json-writer.c
 * AUTHOR: James Johnson 
 * CREATE DATE: 30-November-2011
 * PROJECT: shared
 * COPYRIGHT: UQ, 2011
 * VERSION: $Revision: 1.0 $
 * DESCRIPTION: Writes Javascript Object Notation.
 */
#include <assert.h>
#include <libgen.h> // for basename
#include <stdarg.h>
#include <stdint.h>
#include <string.h> // for strdup

#include "json-writer.h"
#include "linked-list.h"
#include "string-builder.h"
#include "utils.h"

enum json_state {
  JSON_ERROR,
  JSON_EMPTY_OBJECT,
  JSON_OBJECT,
  JSON_EMPTY_ARRAY,
  JSON_SL_ARRAY, // single line array
  JSON_ML_ARRAY, // multiple line array
  JSON_PROPERTY,
  JSON_DONE
};
typedef enum json_state JSON_EN;

struct json_writer {
  FILE *file;
  int min_cols;
  int tab_cols;
  int line_cols;
  int indent;
  int column;
  JSON_EN state;
  LINKLST_T *stack;
  STR_T *value_buf;
  STR_T *line_buf;
};

/*
 * pushes a new state onto the stack
 */
static void push_state(LINKLST_T* stack, JSON_EN state) {
  JSON_EN *allocated_state;
  allocated_state = mm_malloc(sizeof(JSON_EN));
  *allocated_state = state;
  linklst_push(allocated_state, stack);
}

/*
 * pops the top state off the stack
 */
static JSON_EN pop_state(LINKLST_T* stack) {
  JSON_EN *allocated_state, state;
  if (!linklst_size(stack)) return JSON_ERROR;
  allocated_state = linklst_pop(stack);
  state = *allocated_state;
  free(allocated_state);
  return state;
}

/*
 * checks that the passed state is one of the allowed states.
 */
static inline void enforce_state(JSON_EN state, int num, ...) {
  int i;
  va_list argp;
  va_start(argp, num);
  for (i = 0; i < num; i++) {
    if (state == va_arg(argp, JSON_EN)) return;
  }
  va_end(argp);
  die("Illegal state.");
}

/*
 * unwind the stack and close any brackets
 * that need closing
 */
static void unwind_stack(JSONWR_T *jsonwr) {
  while (jsonwr->state != JSON_DONE) {
    switch(jsonwr->state) {
    case JSON_EMPTY_OBJECT:
    case JSON_OBJECT:
      jsonwr_end_object_value(jsonwr);
      break;
    case JSON_EMPTY_ARRAY:
    case JSON_SL_ARRAY:
    case JSON_ML_ARRAY:
      jsonwr_end_array_value(jsonwr);
      break;
    case JSON_PROPERTY:
      jsonwr_null_value(jsonwr);
      break;
    default:
      die("Illegal state.");
    }
  }
}

/*
 * converts a string into a JSON allowed string and stores it
 * in the storage string builder. The string may be UTF-8 or ASCII.
 */
static void convert_string(STR_T* storage, const char* string) {
  const char *c;
  unsigned char byte;
  uint32_t codepoint;
  int bytes;
  assert(storage != NULL);
  str_clear(storage);
  str_append(storage, "\"", 1);
  for (c = string; *c != '\0'; c += bytes) {
    byte = *c;
    // check if the high bit is set
    if (byte & 0x80) {// UTF-8 multibyte
      int i;
      // determine the number of bytes in the multibyte
      if ((byte & 0xE0) == 0xC0) {
        bytes = 2;
        codepoint = ((uint32_t)(byte & 31)) << 6;
      } else if ((byte & 0xF0) == 0xE0) {
        bytes = 3;
        codepoint = ((uint32_t)(byte & 15)) << 12;
      } else if ((byte & 0xF8) == 0xF0) {
        bytes = 4;
        codepoint = ((uint32_t)(byte & 7)) << 18;
      } else if ((byte & 0xFC) == 0xF8) {
        bytes = 5;
        codepoint = ((uint32_t)(byte & 3)) << 24;
      } else if ((byte & 0xFE) == 0xFC) {
        bytes = 6;
        codepoint = ((uint32_t)(byte & 1)) << 30;
      } else { // bad byte!
        if ((byte & 0xC0) == 0x80) {
          die("The bit pattern 10xxxxxx is illegal for the first byte of a UTF-8 multibyte.");
        } else if (byte == 0xFE) {
          die("The byte 0xFE is illegal for UTF-8.");
        } else {
          die("The byte 0xFF is illegal for UTF-8.");
        } 
        // stop compiler complaints
        bytes = 1;
        codepoint = 0;
      }
      // read the rest of the bytes
      for (i = 1; i < bytes; i++) {
        byte = c[i];
        if ((byte & 0xC0) != 0x80) die("Expected the bit pattern 10xxxxxx for "
            "a following byte of a UTF-8 multibyte.");
        codepoint = codepoint | (((uint32_t)(byte & 0x3F)) << (6 * (bytes - i - 1)));
      }
      // check for overlong representations by seeing if we could have represented
      // the number with one less byte
      if (codepoint < (1 << (bytes == 2 ? 7 : (6 * (bytes - 2) + (8 - bytes))))) {
        die("The UTF-8 multibyte uses too many bytes for the codepoint it represents.");
      }
    } else { // ASCII byte
      bytes = 1;
      codepoint = byte;
    }
    switch(codepoint) {
    case 8: // backspace
      str_append(storage, "\\b", 2);
      break;
    case 9: // tab
      str_append(storage, "\\t", 2);
      break;
    case 10: // line-feed (newline)
      str_append(storage, "\\n", 2);
      break;
    case 12: // form-feed
      str_append(storage, "\\f", 2);
      break;
    case 13: // carriage return
      str_append(storage, "\\r", 2);
      break;
    case 34: // double quote
      str_append(storage, "\\\"", 2);
      break;
    case 47: // slash
      str_append(storage, "\\/", 2);
      break;
    case 92: // backslash
      str_append(storage, "\\\\", 2);
      break;
    default:
      // check if a control character
      // or if line seperator (U+2028) or if paragraph separator (U+2029)
      // the latter two are valid JSON but not valid Javascript as Javascript
      // can't have unescaped newline characters in a string.
      if (codepoint <= 0x1F || (codepoint >= 0x7F && codepoint <= 0x9F) || 
          codepoint == 0x2028 || codepoint == 0x2029) {
        str_appendf(storage, "\\u%.04x", codepoint);
      } else {
        str_append(storage, c, bytes);
      }
    }
  }
  str_append(storage, "\"", 1);
}

/*
 * writes a new line followed by an indent
 */
static void write_nl_indent(JSONWR_T* jsonwr) {
  int i;
  fputs("\n", jsonwr->file);
  for (i = 0; i < jsonwr->indent; i++) fputc(' ', jsonwr->file);
  jsonwr->column = jsonwr->indent;
}

/*
 * Start either an array or object which is surrounded by brackets
 */
static void write_start(JSONWR_T* jsonwr, JSON_EN new_state) {
  enforce_state(jsonwr->state, 4, JSON_PROPERTY, JSON_EMPTY_ARRAY, 
      JSON_SL_ARRAY, JSON_ML_ARRAY);
  if (jsonwr->state != JSON_PROPERTY) {// an array
    if (jsonwr->state != JSON_ML_ARRAY) {
      fputc('[', jsonwr->file);
      jsonwr->column += 1;
      write_nl_indent(jsonwr);
    }
    if (jsonwr->state == JSON_SL_ARRAY) {
      fputs(str_internal(jsonwr->line_buf), jsonwr->file);
      jsonwr->column += str_len(jsonwr->line_buf);
    }
    if (jsonwr->state != JSON_EMPTY_ARRAY) {
      fputs(", ", jsonwr->file);
      jsonwr->column += 2;
    }
    push_state(jsonwr->stack, JSON_ML_ARRAY);
    if ((jsonwr->column + 1) >= jsonwr->line_cols) write_nl_indent(jsonwr);
  }
  jsonwr->state = new_state;
  jsonwr->column += 1;
  jsonwr->indent += jsonwr->tab_cols;
}

/*
 * Separates multiple values in an array and ensures that the
 * line length is within the preferred value when possible.
 */
static void write_value(JSONWR_T* jsonwr) {
  int line_len, val_len;
  enforce_state(jsonwr->state, 4, JSON_PROPERTY, JSON_EMPTY_ARRAY, 
      JSON_SL_ARRAY, JSON_ML_ARRAY);
  val_len = str_len(jsonwr->value_buf);
  if (jsonwr->state == JSON_EMPTY_ARRAY) {
    if ((jsonwr->indent + 1 + val_len + 2) < jsonwr->line_cols) {
      str_clear(jsonwr->line_buf);
      str_append(jsonwr->line_buf, str_internal(jsonwr->value_buf), val_len);
      jsonwr->state = JSON_SL_ARRAY;
      return; // don't write anything yet
    } else {
      fputc('[', jsonwr->file);
      jsonwr->column += 1;
      write_nl_indent(jsonwr);
    }
  } else if (jsonwr->state == JSON_SL_ARRAY) {
    line_len = str_len(jsonwr->line_buf);
    if ((jsonwr->indent + 1 + line_len + 2 + val_len + 2) < jsonwr->line_cols) {
      str_append(jsonwr->line_buf, ", ", 2);
      str_append(jsonwr->line_buf, str_internal(jsonwr->value_buf), val_len);
      return; // don't write anything yet
    } else {
      fputc('[', jsonwr->file);
      jsonwr->column += 1;
      write_nl_indent(jsonwr);
      fputs(str_internal(jsonwr->line_buf), jsonwr->file);
      jsonwr->column += line_len;
      jsonwr->state = JSON_ML_ARRAY;
    }
  }
  if (jsonwr->state == JSON_ML_ARRAY) {
    fputc(',', jsonwr->file);
    jsonwr->column += 1;
    if ((jsonwr->column + 1 + val_len + 2) < jsonwr->line_cols) {
      fputc(' ', jsonwr->file);
      jsonwr->column += 1;
    } else {
      write_nl_indent(jsonwr);
    }
  }
  fputs(str_internal(jsonwr->value_buf), jsonwr->file);
  jsonwr->column += str_len(jsonwr->value_buf);
  if (jsonwr->state == JSON_PROPERTY) {
    jsonwr->state = pop_state(jsonwr->stack);
  } else { // ARRAY
    jsonwr->state = JSON_ML_ARRAY;
  }
}

/*
 * jsonwr_open
 * Opens a JSON writer which aims to make human readable JSON
 * indented min_cols with each sub-section indented another tab_cols
 * aiming for a maximum line length of line_cols.
 */
JSONWR_T* jsonwr_open(FILE *dest, int min_cols, int tab_cols, int line_cols) {
  JSONWR_T *jsonwr;
  jsonwr = mm_malloc(sizeof(JSONWR_T));
  memset(jsonwr, 0, sizeof(JSONWR_T));
  jsonwr->file = dest;
  jsonwr->min_cols = min_cols;
  jsonwr->tab_cols = tab_cols;
  jsonwr->line_cols = line_cols;
  jsonwr->indent = min_cols + tab_cols;
  jsonwr->column = 0;
  jsonwr->value_buf = str_create(10);
  jsonwr->line_buf = str_create(line_cols);
  jsonwr->state = JSON_EMPTY_OBJECT;
  jsonwr->stack = linklst_create();
  push_state(jsonwr->stack, JSON_DONE);
  fputc('{', jsonwr->file);
  return jsonwr;
}

/*
 * jsonwr_close
 * Closes a JSON writer.
 */
void jsonwr_close(JSONWR_T* jsonwr) {
  unwind_stack(jsonwr);
  linklst_destroy_all(jsonwr->stack, free);
  str_destroy(jsonwr->value_buf, FALSE);
  str_destroy(jsonwr->line_buf, FALSE);
  memset(jsonwr, 0, sizeof(JSONWR_T));
  free(jsonwr);
}

/*
 * jsonwr_property
 * Write a property. The next call must be a value.
 */
void jsonwr_property(JSONWR_T* jsonwr, const char* property) {
  enforce_state(jsonwr->state, 2, JSON_EMPTY_OBJECT, JSON_OBJECT);
  if (jsonwr->state != JSON_EMPTY_OBJECT) fputs(",", jsonwr->file);
  write_nl_indent(jsonwr);
  convert_string(jsonwr->value_buf, property);
  fputs(str_internal(jsonwr->value_buf), jsonwr->file);
  fputs(": ", jsonwr->file);
  push_state(jsonwr->stack, JSON_OBJECT);
  jsonwr->state = JSON_PROPERTY;
}

/*
 * jsonwr_start_object_value
 */
void jsonwr_start_object_value(JSONWR_T* jsonwr) {
  write_start(jsonwr, JSON_EMPTY_OBJECT);
  fputc('{', jsonwr->file);
  jsonwr->column += 1;
}

/*
 * jsonwr_end_object_value
 */
void jsonwr_end_object_value(JSONWR_T* jsonwr) {
  enforce_state(jsonwr->state, 2, JSON_EMPTY_OBJECT, JSON_OBJECT);
  jsonwr->indent -= jsonwr->tab_cols;
  if (jsonwr->state != JSON_EMPTY_OBJECT) write_nl_indent(jsonwr);
  fputc('}', jsonwr->file);
  jsonwr->column += 1;
  jsonwr->state = pop_state(jsonwr->stack);
}

/*
 * jsonwr_start_array_value
 */
void jsonwr_start_array_value(JSONWR_T* jsonwr) {
  write_start(jsonwr, JSON_EMPTY_ARRAY);
  // note we don't write the [ yet
}

/*
 * jsonwr_end_array_value
 */
void jsonwr_end_array_value(JSONWR_T* jsonwr) {
  int line_len;
  enforce_state(jsonwr->state, 3, JSON_EMPTY_ARRAY, JSON_SL_ARRAY, JSON_ML_ARRAY);
  jsonwr->indent -= jsonwr->tab_cols;
  if (jsonwr->state == JSON_ML_ARRAY) {
    write_nl_indent(jsonwr);
  } else {
    line_len = (jsonwr->state == JSON_SL_ARRAY ? str_len(jsonwr->line_buf) : 0);
    if ((jsonwr->column + 1 + line_len + 2) >= jsonwr->line_cols)
      write_nl_indent(jsonwr);
    fputc('[', jsonwr->file);
    jsonwr->column += 1;
  }
  if (jsonwr->state == JSON_SL_ARRAY) {
    fputs(str_internal(jsonwr->line_buf), jsonwr->file);
    jsonwr->column += str_len(jsonwr->line_buf);
  }
  fputc(']', jsonwr->file);
  jsonwr->column += 1;
  jsonwr->state = pop_state(jsonwr->stack);
}

/*
 * jsonwr_null_value
 * Write a null value.
 */
void jsonwr_null_value(JSONWR_T* jsonwr) {
  str_clear(jsonwr->value_buf);
  str_append(jsonwr->value_buf, "null", 4);
  write_value(jsonwr);
}

/*
 * jsonwr_str_value
 * Write a string value.
 */
void jsonwr_str_value(JSONWR_T* jsonwr, const char* value) {
  convert_string(jsonwr->value_buf, value);
  write_value(jsonwr);
}

/*
 * jsonwr_nstr_value
 * Write a string or a null value.
 */
void jsonwr_nstr_value(JSONWR_T* jsonwr, const char* value) {
  if (value == NULL) jsonwr_null_value(jsonwr);
  else jsonwr_str_value(jsonwr, value);
}

/*
 * jsonwr_lng_value
 * Write a number
 */
void jsonwr_lng_value(JSONWR_T* jsonwr, long value) {
  str_clear(jsonwr->value_buf);
  str_appendf(jsonwr->value_buf, "%ld", value);
  write_value(jsonwr);
}

/*
 * jsonwr_dbl_value
 * Write a number.
 */
void jsonwr_dbl_value(JSONWR_T* jsonwr, double value) {
  str_clear(jsonwr->value_buf);
  str_appendf(jsonwr->value_buf, "%g", value);
  write_value(jsonwr);
}

/*
 * jsonwr_log10num_value
 * Write a number that has been converted into log base 10
 * because it may be too small to represent.
 */
void jsonwr_log10num_value(JSONWR_T* jsonwr, double value, int prec) {
  double m, e;
  m = 0;
  e = 0;
  if (value > -HUGE_VAL && value < HUGE_VAL) { // normal value
    e = floor(value);
    m = exp10(value - e);
    // check that rounding up won't cause a 9.9999 to go to a 10
    if (m + (.5 * pow(10,-prec)) >= 10) {
      m = 1;
      e += 1;
    }
    str_clear(jsonwr->value_buf);
    str_appendf(jsonwr->value_buf, "\"%.*fe%+04.0f\"", prec, m, e);
    write_value(jsonwr);
  } else if (value >= HUGE_VAL) { // infinity
    str_clear(jsonwr->value_buf);
    str_appendf(jsonwr->value_buf, "\"inf\"");
    write_value(jsonwr);
  } else { // negative infinity
    str_clear(jsonwr->value_buf);
    str_appendf(jsonwr->value_buf, "\"%.*fe%+04.0f\"", prec, 0, 0);
    write_value(jsonwr);
  }
}

/*
 * jsonwr_bool_value
 * Write a boolean value
 */
void jsonwr_bool_value(JSONWR_T* jsonwr, int value) {
  str_clear(jsonwr->value_buf);
  if (value) {
    str_append(jsonwr->value_buf, "true", 4);
  } else {
    str_append(jsonwr->value_buf, "false", 5);
  }
  write_value(jsonwr);
}

/****************************************************************************
 * Convenience methods
 ****************************************************************************/
/*
 * jsonwr_str_array_value
 * Writes an array of strings.
 */
void jsonwr_str_array_value(JSONWR_T* jsonwr, char** values, int count) {
  int i;
  jsonwr_start_array_value(jsonwr);
  for (i = 0; i < count; i++) jsonwr_str_value(jsonwr, values[i]);
  jsonwr_end_array_value(jsonwr);
}

/*
 * jsonwr_nstr_array_value
 * Writes an array of strings which can be null.
 */
void jsonwr_nstr_array_value(JSONWR_T* jsonwr, char** values, int count) {
  int i;
  jsonwr_start_array_value(jsonwr);
  for (i = 0; i < count; i++) jsonwr_nstr_value(jsonwr, values[i]);
  jsonwr_end_array_value(jsonwr);
}

/*
 * jsonwr_lng_array_value
 * Writes an array of long integers.
 */
void jsonwr_lng_array_value(JSONWR_T* jsonwr, long* values, int count) {
  int i;
  jsonwr_start_array_value(jsonwr);
  for (i = 0; i < count; i++) jsonwr_lng_value(jsonwr, values[i]);
  jsonwr_end_array_value(jsonwr);
}

/*
 * jsonwr_dbl_array_value
 * Writes an array of doubles.
 */
void jsonwr_dbl_array_value(JSONWR_T* jsonwr, double* values, int count) {
  int i;
  jsonwr_start_array_value(jsonwr);
  for (i = 0; i < count; i++) jsonwr_dbl_value(jsonwr, values[i]);
  jsonwr_end_array_value(jsonwr);
}

/*
 * jsonwr_log10num_array_value
 * Writes an array of doubles.
 */
void jsonwr_log10num_array_value(JSONWR_T* jsonwr, double* values, int prec, int count) {
  int i;
  jsonwr_start_array_value(jsonwr);
  for (i = 0; i < count; i++) jsonwr_log10num_value(jsonwr, values[i], prec);
  jsonwr_end_array_value(jsonwr);
}

/*
 * jsonwr_bool_array_value
 * Writes an array of booleans.
 */
void jsonwr_bool_array_value(JSONWR_T* jsonwr, int* values, int count) {
  int i;
  jsonwr_start_array_value(jsonwr);
  for (i = 0; i < count; i++) jsonwr_bool_value(jsonwr, values[i]);
  jsonwr_end_array_value(jsonwr);
}

/*
 * jsonwr_null_prop
 */
void jsonwr_null_prop(JSONWR_T* jsonwr, const char* property) {
  jsonwr_property(jsonwr, property);
  jsonwr_null_value(jsonwr);
}

/*
 * jsonwr_str_prop
 */
void jsonwr_str_prop(JSONWR_T* jsonwr, const char* property, const char* value) {
  jsonwr_property(jsonwr, property);
  jsonwr_str_value(jsonwr, value);
}

/*
 * jsonwr_nstr_prop
 */
void jsonwr_nstr_prop(JSONWR_T* jsonwr, const char* property, const char* value) {
  jsonwr_property(jsonwr, property);
  jsonwr_nstr_value(jsonwr, value);
}

/*
 * jsonwr_lng_prop
 */
void jsonwr_lng_prop(JSONWR_T* jsonwr, const char* property, long value) {
  jsonwr_property(jsonwr, property);
  jsonwr_lng_value(jsonwr, value);
}

/*
 * jsonwr_dbl_prop
 */
void jsonwr_dbl_prop(JSONWR_T* jsonwr, const char* property, double value) {
  jsonwr_property(jsonwr, property);
  jsonwr_dbl_value(jsonwr, value);
}

/*
 * jsonwr_log10num_prop
 */
void jsonwr_log10num_prop(JSONWR_T* jsonwr, const char* property, double value, int prec) {
  jsonwr_property(jsonwr, property);
  jsonwr_log10num_value(jsonwr, value, prec);
}

/*
 * jsonwr_bool_prop
 */
void jsonwr_bool_prop(JSONWR_T* jsonwr, const char* property, int value) {
  jsonwr_property(jsonwr, property);
  jsonwr_bool_value(jsonwr, value);
}

/*
 * jsonwr_str_array_prop
 */
void jsonwr_str_array_prop(JSONWR_T* jsonwr, const char* property, char** values, int count) {
  jsonwr_property(jsonwr, property);
  jsonwr_str_array_value(jsonwr, values, count);
}

/*
 * jsonwr_nstr_array_prop
 */
void jsonwr_nstr_array_prop(JSONWR_T* jsonwr, const char* property, char** values, int count) {
  jsonwr_property(jsonwr, property);
  jsonwr_nstr_array_value(jsonwr, values, count);
}

/*
 * jsonwr_lng_array_prop
 */
void jsonwr_lng_array_prop(JSONWR_T* jsonwr, const char* property, long* values, int count) {
  jsonwr_property(jsonwr, property);
  jsonwr_lng_array_value(jsonwr, values, count);
}

/*
 * jsonwr_dbl_array_prop
 */
void jsonwr_dbl_array_prop(JSONWR_T* jsonwr, const char* property, double* values, int count) {
  jsonwr_property(jsonwr, property);
  jsonwr_dbl_array_value(jsonwr, values, count);
}

/*
 * jsonwr_log10num_array_prop
 */
void jsonwr_log10num_array_prop(JSONWR_T* jsonwr, const char* property, 
    double* values, int prec, int count) {
  jsonwr_property(jsonwr, property);
  jsonwr_log10num_array_value(jsonwr, values, prec, count);
}

/*
 * jsonwr_bool_array_prop
 */
void jsonwr_bool_array_prop(JSONWR_T* jsonwr, const char* property, int* values, int count) {
  jsonwr_property(jsonwr, property);
  jsonwr_bool_array_value(jsonwr, values, count);
}


/*
 * Bonus utilities
 */

/*
 * jsonwr_arg_prop
 *
 * Outputs an argv array while removing the path from the program name
 */
void jsonwr_args_prop(JSONWR_T* jsonwr, const char* property, int argc, char** argv) {
  char *prog;
  int i;
  jsonwr_property(jsonwr, property);
  jsonwr_start_array_value(jsonwr);
  prog = strdup(argv[0]);
  jsonwr_str_value(jsonwr, basename(prog));
  free(prog);
  for (i = 1; i < argc; i++) {
    jsonwr_str_value(jsonwr, argv[i]);
  }
  jsonwr_end_array_value(jsonwr);
}

/*
 * jsonwr_arg_prop
 *
 * Outputs a job description.
 */
void jsonwr_desc_prop(JSONWR_T* jsonwr, const char* property, 
    const char* desc_file, const char* desc_message) {
  char *desc;
  desc = get_job_description(desc_file, desc_message);
  if (desc) {
    jsonwr_str_prop(jsonwr, property, desc);
    free(desc);
  }
}
