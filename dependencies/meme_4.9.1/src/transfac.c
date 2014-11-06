#include <assert.h>
#include <math.h>
#include <stdio.h>
#include "transfac.h"
#include "../config.h"
#include "io.h"
#include "motif-spec.h"
#include "string-list.h"
#include "simple-getopt.h"
#include "utils.h"

#define MAX_CONSENSUS_LENGTH 100

/***********************************************************************
 * Data structure describing a transfac motif
 ***********************************************************************/
struct transfac_motif {
  char *accession;
  char *id;
  char *name;
  char *description;
  STRING_LIST_T *species_list;
  char *consensus;
  MATRIX_T *counts;
};

/***********************************************************************
 * Allocate memory for a MEME motif
 ***********************************************************************/
MOTIF_T* allocate_motif(
  char *id, 
  ALPH_T alph, 
  MATRIX_T* scores,
  MATRIX_T* freqs
){
	MOTIF_T* motif = mm_malloc(sizeof(MOTIF_T));

	assert(id != NULL);

	if (freqs != NULL || scores != NULL) {
    die(
      "A matrix of scores, or frequencies, or both, "
      "must be provided when allocating a motif\n"
    );
  }

	motif->length = get_num_rows(motif->freqs);
	motif->alph = alph;
	motif->flags = 0;
	motif->evalue = 0.0;
        motif->log_evalue = -HUGE_VAL;
	motif->num_sites = 0.0;
	motif->complexity = 0.0;

	int length = strlen(id) + 1;
	strncpy(motif->id, id, MIN(length, MAX_MOTIF_ID_LENGTH));

  if (scores != NULL) {
	  motif->scores = duplicate_matrix(scores);
  }
  if (scores != NULL) {
	  motif->freqs = duplicate_matrix(freqs);
  }

  motif->url = NULL;
  motif->trim_left = 0;
  motif->trim_right = 0;

	return motif;
}

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
) {

  TRANSFAC_MOTIF_T *motif = mm_malloc(sizeof(TRANSFAC_MOTIF_T));
  motif->accession = NULL;
  motif->id = NULL;
  motif->name = NULL;
  motif->description = NULL;
  motif->species_list = NULL;
  motif->consensus = NULL;
  motif->counts = NULL;

  copy_string(&motif->accession, accession);
  copy_string(&motif->id, id);
  copy_string(&motif->name, name);
  copy_string(&motif->description, description);
  copy_string(&motif->consensus, consensus);

  if (species_list != NULL) {
    motif->species_list = copy_string_list(species_list);
  }

  if (counts != NULL) {
    motif->counts = duplicate_matrix(counts);
  }

  return motif;
}


/*******************************************************************************
 * Free TRANSFAC motif structure
 ******************************************************************************/
void free_transfac_motif(TRANSFAC_MOTIF_T *motif) {
  myfree(motif->accession);
  myfree(motif->id);
  myfree(motif->name);
  myfree(motif->description);
  myfree(motif->consensus);
  free_string_list(motif->species_list);
  free_matrix(motif->counts);
  myfree(motif);
}

/*******************************************************************************
 * Return TRANSFAC ACCESSION string.
 * Caller is responsible for freeing string.
 ******************************************************************************/
char *get_transfac_accession(TRANSFAC_MOTIF_T *motif) {
  char *accession = NULL;
  copy_string(&accession, motif->accession);
  return accession;
}

/*******************************************************************************
 * Return TRANSFAC ID string.
 * Caller is responsible for freeing string.
 ******************************************************************************/
char *get_transfac_id(TRANSFAC_MOTIF_T *motif) {
  char *id = NULL;
  copy_string(&id, motif->id);
  return id;
}

/*******************************************************************************
 * Return TRANSFAC NAME string.
 * Caller is responsible for freeing string.
 ******************************************************************************/
char *get_transfac_name(TRANSFAC_MOTIF_T *motif) {
  char *name = NULL;
  copy_string(&name, motif->name);
  return name;
}

/*******************************************************************************
 * Return TRANSFAC DESCRIPTION string.
 * Caller is responsible for freeing string.
 ******************************************************************************/
char *get_transfac_description(TRANSFAC_MOTIF_T *motif) {
  char *description = NULL;
  copy_string(&description, motif->description);
  return description;
}

/*******************************************************************************
 * Return TRANSFAC consensus string.
 * Caller is responsible for freeing string.
 ******************************************************************************/
char *get_transfac_consensus(TRANSFAC_MOTIF_T *motif) {
  char *consensus = NULL;
  copy_string(&consensus, motif->consensus);
  return consensus;
}

/*******************************************************************************
 * Return count of TRANSFAC species strings.
 ******************************************************************************/
int get_transfac_num_species(TRANSFAC_MOTIF_T *motif) {
  return motif->species_list == NULL 
    ? 0 : get_num_strings(motif->species_list);
}

/*******************************************************************************
 * Return the nth TRANSFAC species string.
 * Caller is responsible for freeing string.
 ******************************************************************************/
