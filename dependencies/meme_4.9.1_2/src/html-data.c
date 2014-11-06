/**************************************************************************
 * FILE: html-data.c
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

#include <assert.h>
#include <ctype.h>
#include <string.h>

#include "html-data.h"
#include "linked-list.h"
#include "red-black-tree.h"
#include "string-builder.h"
#include "string-match.h"
#include "utils.h"

/*
 * Possible parser states
 */
enum hdata_state {
  HDATA_READY, // looking for <
  HDATA_TAGNAME, // seen < so reading following to determine if it's a tag or comment or just stuff
  HDATA_INTAG, // seen valid html tag name, now reading attributes until end of tag
  HDATA_ATTRNAME, // reading attribute name
  HDATA_ATTRVALUE, // parse the value of a generic attribute
  HDATA_SINGLEQUOTE, // parse an attribute ending in a single quote
  HDATA_DOUBLEQUOTE, // parse an attribute ending in a double quote
  HDATA_NOQUOTE, // parse an unquoted attribute value
  HDATA_COMMENT, // after a <!-- so looking for a -->
  HDATA_SKIP // skiping content until a specific close tag
};
typedef enum hdata_state HDATA_STATE_EN;

/*
 * Possible ways of using the current attribute value
 */
enum hdata_attr {
  HDATA_IGNORE, // the current attribute can be safely ignored
  HDATA_INPUT_TYPE, // the current attribute is the type attribute of an input tag
  HDATA_INPUT_NAME, // the current attribute is the name attribute of an input tag
  HDATA_INPUT_VALUE // the current attribute is the value attribute of an input tag
};
typedef enum hdata_attr HDATA_ATTR_EN;

/*
 * Keeps track of tag sightings
 */
struct tag {
  char *name; // name of the tag
  long opened; // number of times an open tag has been seen
  long closed; // number of times a close tag has been seen
  long self_closed; // number of times a self closed tag has been seen
  short last; // 0 = self closed (or unseen), 1 = opened last, -1 = closed last
  BOOLEAN_T is_input_tag; // true if input tag
  BMSTR_T *skip; // should the content be skipped (for script and style tags)
};
typedef struct tag TAG_T;

/*
 * Keeps track of parser state
 */
struct hdata {
  HDATA_FN callback; // callback function to alert the user about a hidden input
  void *data; // data the user is using to keep track of state
  size_t max_attr_len; // maximum attribute length to store
  HDATA_STATE_EN state; // current state of parser
  HDATA_ATTR_EN attr; // current attribute
  TAG_T *tag; // current tag
  long tag_count; // total count of html tags sighted
  BOOLEAN_T leading_slash; // does the current tag have a leading slash
  BOOLEAN_T trailing_slash; // does the current tag have a trailing slash
  BOOLEAN_T type_hidden; // does the input tag have a type="hidden" attribute
  char *name; // the name of the input tag
  long name_overflow; // num of name chars discarded
  char *value; // the value of the input tag
  long value_overflow; // num of value chars discarded
  STR_T *strb; // string builder
  long overflow; // number of chars that couldn't fit in the string builder
  RBTREE_T *tags; // names of valid html tags
  BMSTR_T *comment_end; // pattern to match the end of a comment
  RBTREE_T *prior_states; // used by update to ensure no accidental infinite loops
};

// Assumptions needed by this parser
// - Assume that a <!-- means everything can be ignored until a --> is found except
//   inside quoted attributes of tags and the content of script and style tags 
//   where it will be ignored
// - Assume all <tag , where tag is a html element name, will have 
//   optional attributes and probably a > to end
// - Assume all </tag , where tag is a html element name, will probably have a 
//   > to end. 
// - Assume that a <tag or </tag within a tag but not within a quoted 
//   attribute value terminates the previous tag as if a > was present
// - Assume the optional attributes of a tag will be defined as attr="..." or
//   attr='...' or attr=...[whitespace] or attr where the first 2 forms are 
//   called quoted attributes
// - Assume the attr token does not contain > or /
// - Assume the whitespace terminated form of the attribute will not contain >
//   or /
// - Assume if the attribute has no attr= then the attributes value is its name
// - Assume the ... of the quoted attributes may be anything but the terminating
//   quote.
// - Assume that a forward slash seperated by zero or more whitespace from a >
//   means that a script or style tag has no content to ignore
// - Assume elements <script and <style will have a matching
//   > to terminate the tag because for the purpose of content we treat them 
//   like comments and ignore it.
// - Assume elements <input will probably have attributes type, name and value
//   as well as a matching > 
//

