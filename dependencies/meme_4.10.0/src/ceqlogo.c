/* ----------------------- Implementation -----------------------------

  Module name   : ceqlogo.c

  Description: Create logos for a given set of motifs/PWMs.

  Author   : S. Maetschke

  Copyright: Institute for Molecular Bioscience (IMB)

------------------------------------------------------------------------ */

/* ---------------------------- Includes ------------------------------- */
#include <ctype.h>
#include <errno.h>
#include <getopt.h>
#include <limits.h>
#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdarg.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#include "macros.h"
#include "matrix.h"
#include "alphabet.h"
#include "string-builder.h"
#include "motif.h"
#include "user.h"
#include "utils.h"
#include "motif-in.h"
#include "config.h"
#include "dir.h"
#include "eps2png.h"

#include "ceqlogo.h"

/* ----------------------- Global Types -------------------------------- */

/** Set of logo line specific parameters */
typedef struct {
  MATRIX_T* mat;      /* PWM of the logo */
  ALPH_T    alphabet; /* Alphabet used by the PWM */
  int       snum;     /* Number of samples the PWM is derived from. Can be 0 */
  int       ltrim;    /* Number of positions from the left to display trimmed */
  int       rtrim;   /* Number of positions from the right to display trimmed */
  int       shift;    /* horizontal shift of a logo line */
  char*     label;    /* label for the logo line */
} LP_LINE_T;

/** Set of general logo parameters that get replaced within the template file*/
typedef struct {
  LP_LINE_T* lines; // logo line specific parameters
  int l_num; // number of logo lines
  int shift; // shift added to the line's own shift to make it non-negative
  // parameters which will be converted to tokens for substitution
  ALPH_T alphabet; // alphabet of logo
  double logo_height;
  double logo_width;
  double error_bar_fraction;
  double tic_bits;
  char* title;
  char* fineprint;
  char* x_axis_label;
  char* y_axis_label;
  bool ssc;
  bool show_y_axis;
  bool show_ends;
  bool show_error_bar;
  bool show_outline;
  bool show_numbering;
  bool show_box;
} LP_PARAMS_T;

/** Describes a letter and its height within the letter stack of a logo */
typedef struct {
  double height;
  char   letter;
} LSTACK_T;

/* Stores a motif load command */
typedef struct {
  char* file;
  char* id;
  int samples;
  int shift;
  bool rc;
  MOTIF_T* motif_by_id;
  MOTIF_T* motif_by_pos;
} INPUT_T;

/* stores processed command-line options */
typedef struct {
  char* output_file;
  char* format;
  double pseudocount; // pseudocount applied to motifs in logo
  bool prefer_ids;
  bool run_tests;
  INPUT_T *inputs;
  int input_num;
  LP_PARAMS_T* params;
} OPTIONS_T;

/* ----------------------- Global Constants ---------------------------- */
// Path to the template file
#define TEMPLATE_EPS "template.eps"
/* -------------------------- Global Variables -------------------------- */
// Number of open files
static size_t F_counter = 0;
/* ------------------------- Error Functions -------------------------- */

/*
  Description   : Prints the current file and line number to stderr.
  Parameter     :
    file        : File name.
    line        : Line number.
  Example       :
    E_position(__FILE__,__LINE__);
*/
static void E_position(char* file, int line) {
  fprintf(stderr, "\nError in: %s\nLine: %d\n", file, line);
}

/*
  Description   : Prints out an error message to stderr.


  Parameters    :
    format      : Format string like printf.
    ...         : arguments described in the format string.

  Example       :
    int errno;
    E_error("This the error number %d\n", errno);
*/
static void E_error(char* format, ...) {
  va_list argptr;

  va_start(argptr, format);
    vfprintf(stderr, format, argptr);
  va_end(argptr);
}

/*
  Description   : Prints out file name and line number for a
                  system error to stderr.
                  System errors are errors in the usage of code
                  and should not be used to signal input errors
                  of the user. For that purpose use E_error().
                  The function stops the execution of the application
                  and exits with error code -1.

  Parameters    :
    format      : Format string like printf.
    ...         : arguments described in the format string.

  Example       :
    int errlevel;
    E_system("This a system error. Error level %d", errlevel);
*/
#define E_system  E_position(__FILE__,__LINE__) , _E_system
static void _E_system(char* format, ...) {
  va_list argptr;

  fprintf(stderr, "\nSYSTEM ERROR:\n");
  va_start(argptr, format);
    vfprintf(stderr, format, argptr);
  va_end(argptr);
  exit(-1);
}

/* ------------------------- String Functions -------------------------- */

/*
  Description   : Returns a time & date string according to the specified
                  format. (Non-reentrant!)
  Parameter     :
    format      : e.g. %b %d, %Y; %H:%M:%S   (see time.h)
  Return Values :
    function    : String with time & date.
  Example       :
    STR_time(""); -> Jan 10, 1987; 17:55:55
*/
static char* STR_time(char* format) {
  static char timestr[100];
  time_t now = time(NULL);
  strftime(timestr,100,format, localtime(&now));
  return(timestr);
}

/*
  Description   : Converts a integer into a string (Non-reentrant!).
  Parameter     :
    value       : Integer value to convert.
  Return Values :
    function    : String containing the integer.
*/
static char* STR_int(int value) {
  static char buffer[20];
  snprintf(buffer, sizeof(buffer), "%d", value);
  return buffer;
}

/*
  Description   : Converts a double into a string (Non-reentrant!)
  Parameter     :
    value       : Double value to convert.
  Return Values :
    function    : String containing the double.
*/
static char* STR_dbl(double value) {
  static char buffer[50];
  snprintf(buffer, sizeof(buffer), "%g", value);
  return buffer;
}

/*
  Description   : Converts a boolean into a string
  Parameter     :
    value       : Boolean value to convert.
  Return Values :
    function    : String containing the boolean.
*/
#define STR_bool(value) ((value) ? "true" : "false")

/* ------------------------- File Access Functions -------------------------- */

/*
  Description   : Opens a file. Alway use the macro.
  Parameter     :
    name        : Name/path of the file.
    type        : File type, e.g. "r"
  Return Values :
    function    : Returns a file handle or prints out an error message and
                  exits if the file couldn't be opened.
  Example       :
    FILE *f = F_open("mytext.txt","r");
    F_close(f);
*/
#define F_open(name, type) _F_open((name),(type), __LINE__, __FILE__)
static FILE* _F_open(const char *name, const char *type, int line, const char *file) {
  FILE *fp = fopen(name, type);
  if(fp == NULL) {
    E_system("Cannot open file '%s'\n(line: %d, file: %s)\n", name, line, file);
  }
  F_counter++;
  return(fp);
}

/*
  Description   : Opens a temporary file. Alway use the macro.
  Parameter     :
    name        : Mutable name/path of the file with pattern XXXXXX on end.
    type        : File type, e.g. "r"
  Return Values :
    name        : The full file name (with the X's overwritten)
    function    : Returns a file handle or prints out an error message and
                  exits if the file couldn't be opened.
  Example       :
    FILE *f = F_open("mytext.txt","r");
    F_close(f);
*/
#define F_temp(name, type) _F_temp((name), (type), __LINE__, __FILE__)
static FILE* _F_temp(char *name, const char *type, int line, const char *file) {
  int fd;
  FILE *fp;
  fd = mkstemp(name);
  if (fd == -1)  {
    E_system("Cannot generate temporary file '%s'\n(line: %d, file: %s)\n", name, line, file);
  }
  fp = fdopen(fd, type);
  if(fp == NULL) {
    E_system("Cannot open file '%s'\n(line: %d, file: %s)\n", name, line, file);
  }
  F_counter++;
  return(fp);
}

