/*
 * FILE: html-monolith.h
 * AUTHOR: James Johnson 
 * CREATE DATE: 30-November-2011
 * PROJECT: shared
 * COPYRIGHT: UQ, 2011
 * VERSION: $Revision: 1.0 $
 * DESCRIPTION: It's easier for the user to handle one html results file than 
 * a html file, a couple of javascript files and a couple of css files. However 
 * when developing it is easier to keep the files seperate. This provides the
 * tools to merge all the disparate resources into one file as well as easily
 * generate JSON data to subsitiute for an included script file at run time.
 */

#include "json-writer.h"
#include "utils.h"

typedef struct monolith_writer HTMLWR_T;

/*
 * htmlwr_create
 * Creates a html monolith writer.
 * Takes the directory to search for sources, the name of the primary source.
 * Returns null if the primary source does not exist.
 */
HTMLWR_T* htmlwr_create(const char* search_dir, const char* primary_source);

/*
 * htmlwr_destroy
 * Destroys a html monolith writer.
 */
void htmlwr_destroy(HTMLWR_T* htmlwr);

/*
 * htmlwr_set_dest_name
 * Sets the output directory and file name.
 */
void htmlwr_set_dest_name(HTMLWR_T* htmlwr, const char* directory, const char* name);

/*
 * htmlwr_set_dest_path
 * Sets the full path to the output file.
 */
void htmlwr_set_dest_path(HTMLWR_T* htmlwr, const char* path);

/*
 * htmlwr_set_dest_file
 * Sets the output file and if it should be closed automatically.
 */
void htmlwr_set_dest_file(HTMLWR_T* htmlwr, FILE* file, BOOLEAN_T close);

/*
 * htmlwr_replace
 * Tell the html monolith writer to stop when it encounters the replaced script
 * and let the user write a Javascript Object Notation variable in its place.
 */
void htmlwr_replace(HTMLWR_T* htmlwr, const char* replaced_script, const char* variable_name);

/*
 * htmlwr_replacing
 * Returns the variable name of the json object being written.
 */
const char* htmlwr_replacing(HTMLWR_T* htmlwr);

/*
 * htmlwr_output
 * Writes until encountering the next script to be replaced with JSON or the end
 * of the primary source. Returns the JSON writer for the next replacement or
 * NULL on EOF.
 */
JSONWR_T* htmlwr_output(HTMLWR_T* htmlwr);
