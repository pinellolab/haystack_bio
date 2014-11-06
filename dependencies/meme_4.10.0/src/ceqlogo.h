/* ---------------------------- Header ---------------------------------

  Module name   : ceqlogo.h

  Description: Create logos for a given set of motifs/PWMs.

  Author   : S. Maetschke

  Copyright: Institute for Molecular Bioscience (IMB)

------------------------------------------------------------------------ */

#ifndef __CEQLOGO
#define __CEQLOGO

#include <stdbool.h>

#include "motif.h"
#include "utils.h"

/* ----------------------- Global Prototypes --------------------------- */

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
void CL_create2(
  MOTIF_T *motif1,
  char* label1,
  MOTIF_T *motif2,
  char* label2,
  bool errbars,                    // use errorbars
  bool ssc,                        // use small sample correction
  double height,
  double width,
  int shift,
  char* program,
  char* path,
  bool make_eps,
  bool make_png
);

/*
  Description   : A convenience function that creates an EPS and a PNG output
                  file with one logo.
                  The function performs a system call to "gs" (ghostscript)!
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
  Global Var.   : Uses GS_PATH

  Example       :
    CL_create1(motif, FALSE, FALSE, "MEME (no ssc)", "myfolder/logo", TRUE, TRUE);
*/
void CL_create1(
  MOTIF_T *motif, 
  bool errbars,
  bool ssc,
  char* program,
  char* path,
  bool make_eps,
  bool make_png
); 

#endif