/*
  Description   : Closes a file. Alway use the macro.
  Parameter     :
    fp          : File pointer.
  Example       :
    FILE *f = F_open("mytext.txt","r");
    F_close(f);
*/
#define F_close(fp) _F_close((fp), __LINE__, __FILE__)
static void _F_close(FILE *fp, int line, const char *file) {
  if(fp != NULL) {
    fclose(fp);
  } else {
    fprintf(stderr,"File with NULL handle closed!\n(line: %d, file: %s)\n",line, file);
  }
  F_counter--;
}

/*
  Description   : Copies a file to stdout.
  Parameter     :
    file_path   : The file to copy to stdout.
*/
static void F_copy_to_stdout(const char *file_path) {
  int n;
  char buffer[4096];
  FILE *file;
  file = F_open(file_path, "r");
  while ((n = fread(buffer, 1, sizeof(buffer), file))) {
    fwrite(buffer, 1, n, stdout);
  }
  F_close(file);
}

/*
  Description   : Reads an input file line by line, replaces all ${tokens}
                  by their values and writes the result to the given
                  output file.
  Parameter     :
    tokens      : Token set.
    fin         : Input file.
    fout        : Output file.
    len         : Maximum line length in input file.
*/
static void F_token_replace(RBTREE_T* tokens, FILE *fin, FILE *fout, size_t len) {
  char *line = (char*)mymalloc(len*sizeof(char));

  if(fin && fout) {
    while(fgets( line, len, fin ) != NULL ) {   /* Read input lines */
      char name[100];
      char* value;
      char* start = NULL;
      char* end   = NULL;
      char* seg   = line;

      while( (start = strstr(seg, "{$")) != NULL ) {  /* search param. start */
        end = strchr(start,'}');                      /* search param. end */
        if(end == NULL) {
          E_error("Missing bracket after parameter name!\nline=%s\n",line);
          break;
        }
        *start = '\0';                             /* term. line segment */
        *end   = '\0';                             /* term. param. name */
        strcpy(name, start+1);                     /* get param. name */
        value = (char*)rbtree_get(tokens, name);
        if(value == NULL)
          E_system("Invalid parameter in template: %s\n", name+1);
        fputs(seg, fout);                          /* write line segment */
        fputs(value==NULL ? "" : value, fout);     /* write param. value */
        seg = end+1;                               /* jump to next line seg */
      }
      fputs(seg, fout);                            /* write remainder */
    }
  }

  myfree(line);
}

/*
  Description   : Checks if all files that have been opened were closed
                  as well. Prints out an error message if not all files
                  were closed.
  Global Var.   :
    F_counter   : Counter for open files.
*/
static void F_check() {
  if (F_counter != 0) {
    E_system("File open/close calls were imbalanced: %d", F_counter);
  }
}

/* ----------------------- Logo Params Functions -----------------------------

  Description: Describes the parameters of logo. Two types of parameters
    are distinguished. General parameters which concern the entire logo/diagram
    and line parameters which describe properties of a specific logo line.

------------------------------------------------------------------------ */

/*
  Description   : Creates a and inits the parameter set.
  Return Values :
    function    : Pointer to the created parameter set. Must be freed!
  Example       :
    LP_PARAMS_T* params = LP_create();
    LP_free(params);
*/
static LP_PARAMS_T* LP_create() {
  LP_PARAMS_T* params = (LP_PARAMS_T*)mymalloc(sizeof(LP_PARAMS_T));
  memset(params, 0, sizeof(LP_PARAMS_T));
  params->l_num = 0;
  params->lines = NULL;
  params->shift = 0;
  params->alphabet = INVALID_ALPH;
  params->logo_height = 0.0;
  params->logo_width = 0.0;
  params->error_bar_fraction = 0.0;
  params->tic_bits = 0.0;
  params->title = NULL;
  params->fineprint = NULL;
  params->x_axis_label = NULL;
  params->y_axis_label = NULL;
  params->ssc = FALSE;
  params->show_ends = FALSE;
  params->show_error_bar = FALSE;
  params->show_outline = FALSE;
  params->show_box = FALSE;
  params->show_numbering = TRUE;
  params->show_y_axis = TRUE;
  return params;
}

/*
  Description   : Frees a parameter set.
  Parameter     :
    params      : Parameter set to free.
*/
static void LP_free(LP_PARAMS_T* params) {
  int i;
  LP_LINE_T *line;
  /* free all logo line specific parameters */
  for (i = 0; i < params->l_num; i++) {
    line = params->lines+i;
    free_matrix(line->mat);
    myfree(line->label);
  }
  myfree(params->lines);
  if (params->title != NULL) free(params->title);
  if (params->fineprint != NULL) free(params->fineprint);
  if (params->x_axis_label != NULL) free(params->x_axis_label);
  if (params->y_axis_label != NULL) free(params->y_axis_label);
  memset(params, 0, sizeof(LP_PARAMS_T));
  myfree(params);
}

/*
  Description   : Set the title of the logo.
  Parameter     :
    params      : Parameter set
    title       : Logo title
*/
static void LP_set_title(LP_PARAMS_T* params, const char* title) {
  if (params->title != NULL) free(params->title);
  if (title != NULL) {
    params->title = strdup(title);
  } else {
    params->title = NULL;
  }
}

/*
  Description   : Set the fineprint of the logo.
  Parameter     :
    params      : Parameter set
    title       : Logo fineprint
*/
static void LP_set_fineprint(LP_PARAMS_T* params, const char* fineprint) {
  if (params->fineprint != NULL) free(params->fineprint);
  if (fineprint != NULL) {
    params->fineprint = strdup(fineprint);
  } else {
    params->fineprint = NULL;
  }
}

/*
  Description   : Set the x-axis label of the logo.
  Parameter     :
    params      : Parameter set
    title       : Logo x-axis label
*/
static void LP_set_x_axis_label(LP_PARAMS_T* params, const char* x_axis_label) {
  if (params->x_axis_label != NULL) free(params->x_axis_label);
  if (x_axis_label != NULL) {
    params->x_axis_label = strdup(x_axis_label);
  } else {
    params->x_axis_label = NULL;
  }
}

/*
  Description   : Set the y-axis label of the logo.
  Parameter     :
    params      : Parameter set
    title       : Logo y-axis label
*/
static void LP_set_y_axis_label(LP_PARAMS_T* params, const char* y_axis_label) {
  if (params->y_axis_label != NULL) free(params->y_axis_label);
  if (y_axis_label != NULL) {
    params->y_axis_label = strdup(y_axis_label);
  } else {
    params->y_axis_label = NULL;
  }
}