/*
 * List of html tag names with the exception of input, script and style
 * TODO add html 5 tags as well.
 */
char * tag_names[] = {
  "a", "abbr", "acronym", "address", "applet", "area", "b", "base", "basefont", 
  "bdo", "big", "blockquote", "body", "br", "button", "caption", "center", 
  "cite", "code", "col", "colgroup", "dd", "del", "dfn", "dir", "div", "dl", 
  "dt", "em", "fieldset", "font", "form", "frame", "frameset", "h1", "h2", "h3",
  "h4", "h5", "h6", "head", "hr", "html", "i", "iframe", "img", "ins", 
  "isindex", "kbd", "label", "legend", "li", "link", "map", "menu", "meta", 
  "noframes", "noscript", "object", "ol", "optgroup", "option", "p", "param", 
  "pre", "q", "s", "samp", "select", "small", "span", "strike", 
  "strong", "sub", "sup", "table", "tbody", "td", "textarea", "tfoot", 
  "th", "thead", "title", "tr", "tt", "u", "ul", "var", "xmp", "" 
};

#define MAX_TAG_LEN 20
#define MAX_ATTR_LEN 10

/*
 * Create a tag datastructure and append it to a set.
 */
static void add_tag(RBTREE_T *tags, char *name, BOOLEAN_T skip, BOOLEAN_T is_input_tag) {
  TAG_T *tag;
  tag = (TAG_T*)mm_malloc(sizeof(TAG_T));
  memset(tag, 0, sizeof(TAG_T));
  tag->name = name;
  tag->opened = 0;
  tag->closed = 0;
  tag->last = 0;
  tag->is_input_tag = is_input_tag;
  tag->skip = NULL;
  if (skip) {
    STR_T *endtag;
    int len;
    len = strlen(name);
    endtag = str_create(len+2);
    str_append(endtag, "</", 2);
    str_append(endtag, name, len);
    tag->skip = bmstr_create2(str_internal(endtag), 1);
    str_destroy(endtag, FALSE);
  }
  rbtree_put(tags, name, tag);
}

/*
 * Destroy a tag datastructure.
 */
void destroy_tag_record(void *p) {
  TAG_T *tag = (TAG_T*)p;
  if(tag->skip) bmstr_destroy(tag->skip);
  memset(tag, 0, sizeof(TAG_T));
  free(tag);
}

/*
 * Create the datastructure to record tag sightings
 */
static RBTREE_T* init_tags() {
  RBTREE_T *tags;
  int i;
  tags = rbtree_create(rbtree_strcasecmp, NULL, NULL, NULL, destroy_tag_record);
  for (i = 0; tag_names[i][0] != '\0'; ++i) {
    add_tag(tags, tag_names[i], FALSE, FALSE);
  }
  add_tag(tags, "script", TRUE, FALSE);
  add_tag(tags, "style", TRUE, FALSE);
  add_tag(tags, "input", FALSE, TRUE);
  return tags;
}

/*
 * Appends a maximum number of characters.
 * returns the count that couldn't be appended.
 */
static size_t max_append(STR_T *strb, const char *str, size_t len, size_t max) {
  int add, existing;
  existing = str_len(strb);
  add = 0;
  if (existing < max) {
    add = max - existing;
    if (add > len) add = len;
    str_append(strb, str, add);
  }
  return len - add;
}


/*
 * hdata_create
 * Creates a html parser which extracts the name and value
 * attributes of input tags with type hidden.
 */
