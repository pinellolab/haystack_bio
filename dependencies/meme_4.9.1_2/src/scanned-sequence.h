
#ifndef SCANNED_SEQUENCE_H
#define SCANNED_SEQUENCE_H

typedef struct scanned_site SCANNED_SITE_T;
struct scanned_site {
  int m_num;
  char strand;
  long position;
  double pvalue;
};

typedef struct scanned_seq SCANNED_SEQ_T;
struct scanned_seq {
  char *seq_id;
  long length;
  double pvalue;
  long site_count;
  SCANNED_SITE_T *sites;
};

/*
 * Create a scanned sequence structure.
 * The seq_id is copied.
 */
SCANNED_SEQ_T* sseq_create(const char *seq_id, long length, double pvalue, 
    long site_count);
/*
 * Create a scanned sequence structure.
 * The seq_id is used (not copied). The length is initially set to
 * zero as are the number of allocated sites. This is intended
 * to be used with the sseq_add_site and sseq_add_spacer methods
 * to build up the correct length and sites
 */
SCANNED_SEQ_T* sseq_create2(char *seq_id, double pvalue);

/*
 * Destroy the scanned sequence structure and all included scanned 
 * site structures.
 */
void sseq_destroy(void *sseq);

/*
 * Set the scanned site at a offset. 
 */
void sseq_set(SCANNED_SEQ_T *sseq, int index, int m_num,
    char strand, long position, double pvalue);

/*
 * Add a scanned site. Intended to be used with sseq_create2
 */
void sseq_add_site(SCANNED_SEQ_T *sseq, int motif_num, int motif_len, char strand, double pvalue);

/*
 * Add a spacer. Intended to be used with sseq_create2
 */
void sseq_add_spacer(SCANNED_SEQ_T *sseq, int spacer_len);

#endif