/*
  Description   : Getter for the parameters of a logo line.
  Parameter     :
    params      : Parameter set.
    line        : line number.

  Return Values :
    function    : Returns the address of the parameter block for the specified
                  logo line.
*/
static LP_LINE_T* LP_get_line(LP_PARAMS_T* params, int line) {
  if(line >= params->l_num)
    E_system("Invalid line number %d!\n", line);
  return(&(params->lines[line]));
}

/*
  Description   : Adds an additional line to the logo.

  Parameter     :
    params      : Parameter set.
    mat         : PWM. The number of matrix columns must be greater than or equal
                  to the size of the alphabet.
    snum        : Number of samples the PWM was derived from. Can be 0 and is
                  only used to calc. a small sample correction and error bars.
    ltrim       : Number of positions from the left to display trimmed.
    rtrim       : Number of positions from the right to display trimmed.
    alphabet    : Alphabet used by the PWM.
                  Must match with the type of the logo.
    shift       : Shifting of the logo. Can be negative.
    label       : label for the logo line. Can be NULL.

  Return Values :
    params      : Some parameters of the set will be modified.
*/
static void LP_add_line(LP_PARAMS_T* params, MATRIX_T* mat, int snum, int ltrim,
        int rtrim, ALPH_T alphabet, int shift, char* label) {
  if (params->alphabet == INVALID_ALPH) {
    // if we don't know the alphabet then set it from the motif alphabet
    params->alphabet = alphabet;
  } else if (params->alphabet != alphabet) {
    E_system("Alphabet and logo type (DNA,RNA,AA) do not match!\n");
  }
  if(alph_size(alphabet, ALPH_SIZE) > mat->num_cols)
    E_system("Alphabet size and matrix dimensions do not match!\n");

  /* add an empty line */
  params->l_num++;
  Resize(params->lines, params->l_num, LP_LINE_T);

  /* fill the added line */
  LP_LINE_T* line = LP_get_line(params, params->l_num-1);
  line->mat = duplicate_matrix(mat);
  line->snum = snum;
  line->ltrim = ltrim;
  line->rtrim = rtrim;
  line->shift = shift;
  line->label = label == NULL ? NULL : strdup(label);

  // update the global shift
  if (shift < -(params->shift)) params->shift = -shift;
}


/* ----------------------- Letter Stack Functions ----------------------------

  Description: Implements a letter stack to generate a logo.
               The stack is calculated on base of a Position Weight Matrix
               (PWM) that contains the probabilites for letters
               (nucleotides, amino acids) within a set of aligned
               sequences.
               The columns of the matrix represent the alphabet
               and the rows the positions within the alignment.
               The letters of the alphabet are assumed to label
               the corresponding columns of the matrix.

               The letter stacks are calculated by the following equations found in
               Schneider and Stephens paper "Sequence Logos: A New Way to Display
               Consensus Sequences":

                 height = f(b,l) * R(l)                            (1)

               where f(b,l) is the frequency of base or amino acid "b" at position
               "l". R(l) is amount of information present at position "l" and can
               be quantified as follows:

                 R(l) for amino acids   = log(20) - (H(l) + e(n))    (2a)
                 R(l) for nucleic acids =    2    - (H(l) + e(n))    (2b)

               where log is taken base 2, H(l) is the uncertainty at position "l",
               and e(n) is the error correction factor for small "n". H(l) is
               computed as follows:

                   H(l) = - (Sum f(b,l) * log[ f(b,l) ])             (3)

               where again, log is taken base 2. f(b,l) is the frequency of base
               "b" at position "l". The sum is taken over all amino acids or
               bases, depending on which the data is.

               Currently, logo.pm uses an approximation for e(n), given by:

                   e(n) = (s-1) / (2 * ln2 * n)                      (4)

               Where s is 4 for nucleotides, 20 for amino acids ; n is the number
               of sequences in the alignment. e(n) also  gives the height of error
               bars.
               The code and the comment above are based on the Perl code from logo.pm
               which is part of weblogo: http://weblogo.berkeley.edu

*/

/*
  Description   : Creates a letter stack data structure.
                  Don't forget to free the letter stack: LS_free().
  Parameter     :
    size        : Size of the letter stack (=alphabet size/number of
                  PWM columns).
  Return Values :
    function    : Returns a letter stack of the specified size.
  Example       :
    LSTACK_T* lstack = LS_create(4);
    LS_free(lstack);
*/
static LSTACK_T* LS_create(size_t size) {
  return (LSTACK_T*)mymalloc(sizeof(LSTACK_T) * size);
}

/*
  Description   : Frees the letter stack.
  Parameter     :
    lstack      : Letter stack.
  Example       :
    LSTACK_T* lstack = LS_create(4);
    LS_free(lstack);
*/
static void LS_free(LSTACK_T* lstack) {
   myfree(lstack);
}

/*
  Description   : Calculates the max. entropy for the given alphabet size.
  Parameter     :
    asize       : Size of the alphabet.
  Return Values :
    function    : Returns log_2();
  Example       :
    double Hmax = LS_calc_Hmax(4);
*/
static double LS_calc_Hmax(int asize) {
  return log(asize) / log(2.0);
}

/*
  Description   : Calculates the entropy for a matrix row (= position of
                  the logo).
  Parameter     :
    row         : Matrix row. First row is zero.
    mat         : PWM.
    asize       : Size of the alphabet. The number of matrix columns must be
                  greater than or equal to asize.
  Return Values :
    function    : Returns the entropy for the specified matrix row.
  Example       :
    double H = LS_calc_H(0,mat,4);
*/
static double LS_calc_H(ARRAY_T* items, int asize) {
  int col;
  double H = 0.0;
  for (col = 0; col < asize; col++) {
    double p = get_array_item(col, items);
    if (p > 0.0) H -= p * (log(p) / log(2.0));
  }
  return H;
}

/*
  Description   : Calculate the error correction factor for small numbers
                  of samples (= number of aligned sequences).
  Parameter     :
    n           : Number of samples. Zero is valid - in this case the
                  correction factor is zero as well.
    asize       : Size of the alphabet.
  Return Values :
    function    : Returns the error correction factor.
  Example       :
    double e = LS_calc_e(0,4);
*/
static double LS_calc_e(int n, int asize) {
  if (n == 0) return 0.0;
  return (asize - 1) / (2.0 * log(2.0) * n);
}

/*
  Description   : Calculate the information content for a matrix row,
                  taking the error correction factor into account.
  Parameter     :
    row         : Matrix row. First row is zero.
    n           : Number of samples. Zero is valid - in this case the
                  correction factor is zero as well.
    mat         : PWM.
    asize       : Size of the alphabet. The number of matrix columns must be
                  greater than or equal to asize.
    ssc         : Use small sample correction
  Return Values :
    function    : Return the information content for a matrix row
                  (= position within the logo).
  Example       :
    double R = LS_calc_R(0,0,mat,4,TRUE);
*/
static double LS_calc_R(ARRAY_T* items, int asize, int n, bool ssc) {
  return(LS_calc_Hmax(asize) - 
    (LS_calc_H(items, asize) + (ssc ? LS_calc_e(n, asize) : 0 ))
  );
}


