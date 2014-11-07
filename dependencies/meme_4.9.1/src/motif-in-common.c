
#include <strings.h>

#include "motif-in-common.h"
#include "string-match.h"


/*
 * Try to determine if this filename
 * looks like it should contain a file with
 * content revelant to this parser.
 *
 * 0 = Unknown or name not given
 * 1 = has expected extension or filename contains keyword
 * 2 = has both keyword and extension
 */
int file_name_match(const char *keyword, const char *exten, const char *fname) {
  short score;
  int i, ls;
  const char *ext;
  BMSTR_T *keyword_str;

  if (!fname) return 0;

  ext = fname;
  ls = 0;
  score = 0;
  for (i = 0; fname[i] != '\0'; ++i) {
    if (fname[i] == '/') {
      ls = i;
    } else if (fname[i] == '.') {
      ext = fname+(i+1);
    }
  }

  if (strcasecmp(ext, exten) == 0) score++;

  keyword_str = bmstr_create2(keyword, 1);
  if (bmstr_substring(keyword_str, fname+(ls + 1), i - ls - 1) >= 0) score++;
  bmstr_destroy(keyword_str);

  return score;
}
