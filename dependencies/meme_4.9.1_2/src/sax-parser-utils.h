
#ifndef SAX_PARSER_UTILS_H
#define SAX_PARSER_UTILS_H

#include "utils.h"

/*****************************************************************************
 * Error codes that can be passed to the error function
 ****************************************************************************/
#define PARSE_ATTR_DUPLICATE 1
#define PARSE_ATTR_BAD_VALUE 2
#define PARSE_ATTR_MISSING 3

/*****************************************************************************
 * Possible character defines
 ****************************************************************************/
#define IGNORE NULL
#define ALL_CHARS accept_all_chars
#define ALL_BUT_SPACE accept_all_chars_but_isspace

/*****************************************************************************
 * Datastructure for a multi-valued attribute.
 ****************************************************************************/
typedef struct multi MULTI_T;
struct multi {
  int count;        // Number of options a value could take
  char **options;   // List of options a value could take in alphabetical order
  int *outputs;     // List of outputs for the options
  int *target;      // Pointer to the output integer.
};

/*****************************************************************************
 * A structure to hold the program version
 ****************************************************************************/
struct prog_version {
  int major;
  int minor;
  int patch;
};

/*****************************************************************************
 * A buffer for the characters between elements.
 ****************************************************************************/
typedef struct charbuf CHARBUF_T;
struct charbuf {
  int (*accept) (char); // should the character be stored in the buffer
  char *buffer;         // the buffer
  int pos;              // the position in the buffer to store the next char
  int size;             // the current size of the buffer
};

/*****************************************************************************
 * Load a string attribute when used with parse attributes. 
 * Expects that the data attribute should be a pointer to a string.
 * Note that users will need to duplicate the string should they wish to 
 * keep it.
 ****************************************************************************/
int ld_str(char *value, void *data);

/*****************************************************************************
 * Use with parse attributes to load a character attribute.
 * Expects that the data attribute should be a pointer to a character.
 ****************************************************************************/
int ld_char(char *value, void *data);

/*****************************************************************************
 * Use with parse attributes to load a integer attribute.
 * Expects that the data attribute should be a pointer to an integer.
 ****************************************************************************/
int ld_int(char *value, void *data);

/*****************************************************************************
 * Use with parse attributes to load a long integer attribute.
 * Expects that the data attribute should be a pointer to a long integer.
 ****************************************************************************/
int ld_long(char *value, void *data);

/*****************************************************************************
 * Use with parse attributes to load a double precision floating point 
 * number attribute.
 * Expects that the data attribute should be a pointer to a double.
 ****************************************************************************/
int ld_double(char *value, void *data);

/*****************************************************************************
 * Use with parse attributes to load a double that is in the range 0 to 1
 * inclusive and hence could be used to represent a probability.
 * Expects that the data attribute should be a pointer to a double.
 ****************************************************************************/
int ld_pvalue(char *value, void *data);

/*****************************************************************************
 * Use with parse attributes to load an E-value which may be too small to
 * represent with a double.
 * Expects that the data attribute should be a pointer to a double.
 ****************************************************************************/
int ld_log10_ev(char *value, void *data);

/*****************************************************************************
 * Use with parse attributes to load an p-value which may be too small to
 * represent with a double.
 * Expects that the data attribute should be a pointer to a double.
 ****************************************************************************/
int ld_log10_pv(char *value, void *data);

/*****************************************************************************
 * Use with parse attributes to load a multi-valued attribute.
 * Expects that the data attribute is a pointer to a structure of type 
 * MULTI_T which contains a list of possible strings for the multi-valued 
 * attribute and a list of integer output codes to represent them as well
 * as the pointer to the integer that is to store the output. Note
 * that the list of strings must be alphabetically ordered.
 ****************************************************************************/
int ld_multi(char *value, void *data);

/*****************************************************************************
 * Use with parse attributes to load a program version attribute.
 * Expects that the data attribute is a pointer to a prog_version structure.
 * Note that the program version string is assumed to be dot separated 
 * numbers like 4.3.2.
 ****************************************************************************/
int ld_version(char *value, void *data);

/*****************************************************************************
 * Parses the list of attributes given to the libxml startElement callback.
 * To do the parsing it takes a list of alphabetically ordered attribute names
 * and a list of associated parsers, in-out data to be passed to the parsers,
 * booleans to indicate if the attribute is required and booleans to indicate
 * if the attribute was actually read.
 ****************************************************************************/
void parse_attributes(void (*error)(void *, int, char*, char*, char*), 
    void *state, char *tag, const xmlChar **attrs, 
    int count, char **names, int (**parsers)(char*, void*), void **parser_data, 
    BOOLEAN_T *required, BOOLEAN_T *done);

/*****************************************************************************
 * Character filter that accepts everything
 ****************************************************************************/
int accept_all_chars(char ch);

/*****************************************************************************
 * Character filter that accepts everything that isn't a space
 ****************************************************************************/
int accept_all_chars_but_isspace(char ch);

/*****************************************************************************
 * Stores characters which pass the acceptance test into the buffer 
 * automatically expanding the buffer if required.
 ****************************************************************************/
void store_xml_characters(CHARBUF_T *buf, const char *ch, int len);

#endif
