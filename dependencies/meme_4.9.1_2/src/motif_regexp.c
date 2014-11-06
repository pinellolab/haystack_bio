/*
 * motif_regexp.c
 *
 *  Created on: 22/09/2008
 *      Author: rob
 */
#include "motif_regexp.h"
#include "motif-spec.h"
#include <math.h>

/********************************************************************
  The nucleic acid codes supported are:

        A --> adenosine           M --> A C (amino)
        C --> cytidine            S --> G C (strong)
        G --> guanine             W --> A T (weak)
        T --> thymidine           B --> G T C
        U --> uridine             D --> G A T
        R --> G A (purine)        H --> A C T
        Y --> T C (pyrimidine)    V --> G C A
        K --> G T (keto)          N --> A G C T (any)

  The accepted amino acid codes are:

    A  alanine                         P  proline
    B  aspartate or asparagine         Q  glutamine
    C  cystine                         R  arginine
    D  aspartate                       S  serine
    E  glutamate                       T  threonine
    F  phenylalanine                   U  any
    G  glycine                         V  valine
    H  histidine                       W  tryptophan
    I  isoleucine                      Y  tyrosine
    K  lysine                          Z  glutamate or glutamine
    L  leucine                         X  any
    M  methionine
    N  asparagine            (From alphabet.h)
 ********************************************************************/

#define MAX_MOTIF_WIDTH 100

#define xstr(s) str(s)
#define str(s) #s

ARRAYLST_T* read_regexp_file(
   char *filename, // IN Name of file containing regular expressions
   ARRAYLST_T *motifs // IN/OUT The retrieved motifs, allocated and returned if NULL
) 
{
  FILE*      motif_file; // file containing the motifs.
  const char * pattern = "%" xstr(MAX_MOTIF_ID_LENGTH) "s\t%" xstr(MAX_MOTIF_WIDTH) "s";
  char motif_name[MAX_MOTIF_ID_LENGTH + 1];
  char motif_regexp[MAX_MOTIF_WIDTH + 1];
  ARRAY_T* these_freqs;
  MOTIF_T* m;
  int i;
  ALPH_T alph;

  //Set things to the defaults.
  if (motifs == NULL) {
    motifs = arraylst_create();
  }

  // Open the given MEME file.
  if (open_file(filename, "r", TRUE, "motif", "motifs", &motif_file) == 0)
    exit(1);

  //Set alphabet - ONLY supports dna.
  alph = DNA_ALPH;

  while (fscanf(motif_file, pattern, motif_name, motif_regexp) == 2) {
    /*
     * Now we:
     * 1. Allocate new motif
     * 2. Assign name
     * 3. Convert regexp into frequency table.
     */
    m = mm_malloc(sizeof(MOTIF_T*));
    memset(m, 0, sizeof(MOTIF_T));
    set_motif_id(motif_name, strlen(motif_name), m);
    set_motif_strand('+', m);
    m->length = strlen(motif_regexp);
    /* Store the evalue of the motif. */
    m->evalue = 0;
    m->log_evalue = -HUGE_VAL;
    /* Store the alphabet size in the motif. */
    m->alph = alph;
    m->flags = MOTIF_HAS_AMBIGS | MOTIF_BOTH_STRANDS;
    /* Allocate memory for the matrix. */
    m->freqs = allocate_matrix(m->length, alph_size(alph, ALL_SIZE));
    init_matrix(0, m->freqs);
    m->url = strdup("");
    //Set motif frequencies here.
    for (i=0;i<strlen(motif_regexp);i++) {
      switch(toupper(motif_regexp[i])) {
      case 'A':
        set_matrix_cell(i,alph_index(alph, 'A'),1,m->freqs);
        break;
      case 'C':
        set_matrix_cell(i,alph_index(alph, 'C'),1,m->freqs);
        break;
      case 'G':
        set_matrix_cell(i,alph_index(alph, 'G'),1,m->freqs);
        break;
      case 'T':
        set_matrix_cell(i,alph_index(alph, 'T'),1,m->freqs);
        break;
      case 'U':
        set_matrix_cell(i,alph_index(alph, 'U'),1,m->freqs);
        break;
      case 'R': //purines
        set_matrix_cell(i,alph_index(alph, 'G'),1,m->freqs);
        set_matrix_cell(i,alph_index(alph, 'A'),1,m->freqs);
        break;
      case 'Y': //pyramidines
        set_matrix_cell(i,alph_index(alph, 'T'),1,m->freqs);
        set_matrix_cell(i,alph_index(alph, 'C'),1,m->freqs);
        break;
      case 'K': //keto
        set_matrix_cell(i,alph_index(alph, 'G'),1,m->freqs);
        set_matrix_cell(i,alph_index(alph, 'T'),1,m->freqs);
        break;
      case 'M': //amino
        set_matrix_cell(i,alph_index(alph, 'A'),1,m->freqs);
        set_matrix_cell(i,alph_index(alph, 'C'),1,m->freqs);
        break;
      case 'S': //strong
        set_matrix_cell(i,alph_index(alph, 'G'),1,m->freqs);
        set_matrix_cell(i,alph_index(alph, 'C'),1,m->freqs);
        break;
      case 'W': //weak
        set_matrix_cell(i,alph_index(alph, 'A'),1,m->freqs);
        set_matrix_cell(i,alph_index(alph, 'T'),1,m->freqs);
        break;
      case 'B':
        set_matrix_cell(i,alph_index(alph, 'G'),1,m->freqs);
        set_matrix_cell(i,alph_index(alph, 'T'),1,m->freqs);
        set_matrix_cell(i,alph_index(alph, 'C'),1,m->freqs);
        break;
      case 'D':
        set_matrix_cell(i,alph_index(alph, 'G'),1,m->freqs);
        set_matrix_cell(i,alph_index(alph, 'A'),1,m->freqs);
        set_matrix_cell(i,alph_index(alph, 'T'),1,m->freqs);
        break;
      case 'H':
        set_matrix_cell(i,alph_index(alph, 'A'),1,m->freqs);
        set_matrix_cell(i,alph_index(alph, 'C'),1,m->freqs);
        set_matrix_cell(i,alph_index(alph, 'T'),1,m->freqs);
        break;
      case 'V':
        set_matrix_cell(i,alph_index(alph, 'G'),1,m->freqs);
        set_matrix_cell(i,alph_index(alph, 'C'),1,m->freqs);
        set_matrix_cell(i,alph_index(alph, 'A'),1,m->freqs);
        break;
      case 'N':
        set_matrix_cell(i,alph_index(alph, 'A'),1,m->freqs);
        set_matrix_cell(i,alph_index(alph, 'C'),1,m->freqs);
        set_matrix_cell(i,alph_index(alph, 'G'),1,m->freqs);
        set_matrix_cell(i,alph_index(alph, 'T'),1,m->freqs);
        break;
      }
      // spread out the 1s for the ambigs
      normalize_subarray(0, alph_size(alph, ALPH_SIZE), 0.1, 
          get_matrix_row(i, m->freqs));
      calc_ambigs(alph, FALSE, get_matrix_row(i, m->freqs));
    }

    /* Compute and store the motif complexity. */
    m->complexity = compute_motif_complexity(m);

    // add the new motif to the list
    arraylst_add(m, motifs);
  }
  fclose(motif_file);
  return motifs;
}

