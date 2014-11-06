/********************************************************************
 * FILE: alphabet.c
 * AUTHOR: James Johnson
 * CREATE DATE: 21 July 2011
 * PROJECT: all
 * COPYRIGHT: 2011 UQ
 * DESCRIPTION: Define the amino acid and nucleotide alphabets as
 * well as any utility functions. Many of the functions are rewrites
 * of the original ones which made use of mutable global variables.
 ********************************************************************/

#include "alphabet.h"
#include "string-builder.h"
#include "regex-utils.h"
#include "red-black-tree.h"

#include <assert.h>
#include <ctype.h>

// The number of bytes to read per chunk of background file
#define BG_CHUNK_SIZE 100

/*
 * The regular expression to match a background frequency
 * from a background file
 */
const char *BGFREQ_RE = "^[[:space:]]*([a-zA-Z])[[:space:]]+"
    "([+]?[0-9]*\\.?[0-9]+([eE][-+]?[0-9]+)?)[[:space:]]*$";
/*
 * The names of the alphabets to make printing error messages easier
 */
char const * const ALPH_NAME[] = {"UNKNOWN", "AA", "DNA"};

/*
 * The non-ambiguous letters of the alphabets
 */
char const * const ALPH_CODE[] = {"", "ACDEFGHIKLMNPQRSTVWY", "ACGT"};

/*
 * The wildcards of the alphabets
 */
const char const ALPH_WILD[] = {0, 'X', 'N'};

/*
 * The count of non-ambiguous letters
 */
const int const ALPH_ASIZE[] = {0, PROTEIN_ASIZE, DNA_ASIZE};

/*
 * The ambiguous letters of the alphabets
 */
char const * const ALPH_AMCODE[] = {"", "BUXZ", "URYKMSWBDHVN"};

/*
 * The count of ambiguous letters
 */
const int const ALPH_AMSIZE[] = {0, 4, 12};

/*
 * The indexes assigned to letters get weird for the ambiguous characters and 
 * Uracil because this is at the moment designed to return the same results as 
 * the old alphabet.c because I have to change everything over before I can 
 * make it more logical.  
 */
short const ALPH_INDEX_AA[] = {
   0, /* A */ 20, /* B */  1, /* C */  2, /* D */  3, /* E */  4, /* F */ 
   5, /* G */  6, /* H */  7, /* I */ -1, /* J */  8, /* K */  9, /* L */
  10, /* M */ 11, /* N */ -1, /* O */ 12, /* P */ 13, /* Q */ 14, /* R */
  15, /* S */ 16, /* T */ 21, /* U */ 17, /* V */ 18, /* W */ 22, /* X */
  19, /* Y */ 23  /* Z */
};

short const ALPH_INDEX_DNA[] = {
   0, /* A */ 11, /* B */  1, /* C */ 12, /* D */ -1, /* E */ -1, /* F */ 
   2, /* G */ 13, /* H */ -1, /* I */ -1, /* J */  7, /* K */ -1, /* L */
   8, /* M */ 15, /* N */ -1, /* O */ -1, /* P */ -1, /* Q */  5, /* R */
   9, /* S */  3, /* T */  4, /* U */ 14, /* V */ 10, /* W */ -1, /* X */
   6, /* Y */ -1  /* Z */
};

/*
 * Lookup tables for the indexes of letters in the alphabet
 */
short const * const ALPH_INDEX[] = {NULL, ALPH_INDEX_AA, ALPH_INDEX_DNA};

/*
 * Non-redundant Database frequencies for AA
 */
PROB_T const ALPH_NRDB_AA[] = {
  0.073164, /* A */ 0.018163, /* C */ 0.051739, /* D */ 0.062340, /* E */ 
  0.040283, /* F */ 0.069328, /* G */ 0.022428, /* H */ 0.056282, /* I */
  0.058493, /* K */ 0.091712, /* L */ 0.023067, /* M */ 0.046077, /* N */ 
  0.050674, /* P */ 0.040755, /* Q */ 0.051897, /* R */ 0.073802, /* S */ 
  0.059411, /* T */ 0.064362, /* V */ 0.013341, /* W */ 0.032682  /* Y */
};

