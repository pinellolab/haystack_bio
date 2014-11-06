
#include <assert.h>
#include <stdarg.h>
#include <string.h>
#include <strings.h>

#include "string-builder.h"
#include "utils.h"

struct str {
  int space; // maximum storable string length (allocated space is one more for null byte)
  int min; // minimum allowed storage space
  int size; // current string length
  char *data; //internal string
};

/*
 * str_create
 * Creates a string builder with the intial 
 * capacity as specified.
 */
STR_T* str_create(int capacity) {
  STR_T *strb;
  assert(capacity >= 0);

  strb = mm_malloc(sizeof(STR_T));
  strb->space = capacity;
  strb->min = capacity;
  strb->size = 0;
  strb->data = mm_malloc(sizeof(char) * (capacity+1));
  strb->data[0] = '\0';
  return strb;
}

/*
 * str_destroy
 * Destroys the string builder. If keep_string
 * is specified it returns the string otherwise
 * it destroys the string and returns null.
 */
char* str_destroy(STR_T *strb, int keep_string) {
  char *str;
  if (keep_string) {
    str = strb->data;
  } else {
    str = NULL;
    memset(strb->data, 0, sizeof(char) * (strb->space + 1));
    free(strb->data);
  }
  memset(strb, 0, sizeof(STR_T));
  free(strb);
  return str;
}

/*
 * Resizes the string builder.
 */
static void resize(STR_T *strb, int min) {
  int new_size;
  if (min < strb->size) min = strb->size;
  // check if we need to expand or shrink
  if (min * 4 < strb->space && strb->min < strb->space) {
    //shrink to min * 2
    new_size = min * 2;
    // enforce minimum size
    if (new_size < strb->min) new_size = strb->min;
    // reallocate
    strb->data = mm_realloc(strb->data, sizeof(char) * (new_size + 1));
    strb->space = new_size;
  } else if (min > strb->space) {
    //expand to min * 2;
    new_size = min * 2;
    // reallocate
    strb->data = mm_realloc(strb->data, sizeof(char) * (new_size + 1));
    strb->space = new_size;
  }
  assert(strb->space >= min);
}

/*
 * vreplacef
 * Replaces a string in the range specified with a formatted string in the 
 * string builder. Uses a VA list.
 */
static void vreplacef(STR_T *strb, int start, int end, const char *fmt, va_list ap) {
  va_list ap_copy;
  int len, needed;
  assert(start <= end);
  assert(start >= 0);
  assert(end <= strb->size);
  va_copy(ap_copy, ap);
  // find the length of the string to insert
  len = vsnprintf(strb->data+(strb->size), 0, fmt, ap_copy);
  // find how much more space we need (may be negative)
  needed = len - (end - start);
  if (needed != 0) {
    if (needed > 0) resize(strb, strb->size + needed);
    if (end < strb->size) 
      memmove(strb->data+(end + needed), strb->data+end, strb->size - end);
  }
  // drop in the replacement without null termination
  vsnprintf(strb->data+start, len, fmt, ap);
  // update the size
  strb->size = strb->size + needed;
  // ensure null termination
  strb->data[strb->size] = '\0';
  // it might be possible to shrink the memory usage
  if (needed < 0) resize(strb, strb->size);
}

/*
 * str_append
 * Appends a string to the string builder.
 */
void str_append(STR_T *strb, const char *str, int len) {
  int i;
  resize(strb, strb->size + len);
  for (i = 0; i < len && str[i] != '\0'; i++) {
    strb->data[strb->size + i] = str[i];
  }
  strb->size += i;
  strb->data[strb->size] = '\0';
}

/*
 * str_appendf
 * Appends a formatted string to the string builder.
 */
void str_appendf(STR_T *strb, const char *fmt, ...) {
  int avaliable, needed;
  va_list ap;

  avaliable = strb->space - strb->size;
  va_start(ap, fmt);
  needed = vsnprintf((strb->data)+(strb->size), avaliable + 1, fmt, ap);
  va_end(ap);
  if (needed > avaliable) {
    resize(strb, strb->size + needed);
    avaliable = strb->space - strb->size;
    va_start(ap, fmt);
    needed = vsnprintf((strb->data)+(strb->size), avaliable + 1, fmt, ap);
    va_end(ap);
    assert(needed <= avaliable);
  }
  strb->size += needed;
}

/*
 * str_insert
 * Inserts a string at the offset to the string builder.
 */
void str_insert(STR_T *strb, int offset, const char *str, int len) {
  str_replace(strb, offset, offset, str, len);
}

/*
 * str_insertf
 * Inserts a formatted string at the offset to the string builder.
 */
void str_insertf(STR_T *strb, int offset, const char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  vreplacef(strb, offset, offset, fmt, ap);
  va_end(ap);
}

/*
 * str_replace
 * Replaces a string in the range specified with a passed string in 
 * the string builder.
 */
