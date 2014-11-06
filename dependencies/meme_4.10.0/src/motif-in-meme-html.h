
#ifndef MOTIF_IN_MEME_HTML_H
#define MOTIF_IN_MEME_HTML_H

#include "motif.h"
#include "motif-in-common.h"

void* mhtml_create(const char *optional_filename, int options);
void mhtml_destroy(void *data);
void mhtml_update(void *data, const char *chunk, size_t size, short end);
short mhtml_has_format_match(void *data);
short mhtml_has_error(void *data);
char* mhtml_next_error(void *data);
short mhtml_has_motif(void *data);
MOTIF_T* mhtml_next_motif(void *data);
ALPH_T mhtml_get_alphabet(void *data);
int mhtml_get_strands(void *data);
BOOLEAN_T mhtml_get_bg(void *data, ARRAY_T **bg);
void* mhtml_motif_optional(void *data, int option);
void* mhtml_file_optional(void *data, int option);
#endif