/*
  Description   : A comparator function to sort the letter stack according
                  to letter size (smallest to largest) or alphabetical order
                  when they are of equal height.
  Parameter     :
    lstack1     : Reference to a first stack element.
    lstack2     : Reference to a second stack element.
  Return Values :
    function    : +1 if height of first stack element is greater than
                  height of second stack element, and -1 otherwise.
*/
static int LS_comp(const void *lstack1, const void *lstack2) {
  LSTACK_T *ls1, *ls2;
  ls1 = (LSTACK_T*)lstack1;
  ls2 = (LSTACK_T*)lstack2;
  if (ls1->height == ls2->height) {
    return (ls1->letter < ls2->letter) ? -1 : 1;
  } else if (ls1->height < ls2->height) {
    return -1;
  }
  return 1;
}

/*
  Description   : Fills the letter stack (= calculates the height of
                  stack letters and sorts the stack according to
                  height).
  Parameter     :
    row         : Motif matrix row.
    alph        : Motif alphabet.
    n           : Number of samples. Zero is valid - in this case the
                  correction factor is zero as well.
    ssc         : use small sample correction
    lstack      : Letter stack to store the result. Must be of appropriate size!

  Return Values :
      lstack    : The content of the letter stack is filled.
*/
static void LS_calc(ARRAY_T* items, ALPH_T alph, size_t n, bool ssc, LSTACK_T* lstack) {
  int asize, col;
  double sum, stack_height;
  asize = alph_size(alph, ALPH_SIZE);
  // sanity check
  if (asize > items->num_items)E_system("Too few columns! Columns=%d, Alphabet=%s\n", 
      items->num_items, alph_name(alph));
  for (col = 0, sum = 0; col < asize; col++) sum += get_array_item(col, items);
  if ((sum > 1.0 ? sum - 1.0 : 1.0 - sum) > 0.1) E_system("Motif probabilities do not sum to 1.");
  // Over all letters of the alphabet
  stack_height = LS_calc_R(items, asize, n, ssc);
  for (col = 0; col < asize; col++) {
    lstack[col].letter = alph_char(alph, col);
    lstack[col].height = get_array_item(col, items) * stack_height;
  }
  // Sort according to height
  qsort(lstack, asize, sizeof(LSTACK_T), LS_comp);
}

/* ------------------------- EPS Output Functions ------------------------- */
/*
  Description   : Getter for as string with color definitions.
*/
static char* EPS_colordef() {
  char* colordef =
    "/black [0 0 0] def\n"
    "/red [0.8 0 0] def\n"
    "/green [0 0.5 0] def\n"
    "/blue [0 0 0.8] def\n"
    "/yellow [1 1 0] def\n"
    "/purple [0.8 0 0.8] def\n"
    "/magenta [1.0 0 1.0] def\n"
    "/cyan [0 1.0 1.0] def\n"
    "/pink [1.0 0.8 0.8] def\n"
    "/turquoise [0.2 0.9 0.8] def\n"
    "/orange [1 0.7 0] def\n"
    "/lightred [0.8 0.56 0.56] def\n"
    "/lightgreen [0.35 0.5 0.35] def\n"
    "/lightblue [0.56 0.56 0.8] def\n"
    "/lightyellow [1 1 0.71] def\n"
    "/lightpurple [0.8 0.56 0.8] def\n"
    "/lightmagenta [1.0 0.7 1.0] def\n"
    "/lightcyan [0.7 1.0 1.0] def\n"
    "/lightpink [1.0 0.9 0.9] def\n"
    "/lightturquoise [0.81 0.9 0.89] def\n"
    "/lightorange [1 0.91 0.7] def\n";
  return(colordef);
}

/*
  Description   : Getter for string with a color dictionary for DNA symbols.
*/
static char* EPS_DNAcolordict() {
  char* colordict =
    "/fullColourDict <<\n"
    " (G)  orange\n"
    " (T)  green\n"
    " (C)  blue\n"
    " (A)  red\n"
    " (U)  green\n"
    ">> def\n"
    "/mutedColourDict <<\n"
    " (G)  lightorange\n"
    " (T)  lightgreen\n"
    " (C)  lightblue\n"
    " (A)  lightred\n"
    " (U)  lightgreen\n"
    ">> def\n"
    "/colorDict fullColourDict def\n";
  return(colordict);
}

/*
  Description   : Getter for as string with a color dictionary for amino acid
                  symbols.
*/
static char* EPS_AAcolordict() {
  char* colordict =
    "/fullColourDict <<\n"
    " (A)  blue\n"
    " (C)  blue\n"
    " (F)  blue\n"
    " (I)  blue\n"
    " (L)  blue\n"
    " (V)  blue\n"
    " (W)  blue\n"
    " (M)  blue\n"
    " (N)  green\n"
    " (Q)  green\n"
    " (S)  green\n"
    " (T)  green\n"
    " (D)  magenta\n"
    " (E)  magenta\n"
    " (K)  red\n"
    " (R)  red\n"
    " (H)  pink\n"
    " (G)  orange\n"
    " (P)  yellow\n"
    " (Y)  turquoise\n"
    ">> def\n"
    "/mutedColourDict <<\n"
    " (A)  lightblue\n"
    " (C)  lightblue\n"
    " (F)  lightblue\n"
    " (I)  lightblue\n"
    " (L)  lightblue\n"
    " (V)  lightblue\n"
    " (W)  lightblue\n"
    " (M)  lightblue\n"
    " (N)  lightgreen\n"
    " (Q)  lightgreen\n"
    " (S)  lightgreen\n"
    " (T)  lightgreen\n"
    " (D)  lightmagenta\n"
    " (E)  lightmagenta\n"
    " (K)  lightred\n"
    " (R)  lightred\n"
    " (H)  lightpink\n"
    " (G)  lightorange\n"
    " (P)  lightyellow\n"
    " (Y)  lightturquoise\n"
    ">> def\n"
    "/colorDict fullColourDict def\n";
  return(colordict);
}

/*
  Description   : Creates the string that actualy encodes the motif logo.
  Parameter     :
    params      : Parameter set.
*/
static char* EPS_data_token(LP_PARAMS_T* params) {
  int lnr, pos, stack_pos, asize, num_rows, shift;
  LP_LINE_T *line;
  LSTACK_T *lstack;
  STR_T *data;
  data = str_create(0);
  asize = alph_size(params->alphabet, ALPH_SIZE);
  lstack = LS_create(asize);
  for (lnr = 0; lnr < params->l_num; lnr++) {
    line = LP_get_line(params, lnr);
    num_rows = get_num_rows(line->mat);
    str_append2(data, "\nStartLine\n");
    shift = line->shift + params->shift;
    for (pos = 0; pos < shift; pos++) str_append2(data, "() startstack\nendstack\n\n");
    if (line->ltrim > 0) { // mute the colour to indicate trimming
      str_appendf(data, "MuteColour\nDrawTrimEdge\n%d DrawTrimBg\n", line->ltrim);
    }
    for (pos = 0; pos < num_rows; pos++) {
      if (pos == line->ltrim && pos != 0) { // enable full colour
        str_append2(data, "DrawTrimEdge\nRestoreColour\n");
      } else if (pos == (num_rows - line->rtrim)) {// mute the colour
        str_appendf(data, "MuteColour\n%d DrawTrimBg\n", line->rtrim);
      }
      str_appendf(data, "(%d) startstack\n", pos+1);
      LS_calc(get_matrix_row(pos, line->mat), params->alphabet, line->snum, params->ssc, lstack);
      for (stack_pos = 0; stack_pos < asize; stack_pos++) {
        if (lstack[stack_pos].height <= 0) continue;
        str_appendf(data, " %f (%c) numchar\n", lstack[stack_pos].height, lstack[stack_pos].letter);
      }
      if (params->show_error_bar) str_appendf(data, " %f Ibeam\n", LS_calc_e(line->snum, asize));
      str_append2(data, "endstack\n\n");
    }
    if (line->rtrim > 0 || line->ltrim == num_rows) { // enable full colour
      str_append2(data, "RestoreColour\n");
    }
    str_append2(data, "EndLine\n");
  }
  LS_free(lstack);
  return str_destroy(data, TRUE);
}

