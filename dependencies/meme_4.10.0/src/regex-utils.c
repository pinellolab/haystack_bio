/**************************************************************************
 * FILE: regex-utils.c
 * AUTHOR: James Johnson 
 * CREATE DATE: 11-July-2011
 * PROJECT: shared
 * COPYRIGHT: UQ, 2011
 * VERSION: $Revision: 1.0 $
 * DESCRIPTION: Some utility functions for working with POSIX regular 
 * expressions. Mainly for extracting or comparing submatches.
 **************************************************************************/
#include <errno.h>
#include <limits.h>

#include "regex-utils.h"
#include "utils.h"


/*****************************************************************************
 * Compile a regular expression but if an error code is returned then
 * exit with an error message.
 ****************************************************************************/
void regcomp_or_die(const char *name, regex_t *preg, const char *regex, 
    int cflags) {
  int code;
  char errbuf[100];
  code = regcomp(preg, regex, cflags);
  if (code != 0) {
    regerror(code, preg, errbuf, sizeof(errbuf));
    die("Error compiling %s regex \"%s\": %s", name, regex, errbuf);
  }
}

/*****************************************************************************
 * Execute a regular expression but if an error code is returned then
 * exit with an error message. Unlike the real regexec this returns
 * true when a match is found.
 ****************************************************************************/
int regexec_or_die(const char *name, const regex_t *preg, const char *string, 
    size_t nmatch, regmatch_t pmatch[], int eflags) {
  int code;
  char errbuf[100];
  code = regexec(preg, string, nmatch, pmatch, eflags);
  if (code != 0 && code != REG_NOMATCH) {
    regerror(code, preg, errbuf, sizeof(errbuf));
    die("Error executing %s regex on \"%s\": %s", name, string, errbuf);
  }
  return (code == 0);
}

/*****************************************************************************
 * Output the error that occurred when using the regex and die.
 ****************************************************************************/
void regex_err(const char *name, regex_t *regex, int match_ret) {
  char msgbuf[100];
  regerror(match_ret, regex, msgbuf, sizeof(msgbuf));
  die("Error parsing %s with RegExp: %s\n", msgbuf);
}

/*****************************************************************************
 * Get the integer matched by the regular expression match.
 ****************************************************************************/
int regex_int(regmatch_t *match, const char *str, 
    int nomatch_default) {
  char storage[50];
  char *buffer;
  size_t i, j, len;
  long lnum;

  if (match->rm_so == -1) {
    return nomatch_default; // no match
  } else {
    len = match->rm_eo - match->rm_so;
    if ((len * sizeof(char)) < sizeof(storage)) {
      buffer = storage; 
    } else {
      buffer = mm_malloc(sizeof(char) * (len + 1));
    }
    for (i = 0, j = match->rm_so; i < len; ++i, ++j) buffer[i] = str[j];
    buffer[i] = '\0';
    lnum = strtol(buffer, NULL, 10);
    if (buffer != storage) free(buffer);
    if (lnum > INT_MAX) {
      errno = ERANGE;
      return INT_MAX;
    } else if (lnum < INT_MIN) {
      errno = ERANGE;
      return INT_MIN;
    } 
    return (int)lnum;
  }
}

/*****************************************************************************
 * Get the double matched by the regular expression match.
 ****************************************************************************/
double regex_dbl(regmatch_t *match, const char *str) {
  char storage[50];
  char *buffer;
  size_t i, j, len;
  double num;

  if (match->rm_so == -1) {
    return 0; // no match
  } else {
    len = match->rm_eo - match->rm_so;
    if ((len * sizeof(char)) < sizeof(storage)) {
      buffer = storage; 
    } else {
      buffer = mm_malloc(sizeof(char) * (len + 1));
    }
    for (i = 0, j = match->rm_so; i < len; ++i, ++j) buffer[i] = str[j];
    buffer[i] = '\0';
    num = strtod(buffer, NULL);
    if (buffer != storage) free(buffer);
    return num;
  }
}

/*****************************************************************************
 * Get the positive double matched by the regular expression match 
 * in log base 10.
 ****************************************************************************/
double regex_log10_dbl(regmatch_t *match, const char *str) {
  char storage[50];
  char *buffer;
  size_t i, j, len;
  double log10_num;

  if (match->rm_so == -1) {
    return 0; // no match
  } else {
    len = match->rm_eo - match->rm_so;
    if ((len * sizeof(char)) < sizeof(storage)) {
      buffer = storage; 
    } else {
      buffer = mm_malloc(sizeof(char) * (len + 1));
    }
    for (i = 0, j = match->rm_so; i < len; ++i, ++j) buffer[i] = str[j];
    buffer[i] = '\0';
    log10_num = log10_evalue_from_string(buffer);
    if (buffer != storage) free(buffer);
    return log10_num;
  }
}

/*****************************************************************************
 * Compare the string matched by the regular expression match with a provided
 * string.
 ****************************************************************************/
int regex_cmp(regmatch_t *match, const char *str, const char *like) {
  int i,j;
  for (i = match->rm_so, j = 0; i < match->rm_eo; ++i, ++j) {
    if (str[i] != like[j] || like[j] == '\0') {
      return ((unsigned char)(str[i])) - ((unsigned char)(like[j]));
    }
  }
  // hit the end of the string at the same time
  if (like[j] == '\0') return 0;
  //match shorter than the comparison string
  return -1;
}

/*****************************************************************************
 * Compare ignoring case the string matched by the regular expression match 
 * with a provided string.
 ****************************************************************************/
int regex_casecmp(regmatch_t *match, const char *str, const char *like) {
  int i,j;
  for (i = match->rm_so, j = 0; i < match->rm_eo; ++i, ++j) {
    if (tolower(str[i]) != tolower(like[j]) || like[j] == '\0') {
      return tolower(str[i]) - tolower(like[j]);
    }
  }
  // hit the end of the string at the same time
  if (like[j] == '\0') return 0;
  //match shorter than the comparison string
  return -1;
}

/*****************************************************************************
 * Get (and copy) the string matched by the regular expression match.
 ****************************************************************************/
char* regex_str(regmatch_t *match, const char *str) {
  size_t len;
  char *result;
  if (match->rm_so == -1) {
    return strdup("");
  } else {
    // equivalent to strndup(str+(match->rm_so), (match->rm_eo - match->rm_so));
    // but strndup is a GNU extension not available on Macs.
    len = match->rm_eo - match->rm_so;
    result = mm_malloc(sizeof(char) * (len + 1));
    strncpy(result, str+(match->rm_so), len);
    result[len] = '\0'; // terminate because strncpy doesn't
    return result;
  }
}

/*****************************************************************************
 * Get the string matched by the regular expression match and copy it into
 * the target buffer.
 ****************************************************************************/
 void regex_strncpy(regmatch_t *match, const char *str, char *dest, size_t dest_size) {
  int i, j;
  for (i = match->rm_so, j = 0; i < match->rm_eo && j < dest_size; ++i, ++j) {
    dest[j] = str[i];
  }
  if (j >= dest_size) j = dest_size - 1;
  dest[j] = '\0';
}

/*****************************************************************************
 * Get the character matched by the regular expression match.
 ****************************************************************************/
char regex_chr(regmatch_t *match, const char *str) {
  // check for a match of exactly one character
  if (match->rm_so == -1 || match->rm_so != match->rm_eo - 1) {
    return 0;
  } else {
    return str[match->rm_so];
  }
}
