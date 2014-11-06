/**************************************************************************
 * FILE: xml-out.c
 * CREATE DATE: 4/May/2010
 * AUTHOR: James Johnson
 * PROJECT: shared
 * COPYRIGHT: UQ, 2010
 * DESCRIPTION: Utility functions for writing XML files.
 **************************************************************************/
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#define BYTETOBINARYPATTERN "%d%d%d%d%d%d%d%d"
#define BYTETOBINARY(byte)  \
  (byte & 0x80 ? 1 : 0), \
  (byte & 0x40 ? 1 : 0), \
  (byte & 0x20 ? 1 : 0), \
  (byte & 0x10 ? 1 : 0), \
  (byte & 0x08 ? 1 : 0), \
  (byte & 0x04 ? 1 : 0), \
  (byte & 0x02 ? 1 : 0), \
  (byte & 0x01 ? 1 : 0) 

/*
 * Reads a UTF-8 codepoint. The string must be null terminated.
 */
static inline int read_utf8_codepoint(char *str, uint32_t *codepoint_out) {
  unsigned char byte;
  uint32_t codepoint;
  int bytes, i;
  byte = *str;
  if (byte & 0x80) {// UTF-8 multibyte
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
        fprintf(stderr, "The bit pattern 10xxxxxx is illegal for the first "
            "byte of a UTF-8 multibyte.\n");
      } else if (byte == 0xFE) {
        fprintf(stderr, "The byte 0xFE is illegal for UTF-8.\n");
      } else {
        fprintf(stderr, "The byte 0xFF is illegal for UTF-8.\n");
      } 
      exit(1);
      // stop compiler complaints
      bytes = 1;
      codepoint = 0;
    }
    // read the rest of the bytes
    for (i = 1; i < bytes; i++) {
      if (str[i] == '\0') {
        return i - bytes;
      }
      byte = str[i];
      if ((byte & 0xC0) != 0x80) {
        fprintf(stderr, "Expected the bit pattern 10xxxxxx for a following "
            "byte of a UTF-8 multibyte.\n");
        exit(1);
      }
      codepoint = codepoint | (((uint32_t)(byte & 0x3F)) << (6 * (bytes - i - 1)));
    }
    // check for overlong representations by seeing if we could have represented
    // the number with one less byte
    if (bytes > 1 && codepoint < (1 << (bytes == 2 ? 7 : (6 * (bytes - 2) + (8 - bytes))))) {
      fprintf(stderr, "The UTF-8 multibyte uses too many bytes (%d) for the "
          "codepoint (%zu) it represents.\n", bytes, codepoint);
      for (i = 0; i < bytes; i++) {
        fprintf(stderr, BYTETOBINARYPATTERN " ", BYTETOBINARY((unsigned char)str[i]));
      }
      fputs("\n", stderr);
      exit(1);
    }
  } else { // ASCII byte
    bytes = 1;
    codepoint = byte;
  }
  *codepoint_out = codepoint;
  return bytes;
}

/**********************************************************************/
/*
 * scans through a null terminated input string and copies it into the
 * buffer replacing '&' with '&amp;', '<' with '&lt;', '>' with '&gt;' and
 * optionally '"' with '&quot;'. Extra is an optional parameter but if it
 * is specified then it will contain the amount that buffer needs to be
 * expanded by to fit the entire translated input. The buffer is returned
 * and is always null terminated.
 *
 * WARNING: do not use this twice in one printf with the same buffer as the
 * second call will overwrite the buffer before the printf is evaluated.
 */
/**********************************************************************/
char* replace_xml_chars(char *input, char *buffer, int buffer_size, int replace_quote, int *extra) {
  int i, j, k, used;
  char *copy;
  uint32_t cp;
  for (i = 0, j = 0, used = 1, cp = 0; input[i] != '\0'; i += used) {
    used = read_utf8_codepoint(input+i, &cp);
    if (used <= 0) break;
    //put the next item to be copied across into the copy string
    switch (cp) {
      case '&': copy = "&amp;"; break;
      case '<': copy = "&lt;"; break;
      case '>': copy = "&gt;"; break;
      case 9: 
        if (!replace_quote) goto dupe_codepoint;
        copy = "&#9;"; break;
      case 10:
        if (!replace_quote) goto dupe_codepoint;
        copy = "&#10;"; break;
      case 13:
        if (!replace_quote) goto dupe_codepoint;
        copy = "&#13;"; break;
      case '"':
        if (!replace_quote) goto dupe_codepoint;
        copy = "&quot;"; break;
      default://copy across the character (if possible)
        // check that the codepoint can be represented in XML 1.0
        // It must be within the allowed ranges:
        // U+0009, U+000A, U+000D (these three are handled above)
        // U+0020–U+D7FF, U+E000–U+FFFD
        // U+10000–U+10FFFF
        // also not in the discouraged ranges:
        // U+007F–U+0084, U+0086–U+009F
        // 
        if (!((cp >= 20 && cp < 127) || (cp > 132 && cp < 134) || 
            (cp > 159 && cp <= 55295) || (cp >= 57344 && cp <= 65533) ||
            (cp >= 65536 && cp <= 1114111))) {
          // not possible to represent! skip!
          continue;
        }
dupe_codepoint:
        if ((j + used) <= buffer_size) {
          for (k = 0; k < used; k++) buffer[j+k] = input[i+k];
        }
        j += used;
        continue;
    }
    //copy the item into the buffer
    for (k = 0; copy[k] != '\0'; ++k, ++j) {
      if (j < buffer_size) {
        buffer[j] = copy[k];
      }
    }
  }
  if (j >= buffer_size) {
    if (extra) *extra = j - buffer_size + 1;
    if (buffer_size > 0) buffer[buffer_size - 1] = '\0';
  } else {
    if (extra) *extra = 0;
    buffer[j] = '\0';
  }
  return buffer;
} /* replace_xml_chars */


/**********************************************************************/
/*
 * scans through a null terminated input string and copies it into the
 * buffer replacing '&' with '&amp;', '<' with '&lt;', '>' with '&gt;' and
 * optionally '"' with '&quot;'. If the buffer is not large enough it
 * will be expanded. The offset can be used to append to an existing string
 * already in the buffer.
 *
 * WARNING: do not use this twice in one printf with the same buffer as the
 * second call will overwrite the buffer before the printf is evaluated.
 */
/**********************************************************************/
char* replace_xml_chars2(char *input, char **expandable_buffer, int *buffer_size, int offset, int replace_quote) {
  char *expanded_buffer, *out;
  int extra = 0;
  assert(*buffer_size >= 0);
  assert(offset >= 0);
  out = replace_xml_chars(input, (*expandable_buffer)+offset, *buffer_size - offset, replace_quote, &extra);
  if (extra) {
    expanded_buffer = realloc(*expandable_buffer, sizeof(char) * (*buffer_size + extra));
    if (expanded_buffer) {
      *expandable_buffer = expanded_buffer;
      *buffer_size += extra;
    } else {
      //memory allocation failure
      fprintf(stderr, "FATAL: replace_xml_chars2 - realloc failed to expand buffer by %d bytes.\n", extra);
      exit(1);
    }
    out = replace_xml_chars(input, (*expandable_buffer)+offset, *buffer_size - offset, replace_quote, &extra);
  }
  assert(extra == 0);
  return out;
} /* replace_xml_chars2 */
