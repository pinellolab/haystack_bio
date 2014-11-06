
#ifndef MOTIF_IN_COMMON_H
#define MOTIF_IN_COMMON_H

#include "motif-in-flags.h"
#include "scanned-sequence.h"


/*
 * Try to determine if this filename
 * looks like it should contain a file with
 * content revelant to this parser.
 *
 * 0 = Unknown or name not given
 * 1 = has expected extension or filename contains keyword
 * 2 = has both keyword and extension
 */
int file_name_match(const char *keyword, const char *exten, const char *fname);

#endif
