
#include <getopt.h>
#include <stdarg.h>
#include <stdio.h>

#include "ceqlogo.h"
#include "io.h"
#include "motif-in.h"
#include "red-black-tree.h"
#include "string-builder.h"
#include "user.h"
#include "utils.h"


#define DEFAULT_PSEUDOCOUNTS 0


VERBOSE_T verbosity = QUIET_VERBOSE;

typedef struct meme2images_options {
  BOOLEAN_T png;
  BOOLEAN_T eps;
  BOOLEAN_T rc;
  char *dir;
  char *motifs_file;
  RBTREE_T *ids;
} OPTIONS_T;

/**************************************************************************
 * Prints a usage message and exits. 
 * If given an error message it prints that first and will exit with
 * return code of EXIT_FAILURE.
 **************************************************************************/
static void usage(char *format, ...) {
  va_list argp;

  char *usage = 
    "Usage:\n" 
    "    meme2images [options] <motifs file> <output directory>\n"
    "Options:\n"
    "    -motif <ID>      output only a selected motif; repeatable\n"
    "    -eps             output logos in eps format\n"
    "    -png             output logos in png format\n"
    "    -rc              output reverse complement logos\n"
    "    -help            print this usage message\n"
    "Description:\n"
    "    Creates motif logos from any MEME or DREME motif format.\n";

  if (format) {
    va_start(argp, format);
    vfprintf(stderr, format, argp);
    va_end(argp);
    fprintf(stderr, "\n");
    fputs(usage, stderr);
    fflush(stderr);
  } else {
    puts(usage);
  }
  if (format) exit(EXIT_FAILURE);
  exit(EXIT_SUCCESS);
}

/**************************************************************************
 * Process the command line arguments and put the results in the
 * options structure.
 **************************************************************************/
void process_arguments(int argc, char **argv, OPTIONS_T *options) {
  int option_index = 1;
  BOOLEAN_T bad_argument = FALSE;

  struct option meme2images_options[] = {
    {"motif", required_argument, NULL, 'm'},
    {"eps", no_argument, NULL, 'e'},
    {"png", no_argument, NULL, 'p'},
    {"rc", no_argument, NULL, 'r'},
    {"help", no_argument, NULL, 'h'},
    {NULL, 0, NULL, 0} //boundary indicator
  };

  // set defaults
  options->png = FALSE;
  options->eps = FALSE;
  options->rc = FALSE;
  options->dir = NULL;
  options->motifs_file = NULL;
  options->ids = rbtree_create(rbtree_strcmp, NULL, NULL, NULL, NULL);

  // parse optional arguments
  while (1) {
    int opt = getopt_long_only(argc, argv, "", meme2images_options, NULL);
    if (opt == -1) break;
    switch (opt) {
      case 'm':    //-motif <ID>
        rbtree_make(options->ids, optarg, NULL);
        break;
      case 'e':    //-eps
        options->eps = TRUE;
        break;
      case 'p':    //-png
        options->png = TRUE;
        break;
      case 'r':    //-rc
        options->rc = TRUE;
        break;
      case 'h':           //-help
        usage(NULL);
        break;
      case '?':           //unrecognised or ambiguous argument
        bad_argument = TRUE;
    }
  }
  if (bad_argument) usage("One or more unknown or ambiguous options were supplied.");
  option_index = optind;

  // get the motif file
  if (option_index >= argc) usage("No motif file!");
  options->motifs_file = argv[option_index++];
  if (!file_exists(options->motifs_file)) 
    usage("Motif file \"%s\" does not exist!", options->motifs_file);

  // get the output directory
  if (option_index >= argc) usage("No output directory specified!");
  options->dir = argv[option_index++];

  // if no option is set then output both png and eps
  if (!(options->png || options->eps)) {
    options->png = TRUE;
    options->eps = TRUE;
  }
}


/**************************************************************************
 * Copy the name while replacing non-alphanumeric characters
 **************************************************************************/
void copy_and_sanatise_name(char *target, char *source, int max_len) {
  int i;
  i = 0;
  while (i < max_len && source[i] != '\0') {
    if ((source[i] >= 'a' && source[i] <= 'z') || 
        (source[i] >= 'A' && source[i] <= 'Z') || 
        (source[i] >= '0' && source[i] <= '9') ||
        source[i] == '_') {
      target[i] = source[i];
    } else {
      target[i] = '_';
    }
    ++i;
  }
  if (i < max_len) {
    target[i] = '\0';
  }
}

/**************************************************************************
 * Generate logos for a motif
 * Warning, this may modify the path and motif arguments.
 **************************************************************************/
static void generate_motif_logos(OPTIONS_T *options, STR_T *path, MOTIF_T *motif) {
  int path_len;
  char name[MAX_MOTIF_ID_LENGTH + 1];

  copy_and_sanatise_name(name, get_motif_id(motif), MAX_MOTIF_ID_LENGTH);
  name[MAX_MOTIF_ID_LENGTH] = '\0';
  path_len = str_len(path);
  
  str_appendf(path, "logo%s", name);
  CL_create1(motif, FALSE, FALSE, "MEME (no SSC)", str_internal(path), 
      options->eps, options->png);

  if (options->rc) {
    str_truncate(path, path_len);
    str_appendf(path, "logo_rc%s", name);
    reverse_complement_motif(motif);
    CL_create1(motif, FALSE, FALSE, "MEME (no SSC)", str_internal(path), 
        options->eps, options->png);
  }

  str_truncate(path, path_len);
}

/**************************************************************************
 * Generate logos for all motifs in a file
 **************************************************************************/
static void generate_file_logos(OPTIONS_T *options) {
  STR_T *path;
  MREAD_T *mread;
  MOTIF_T *motif;
  // file path buffer
  path = str_create(100);
  str_append(path, options->dir, strlen(options->dir));
  if (str_char(path, -1) != '/') str_append(path, "/", 1);
  // create output directory
  if (create_output_directory(str_internal(path), TRUE, FALSE)) 
    exit(EXIT_FAILURE);
  // open motif file
  mread = mread_create(options->motifs_file, OPEN_MFILE);
  while (mread_has_motif(mread)) {
    motif = mread_next_motif(mread);
    if (rbtree_size(options->ids) == 0 || rbtree_find(options->ids, get_motif_id(motif)) != NULL) {
      generate_motif_logos(options, path, motif);
    }
    destroy_motif(motif);
  }
  mread_destroy(mread);
  str_destroy(path, FALSE);
}

/**************************************************************************
 * Run the program
 **************************************************************************/
int main(int argc, char** argv) {
  OPTIONS_T options;
  process_arguments(argc, argv, &options);
  generate_file_logos(&options);
  rbtree_destroy(options.ids);
  return EXIT_SUCCESS;
}
