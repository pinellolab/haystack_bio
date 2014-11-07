#ifndef  TRANSFAC_H
#define TRANSFAC_H

#include "array-list.h"
#include "matrix.h"
#include "motif.h"
#include "string-list.h"

typedef struct transfac_motif TRANSFAC_MOTIF_T;

/***********************************************************************
 * Allocate memory for a transfac motif
 ***********************************************************************/
TRANSFAC_MOTIF_T *new_transfac_motif(
  const char *accession,
  const char *id,
  const char *name,
  const char *description,
  const char *consensus,
  STRING_LIST_T *species_list,
  MATRIX_T *counts
);

/*******************************************************************************
 * Free TRANSFAC motif structure
 ******************************************************************************/
void free_transfac_motif(TRANSFAC_MOTIF_T *motif);

/***********************************************************************
 * Read motifs from a TRANSFAC file.
 ***********************************************************************/
ARRAYLST_T *read_motifs_from_transfac_file (
  const char* transfac_filename  // Name of TRANSFAC file or '-' for stdin IN
);

/*******************************************************************************
 * Return TRANSFAC ACCESSION string.
 * Caller is responsible for freeing string.
 ******************************************************************************/
char *get_transfac_accession(TRANSFAC_MOTIF_T *motif);

/*******************************************************************************
 * Return TRANSFAC ID string.
 * Caller is responsible for freeing string.
 ******************************************************************************/
char *get_transfac_id(TRANSFAC_MOTIF_T *motif);

/*******************************************************************************
 * Return TRANSFAC NAME string.
 * Caller is responsible for freeing string.
 ******************************************************************************/
char *get_transfac_name(TRANSFAC_MOTIF_T *motif);

/*******************************************************************************
 * Return TRANSFAC DESCRIPTION string.
 * Caller is responsible for freeing string.
 ******************************************************************************/
char *get_transfac_description(TRANSFAC_MOTIF_T *motif);

/*******************************************************************************
 * Return TRANSFAC consensus string.
 * Caller is responsible for freeing string.
 ******************************************************************************/
char *get_transfac_consensus(TRANSFAC_MOTIF_T *motif);

/*******************************************************************************
 * Return count of TRANSFAC SPECIES strings.
 ******************************************************************************/
int get_transfac_num_species(TRANSFAC_MOTIF_T *motif);

/*******************************************************************************
 * Return the nth TRANSFAC species string.
 * Caller is responsible for freeing string.
 ******************************************************************************/
char *get_transfac_nth_species(int n, TRANSFAC_MOTIF_T *motif);

/*******************************************************************************
 * Return matrix of TRANSFAC counts.
 * Caller is responsible for freeing the MATRIX_T.
 ******************************************************************************/
MATRIX_T *get_transfac_counts(TRANSFAC_MOTIF_T *motif);

/***********************************************************************
 * Converts a TRANSFAC motif to a MEME motif.
 * Caller is responsible for freeing the returned MOTIF_T.
 ***********************************************************************/
MOTIF_T *convert_transfac_motif_to_meme_motif(
  char *id, 
  int pseudocount,
  ARRAY_T *bg,
  TRANSFAC_MOTIF_T *motif
);

/***********************************************************************
 * Converts a list of TRANSFAC motifs to a list MEME motif.
 * Caller is responsible for freeing the returned ARRAYLST
 ***********************************************************************/
ARRAYLST_T *convert_transfac_motifs_to_meme_motifs(
  BOOLEAN_T use_accession,
  int pseudocount,
  ARRAY_T *bg,
  ARRAYLST_T *tfac_motifs
);

#endif