/*
 * Non-redundant Database frequencies for DNA
 */
PROB_T const ALPH_NRDB_DNA[] = {
  0.281774, /* A */ 0.222020, /* C */ 0.228876, /* G */ 0.267330 /* T */
};

/*
 * Non-redundant Database frequencies for the alphabets
 */
PROB_T const * const ALPH_NRDB[] = {NULL, ALPH_NRDB_AA, ALPH_NRDB_DNA};

/*
 * Get the name of the alphabet - useful for error messages
 */
const char* alph_name(ALPH_T alph) {
  return ALPH_NAME[alph];
}

/*
 * Get the size of the alphabet
 */
int alph_size(ALPH_T alph, ALPH_SIZE_T which_size) {
  switch (which_size) {
    case ALPH_SIZE:
      return ALPH_ASIZE[alph];
    case AMBIG_SIZE:
      return ALPH_AMSIZE[alph];
    case ALL_SIZE:
      return ALPH_ASIZE[alph] + ALPH_AMSIZE[alph];
    default:
      return 0;
  }
}

/*
 * Get the letter at an index of the alphabet
 */
char alph_char(ALPH_T alph, int index) {
  assert(index >= 0);
  if (index < ALPH_ASIZE[alph]) {
    return ALPH_CODE[alph][index];
  }
  index -= ALPH_ASIZE[alph];
  assert(index < ALPH_AMSIZE[alph]);
  return ALPH_AMCODE[alph][index];
}

/*
 * Get the alphabet string
 */
const char* alph_string(ALPH_T alph) {
  return ALPH_CODE[alph];
}

/*
 * Get the wildcard letter of the alphabet
 */
char alph_wildcard(ALPH_T alph) {
  assert(ALPH_WILD[alph]);
  return ALPH_WILD[alph];
}

/*
 * Determine if a letter is known in this alphabet
 */
BOOLEAN_T alph_is_known(ALPH_T alph, char letter) {
  int index;
  index = alph_index(alph, letter);
  return (index != -1);
}

/*
 * Determine if a letter is a concrete representation
 */
BOOLEAN_T alph_is_concrete(ALPH_T alph, char letter) {
  int index;
  index = alph_index(alph, letter);
  return (index != -1 && index < ALPH_ASIZE[alph]);
}

/*
 * Determine if a letter is ambiguous
 */
BOOLEAN_T alph_is_ambiguous(ALPH_T alph, char letter) {
  int index;
  index = alph_index(alph, letter);
  return (index >= ALPH_ASIZE[alph]);
}

/*
 *  Tests the letter against the alphabet. If the alphabet is unknown
 *  it attempts to work it out and set it from the letter.
 *  For simplicy this assumes you will pass indexes in asscending order.
 *  Returns false if the letter is unacceptable
 */
BOOLEAN_T alph_test(ALPH_T *alpha, int index, char letter) {
  char uc_letter;
  uc_letter = toupper(letter);
  if (*alpha == INVALID_ALPH) {
    switch (index) {
      case 0:
        return (uc_letter == 'A');
      case 1:
        return (uc_letter == 'C');
      case 2:
        if (uc_letter == 'D') {
          *alpha = PROTEIN_ALPH;
          return TRUE;
        }
        return (uc_letter == 'G'); // DNA or RNA
      case 3:
        if (uc_letter == 'T') {
          *alpha = DNA_ALPH;
        } else if (uc_letter == 'U') {
          *alpha = DNA_ALPH; //FIXME need RNA but substitute DNA for now
        } else {
          return FALSE;
        }
        return TRUE;
      default:// Bad state!
        die("Should not still be attempting to guess by the 5th letter "
            "(index = %d).", index);
        return FALSE;
    }
  } else {
    if (index >= alph_size(*alpha, ALPH_SIZE)) return FALSE; // index too big
    return (uc_letter == alph_char(*alpha, index));
  }
}

/*
 *  Tests the alphabet string and attempts to return the ALPH_T.
 *  If the alphabet is from some buffer a max size can be set
 *  however it will still only test until the first null byte.
 *  If the string is null terminated just set a max > 20 
 */