void str_replace(STR_T *strb, int start, int end, const char *str, int len) {
  int i, needed;
  // find the length of the string to insert
  for (i = 0; i < len; ++i) if (str[i] == '\0') break;
  len = i;
  // find how much more space we need (may be negative)
  needed = len - (end - start);
  if (needed != 0) {
    if (needed > 0) resize(strb, strb->size + needed);
    if (end < strb->size) 
      memmove(strb->data+(end + needed), strb->data+end, strb->size - end);
  }
  // drop in the replacement without null termination
  memcpy(strb->data+start, str, len);
  // update the size
  strb->size = strb->size + needed;
  // ensure null termination
  strb->data[strb->size] = '\0';
  // it might be possible to shrink the memory usage
  if (needed < 0) resize(strb, strb->size);
}

/*
 * str_replacef
 * Replaces a string in the range specified with a formatted string in 
 * the string builder.
 */
void str_replacef(STR_T *strb, int start, int end, const char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  vreplacef(strb, start, end, fmt, ap);
  va_end(ap);
}

/*
 * str_delete
 * Removes a string in the range specified from the string builder.
 */
void str_delete(STR_T *strb, int start, int end) {
  str_replace(strb, start, end, "", 0);
}

/*
 * str_truncate
 * Truncates the string in the string builder to the
 * specified length. If the value is negative
 * then the string will be shortened by that amount.
 */
void str_truncate(STR_T *strb, int length) {
  if (length < 0) {
    // assume the user wants to shorten the string
    length = strb->size + length;
    if (length < 0) length = 0;
  }
  if (strb->size > length) {
    strb->data[length] = '\0';
    strb->size = length;
    resize(strb, strb->size);
  }
}

/*
 * str_clear
 * Truncates the string to length 0.
 */
void str_clear(STR_T *strb) {
  str_truncate(strb, 0);
}

/*
 * str_len
 * Returns the length of the string under construction.
 */
int str_len(STR_T *strb) {
  return strb->size;
}

/*
 * str_internal
 * Returns the internal string.
 *
 * !!WARNING!!
 * You should only use this in cases where the string is not going to be 
 * modified. Also be aware that when the string builder is destroyed the string 
 * will also be freed unless the destructor is specifically instructed not to.
 */
char* str_internal(STR_T *strb) {
  return strb->data;
}

/*
 * str_len
 * Returns the character at the given position. Negative positions
 * are counted from the end so a pos of -1 returns the character 
 * before the null byte.
 */
char str_char(STR_T *strb, int pos) {
  if (pos < 0) {
    pos = -pos;
    assert(pos <= strb->size);
    return strb->data[strb->size - pos];
  } else {
    assert(pos < strb->size);
    return strb->data[pos];
  }
}

/*
 * str_copy
 * Returns a copy of the internal string. The caller is responsible for
 * freeing the memory.
 */
char* str_copy(STR_T *strb) {
  char *copy;
  copy = mm_malloc(sizeof(char) * (strb->size + 1));
  strncpy(copy, strb->data, strb->size);
  copy[strb->size] = '\0';
  return copy;
}

/*
 * str_subcopy
 * Returns the specified substring copy of the internal string. The caller is
 * responsible for freeing the memory.
 */
char* str_subcopy(STR_T *strb, int start, int end) {
  char *copy;
  int len;
  assert(start <= end);
  assert(end <= strb->size);
  len = end - start;
  copy = mm_malloc(sizeof(char) * (len + 1));
  strncpy(copy, (strb->data)+start, len);
  copy[len] = '\0';
  return copy;
}

/*
 * str_cmp
 * Applies strcmp to the strings.
 */
int str_cmp(const STR_T *strb, const char *s2) {
  return strcmp(strb->data, s2);
}

/*
 * str_ncmp
 * Applies strncmp to the strings.
 */
int str_ncmp(const STR_T *strb, const char *s2, size_t n) {
  return strncmp(strb->data, s2, n);
}

/*
 * str_casecmp
 * Applies strcasecmp to the strings.
 */
int str_casecmp(const STR_T *strb, const char *s2) {
  return strcasecmp(strb->data, s2);
}

/*
 * str_ncasecmp
 * Applies strncasecmp to the strings.
 */
int str_ncasecmp(const STR_T *strb, const char *s2, size_t n) {
  return strncasecmp(strb->data, s2, n);
}

/*
 * str_fit
 * Shrinks the allocated memory to fit the string
 * exactly. Removes any minimum set by ensure.
 */
void str_fit(STR_T *strb) {
  if (strb->size != strb->space) {
    strb->data = mm_realloc(strb->data, sizeof(char) * (strb->size + 1));
    strb->space = strb->size;
    strb->min = 0;
  }
}

/*
 * str_ensure
 * Ensures the requested capacity.
 */
void str_ensure(STR_T *strb, int new_capacity) {
  if (strb->space < new_capacity) {
    strb->data = mm_realloc(strb->data, sizeof(char) * (new_capacity + 1));
    strb->space = new_capacity;
  }
  strb->min = new_capacity;
}