/*
  Description   : Calculates the maximum width of the logos.
  Parameter     :
    params      : Parameter set.
*/
static int EPS_stacks_per_line(LP_PARAMS_T* params) {
  int max, lnr, count;
  LP_LINE_T *line;
  max = 0;
  for (lnr = 0; lnr < params->l_num; lnr++) {
    line = LP_get_line(params, lnr);
    count = get_num_rows(line->mat) + line->shift + params->shift;
    if(count > max) max = count;
  }
  return max;
}


/*
  Description   : Creates a map of strings containing settings for changable
                  parts of the motif logos.
  Parameter     :
    params      : Parameter set.
*/
static RBTREE_T* EPS_tokens(LP_PARAMS_T* params) {
  char buffer[50];
  RBTREE_T *tokens;
  double c, width, height;
  int stacks;
  // setup the token map
  tokens = rbtree_create(rbtree_strcmp, NULL, NULL, NULL, NULL);
  // add the data token
  rbtree_put(tokens, "$DATA", EPS_data_token(params));
  // enable value copying
  rbtree_alter_value_copy(tokens, rbtree_strcpy);
  rbtree_alter_value_free(tokens, free);

  // calculate dimension tokens
  c = 72/2.54; // conversion cm to pts
  stacks = EPS_stacks_per_line(params);
  if (params->logo_width > 0) {
    width = params->logo_width;
  } else {
    width = stacks + 2;
    if (width > MAXLOGOWIDTH) width = MAXLOGOWIDTH;
  }
  height = (params->logo_height > 0 ? params->logo_height : LOGOHEIGHT * params->l_num);
  rbtree_put(tokens, "$LOGOHEIGHT", STR_dbl(height));
  rbtree_put(tokens, "$LOGOWIDTH", STR_dbl(width));
  rbtree_put(tokens, "$BOUNDINGHEIGHT", STR_int((int)(height * c)));
  rbtree_put(tokens, "$BOUNDINGWIDTH", STR_int((int)(width * c)));
  // the EPS should probably calculate this as it does for the stack width
  rbtree_put(tokens, "$LOGOLINEHEIGHT", STR_dbl(height / params->l_num));
  // Letter stacks per line
  rbtree_put(tokens, "$CHARSPERLINE", STR_int(stacks));

  // calculate alphabet specific tokens
  if (params->alphabet == DNA_ALPH) {
    rbtree_put(tokens, "$BARBITS", "2.0");
    rbtree_put(tokens, "$LOGOTYPE", "NA");
    rbtree_put(tokens, "$COLORDICT", EPS_DNAcolordict());
  } else if (params->alphabet == PROTEIN_ALPH) {
    rbtree_put(tokens, "$BARBITS", "4.3");
    rbtree_put(tokens, "$LOGOTYPE", "AA");
    rbtree_put(tokens, "$COLORDICT", EPS_AAcolordict());
  }

  // Date of creation
  rbtree_put(tokens, "$CREATIONDATE", STR_time("%d.%m.%y %H:%M:%S"));

  // other configurable parameters
  // Length of error bars [0..1]
  rbtree_put(tokens, "$ERRORBARFRACTION", (params->error_bar_fraction > 0 ? STR_dbl(params->error_bar_fraction) : "1.0"));
  // Tics on y-axis (EPS only)
  rbtree_put(tokens, "$TICBITS", (params->tic_bits > 0 ? STR_dbl(params->tic_bits) : "1"));
  // Label for title
  rbtree_put(tokens, "$TITLE", (params->title != NULL ? params->title : ""));
  // Fine print, any text you like
  rbtree_put(tokens, "$FINEPRINT", (params->fineprint != NULL ? params->fineprint : STR_time("Ceqlogo %d.%m.%y %H:%M")));
  // Label for x-axis
  rbtree_put(tokens, "$XAXISLABEL", (params->x_axis_label != NULL ? params->x_axis_label : ""));
  // Label for y-axis
  rbtree_put(tokens, "$YAXISLABEL", (params->y_axis_label != NULL ? params->y_axis_label : "bits"));
  // Toggles small sample correction : "true" or "false"
  rbtree_put(tokens, "$SSC", STR_bool(params->ssc));
  // Toggles y-axis: "true" or "false"
  rbtree_put(tokens, "$YAXIS", STR_bool(params->show_y_axis));
  // Toggles end descr. of x-axis:  "true" or "false"
  rbtree_put(tokens, "$SHOWENDS", STR_bool(params->show_ends));
  // Toggles error bars:  "true" or "false" (EPS only)
  rbtree_put(tokens, "$ERRBAR", STR_bool(params->show_error_bar));
  // Toggles outlining of logo letters:  "true" or "false"
  rbtree_put(tokens, "$OUTLINE", STR_bool(params->show_outline));
  // Toggles numbering of x-axis:  "true" or "false"
  rbtree_put(tokens, "$NUMBERING", STR_bool(params->show_numbering));
  // Toggles box around logo letters:  "true" or "false"
  rbtree_put(tokens, "$SHOWINGBOX", STR_bool(params->show_box));
  
  // These tokens have fixed values
  // Description string of output creator        
  rbtree_put(tokens, "$CREATOR", "Ceqlogo");
  // Font size for labels (in pts)
  rbtree_put(tokens, "$FONTSIZE", "12");
  // Font size for title (in pts)
  rbtree_put(tokens, "$TITLEFONTSIZE", "12");
  // Font size for fineprint (in pts)
  rbtree_put(tokens, "$SMALLFONTSIZE", "6");
  // Top margin in cm
  rbtree_put(tokens, "$TOPMARGIN", "0.9");
  // Bottom margin in cm
  rbtree_put(tokens, "$BOTTOMMARGIN", "0.9");
  // Colordefintion table (only EPS)
  rbtree_put(tokens, "$COLORDEF", EPS_colordef());

  return tokens;
}

/*
  Description   : Writes the logo described by the given parameter set
                  in EPS format to the given file.
  Parameter     :
    params      : Parameter set.
    fout        : File pointer to open output file.
*/
static void EPS_output2(LP_PARAMS_T* params, FILE *fout) {
  char *template;
  FILE *fin;
  RBTREE_T *tokens;
  tokens = EPS_tokens(params);
  template = make_path_to_file(get_meme_etc_dir(), TEMPLATE_EPS);
  fin  = F_open(template, "r");
  F_token_replace(tokens, fin, fout, 1000);
  F_close(fin);
  free(template);
  rbtree_destroy(tokens);
}