ALPH_T alph_type(const char *alphabet, int max) {
  int i;
  ALPH_T alph;
  alph = INVALID_ALPH;
  for (i = 0; i < max && alphabet[i] != '\0'; ++i) {
    if (!alph_test(&alph, i, alphabet[i])) return INVALID_ALPH;
  }
  if (i != ALPH_ASIZE[alph]) return INVALID_ALPH;
  return alph;
}

/*
 * Make one position in an array the sum of a set of other positions.
 */
static void calc_ambig(ALPH_T alph, BOOLEAN_T log_space, char ambig, 
    char *sources, ARRAY_T *array) {
  char *source;
  PROB_T sum;
  PROB_T value;

  sum = 0.0;
  for (source = sources; *source != '\0'; ++source) {
    value = get_array_item(alph_index(alph, *source), array);
    if (log_space) {
      sum = LOG_SUM(sum, value);
    } else {
      sum += value;
    }
  }
  set_array_item(alph_index(alph, ambig), sum, array);
}

/*
 * Calculate the values of the ambiguous letters from sums of
 * the normal letter values.
 */
void calc_ambigs(ALPH_T alph, BOOLEAN_T log_space, ARRAY_T* array) {
  switch (alph) {
  case PROTEIN_ALPH :
    calc_ambig(alph, log_space, 'B', "DN", array);
    calc_ambig(alph, log_space, 'Z', "EQ", array);
    calc_ambig(alph, log_space, 'U', "ACDEFGHIKLMNPQRSTVWY", array);
    calc_ambig(alph, log_space, 'X', "ACDEFGHIKLMNPQRSTVWY", array);
    break;
  case DNA_ALPH:
    calc_ambig(alph, log_space, 'U', "T", array);
    calc_ambig(alph, log_space, 'R', "GA", array);
    calc_ambig(alph, log_space, 'Y', "TC", array);
    calc_ambig(alph, log_space, 'K', "GT", array);
    calc_ambig(alph, log_space, 'M', "AC", array);
    calc_ambig(alph, log_space, 'S', "GC", array);
    calc_ambig(alph, log_space, 'W', "AT", array);
    calc_ambig(alph, log_space, 'B', "GTC", array);
    calc_ambig(alph, log_space, 'D', "GAT", array);
    calc_ambig(alph, log_space, 'H', "ACT", array);
    calc_ambig(alph, log_space, 'V', "GCA", array);
    calc_ambig(alph, log_space, 'N', "ACGT", array);
    break;
  default :
    die("Alphabet uninitialized.\n");
  }
}


/*
 * Replace the elements an array of frequences with the average
 * over complementary bases.
 */
void average_freq_with_complement(ALPH_T alph, ARRAY_T *freqs) {
  int a_index, t_index, g_index, c_index;
  double at_freq, gc_freq;

  assert(alph == DNA_ALPH);

  a_index = alph_index(alph, 'A');
  t_index = alph_index(alph, 'T');
  g_index = alph_index(alph, 'G');
  c_index = alph_index(alph, 'C');

  at_freq = (get_array_item(a_index, freqs) + 
      get_array_item(t_index, freqs)) / 2.0;
  gc_freq = (get_array_item(g_index, freqs) + 
      get_array_item(c_index, freqs)) / 2.0;
  set_array_item(a_index, at_freq, freqs);
  set_array_item(t_index, at_freq, freqs);
  set_array_item(g_index, gc_freq, freqs);
  set_array_item(c_index, gc_freq, freqs);

}

/*
 * Load the non-redundant database frequencies into the array.
 */
ARRAY_T* get_nrdb_frequencies(ALPH_T alph, ARRAY_T *freqs) {
  int i, size;
  const PROB_T *nrdb_freqs;

  size = ALPH_ASIZE[alph];
  if (freqs == NULL) freqs = allocate_array(alph_size(alph, ALL_SIZE));
  assert(get_array_length(freqs) >= alph_size(alph, ALL_SIZE));
  nrdb_freqs = ALPH_NRDB[alph];
  for (i = 0; i < size; ++i) {
    set_array_item(i, nrdb_freqs[i], freqs);
  }
  normalize_subarray(0, size, 0.0, freqs);
  calc_ambigs(alph, FALSE, freqs);
  return freqs;
}