HDATA_T* hdata_create(HDATA_FN callback, void *data, size_t max_attr_len) {
  HDATA_T *ctxt;
  assert(callback != NULL);
  ctxt = mm_malloc(sizeof(HDATA_T));
  memset(ctxt, 0, sizeof(HDATA_T));
  ctxt->callback = callback;
  ctxt->data = data;
  ctxt->max_attr_len = max_attr_len;
  ctxt->state = HDATA_READY;
  ctxt->attr = HDATA_IGNORE;
  ctxt->tag = NULL;
  ctxt->tag_count = 0;
  ctxt->leading_slash = FALSE;
  ctxt->trailing_slash = FALSE;
  ctxt->type_hidden = FALSE;
  ctxt->name = NULL;
  ctxt->name_overflow = 0;
  ctxt->value = NULL;
  ctxt->value_overflow = 0;
  ctxt->strb = str_create(10);
  ctxt->tags = init_tags();
  ctxt->comment_end = bmstr_create("-->");
  ctxt->prior_states = rbtree_create(rbtree_intcmp, rbtree_intcpy, free, NULL, NULL);
  return ctxt;
}

/*
 * hdata_destroy
 * Destroys the html parser.
 */
void hdata_destroy(HDATA_T *ctxt) {
  if (ctxt->name) free(ctxt->name);
  if (ctxt->value) free(ctxt->value);
  str_destroy(ctxt->strb, 0);
  rbtree_destroy(ctxt->tags);
  bmstr_destroy(ctxt->comment_end);
  rbtree_destroy(ctxt->prior_states);
  memset(ctxt, 0, sizeof(HDATA_T));
  free(ctxt);
}

/*
 * Update the tallies for a tag and if it's an input
 * tag with all the requsite fields then do a callback.
 */
static void complete_tag(HDATA_T *ctxt) {
  // update the tallies
  if (ctxt->leading_slash) { // close tag
    ctxt->tag->closed++;
    ctxt->tag->last = -1;
  } else if (ctxt->trailing_slash) { // self-closed tag
    ctxt->tag->self_closed++;
    ctxt->tag->last = 0;
  } else { // open tag
    ctxt->tag->opened++;
    ctxt->tag->last = 1;
  }
  // check if we have an input tag that had the required fields
  if (ctxt->tag->is_input_tag && ctxt->type_hidden && ctxt->name && ctxt->value) {
    // do the callback
    //printf("Callback-name: %s\nCallback-value: %s\n", ctxt->name, ctxt->value);
    ctxt->callback(ctxt, ctxt->data, 
        ctxt->name, ctxt->name_overflow, 
        ctxt->value, ctxt->value_overflow);
  }
  // reset vars
  ctxt->attr = HDATA_IGNORE;
  ctxt->type_hidden = 0;
  if (ctxt->name) free(ctxt->name);
  ctxt->name = NULL;
  ctxt->name_overflow = 0;
  if (ctxt->value) free(ctxt->value);
  ctxt->value = NULL;
  ctxt->value_overflow = 0;
  ctxt->leading_slash = FALSE;
  ctxt->trailing_slash = FALSE;
}

/*
 * Set an attribute value if we're not ignoring it.
 */
static void set_attr(HDATA_T *ctxt) {
  switch (ctxt->attr) {
    case HDATA_IGNORE:
      break;
    case HDATA_INPUT_TYPE:
      ctxt->type_hidden = (str_casecmp(ctxt->strb, "hidden") == 0);
      break;
    case HDATA_INPUT_NAME:
      if (ctxt->name) free(ctxt->name);
      ctxt->name = str_copy(ctxt->strb);
      ctxt->name_overflow = ctxt->overflow;
      break;
    case HDATA_INPUT_VALUE:
      if (ctxt->value) free(ctxt->value);
      ctxt->value = str_copy(ctxt->strb);
      ctxt->value_overflow = ctxt->overflow;
      break;
    default:
      die("Illegal attribute state\n");
  }
  str_clear(ctxt->strb);
  ctxt->overflow = 0;
  ctxt->attr = HDATA_IGNORE;
}

/*
 * Looking for tags.
 */
static size_t state_ready(HDATA_T *ctxt, const char *chunk, size_t size) {
  assert(size > 0);
  //look for the start of a tag
  int i;
  for (i = 0; i < size; ++i) {
    if (chunk[i] == '<') {
      ctxt->state = HDATA_TAGNAME;
      return i+1;
    }
  }
  return size;
}

/*
 * Reads the name of the tag and checks if it is a html tag.
 */
