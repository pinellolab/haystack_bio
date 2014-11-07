/**************************************************************************
 * FILE: regex-utils.h
 * AUTHOR: James Johnson 
 * CREATE DATE: 11-July-2011
 * PROJECT: shared
 * COPYRIGHT: UQ, 2011
 * VERSION: $Revision: 1.0 $
 * DESCRIPTION: Some utility functions for working with POSIX regular 
 * expressions. Mainly for extracting or comparing submatches.
 **************************************************************************/

#ifndef REGEX_UTILS_H
#define REGEX_UTILS_H

#include <regex.h>

#define NUM_RE "([+-]?[0-9]*\\.?[0-9]+([eE][-+]?[0-9]+)?)"
#define POS_NUM_RE "([+]?[0-9]*\\.?[0-9]+([eE][-+]?[0-9]+)?)"

/*****************************************************************************
 * Compile a regular expression but if an error code is returned then
 * exit with an error message.
 ****************************************************************************/
void regcomp_or_die(const char *name, regex_t *preg, const char *regex, 
    int cflags);

/*****************************************************************************
 * Execute a regular expression but if an error code is returned then
 * exit with an error message. Unlike the real regexec this returns
 * true when a match is found.
 ****************************************************************************/
int regexec_or_die(const char *name, const regex_t *preg, const char *string, 
    size_t nmatch, regmatch_t pmatch[], int eflags);

/*****************************************************************************
 * Output the error that occurred when using the regex and die.
 ****************************************************************************/
void regex_err(const char *name, regex_t *regex, int match_ret);

/*****************************************************************************
 * Get the integer matched by the regular expression match.
 ****************************************************************************/
int regex_int(regmatch_t *match, const char *str, 
    int nomatch_default);

/*****************************************************************************
 * Get the double matched by the regular expression match.
 ****************************************************************************/
double regex_dbl(regmatch_t *match, const char *str);

/*****************************************************************************
 * Get the positive double matched by the regular expression match 
 * in log base 10.
 ****************************************************************************/
double regex_log10_dbl(regmatch_t *match, const char *str);

/*****************************************************************************
 * Get the character matched by the regular expression match.
 ****************************************************************************/
char regex_chr(regmatch_t *match, const char *str);

/*****************************************************************************
 * Get (and copy) the string matched by the regular expression match.
 ****************************************************************************/
char* regex_str(regmatch_t *match, const char *str);

/*****************************************************************************
 * Get the string matched by the regular expression match and copy it into
 * the target buffer.
 ****************************************************************************/
void regex_strncpy(regmatch_t *match, const char *str, 
    char *dest, size_t dest_size);

/*****************************************************************************
 * Compare the string matched by the regular expression match with a provided
 * string.
 ****************************************************************************/
int regex_cmp(regmatch_t *match, const char *str, 
    const char *like);

/*****************************************************************************
 * Compare ignoring case the string matched by the regular expression match 
 * with a provided string.
 ****************************************************************************/
int regex_casecmp(regmatch_t *match, const char *str, 
    const char *like);

#endif
