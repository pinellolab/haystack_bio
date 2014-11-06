
#ifndef MOTIF_IN_MEME_TEXT_H
#define MOTIF_IN_MEME_TEXT_H

#include "motif.h"
#include "motif-in-common.h"

void* mtext_create(const char *optional_filename, int options);
void mtext_destroy(void *data);
void mtext_update(void *data, const char *chunk, size_t size, short end);
short mtext_has_format_match(void *data);
short mtext_has_error(void *data);
char* mtext_next_error(void *data);
short mtext_has_motif(void *data);
MOTIF_T* mtext_next_motif(void *data);
ALPH_T mtext_get_alphabet(void *data);
int mtext_get_strands(void *data);
BOOLEAN_T mtext_get_bg(void *data, ARRAY_T **bg);
void* mtext_motif_optional(void *data, int option);
void* mtext_file_optional(void *data, int option);

#endif

