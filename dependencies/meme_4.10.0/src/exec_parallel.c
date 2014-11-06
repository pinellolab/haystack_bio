/***********************************************************************
*                                                                      *
* MEME                                                                 *
* Copyright 1994-2013, The Regents of the University of California     *
* and The University of Queensland.
* Author: Timothy L. Bailey                                            *
*                                                                      *
***********************************************************************/
/* exec_parallel.c */

#include "meme.h"
#include "utils.h"
#include <sys/param.h>
#include <errno.h>

#define xstr(s) str(s)
#define str(s) #s

/*
  Get the real path of the current executable.
*/
char *get_exe_path(char *argv0) {
  char *path = NULL;
  size_t size = 0;

#ifdef _WIN64
  printf("Windows 64-bit is not supported in exec_parallel.c\n");
  exit(1);
#elif _WIN32
  printf("Windows 32-bit is not supported in exec_parallel.c\n");
  exit(1);
#elif __APPLE__ || __linux || __unix || __posix
  //printf("I'm Apple\n");
  //(void) _NSGetExecutablePath(NULL, &size);
  //path = (void *) malloc(size);
  //if (_NSGetExecutablePath(path, &size) != 0) path = NULL;
  //printf("I'm Linux\n");
  //#include<unistd.h>
  //size_t bufsize = 1;
  //while (size <= bufsize+1) {
  //  size += 1024;
  //  if (path) free(path);
  //  path = (void *) malloc(size);
  //  bufsize = readlink("/proc/self/exe", path, size-1);
  //  if (bufsize == -1) { path = NULL; break;}
  //  path[bufsize] = '\0';
  // }
  int cmd_len = strlen(argv0) + 6;
  char *cmd = (void *) malloc(cmd_len+1);
  sprintf(cmd, "which %s", argv0);
  FILE *file = popen(cmd, "r");
  size = 1024;
  path = (void *) malloc(size);
  path = fgets(path, size, file);
  if (path) path[strlen(path)-1] = '\0'; // Remove the \n
  pclose(file);
#else
  fputs("Not sure how to find path to current executable in exec_parallel.c. Using argv[0]...\n");
  path = argv0;				// Might work sometimes!
#endif
  //fputs("Executable path is: ", stderr); fputs(path, stderr); fputs("\n", stderr);

  // Now resolve links--probably isn't really necessary.
  char *resolved_path = malloc(PATH_MAX*sizeof(char)+1);
  if (realpath(path, resolved_path) == 0) { 
    fputs("realpath failed in exec_parallel.c\n", stderr);
    exit(1);
  } else {
    return(resolved_path);
  }

} // get_exe_path


/*
   Check arguments to see if -p <np> was given.
   If so, use execvp() to run program as input to MPI.
   The particular MPI command is set in the macro MPI_CMD and
   must end just before the number of processors is to be
   specified (<np>).
*/
int exec_parallel(
  int argc,
  char *argv[]  
) {
  int i;

  // Get the real path to the current executable.
  char *resolved_path = get_exe_path(argv[0]);
  // don't use printf here!
  //fputs("argv: \n",stderr); for(i=0;i<argc;i++){fputs(argv[i], stderr); fputs("\n", stderr);}
  //fputs("Program called as: ", stderr); fputs(argv[0], stderr); fputs("\n", stderr); 
  //fputs("Real path is: ", stderr); fputs(resolved_path, stderr); fputs("\n", stderr); 

  // Scan args for -p so we can launch using MPI.
  char **args = (char **)calloc(argc, sizeof(char *));	// args to MPI run command
  char *np = NULL;
  int iargs = 0;
  for (i=0; i<argc; i++) {
    // Scan for -p and remove from args, and save np. 
    // If more than one -p given, last one counts.
    if (strcmp(argv[i], "-p") == 0) {
      if (++i == argc) die("Illegal use of `-p <np>': missing number of processors\n");
      np = argv[i];			// set number of processes
    } else {
      args[iargs++] = argv[i];		// add arguments (other than -p <np>) to MPI run args
    }
  }
  //fputs("np is: ", stderr); fputs(np, stderr); fputs("\n", stderr); 
 
  if (np != NULL) {
    // Convert MPI_CMD and np to a string. 
    char *mpi_cmd = (char*) malloc(strlen(xstr(MPI_CMD))+strlen(np)+2);
    sprintf(mpi_cmd, "%s %s", xstr(MPI_CMD), np);

    // Split MPI_CMD and np string on whitespace.
    if (mpi_cmd != NULL) {
      char **mpi_args = (char **)calloc(strlen(mpi_cmd), sizeof(char *));
      char *token = strtok(mpi_cmd, "\t ");	// split on whitespace
      int n = 0;				// number tokens in mpi_cmd
      while (token != NULL) {
	mpi_args[n++] = token; 
	token = strtok(NULL, "\t ");
      }

      // Increase the size of the args array and shift arguments down to make room for mpi_cmd
      int new_iargs = iargs+n;
      args = (char **) realloc(args, (new_iargs+1) * sizeof(char *));
      for (i=iargs-1; i >= 0; i--) args[n+i] = args[i];	// shift n places down
      args[n] = resolved_path;				// put real path to parallel program here
      for (i=0; i < n; i++) args[i] = mpi_args[i];	// put in mpi_cmd arguments
      args[iargs+n] = NULL;				// required by execvp
      //for(i=0;i<new_iargs;i++){fputs(args[i], stderr); fputs("\n", stderr);}
    }

    // Run via MPI.
    if (mpi_cmd != NULL) {
      execvp(args[0], args);
      return(0);
    }
  }

  return(0);
} // exec_parallel