char *get_transfac_nth_species(int n, TRANSFAC_MOTIF_T *motif) {
  char *species = NULL;
  copy_string(&species, get_nth_string(n, motif->species_list));
  return species;
}

/*******************************************************************************
 * Return matrix of TRANSFAC counts.
 * Caller is responsible for freeing the MATRIX_T.
 ******************************************************************************/
MATRIX_T *get_transfac_counts(TRANSFAC_MOTIF_T *motif) {
  MATRIX_T *counts = NULL;
  if (motif->counts != NULL) {
    counts = duplicate_matrix(motif->counts);
  }
  return counts;
}

/*******************************************************************************
 * WARNING THIS FUNCTION MODIFIES THE CONTENTS OF THE PASSED IN STRING
 *
 * This function splits a string at the first occurence of a delmiter
 * by replacing the delimiter with a null character.
 * After the call has returned, the passed in pointer points to the left-hand 
 * of the split and the return pointer points to the right hand portion.
 *
 * RETURNS a pointer to the portion of the string following the first 
 * occurence of the delimiter, or NULL if no occurences of the delimter
 * are found in the string.
 ******************************************************************************/
char *split(char *string, int delimiter) {

  char *split_point = index(string, delimiter);

  if (split_point == NULL) {
    // Couldn't split string 
    return NULL;
  }
  // Split string at delimiter by replacing with null character
  *split_point = 0;

  // Right-hand of split doesn't include the delimiter position
  return ++split_point;
}

/*******************************************************************************
 * WARNING THIS FUNCTION MODIFIES THE CONTENTS OF THE PASSED IN STRING
 * Trims initial and trailing white space from a string
 ******************************************************************************/
void trim(char *s) {
  size_t len = strlen(s);
  char *copy;
  copy_string(&copy, s);
  char *start = copy;
  char *end = copy + len - 1;
  while(isspace(*end)) { 
    *end = '\0';
    --end;
  }
  while(isspace(*start)) { 
    ++start;
  }
  if (start != copy || end != (copy + len - 1)) {
    strcpy(s, start);
  }
  myfree(copy);
}

/***********************************************************************
 * Read TRANSFAC motifs from a TRANSFAC file.
 * Returns an arraylist of pointers to TRANSFAC_MOTIF_T
 ***********************************************************************/
ARRAYLST_T *read_motifs_from_transfac_file (
  const char* transfac_filename  // Name of TRANSFAC file or '-' for stdin IN
) {

  // Create dynamic storage for motifs
  ARRAYLST_T *motif_list = arraylst_create();

  // Open the TRANFAC file for reading.
  FILE *transfac_file = NULL;
  if (open_file(
        transfac_filename, 
        "r", 
        TRUE, // Allow '-' for stdin
        "transfac file", 
        "",
        &transfac_file
      ) == FALSE) {
    exit(1);
  }

  // Read and parse the TRANFAC file.
  int num_bases = 4;
  char *line = NULL;
  while ((line = getline2(transfac_file)) != NULL) {

    // Split the line into an initial tag and everything else.
    char *this_accession = split(line, ' ');
    char *tag = line;

    // Have we reached a new matrix?
    if (strcmp(tag, "AC") == 0) {

      trim(this_accession);

      char *this_id = NULL;
      char *this_name = NULL;
      char *this_descr = NULL;
      char *this_species = NULL;
      char this_consensus[MAX_CONSENSUS_LENGTH];
      STRING_LIST_T *species_list = new_string_list();

      // Old versions of TRANSFAC use pee-zero; new use pee-oh.
      while (strcmp(tag, "PO") != 0 && strcmp(tag, "P0") != 0) {

        line = getline2(transfac_file);
        if (line == NULL) {
          die ("Can't find PO line for TRANSFAC matrix %s.\n", this_accession);
        }
        char *data = split(line, ' ');
        if (data != NULL) { 
          trim(data); 
        }
        tag = line;

        // Store the id line.
        if (strcmp(tag, "ID") == 0) { 
          this_id = strdup(data); 
        }
        // Store the species line.
        else if (strcmp(tag, "BF") == 0) { 
          add_string(data, species_list);
        }
        // Store the name line.
        else if (strcmp(tag, "NA") == 0) { 
          this_name = strdup(data); 
        }
        // Store the description line.
        else if (strcmp(tag, "DE") == 0) { 
          this_descr = strdup(data);
        }
      }

      // Read the motif.
      int motif_position = 0;
      int num_seqs = 0;
      this_consensus[0] = 0;
      MATRIX_T *motif_counts = allocate_matrix(40,4);
      while (TRUE) {

        line = getline2(transfac_file);
        if (line == NULL) {
          break;
        }

        char *data = split(line, ' ');
        if (data != NULL) { 
          trim(data); 
        }
        tag = line;

        // Look for the end of the motif.
        if ((strcmp(tag, "XX\n") == 0) || (strcmp(tag, "//\n") == 0)) {
         break;
        }

        motif_position = atoi(tag);

        // Store the contents of this row.
        int count[4];
        char consensus;
        sscanf(
          data, 
          "%d %d %d %d %c", 
          &(count[0]), 
          &(count[1]), 
          &(count[2]), 
          &(count[3]),
          &consensus
        );
        int i_base;
        for (i_base = 0; i_base < num_bases; i_base++) {
          set_matrix_cell(motif_position - 1, i_base, count[i_base], motif_counts);
        }
        this_consensus[motif_position - 1] = consensus;
        
      }

      this_consensus[motif_position] = 0;
      TRANSFAC_MOTIF_T *motif = new_transfac_motif(
        this_accession,
        this_id,
        this_name,
        this_descr,
        this_consensus,
        species_list,
        motif_counts
      );
      arraylst_add(motif, motif_list);

    }
  }

  fclose(transfac_file);
  return motif_list;

}

