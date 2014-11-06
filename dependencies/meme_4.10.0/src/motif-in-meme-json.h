
#ifndef MOTIF_IN_MEME_JSON_H
#define MOTIF_IN_MEME_JSON_H

#include "motif.h"
#include "motif-in-common.h"

void* mhtml2_create(const char *optional_filename, int options);
void mhtml2_destroy(void *data);
void mhtml2_update(void *data, const char *chunk, size_t size, short end);
short mhtml2_has_format_match(void *data);
short mhtml2_has_error(void *data);
char* mhtml2_next_error(void *data);
short mhtml2_has_motif(void *data);
MOTIF_T* mhtml2_next_motif(void *data);
ALPH_T mhtml2_get_alphabet(void *data);
int mhtml2_get_strands(void *data);
BOOLEAN_T mhtml2_get_bg(void *data, ARRAY_T **bg);
void* mhtml2_motif_optional(void *data, int option);
void* mhtml2_file_optional(void *data, int option);
#endif

