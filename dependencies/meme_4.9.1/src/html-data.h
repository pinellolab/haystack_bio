/**************************************************************************
 * FILE: html-data.h
 * AUTHOR: James Johnson 
 * CREATE DATE: 01 June 2011
 * PROJECT: shared
 * COPYRIGHT: UQ, 2009
 * VERSION: $Revision: 1.0 $
 * DESCRIPTION: A callback based push parser for html documents designed
 * to extract the <input type="hidden" ...> tags and get their name
 * and value attributes. Automatically skips the contents of comments and
 * script or style tags. Keeps a count of all valid html tags that it sees.
 **************************************************************************/

#ifndef HTML_DATA_H
#define HTML_DATA_H

typedef struct hdata HDATA_T;

/*
 * HDATA_FN
 * Callback function prototype.
 * info = used to get more information on tags seen
 * ctx = the supplied user data
 * name = the name of the input tag
 * name_overflow = the number of bytes which couldn't
 *    be included because of the maximum attribute 
 *    length restriction
 * value = the value of the input tag
 * value_overflow = the number of bytes which couldn't
 *    be included because of the maximum attribute
 *    length restriction
 */
typedef void (*HDATA_FN)(HDATA_T *info, void *data, 
    const char *name, long name_overflow, 
    const char *value, long value_overflow);

/*
 * hdata_create
 * Creates a html parser which extracts the name and value
 * attributes of input tags with type hidden.
 */
HDATA_T* hdata_create(HDATA_FN callback, void *data, size_t max_attr_len);

/*
 * hdata_destroy
 * Destroys the html parser.
 */
void hdata_destroy(HDATA_T *ctxt);

/*
 * hdata_update
 * Parses another chunk of a possible html file.
 */
void hdata_update(HDATA_T *ctxt, const char *chunk, size_t size, short end);

/*
 * Get the total number of html tags
 * seen.
 */
long hdata_total_seen(HDATA_T *ctxt);

/*
 * Get the number of times a html tag has been
 * seen. If the tag is not known then 0 will
 * be returned.
 */
long hdata_tag_seen(HDATA_T *ctxt, char *tag);

/*
 * Get the number of times an opening tag has
 * been seen.
 */
long hdata_tag_opened(HDATA_T *ctxt, char *tag);

/*
 * Get the number of times a closing tag has been
 * seen.
 */
long hdata_tag_closed(HDATA_T *ctxt, char *tag);

/*
 * Get the number of times a self-closing tag has
 * been seen.
 */
long hdata_tag_selfclosed(HDATA_T *ctxt, char *tag);

/*
 * Get the number of times a tag still needs to be closed. This
 * doesn't know about tags like img which don't have opening and
 * closing tags (unless the user is trying to write xhtml) and
 * so may report img tags as needing closing tags when they really
 * don't. If there are more closing tags then opening tags it will
 * report a negative number.
 */
long hdata_tag_toclose(HDATA_T *ctxt, char *tag);

/*
 * Get the last seen tag type. 
 * 0 = self-closed (or unseen), 1 = open tag, -1 = close tag
 */
short hdata_tag_last(HDATA_T *ctxt, char *tag);

#endif

