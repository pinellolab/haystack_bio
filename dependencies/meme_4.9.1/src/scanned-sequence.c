
#include <string.h>

#include "scanned-sequence.h"
#include "utils.h"

/*
 * Create a scanned sequence structure.
 * The seq_id is copied.
 */
SCANNED_SEQ_T* sseq_create(const char *seq_id, long length, double log10pvalue, 
    long site_count) {
  SCANNED_SEQ_T *sseq;
  sseq = mm_malloc(sizeof(SCANNED_SEQ_T));
  memset(sseq, 0, sizeof(SCANNED_SEQ_T));
  sseq->seq_id = strdup(seq_id);
  sseq->length = length;
  sseq->pvalue = exp10(log10pvalue);
  sseq->site_count = site_count;
  sseq->sites = (SCANNED_SITE_T*)mm_malloc(sizeof(SCANNED_SITE_T) * site_count);
  memset(sseq->sites, 0, sizeof(SCANNED_SITE_T) * site_count);
  return sseq;
}

/*
 * Create a scanned sequence structure.
 * The seq_id is used (not copied). The length is initially set to
 * zero as are the number of allocated sites. This is intended
 * to be used with the sseq_add_site and sseq_add_spacer methods
 * to build up the correct length and sites
 */
SCANNED_SEQ_T* sseq_create2(char *seq_id, double log10pvalue) {
  SCANNED_SEQ_T *sseq;
  sseq = mm_malloc(sizeof(SCANNED_SEQ_T));
  memset(sseq, 0, sizeof(SCANNED_SEQ_T));
  sseq->seq_id = seq_id;
  sseq->pvalue = exp10(log10pvalue);
  return sseq;
}

/*
 * Destroy the scanned sequence structure and all included scanned 
 * site structures.
 */
void sseq_destroy(void *p) {
  SCANNED_SEQ_T *sseq;
  sseq = (SCANNED_SEQ_T *)p;
  memset(sseq->sites, 0, sizeof(SCANNED_SITE_T) * sseq->site_count);
  free(sseq->sites);
  free(sseq->seq_id);
  memset(sseq, 0, sizeof(SCANNED_SEQ_T));
  free(sseq);
}

/*
 * Set the scanned site at a offset. 
 */
void sseq_set(SCANNED_SEQ_T *sseq, int index, int m_num,
    char strand, long position, double log10pvalue) {
  SCANNED_SITE_T *ssite;
  ssite = sseq->sites+index;
  ssite->m_num = m_num;
  ssite->strand = strand;
  ssite->position = position;
  ssite->pvalue = exp10(log10pvalue);
}

/*
 * Add a scanned site. Intended to be used with sseq_create2
 */
void sseq_add_site(SCANNED_SEQ_T *sseq, int motif_num, int motif_len, 
    char strand, double log10pvalue) {
  SCANNED_SITE_T *ssite;
  sseq->site_count += 1;
  sseq->sites = mm_realloc(sseq->sites, sseq->site_count * sizeof(SCANNED_SEQ_T));
  ssite = sseq->sites+(sseq->site_count - 1);
  ssite->m_num = motif_num;
  ssite->position = sseq->length;
  ssite->strand = strand;
  ssite->pvalue = exp10(log10pvalue);
  sseq->length += motif_len;
}

/*
 * Add a spacer. Intended to be used with sseq_create2
 */
void sseq_add_spacer(SCANNED_SEQ_T *sseq, int spacer_len) {
  sseq->length += spacer_len;
}
