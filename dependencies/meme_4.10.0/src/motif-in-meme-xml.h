
#ifndef MOTIF_IN_MEME_XML_H
#define MOTIF_IN_MEME_XML_H

#include "motif.h"
#include "motif-in-common.h"

void* mxml_create(const char *optional_filename, int options);
void mxml_destroy(void *data);
void mxml_update(void *data, const char *chunk, size_t size, short end);
short mxml_has_motif(void *data);
short mxml_has_format_match(void *data);
short mxml_has_error(void *data);
char* mxml_next_error(void *data);
MOTIF_T* mxml_next_motif(void *data);
ALPH_T mxml_get_alphabet(void *data);
int mxml_get_strands(void *data);
BOOLEAN_T mxml_get_bg(void *data, ARRAY_T **bg);
void* mxml_motif_optional(void *data, int option);
void* mxml_file_optional(void *data, int option);

#endif

