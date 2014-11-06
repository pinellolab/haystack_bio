
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include "config.h"
#include "eps2png.h"
#include "string-builder.h"

// Flags to pass to Ghostscript
#define GHOSTSCRIPT_FLAGS " -q -r100 -dSAFER -dBATCH -dNOPAUSE " \
  "-dDOINTERPOLATE -sDEVICE=pngalpha -dBackgroundColor=16#FFFFFF " \
  "-dTextAlphaBits=4 -dGraphicsAlphaBits=4 -dEPSCrop -sOutputFile="

// Flags to pass to convert when a modern version of Ghostscript is unavaliable
#define CONVERT_FLAGS ""

static inline BOOLEAN_T is_exe(char *path) {
  struct stat file_status;
  if (stat(path, &file_status) != 0) return FALSE;
  if (!(S_ISREG(file_status.st_mode))) return FALSE;
  if (access(path, X_OK) != 0) return FALSE;
  return TRUE;
}

/**************************************************************************
 * Get the version of Ghostscript 
 **************************************************************************/
static float ghostscript_version() {
  static float version = 0;
  if (version != 0.0f) return version;
  STR_T *cmd;
  FILE *pipe;
  if (GHOSTSCRIPT_PATH == NULL || GHOSTSCRIPT_PATH[0] == '\0') {
    version = -1;
  } else if (!is_exe(GHOSTSCRIPT_PATH)) {
    fprintf(stderr, "The given ghostscript \"%s\" is not an executable file.\n",
        GHOSTSCRIPT_PATH);
    version = -1;
  } else {
    cmd = str_create(0);
    str_appendf(cmd, "%s --version", GHOSTSCRIPT_PATH);
    pipe = popen(str_internal(cmd), "r");
    str_destroy(cmd, FALSE);
    if (fscanf(pipe, "%f", &version) != 1) {
      fputs("Ghostscript did not return a number when the version was "
          "queried.\n", stderr);
      version = -1;
    }
    pclose(pipe);
  }
  return version;
}

/**************************************************************************
 * Check if convert is avaliable
 **************************************************************************/
static BOOLEAN_T convert_avaliable() {
  static BOOLEAN_T checked = FALSE;
  static BOOLEAN_T avaliable = FALSE;
  if (checked) return avaliable;
  if (CONVERT_PATH == NULL || CONVERT_PATH[0] == '\0') {
    avaliable = FALSE;
  } else if (!is_exe(CONVERT_PATH)) {
    fprintf(stderr, "The given convert \"%s\" is not an executable file.\n",
        CONVERT_PATH);
    avaliable = FALSE;
  } else {
    avaliable = TRUE;
  }
  checked = TRUE;
  return avaliable;
}

/**************************************************************************
 * Check if converting an eps into a png is plausible
 **************************************************************************/
BOOLEAN_T eps_to_png_possible() {
  return (ghostscript_version() > 9 || convert_avaliable());
}

/**************************************************************************
 * Convert a eps file to a png file.
 **************************************************************************/
void eps_to_png2(const char *eps_path, const char *png_path) {
  STR_T *cmd;
  int ret;
  cmd = str_create(0);
  if (ghostscript_version() > 9) {
    str_appendf(cmd, "%s%s%s %s", GHOSTSCRIPT_PATH, GHOSTSCRIPT_FLAGS,
        png_path, eps_path);

    ret = system(str_internal(cmd));

    if (!(WIFEXITED(ret) && WEXITSTATUS(ret) == 0)) {
      fprintf(stderr, "Warning: conversion to png format using Ghostscript "
          "failed.\n");
    }
  } else if (convert_avaliable()) {
    str_appendf(cmd, "%s%s %s %s", CONVERT_PATH, CONVERT_FLAGS,
        eps_path, png_path);

    ret = system(str_internal(cmd));

    if (!(WIFEXITED(ret) && WEXITSTATUS(ret) == 0)) {
      fprintf(stderr, "Warning: conversion to png format using Image "
          "Magick's convert failed.\n");
    }
  } else {
    fprintf(stderr, "Warning: Can not convert EPS file to PNG as no install "
        "of Image Magick or Ghostscript is usable.\n");
  }
  str_destroy(cmd, FALSE);
}

/**************************************************************************
 * Convert an EPS to a PNG. Assumes you have an EPS file at path.eps and
 * want a png file at path.png
 **************************************************************************/
void eps_to_png(char *path) {
  STR_T *eps_path, *png_path;
  eps_path = str_create(0); str_appendf(eps_path, "%s.eps", path);
  png_path = str_create(0); str_appendf(png_path, "%s.png", path);
  eps_to_png2(str_internal(eps_path), str_internal(png_path));
  str_destroy(eps_path, FALSE);
  str_destroy(png_path, FALSE);
}
