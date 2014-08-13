/**************************************************************************
 * FILE: string-builder.h
 * AUTHOR: James Johnson 
 * CREATE DATE: 02 June 2011
 * PROJECT: shared
 * COPYRIGHT: UQ, 2011
 * VERSION: $Revision: 1.0 $
 * DESCRIPTION: Enables easier assembling of strings from multiple parts.
 **************************************************************************/

#ifndef STRING_BUILDER_H
#define STRING_BUILDER_H

typedef struct str STR_T;

/*
 * str_create
 * Creates a string builder with the intial 
 * capacity as specified.
 */
STR_T* str_create(int capacity);

/*
 * str_destroy
 * Destroys the string builder. If keep_string
 * is specified it returns the string otherwise
 * it destroys the string and returns null.
 */
char* str_destroy(STR_T *strb, int keep_string);

/*
 * str_append
 * Appends a string to the string builder.
 */
void str_append(STR_T *strb, const char *str, int len);

/*
 * str_appendf
 * Appends a formatted string to the string builder.
 */
void str_appendf(STR_T *strb, const char *fmt, ...);

/*
 * str_insert
 * Inserts a string at the offset to the string builder.
 */
void str_insert(STR_T *strb, int offset, const char *str, int len);

/*
 * str_insertf
 * Inserts a formatted string at the offset to the string builder.
 */
void str_insertf(STR_T *strb, int offset, const char *fmt, ...);

/*
 * str_replace
 * Replaces a string in the range specified with a passed string in 
 * the string builder.
 */
void str_replace(STR_T *strb, int start, int end, const char *str, int len);

/*
 * str_replacef
 * Replaces a string in the range specified with a formatted string in 
 * the string builder.
 */
void str_replacef(STR_T *strb, int start, int end, const char *fmt, ...);

/*
 * str_delete
 * Removes a string in the range specified from the string builder.
 */
void str_delete(STR_T *strb, int start, int end);

/*
 * str_truncate
 * Truncates the string in the string builder to the
 * specified length. If the value is negative
 * then the string will be shortened by that amount.
 */
void str_truncate(STR_T *strb, int length);

/*
 * str_clear
 * Truncates the string to length 0.
 */
void str_clear(STR_T *strb);

/*
 * str_len
 * Returns the length of the string under construction.
 */
int str_len(STR_T *strb);

/*
 * str_internal
 * Returns the internal string.
 *
 * !!WARNING!!
 * You should only use this in cases where the string is not going to be 
 * modified. Also be aware that when the string builder is destroyed the string 
 * will also be freed unless the destructor is specifically instructed not to.
 */
char* str_internal(STR_T *strb);

/*
 * str_len
 * Returns the character at the given position. Negative positions
 * are counted from the end so a pos of -1 returns the character 
 * before the null byte.
 */
char str_char(STR_T *strb, int pos);

/*
 * str_copy
 * Returns a copy of the internal string. The caller is responsible for
 * freeing the memory.
 */
char* str_copy(STR_T *strb);

/*
 * str_subcopy
 * Returns the specified substring copy of the internal string. The caller is
 * responsible for freeing the memory.
 */
char* str_subcopy(STR_T *strb, int start, int end);

/*
 * str_cmp
 * Applies strcmp to the strings.
 */
int str_cmp(const STR_T *strb, const char *s2);

/*
 * str_ncmp
 * Applies strncmp to the strings.
 */
int str_ncmp(const STR_T *strb, const char *s2, size_t n);

/*
 * str_casecmp
 * Applies strcasecmp to the strings.
 */
int str_casecmp(const STR_T *strb, const char *s2);

/*
 * str_fit
 * Shrinks the allocated memory to fit the string
 * exactly. Removes any minimum set by ensure.
 */
void str_fit(STR_T *strb);

/*
 * str_ensure
 * Ensures the requested capacity.
 */
void str_ensure(STR_T *strb, int new_capacity);

#endif