static size_t state_tagname(HDATA_T *ctxt, const char *chunk, size_t size) {
  assert(size > 0);
  int i, existing;
  // look for whitespace, < or >
  for (i = 0; i < size; ++i) {
    if (chunk[i] == '<' || chunk[i] == '>' || isspace(chunk[i])) {
      //end of tag name
      break;
    }  
  }
  // store the first 20 chars of the tag name (as no real tags are longer than 10)
  existing = str_len(ctxt->strb);
  max_append(ctxt->strb, chunk, i, MAX_TAG_LEN);
  // if we have more than 3 chars then check if it's a comment start
  if (existing < 3 && str_len(ctxt->strb) >= 3 && str_ncmp(ctxt->strb,"!--", 3) == 0) {
    //found comment
    ctxt->state = HDATA_COMMENT;
    str_clear(ctxt->strb);
    return 3 - existing;
  }
  // check if we found the end of the name
  if (i == size) return size; // no end found yet
  // check for leading /
  if (str_len(ctxt->strb) > 0 && str_char(ctxt->strb, 0) == '/') {
    ctxt->leading_slash = TRUE;
    // remove the leading /
    str_delete(ctxt->strb, 0, 1);
  }
  // check for a trailing / and exclude it
  if (str_len(ctxt->strb) > 0 && str_char(ctxt->strb, -1) == '/') {
    // remove the trailing /
    str_truncate(ctxt->strb, -1);
    // set the trailing slash mode, this is mostly handled by state_intag
    ctxt->trailing_slash = TRUE;
  }
  // find the tag (if it exists)
  ctxt->tag = (TAG_T*)rbtree_get(ctxt->tags, str_internal(ctxt->strb));
  // clean up
  str_clear(ctxt->strb);
  //now check if we found anything
  if (ctxt->tag) {
    ctxt->tag_count++;
    // now parse attributes
    ctxt->state = HDATA_INTAG;
  } else {
    //nope it's a dud, try again
    ctxt->state = HDATA_READY;
  }
  return i;
}

/*
 * Looks for attributes to read and the end of the tag.
 */
static size_t state_intag(HDATA_T *ctxt, const char *chunk, size_t size) {
  assert(size > 0);
  int i;
  // skip whitespace
  for (i = 0; i < size; i++) {
    if (!isspace(chunk[i])) break;
  }
  if (i >= size) return size; // all space, need more chunks!
  if (chunk[i] == '/') {
    ctxt->trailing_slash = TRUE; // note that this might also be set by state_tagname
  } else if (chunk[i] == '>' || chunk[i] == '<') {
    complete_tag(ctxt);
    // check if we need to ignore stuff
    if (ctxt->tag->last == 1 && ctxt->tag->skip) {//TODO last should realy be an enum...
      ctxt->state = HDATA_SKIP;
    } else {
      ctxt->tag = NULL;
      ctxt->state = HDATA_READY;
    }
    if (chunk[i] == '<') return i;
  } else {
    ctxt->trailing_slash = FALSE;
    ctxt->state = HDATA_ATTRNAME;
    return i;
  }
  return i+1;
}

/*
 * Reads an attribute name
 */
static size_t state_attrname(HDATA_T *ctxt, const char *chunk, size_t size) {
  int i;
  for (i = 0; i < size; ++i) {
    if (chunk[i] == '=' || chunk[i] == '<' || chunk[i] == '>' || chunk[i] == '/' || isspace(chunk[i])) {
      break;
    } 
  }
  if (ctxt->tag->is_input_tag) { // input tag, so look at attributes
    // store only the first 10 chars of the attribute name
    // because type, name and value are less than 5 chars
    max_append(ctxt->strb, chunk, i, MAX_ATTR_LEN);
  }
  if (i >= size) return size; // need more chunks!

  // by default ignore attributes
  ctxt->attr = HDATA_IGNORE;
  if (ctxt->tag->is_input_tag) {
    // but if we have an input tag we want to know what they are
    // when they are either "type", "name" or "value"
    if (str_casecmp(ctxt->strb, "type") == 0) {
      ctxt->attr = HDATA_INPUT_TYPE;
    } else if (str_casecmp(ctxt->strb, "name") == 0) {
      ctxt->attr = HDATA_INPUT_NAME;
    } else if (str_casecmp(ctxt->strb, "value") == 0) {
      ctxt->attr = HDATA_INPUT_VALUE;
    }
  }
  if (chunk[i] != '=') { // attribute value is itself
    set_attr(ctxt);
    ctxt->state = HDATA_INTAG;
    return i; 
  } else {
    str_clear(ctxt->strb);
    ctxt->state = HDATA_ATTRVALUE;
    return i+1;
  }
}

