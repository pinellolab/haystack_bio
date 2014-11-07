
#ifndef MOTIF_IN_DREME_XML_H
#define MOTIF_IN_DREME_XML_H

#include "motif.h"
#include "motif-in-common.h"

void* dxml_create(const char *optional_filename, int options);
void dxml_destroy(void *data);
void dxml_update(void *data, const char *chunk, size_t size, short end);
short dxml_has_motif(void *data);
short dxml_has_format_match(void *data);
short dxml_has_error(void *data);
char* dxml_next_error(void *data);
MOTIF_T* dxml_next_motif(void *data);
ALPH_T dxml_get_alphabet(void *data);
int dxml_get_strands(void *data);
BOOLEAN_T dxml_get_bg(void *data, ARRAY_T **bg);
void* dxml_motif_optional(void *data, int option);
void* dxml_file_optional(void *data, int option);

#endif