/*
 * Load uniform frequencies into the array.
 */
ARRAY_T* get_uniform_frequencies(ALPH_T alph, ARRAY_T *freqs) {
  int i, n;

  n = ALPH_ASIZE[alph];
  if (freqs == NULL) freqs = allocate_array(alph_size(alph, ALL_SIZE));
  assert(get_array_length(freqs) >= alph_size(alph, ALL_SIZE));
  for (i = 0; i < n; i++) { 
    set_array_item(i, 1.0/n, freqs); 
  }
  calc_ambigs(alph, FALSE, freqs);
  return freqs;
}

/*
 * Load background file frequencies into the array.
 */
ARRAY_T* get_file_frequencies(ALPH_T *alph, char *bg_filename, ARRAY_T *freqs) {
  regmatch_t matches[4];
  STR_T *line;
  char chunk[BG_CHUNK_SIZE+1], letter[2], *key;
  int size, terminate, offset, i;
  FILE *fp;
  regex_t bgfreq;
  double freq;
  RBTREE_T *letters;
  RBNODE_T *node;
  
  regcomp_or_die("bg freq", &bgfreq, BGFREQ_RE, REG_EXTENDED);
  letters = rbtree_create(rbtree_strcasecmp, rbtree_strcpy, free, rbtree_dblcpy, free);
  line = str_create(100);
  if (!(fp = fopen(bg_filename, "r"))) {
    die("Unable to open background file \"%s\" for reading.\n", bg_filename);
  }
  
  terminate = feof(fp);
  while (!terminate) {
    size = fread(chunk, sizeof(char), BG_CHUNK_SIZE, fp);
    chunk[size] = '\0';
    terminate = feof(fp);
    offset = 0;
    while (offset < size) {
      // skip mac newline
      if (str_len(line) == 0 && chunk[offset] == '\r') {
        offset++;
        continue;
      }
      // find next new line
      for (i = offset; i < size; ++i) {
        if (chunk[i] == '\n') break;
      }
      // append portion up to the new line or end of chunk
      str_append(line, chunk+offset, i - offset);
      // read more if we didn't find a new line
      if (i == size && !terminate) break;
      // move the offset past the new line
      offset = i + 1;
      // handle windows new line
      if (str_char(line, -1) == '\r') str_truncate(line, -1);
      // remove everything to the right of a comment character
      for (i = 0; i < str_len(line); ++i) {
        if (str_char(line, i) == '#') {
          str_truncate(line, i);
          break;
        }
      }
      // check the line for a single letter followed by a number
      if (regexec_or_die("bg freq", &bgfreq, str_internal(line), 4, matches, 0)) {
        // parse the letter and frequency value
        regex_strncpy(matches+1, str_internal(line), letter, 2);
        freq = regex_dbl(matches+2, str_internal(line));
        // check the frequency is acceptable
        if (freq < 0 || freq > 1) {
          die("The background file lists the illegal probability %g for "
            "the letter %s.\n", freq, letter);
        } else if (freq == 0) {
          die("The background file lists a probability of zero for the "
            "letter %s\n", letter);
        }
        if (freq >= 0 && freq <= 1) rbtree_put(letters, letter, &freq);
      }
      str_clear(line);
    }
  }
  // finished with the file so clean up file parsing stuff
  fclose(fp);
  str_destroy(line, FALSE);
  regfree(&bgfreq);
  // guess the alphabet
  if (*alph == INVALID_ALPH) {
    switch (rbtree_size(letters)) {
      case PROTEIN_ASIZE:
        *alph = PROTEIN_ALPH;
        break;
      case DNA_ASIZE:
        *alph = DNA_ALPH;
        break;
      default:
        die("Number of single character entries in background does not match "
            "an alphabet.\n");
    }
  }
  // make the background
  if (freqs == NULL) freqs = allocate_array(alph_size(*alph, ALL_SIZE));
  assert(get_array_length(freqs) >= alph_size(*alph, ALL_SIZE));
  init_array(-1, freqs);
  for (node = rbtree_first(letters); node != NULL; node = rbtree_next(node)) {
    key = (char*)rbtree_key(node);
    i = alph_index(*alph, key[0]);
    freq = *((double*)rbtree_value(node));
    if (i == -1) {
      die("Background contains letter %s which is not in the %s alphabet.\n", 
          key, alph_name(*alph));
    }
    if (get_array_item(i, freqs) != -1) {
      die("Background contains letter %s which has the same meaning as an "
          "already listed letter.\n", key);
    }
    set_array_item(i, freq, freqs);
  }
  // check that all items were set
  for (i = 0; i < alph_size(*alph, ALPH_SIZE); i++) {
    if (get_array_item(i, freqs) == -1) {
      die("Background is missing letter %c.\n", alph_char(*alph, i));
    }
  }
  // disabled for backwards compatability (AMA test was failing)
  //normalize_subarray(0, ALPH_ASIZE[*alph], 0.0, freqs);
  // calculate the values of the ambiguous letters from the concrete ones
  calc_ambigs(*alph, FALSE, freqs);
  // cleanup
  rbtree_destroy(letters);
  // return result
  return freqs;
}