/*
 * Reads an attribute value
 */
static size_t state_attrvalue(HDATA_T *ctxt, const char *chunk, size_t size) {
  assert(size != 0);
  int i;
  //skip spaces because if someone went to the effort of putting an '=' then 
  //surely they also supplied a value?... Only allow this mode if the next item
  //is quoted, otherwise assume they meant no value
  for (i = 0; i < size; ++i) {
    if (!isspace(chunk[i])) break;
  }
  if (i == size) return size; //need more chunks!

  switch (chunk[i]) {
    case '<':
    case '/':
    case '>':// attribute didn't have a value after all
      set_attr(ctxt);
      ctxt->state = HDATA_INTAG;
      return i;
    case '"':
      ctxt->state = HDATA_DOUBLEQUOTE;
      return i + 1;
    case '\'':
      ctxt->state = HDATA_SINGLEQUOTE;
      return i + 1;
    default:
      if (i > 0) { 
        // not quoted so assume that it's actually another
        // attribute and hence the previous attribute gets empty
        // string as its value
        set_attr(ctxt);
        ctxt->state = HDATA_INTAG;
      } else {
        ctxt->state = HDATA_NOQUOTE;
      }
      return i;
  }
}

/*
 * Reads a single quoted attribute value.
 */
static size_t state_singlequote(HDATA_T *ctxt, const char *chunk, size_t size) {
  int i;
  for (i = 0; i < size; ++i) {
    if (chunk[i] == '\'') break;
  }
  if (ctxt->attr != HDATA_IGNORE) {
    ctxt->overflow += max_append(ctxt->strb, chunk, i, ctxt->max_attr_len);
  }
  if (i == size) return size; // need more chunks!
  set_attr(ctxt);
  ctxt->state = HDATA_INTAG;
  return i+1;
}

/*
 * Reads a double quoted attribute value.
 */
static size_t state_doublequote(HDATA_T *ctxt, const char *chunk, size_t size) {
  int i;
  for (i = 0; i < size; ++i) {
    if (chunk[i] == '"') break;
  }
  if (ctxt->attr != HDATA_IGNORE) {
    ctxt->overflow += max_append(ctxt->strb, chunk, i, ctxt->max_attr_len);
  }
  if (i == size) return size; // need more chunks!
  set_attr(ctxt);
  ctxt->state = HDATA_INTAG;
  return i+1;
}

/*
 * Reads an unquoted attribute value.
 */
static size_t state_noquote(HDATA_T *ctxt, const char *chunk, size_t size) {
  int i;
  for (i = 0; i < size; ++i) {
    if (isspace(chunk[i]) || chunk[i] == '>' || chunk[i] == '<' || chunk[i] == '/') break;
  }
  if (ctxt->attr != HDATA_IGNORE) {
    ctxt->overflow += max_append(ctxt->strb, chunk, i, ctxt->max_attr_len);
  }
  if (i == size) return size; // need more chunks!
  set_attr(ctxt);
  ctxt->state = HDATA_INTAG;
  return i+1;
}

/*
 * Looks for the closing tag for whatever is being skipped. This is
 * useful so the content of script and style tags can be ignore.
 */