/*
  Description   : Writes the logo described by the given parameter set
                  in EPS format to the given output path.
  Parameter     :
    params      : Parameter set.
    outpath     : Path to write the file to.
*/
static void EPS_output(LP_PARAMS_T* params, const char *outpath) {
  FILE *fout;
  fout = outpath == NULL ? stdout : F_open(outpath, "w");
  EPS_output2(params, fout);
  if (fout != stdout) F_close(fout);
}

/*
  Description   : Writes the logo described by the given parameter set
                  in PNG format to the given output path.
  Parameter     :
    params      : Parameter set.
    outpath     : Path to write the file to.
*/
static void PNG_output(LP_PARAMS_T* params, const char *outpath) {
  FILE *file;
  char *temp_eps_path, *temp_png_path;
  // make a temporary file to save the EPS
  temp_eps_path = make_path_to_file(get_meme_temp_dir(), "temp_eps_XXXXXX");
  file = F_temp(temp_eps_path, "w");
  // output the EPS to the temporary file
  EPS_output2(params, file);
  // close the temporary file to ensure content is flushed
  F_close(file);
  // write the png
  if (outpath == NULL) {
    // create a temporary file for the PNG
    temp_png_path = make_path_to_file(get_meme_temp_dir(), "temp_png_XXXXXX");
    file = F_temp(temp_png_path, "w");
    F_close(file);
    // make the PNG
    eps_to_png2(temp_eps_path, temp_png_path);
    // output the PNG
    F_copy_to_stdout(temp_png_path);
    // delete the temporary PNG
    if (unlink(temp_png_path)) {
      fprintf(stderr, "Warning: failed to delete temporary PNG: %s\n", strerror(errno));
    }
    free(temp_png_path);
  } else {
    // make the png
    eps_to_png2(temp_eps_path, outpath);
  }
  // delete the temporary EPS
  if (unlink(temp_eps_path)) {
    fprintf(stderr, "Warning: failed to delete temporary PNG: %s\n", strerror(errno));
  }
  free(temp_eps_path);
}

/* ------------------------- CeqLogo Functions ------------------------- */

/*
  Description   : Adds a motif and therefore an additional line to the logo.
                  Multiple calls of add_motifs() are allowed.

  Parameter     :
    params      : Parameter set.
    motif       : Motif to add. The number of matrix columns of the contained
                  PWM must be greater than or equal to the size of the alphabet.
    shift       : Shifting of the logo. Can be negative.
    label       : label for the logo line. Can be NULL.

  Return Values :
    params      : Some parameters of the set will be modified.
*/
static void add_motif(LP_PARAMS_T* params, MOTIF_T *motif,
    bool reverse_complement, int shift, int samples, char* label) {
  assert(get_motif_alph(motif) == DNA_ALPH || !(reverse_complement));
  if (reverse_complement) reverse_complement_motif(motif);
  LP_add_line(params, get_motif_freqs(motif), 
      samples > 0 ? samples : get_motif_nsites(motif), 
      get_motif_trim_left(motif), get_motif_trim_right(motif), 
      get_motif_alph(motif), shift, label);
}

/*
  Description   : A convenience function that creates an EPS and a PNG output
                  file with one or two logos.
                  The function performs a system call to "convert"!
  Parameter     :
    motif1      : First motif. Can be NULL.
    label1      : Label of the first motif (= logo title). Can be NULL.
    motif2      : Second motif. Can be NULL.
    label2      : Label of the second motif (= label for x-axis). Can be NULL.
    errbars     : print error bars if true
    ssc         : use small sample correction if true
    height      : height of logo in cm.; use default if 0
    width       : width of logo in cm.; use default if 0
    shift       : Shift of the first logo relative to the second logo. Can be 0.
    program     : name of program to print in fineprint in lower left 
    path        : Path for the output file WITHOUT an extension.
                  Since an EPS and a PNG file are created, the path should
                  contain only the output folder and the logo name but no
                  file extension. See example.
    make_eps    : make a EPS logo
    make_png    : make a PNG logo
  Global Var.   : Uses GS_PATH 

  Example       :
    CL_create2(motif1, "Motif", NULL, NULL, FALSE, FALSE, 0, "MEME (no SSC)", 
        "myfolder/logo", TRUE, TRUE);
*/
void CL_create2(MOTIF_T *motif1, char* label1, MOTIF_T *motif2, char* label2,
    bool errbars, bool ssc, double height, double width, int shift, 
    char* program, char* path, bool make_eps, bool make_png) {
  STR_T *eps_path, *fineprint;
  LP_PARAMS_T *params;
  // check that we have something to do
  if (!(make_eps || make_png)) return; // nothing to do
  // create some strings
  eps_path = str_create(0);
  str_appendf(eps_path, "%s.eps", path);
  fineprint = str_create(0);
  str_appendf(fineprint, "%s %s", program, STR_time("%d.%m.%y %H:%M"));
  // create logo parameters
  params = LP_create();
  // configure the logo
  LP_set_title(params, label1);
  LP_set_x_axis_label(params, label2);
  LP_set_fineprint(params, str_internal(fineprint));
  params->show_error_bar = errbars;
  params->ssc = ssc;
  if (height) params->logo_height = height;
  if (width) params->logo_width = width;
  add_motif(params, motif1, FALSE, shift, 0, label1);
  if (motif2 != NULL) add_motif(params, motif2, FALSE, 0, 0, label2);
  // Make the EPS format LOGO
  EPS_output(params, str_internal(eps_path));
  // cleanup logo parameters
  LP_free(params);
  // Create PNG format of LOGO if possible and requested
  if (make_png) eps_to_png(path);
  // Delete the EPS logo if requested
  if (!make_eps) { 
    if (unlink(str_internal(eps_path))) {
      fprintf(stderr, "Warning: failed to delete EPS file after "
          "creating PNG: %s\n", strerror(errno));
    }
  }
  // cleanup some strings
  str_destroy(fineprint, FALSE);
  str_destroy(eps_path, FALSE);
}

/*
  Description   : A convenience function that creates an EPS and a PNG output
                  file with one logo.
  Parameter     :
    motif       : Motif
    errbars     : print error bars if true
    ssc         : use small sample correction if true
    program     : name of program to print in fineprint in lower left 
    path        : Path for the output file WITHOUT an extension.
                  Since an EPS and a PNG file can be created, the path should
                  contain only the output folder and the logo name but no
                  file extension. See example.
    make_eps    : Make a EPS file.
    make_png    : Make a PNG file (will make a temporary EPS).

  Example       :
    CL_create1(motif, FALSE, FALSE, "MEME (no ssc)", "myfolder/logo", TRUE, TRUE);
*/
void CL_create1(MOTIF_T *motif, bool errbars, bool ssc, char* program,
    char* path, bool make_eps, bool make_png) {
  CL_create2(motif, NULL, NULL, NULL, errbars, ssc, 0.0, 0.0, 0, program, path, make_eps, make_png);
}

/* ------------------------------ Main --------------------------------- */
#ifdef CL_MAIN