/*
 * Get a background distribution
 *  - by reading values from a file if filename is given, or
 *  - equal to the NRDB frequencies if filename is NULL.
 */
ARRAY_T* get_background(ALPH_T alph, char* bg_filename) {
  ARRAY_T* background;

  if ((bg_filename == NULL) || (strcmp(bg_filename, "nrdb") == 0)) {
    background = get_nrdb_frequencies(alph, NULL);
  } else {
    background = get_file_frequencies(&alph, bg_filename, NULL);
  }

  return(background);
}


/*
 * Take the counts from an ambiguous character and evenly distribute
 * them among the corresponding concrete characters.
 *
 * This function operates in log space.
 */
static void dist_ambig(ALPH_T alph, char ambig, char *concrete_chars, 
    ARRAY_T* freqs) {
  PROB_T ambig_count, concrete_count;
  int ambig_index, num_concretes, i, concrete_index;

  // Get the count to be distributed.
  ambig_index = alph_index(alph, ambig);
  ambig_count = get_array_item(ambig_index, freqs);
  // Divide it by the number of corresponding concrete characters.
  num_concretes = strlen(concrete_chars);
  ambig_count -= my_log2((PROB_T)num_concretes);
  // Distribute it in equal portions to the given concrete characters.
  for (i = 0; i < num_concretes; i++) {
    concrete_index = alph_index(alph, concrete_chars[i]);
    concrete_count = get_array_item(concrete_index, freqs);
    // Add the ambiguous counts.
    concrete_count = LOG_SUM(concrete_count, ambig_count);
    set_array_item(concrete_index, concrete_count, freqs);
  }
  // Set the ambiguous count to zero.
  set_array_item(ambig_index, LOG_ZERO, freqs);
}

/*
 * Take the counts from each ambiguous character and evenly distribute
 * them among the corresponding concrete characters.
 *
 * This function operates in log space.
 */
void dist_ambigs(ALPH_T alph, ARRAY_T* freqs) {
  switch (alph) {
  case PROTEIN_ALPH:
    dist_ambig(alph, 'B', "DN", freqs);
    dist_ambig(alph, 'Z', "EQ", freqs);
    dist_ambig(alph, 'U', "ACDEFGHIKLMNPQRSTVWY", freqs);
    dist_ambig(alph, 'X', "ACDEFGHIKLMNPQRSTVWY", freqs);
    break;
  case DNA_ALPH:
    dist_ambig(alph, 'U', "T", freqs);
    dist_ambig(alph, 'R', "GA", freqs);
    dist_ambig(alph, 'Y', "TC", freqs);
    dist_ambig(alph, 'K', "GT", freqs);
    dist_ambig(alph, 'M', "AC", freqs);
    dist_ambig(alph, 'S', "GC", freqs);
    dist_ambig(alph, 'W', "AT", freqs);
    dist_ambig(alph, 'B', "GTC", freqs);
    dist_ambig(alph, 'D', "GAT", freqs);
    dist_ambig(alph, 'H', "ACT", freqs);
    dist_ambig(alph, 'V', "GCA", freqs);
    dist_ambig(alph, 'N', "ACGT", freqs);
    break;
  default :
    die("Alphabet uninitialized.\n");
  }
}