static size_t state_skip(HDATA_T *ctxt, const char *chunk, size_t size) {
  assert(size > 0);
  int offset, existing, next;
  if (str_len(ctxt->strb) > 0) {
    existing = str_len(ctxt->strb);
    // copy the chunk into the strb before doing the search
    // this is inefficient, but easier and clearer than the alternative
    str_append(ctxt->strb, chunk, size);
    offset = bmstr_substring(ctxt->tag->skip, str_internal(ctxt->strb), str_len(ctxt->strb));
    if (offset < 0) {
      offset = -(offset + 1);
      str_delete(ctxt->strb, 0, offset);
      return size;
    }
    // just in case we don't have the character after
    // to determine the next state make sure the strb
    // is ready for another chunk
    str_delete(ctxt->strb, 0, offset);
    // make the offset relative to the chunk
    offset -= existing;
  } else {
    offset = bmstr_substring(ctxt->tag->skip, chunk, size);
    if (offset  < 0) {
      offset = -(offset + 1);
      str_append(ctxt->strb, chunk+offset, size - offset);
      return size;
    }
    // just in case we don't have the character after
    // to determine the next state make sure the strb
    // is ready for another chunk
    str_append(ctxt->strb, chunk+offset, size - offset); 
  }
  next = offset + bmstr_length(ctxt->tag->skip);
  if (size <= next) return size; //need another chunk before we can make a decision
  // decided if we've found the close tag we were looking for
  if (isspace(chunk[next]) || chunk[next] == '>' || chunk[next] == '<') {
    // change to attribute parsing mode
    ctxt->state = HDATA_INTAG;
    ctxt->leading_slash = TRUE;
    ctxt->trailing_slash = FALSE;
    return next;
  } else {
    // its some other tag, step one char past the instance we found and
    // try to find another instance
    return offset + 1;
  }
}

/*
 * Looks for the end of a comment tag so normal parsing can resume.
 */
static size_t state_comment(HDATA_T *ctxt, const char *chunk, size_t size) {
  assert(size > 0);
  int offset;
  switch (str_len(ctxt->strb)) {
    case 1:
      if (size >= 2) {
        if (chunk[0] == '-' && chunk[1] == '>') {
          ctxt->state = HDATA_READY;
          str_clear(ctxt->strb);
          return 2; 
        } else {
          str_clear(ctxt->strb);
        }
      } else {
        if (chunk[0] == '-') {
          str_append(ctxt->strb, "-", 1);
          return 1;
        }
      }      
      break;
    case 2:
      if (chunk[0] == '-') {
        if (size >= 2) {
          if (chunk[1] == '>') {
            ctxt->state = HDATA_READY;
            str_clear(ctxt->strb);
            return 2; 
          } else {
            str_clear(ctxt->strb);
          }
        } else {
          return 1;
        }
      } else if (chunk[0] == '>') {
        ctxt->state = HDATA_READY;
        str_clear(ctxt->strb);
        return 1; 
      } else {
        str_clear(ctxt->strb);
      }
      break;
  }
  // look for a match in the chunk
  offset = bmstr_substring(ctxt->comment_end, chunk, size);
  if (offset < 0) {
    offset = -(offset + 1);
    str_append(ctxt->strb, chunk+offset, (size-offset)); 
    return size;
  } else {
    ctxt->state = HDATA_READY;
    return offset + 3;
  }
}

/*
 * Dispatches the data based on the state.
 */
static int dispatch(HDATA_T *ctxt, const char *chunk, size_t size) {
  int consumed;
  /*
  printf("\n\n"
      "========STRBUF===========\n"
      "%s\n"
      "=========================\n"
      "++++++++Name-buf+++++++++\n"
      "%s\n"
      "+++++++++++++++++++++++++\n"
      "--------Value-buf--------\n"
      "%s\n"
      "-------------------------\n"
      "~~~~~~~~~~Chunk~~~~~~~~~~\n"
      "%*s\n"
      "~~~~~~~~~~~~~~~~~~~~~~~~~\n",
      str_internal(ctxt->strb),
      (ctxt->name ? ctxt->name : "<NULL>"), 
      (ctxt->value ? ctxt->value : "<NULL>"), 
      (int)size, chunk);
      */
  switch (ctxt->state) {
    case HDATA_READY:
      //printf("State: HDATA_READY\n");
      consumed = state_ready(ctxt, chunk, size);
      break;
    case HDATA_TAGNAME:
      //printf("State: HDATA_TAGNAME\n");
      consumed = state_tagname(ctxt, chunk, size);
      break;
    case HDATA_INTAG:
      //printf("State: HDATA_INTAG\n");
      consumed = state_intag(ctxt, chunk, size);
      break;
    case HDATA_ATTRNAME:
      //printf("State: HDATA_ATTRNAME\n");
      consumed = state_attrname(ctxt, chunk, size);
      break;
    case HDATA_ATTRVALUE:
      //printf("State: HDATA_ATTRVALUE\n");
      consumed = state_attrvalue(ctxt, chunk, size);
      break;
    case HDATA_SINGLEQUOTE:
      //printf("State: HDATA_SINGLEQUOTE\n");
      consumed = state_singlequote(ctxt, chunk, size);
      break;
    case HDATA_DOUBLEQUOTE:
      //printf("State: HDATA_DOUBLEQUOTE\n");
      consumed = state_doublequote(ctxt, chunk, size);
      break;
    case HDATA_NOQUOTE:
      //printf("State: HDATA_NOQUOTE\n");
      consumed = state_noquote(ctxt, chunk, size);
      break;
    case HDATA_SKIP:
      //printf("State: HDATA_SKIP\n");
      consumed = state_skip(ctxt, chunk, size);
      break;
    case HDATA_COMMENT:
      //printf("State: HDATA_COMMENT\n");
      consumed = state_comment(ctxt, chunk, size);
      break;
    default:
      consumed = 0;
      die("Bad state");
  }
  assert(consumed >= 0);
  assert(consumed <= size);
  //printf("Consumed: %d of %d\n", consumed, (int)size);
  return consumed;
}