/*
  Description   : Runs all unit tests of the software.
                  Prints out the results on stdout.
  Parameter     :
    argc        : Number of command line arguments.
    argv        : Array with arguments.

  Example       :
    run_test();
*/
static void run_test() {
  fprintf(stdout, "Tests are not implemented!\n");
}

/*
  Description   : Prints the usage information.
  Example       :
    usage();
*/
static void usage() {
  printf(
   "USAGE: ceqlogo -i <filename> [options]\n"
   "  1. Example: Load a motif called MA0036.1 within a MEME motif file\n"
   "     ceqlogo -i meme.motifs -m MA0036.1 -o logo.eps\n"
   "  2. Example: Load second motif from each of two files and shift the first one\n"
   "     ceqlogo -i2 meme1.motifs -s 2 -i2 meme2.motifs -o logo.png -f PNG\n"
   "\n"
   "Motif loading options:\n"
   "  -i <input filename>        Loads motifs within the file as specified by\n"
   "                              the following -m options.\n"
   "  -i<id/n> <input filename>  Loads the specified motif (n-th position or ID)\n"
   "                              within the file. Can also be combined with the\n"
   "                              -m option.\n"
   "  -m <id/n>                  Loads a motif from the file specified by the\n"
   "                              previous -i option. The motif ID or position\n"
   "                              can be used.\n"
   "  -n <sample number>         Number of samples for previously loaded motif\n"
   "                              (-m or -i).\n"
   "  -s <shift>                 Shift for previously loaded motif (-m or -i).\n"
   "  -r                         Reverse complement previously loaded motif\n"
   "                              (-m or -i).\n"
   "  -p <pseudocounts>          Pseudocounts for loaded motifs; default: 0.\n"
   "  -l                         Prefentially lookup motifs by position;\n"
   "                              default: prefer the ID.\n"
   "\n"
   "Logo options:\n"
   "  -o <output file>           Output file path. Default is stdout.\n"
   "  -f <format>                Format of output (EPS, PNG); default: EPS\n"
   "  -h <logo height>           Height of output logo in cm (real # > 0).\n"
   "  -w <logo width>            Width of output logo in cm (real # > 0).\n"
   "  -t <title label>           Label for title.\n"
   "  -d <fine print>            Descriptive fine print.\n"
   "  -x <x-axis label>          Label for x-axis.\n"
   "  -y <y-axis label>          Label for y-axis; default: \"bits\".\n"
   "  -c <tic bits>              Number of bits between tic marks.\n"
   "  -e <error bar fraction>    Fraction of error bar to show (real # > 0).\n"
   "\n"
   "Logo toggles (all uppercase) \n"
   "  -S   ...................   Turn on small sample correction.\n"
   "  -B   ...................   Turn on bar ends.\n"
   "  -E   ...................   Turn on error bars.\n"
   "  -O   ...................   Turn on outlining of characters.\n"
   "  -X   ...................   Turn on boxing of characters  \n"
   "  -N   ...................   Turn off numbering of x-axis.\n"
   "  -Y   ...................   Turn off y-axis \n"
  );
}

/*
  Description   : Prints out an error message followed by the usage description
                  and then exits with -1.
  Parameter     :
    format      : Format string like printf.
    ...         : arguments described in the format string.

  Example       :
    int errno;
    usage_error("This the error number %d\n", errno);
*/
static void usage_error(char* format, ...) {
  va_list argptr;

  va_start(argptr, format);
    vfprintf(stderr, format, argptr);
  va_end(argptr);
  fprintf(stderr, "\n");
  usage();
  exit(-1);
}

/*
  Description   : Translates a option into a number.
  Parameter     :
    opt         : The option that is being converted.
    value       : The value of the option.
    min         : The minimum allowed value of the option.
    max         : The maximum allowed value of the option.
*/
static double num_arg(const char *opt, const char *value, double min, double max) {
  double num;
  char *endptr;
  num = strtod(value, &endptr);
  // check that the full value was parsed
  if (endptr == value || *endptr != '\0') {
    usage_error("Option %s must specify a number: %s", opt, value);
  }
  // check that the number is within bounds
  if (num < min || num > max) {
    usage_error("Option %s must be between %g and %g: %s", opt, min, max, value);
  }
  return num;
}

/*
  Description   : Translates a option into a integer.
  Parameter     :
    opt         : The option that is being converted.
    value       : The value of the option.
    min         : The minimum allowed value of the option.
    max         : The maximum allowed value of the option.
*/
static int int_arg(const char *opt, const char *value, int min, int max) {
  long num;
  char *endptr;
  num = strtol(value, &endptr, 10);
  // check that the full value was parsed
  if (endptr == value || *endptr != '\0') {
    usage_error("Option %s must specify a whole number: %s", opt, value);
  }
  // check that the number is within bounds
  if (num < min || num > max) {
    usage_error("Option %s must be between %d and %d: %s", opt, min, max, value);
  }
  return (int)num;
}

/*
  Description   : Translates the arguments of a command line into
                  settings.
  Parameter     :
    o           : Settings.
    argc        : Number of arguments within the command line.
    argv        : An array that contains the arguments of the command line.
*/
static void process_options(OPTIONS_T* o, int argc, char* argv[]) {
  LP_PARAMS_T *p;
  INPUT_T *c;
  char *file;
  char *optstring = "-o:c:e:f:h:w:n:t:d:x:y:p:i::m:s:rlSBEONXY";
  struct option longopts[] = {
    {"test", no_argument, NULL, '*'},
    {NULL, 0, NULL, 0} //boundary indicator
  };
  // set option defaults
  o->output_file = NULL;
  o->format = NULL;
  o->pseudocount = 0.0;
  o->prefer_ids = TRUE;
  o->run_tests = FALSE;
  o->inputs = NULL;
  o->input_num = 0;
  o->params = LP_create();
  // parse opts
  p = o->params;
  c = NULL;
  file = NULL;
  while (1) {
    int opt = getopt_long(argc, argv, optstring, longopts, NULL);
    if (opt == -1) break;
    switch (opt) {
      case 'o': o->output_file = optarg; break; // output file
      case 'c': p->tic_bits = num_arg("-c", optarg, 0.1, 4.3); break;
      case 'e': p->error_bar_fraction = num_arg("-e", optarg, 0.01, 1.0); break;
      case 'f': o->format = optarg; break; // EPS or PNG
      case 'h': p->logo_height = num_arg("-h", optarg, 0.01, 100.0); break;
      case 'w': p->logo_width = num_arg("-w", optarg, 0.01, 100.0); break;
      case 't': LP_set_title(p, optarg); break;
      case 'd': LP_set_fineprint(p, optarg); break;
      case 'x': LP_set_x_axis_label(p, optarg); break;
      case 'y': LP_set_y_axis_label(p, optarg); break;
      case 'p': o->pseudocount = num_arg("-p", optarg, 0, INT_MAX); break;
      case 'l': o->prefer_ids = FALSE; break;
      case 'S': p->ssc = FALSE; break; // small sample correction
      case 'B': p->show_ends = TRUE; break; // show ending brackets
      case 'E': p->show_error_bar = TRUE; break; // show error bars
      case 'O': p->show_outline = TRUE; break; // outline characters
      case 'N': p->show_numbering = FALSE; break; // disable numbering of x-axis
      case 'X': p->show_box = TRUE; break; // box characters
      case 'Y': p->show_y_axis = FALSE; break; // show the y-axis
      case '*': o->run_tests = TRUE; break; // run tests
      case '?': // unknown option, getopt will report error message
        usage();
        exit(1);
        break;
      case 1: // untagged
        usage_error("Unexpected argument (did you forget an option?): %s", argv[optind-1]);
        break;
      // Motif loading options
      case 'i':
        // check that a motif filename was actually provided
        if (optind >= argc) usage_error("Option -i must specify a motif filename");
        file = argv[optind++];
        if (optarg != NULL) {
          // add a new input
          Resize(o->inputs, ++(o->input_num), INPUT_T);
          c = o->inputs+(o->input_num - 1);
          c->id = optarg;
          c->file = file;
          c->samples = 0;
          c->shift = 0;
          c->rc = false;
          c->motif_by_id = NULL; c->motif_by_pos = NULL;
        }
        break;
      case 'm': // load a specified motif from the previously specified -i
        if (file == NULL) usage_error("Option -m may only appear after a -i option");
        // add a new input
        Resize(o->inputs, ++(o->input_num), INPUT_T);
        c = o->inputs+(o->input_num - 1);
        c->id = optarg;
        c->file = file;
        c->samples = 0;
        c->shift = 0;
        c->rc = false;
        c->motif_by_id = NULL; c->motif_by_pos = NULL;
        break;
      case 'n':
        if (c == NULL) usage_error("Option -n may only appear after a \"-i<id>\" or \"-m <id>\" option");
        c->samples = int_arg("-n", optarg, 1, INT_MAX);
        break;
      case 's':
        if (c == NULL) usage_error("Option -s may only appear after a \"-i<id>\" or \"-m <id>\" option");
        c->shift = int_arg("-s", optarg, INT_MIN, INT_MAX);
        break;
      case 'r':
        if (c == NULL) usage_error("Option -r may only appear after a \"-i<id>\" or \"-m <id>\" option");
        c->rc = true;
        break;
    }
  }
  if (o->input_num == 0) usage_error("No motifs were specified");
}