/***********************************************************************
 * Converts a TRANSFAC motif to a MEME motif.
 * Caller is responsible for freeing the returned MOTIF_T.
 ***********************************************************************/
MOTIF_T *convert_transfac_motif_to_meme_motif(
  char *id, 
  int pseudocount,
  ARRAY_T *bg,
  TRANSFAC_MOTIF_T *motif
) {
  MATRIX_T *counts = get_transfac_counts(motif);
  if (counts == NULL) {
    die(
      "Unable to convert TRANSFAC motif %s to MEME motif: "
      "missing counts matrix.",
      id
    );
  };

  // Convert the motif counts to frequencies.
  int num_bases = get_num_cols(counts);
  int motif_width = get_num_rows(counts);
  int motif_position = 0;
  MATRIX_T *freqs = allocate_matrix(motif_width, num_bases);
  for (motif_position = 0; motif_position < motif_width; ++motif_position) {
    int i_base = 0;
    int num_seqs = 0; // motif columns may have different counts
    for (i_base = 0; i_base < num_bases; i_base++) {
      num_seqs += get_matrix_cell(motif_position, i_base, counts);
    }
    for (i_base = 0; i_base < num_bases; i_base++) {
        double freq = 
          get_matrix_cell(motif_position, i_base, counts) 
          + (pseudocount * get_array_item(i_base, bg)) / (num_seqs + pseudocount);
        set_matrix_cell(motif_position, i_base, freq, freqs); 
    }
  }

  // Convert the motif counts to log-odds scores
  MATRIX_T *log_odds = allocate_matrix(motif_width, num_bases);
  for (motif_position = 0; motif_position < motif_width; motif_position++) {
    int i_base = 0;
    double p;
    double f;
    double lo;
    int num_seqs = 0; // motif columns may have different counts
    for (i_base = 0; i_base < num_bases; i_base++) {
      num_seqs += get_matrix_cell(motif_position, i_base, counts);
    }
    for (i_base = 0; i_base < num_bases; i_base++) {
      p = get_matrix_cell(motif_position, i_base, counts) / num_seqs;
      f = get_array_item(i_base, bg);
      lo = p > 0 ?  log(p/f)/log(2.0) : -100;
      set_matrix_cell(motif_position, i_base, lo, log_odds); 
    }
  }

  MOTIF_T *meme_motif = allocate_motif(id, DNA_ALPH, NULL, freqs);
  meme_motif->scores = log_odds;
  return meme_motif;
}

/***********************************************************************
 * Converts a list of TRANSFAC motifs to a list MEME motif.
 * If the use_accession parameter is true the TRANSFAC accession
 * is used as the name of the MEME motif. Otherwise the ID is used.
 * Caller is responsible for freeing the returned ARRAYLST
 ***********************************************************************/
ARRAYLST_T *convert_transfac_motifs_to_meme_motifs(
  BOOLEAN_T use_accession,
  int pseudocount,
  ARRAY_T *bg,
  ARRAYLST_T *tfac_motifs
) {
  int num_motifs = arraylst_size(tfac_motifs);
  ARRAYLST_T *meme_motifs = arraylst_create_sized(num_motifs);
  int motif_index = 0;
  for (motif_index = 0; motif_index < num_motifs; ++motif_index) {
    TRANSFAC_MOTIF_T *tfac_motif 
      = (TRANSFAC_MOTIF_T *) arraylst_get(motif_index, tfac_motifs);
    char *name = NULL;
    if (use_accession == TRUE) {
      name = get_transfac_accession(tfac_motif);
      if (name == NULL) {
        die("No accession string found in TRANSFAC motif.");
      }
    }
    else {
      name = get_transfac_id(tfac_motif);
      if (name == NULL) {
        die("No ID string found in TRANSFAC motif.");
      }
    }
    MOTIF_T *meme_motif 
      = convert_transfac_motif_to_meme_motif(name, pseudocount, bg, tfac_motif);
    arraylst_add(meme_motif, meme_motifs);
  }
  return meme_motifs;
}