/*
 * When the file is correct html this function
 * is not called. However if there is some unfinished
 * data this tries to resove it to something sensible.
 */
static void dispatch_end(HDATA_T *ctxt) {
  switch (ctxt->state) {
    case HDATA_TAGNAME:
      // check for leading /
      if (str_char(ctxt->strb, 0) == '/') {
        ctxt->leading_slash = TRUE;
        // remove the leading /
        str_delete(ctxt->strb, 0, 1);
      }
      // check for a trailing /
      if (str_char(ctxt->strb, -1) == '/') {
        ctxt->trailing_slash = TRUE;
        // remove the trailing /
        str_truncate(ctxt->strb, -1);
      }
      // find the tag (if it exists)
      ctxt->tag = (TAG_T*)rbtree_get(ctxt->tags, str_internal(ctxt->strb));
      // clean up
      str_clear(ctxt->strb);
      //now check if we found anything
      if (ctxt->tag) { // now parse attributes
        ctxt->state = HDATA_INTAG;
      } else { //nope it's a dud
        ctxt->state = HDATA_READY;
      }
      break;
    case HDATA_INTAG:
      complete_tag(ctxt);
      ctxt->state = HDATA_READY;
      break;
    case HDATA_ATTRNAME:
      // by default ignore attributes
      ctxt->attr = HDATA_IGNORE;
      if (ctxt->tag->is_input_tag) {
        // but if we have an input tag we want to know what they are
        // when they are either "type", "name" or "value"
        if (str_casecmp(ctxt->strb, "type") == 0) {
          ctxt->attr = HDATA_INPUT_TYPE;
        } else if (str_casecmp(ctxt->strb, "name") == 0) {
          ctxt->attr = HDATA_INPUT_NAME;
        } else if (str_casecmp(ctxt->strb, "value") == 0) {
          ctxt->attr = HDATA_INPUT_VALUE;
        }
      }
      ctxt->state = HDATA_ATTRVALUE;
      break;
    case HDATA_ATTRVALUE:
    case HDATA_SINGLEQUOTE:
    case HDATA_DOUBLEQUOTE:
    case HDATA_NOQUOTE:
      set_attr(ctxt);
      ctxt->state = HDATA_INTAG;
      break;
    case HDATA_SKIP:
    case HDATA_COMMENT:
      ctxt->state = HDATA_READY;
      break;
    default:
      die("Bad state");
  }
}

/*
 * Checks for infinite loops. Every parsing state must either consume
 * some data or change the state to one that hasn't been used at this
 * position. As there are a finite number of states this ensures that
 * parsing will stop at some point or be detected by this function.
 */
static BOOLEAN_T loop_check(HDATA_T *ctxt, int prior_state, int consumed) {
  RBTREE_T *prior_states;
  int new_state;
  BOOLEAN_T is_new_state;
  prior_states = ctxt->prior_states;
  if (consumed == 0) {
    new_state = ctxt->state;
    if (rbtree_size(prior_states) == 0) {
      if (prior_state == new_state) return TRUE;
      rbtree_put(prior_states, &prior_state, NULL);
      rbtree_put(prior_states, &new_state, NULL);
    } else {
      rbtree_lookup(prior_states, &new_state, TRUE, &is_new_state);
      if (!is_new_state) return TRUE;
    }
  } else {
    rbtree_clear(prior_states);
  }
  return FALSE;
}