/*
  Description   : Frees memory allocated to store program settings.
  Parameter     :
    options     : Program settings.
 */
static void cleanup(OPTIONS_T* options) {
  int i;
  INPUT_T *input;
  for (i = 0; i < options->input_num; i++) {
    input = options->inputs+i;
    if (input->motif_by_id != NULL) destroy_motif(input->motif_by_id);
    if (input->motif_by_pos != NULL) destroy_motif(input->motif_by_pos);
    memset(input, 0, sizeof(INPUT_T));
  }
  if (options->inputs != NULL) free(options->inputs);
  LP_free(options->params);
  memset(options, 0, sizeof(OPTIONS_T));
}


void free_rbtree(void* ptr) {
  rbtree_destroy((RBTREE_T*)ptr);
}

/*
  Description   : Loads motifs from a MEME files
  Parameter     :
    pseudocount : The pseudocount to be added to the motifs.
    input_num   : The number of motif inputs.
    input       : The motif inputs.

  Return Values :
    params      : The motif inputs will have the motif_by_x fields filled in.
*/
static void load_all_motifs(double pseudocount, int input_num, INPUT_T* inputs) {
  char numstr[20];
  RBTREE_T *files, *ids;
  RBNODE_T *file;
  char *filename;
  MOTIF_T *motif;
  MREAD_T *mread;
  INPUT_T *input;
  ALPH_T alphabet;
  bool created;
  int i;
  // first create an index of all the files we need to load
  files = rbtree_create(rbtree_strcmp, NULL, NULL, NULL, free_rbtree);
  for (i = 0; i < input_num; i++) {
    file = rbtree_lookup(files, inputs[i].file, true, &created);
    // for each of the files create an index of the motif identifiers
    if (created) rbtree_set(files, file, rbtree_create(rbtree_strcmp, NULL, NULL, NULL, NULL));
    ids = (RBTREE_T*)rbtree_value(file);
    // store the first one... If there are multiple references to the same motif we will fix that later
    rbtree_make(ids, inputs[i].id, inputs+i);
  }
  // now iterate over the files
  for (file = rbtree_first(files); file != NULL; file = rbtree_next(file)) {
    filename = (char*)rbtree_key(file);
    ids = (RBTREE_T*)rbtree_value(file);
    // open the motif file to read
    mread = mread_create(filename, OPEN_MFILE);
    mread_set_pseudocount(mread, pseudocount);
    // iterate over motifs in the file
    i = 1;
    while (mread_has_motif(mread)) {
      motif = mread_next_motif(mread);
      // check if the motif id is interesting
      if ((input = (INPUT_T*)rbtree_get(ids, get_motif_id(motif))) != NULL) {
        input->motif_by_id = duplicate_motif(motif);
      }
      // check if the motif index is interesting
      snprintf(numstr, sizeof(numstr), "%d", i);
      if ((input = (INPUT_T*)rbtree_get(ids, numstr)) != NULL) {
        input->motif_by_pos = duplicate_motif(motif);
      }
      destroy_motif(motif);
      ++i;
    }
  }
  // now handle duplicate references to motifs and missing motifs
  for (i = 0; i < input_num; i++) {
    if (inputs[i].motif_by_id == NULL && inputs[i].motif_by_pos == NULL) {
      // this might be a duplicate, see if we can find it a motif
      ids = (RBTREE_T*)rbtree_get(files, inputs[i].file);
      input = (INPUT_T*)rbtree_get(ids, inputs[i].id);
      if (input == inputs+i) {
        // uhoh, we didn't find one of the requested motifs!
        usage_error("Failed to find motif %s in the file \"%s\"", inputs[i].id, inputs[i].file);
      } else {
        if (input->motif_by_id != NULL) {
          inputs[i].motif_by_id = duplicate_motif(input->motif_by_id);
        }
        if (input->motif_by_pos != NULL) {
          inputs[i].motif_by_pos = duplicate_motif(input->motif_by_pos);
        }
      }
    }
  }
  rbtree_destroy(files);
}

/*
  Description   : Run the program.
  Parameter     :
    argc        : Number of command line arguments.
    argv        : Array with arguments.
 */
VERBOSE_T verbosity = INVALID_VERBOSE;
int main(int argc, char* argv[]) {
  OPTIONS_T options;
  INPUT_T* input;
  MOTIF_T* motif;
  int i;
  process_options(&options, argc, argv);
  if (options.run_tests) {
    run_test();
  } else {
    load_all_motifs(options.pseudocount, options.input_num, options.inputs);
    for (i = 0; i < options.input_num; i++) {
      input = options.inputs+i;
      if (options.prefer_ids) {
        motif = (input->motif_by_id != NULL ? input->motif_by_id : input->motif_by_pos);
      } else {
        motif = (input->motif_by_pos != NULL ? input->motif_by_pos : input->motif_by_id);
      }
      add_motif(options.params, motif, input->rc, input->shift, input->samples, NULL);
    }
    if (options.format == NULL || strcmp(options.format, "PNG") != 0) {
      EPS_output(options.params, options.output_file);
    } else {
      PNG_output(options.params, options.output_file);
    }
  }
  cleanup(&options);
  return(0);
}

#endif