/*
 * hdata_update
 * Parses another chunk of a possible html file.
 */
void hdata_update(HDATA_T *ctxt, const char *chunk, size_t size, short end) {
  /*
  // Debugging code.
  static char *debug_chunk = NULL;
  static size_t debug_size;
  if (debug_chunk != NULL) {
    if (debug_size == size && strncmp(chunk, debug_chunk, debug_size) == 0) {
      printf("Warning, duplicate chunk!\n");
    }
    debug_size = size;
    debug_chunk = mm_realloc(debug_chunk, size);
    strncpy(debug_chunk, chunk, size);
  } else {
    debug_size = size;
    debug_chunk = mm_malloc(size);
    strncpy(debug_chunk, chunk, size);
  }
  // end debugging code
  */
  assert(ctxt != NULL);
  assert(size >= 0);
  int offset, consumed, prior_state;
  rbtree_clear(ctxt->prior_states);
  if (size > 0) {
    assert(chunk != NULL);
    offset = 0;
    while (offset < size) {
      prior_state = ctxt->state;
      consumed = dispatch(ctxt, chunk+offset, size - offset);
      offset += consumed;
      // check for parser problems
      if (loop_check(ctxt, prior_state, consumed)) die("Infinite loop detected.");
      assert(offset <= size);
    }
  }
  if (end) {
    while (ctxt->state != HDATA_READY) {
      prior_state = ctxt->state;
      dispatch_end(ctxt);
      // check for parser problems
      if (loop_check(ctxt, prior_state, 0)) die("Infinite loop detected at end.");
    }
  }
}

/*
 * Get the total number of html tags
 * seen.
 */
long hdata_total_seen(HDATA_T *ctxt) {
  return ctxt->tag_count;
}

/*
 * Get the number of times a html tag has been
 * seen. If the tag is not known then 0 will
 * be returned.
 */
long hdata_tag_seen(HDATA_T *ctxt, char *tag) {
  TAG_T *taginfo = rbtree_get(ctxt->tags, tag);
  if (taginfo != NULL) {
    return taginfo->opened + taginfo->closed + taginfo->self_closed;
  }
  return 0;
}

/*
 * Get the number of times an opening tag has
 * been seen.
 */
long hdata_tag_opened(HDATA_T *ctxt, char *tag) {
  TAG_T *taginfo = rbtree_get(ctxt->tags, tag);
  if (taginfo != NULL) return taginfo->opened;
  return 0;
}

/*
 * Get the number of times a closing tag has been
 * seen.
 */
long hdata_tag_closed(HDATA_T *ctxt, char *tag) {
  TAG_T *taginfo = rbtree_get(ctxt->tags, tag);
  if (taginfo != NULL) return taginfo->closed;
  return 0;
}

/*
 * Get the number of times a self-closing tag has
 * been seen.
 */
long hdata_tag_selfclosed(HDATA_T *ctxt, char *tag) {
  TAG_T *taginfo = rbtree_get(ctxt->tags, tag);
  if (taginfo != NULL) return taginfo->self_closed;
  return 0;
}

/*
 * Get the number of times a tag still needs to be closed. This
 * doesn't know about tags like img which don't have opening and
 * closing tags (unless the user is trying to write xhtml) and
 * so may report img tags as needing closing tags when they really
 * don't. If there are more closing tags then opening tags it will
 * report a negative number.
 */
long hdata_tag_toclose(HDATA_T *ctxt, char *tag) {
  TAG_T *taginfo = rbtree_get(ctxt->tags, tag);
  if (taginfo != NULL) return taginfo->opened - taginfo->closed;
  return 0;
}

/*
 * Get the last seen tag type. 
 * 0 = self-closed (or unseen), 1 = open tag, -1 = close tag
 */
short hdata_tag_last(HDATA_T *ctxt, char *tag) {
  TAG_T *taginfo = rbtree_get(ctxt->tags, tag);
  if (taginfo != NULL) return taginfo->last;
  return 0;
}
