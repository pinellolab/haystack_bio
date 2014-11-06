
#include <assert.h>

#include "json-reader.h"
#include "red-black-tree.h"
#include "string-builder.h"
#include "string-match.h"
#include "utils.h"

enum parser_state {
  // When an error has occurred.
  PS_ERROR,
  // Occurs at the begining and after any JSON variables have been finished.
  // The parser looks for the landmark "@JSON_VAR " and if one is found 
  // transitions to the PS_READ_VAR.
  PS_FIND_LANDMARK,
  // Occurs when reading the variable name of a JSON object.
  // The parser reads the next space delimitered word and stores it as the 
  // variable name. After the name has been read the parser transitions
  // to PS_FIND_START.
  PS_READ_VAR,
  // Occurs when scanning for the start of the JSON datastructure.
  // The parser reads until it encouters a '{' and then pushes PS_FIND_LANDMARK
  // on the stack and transitions to PS_PROPERTY_OR_ENDOBJ.
  PS_FIND_START,
  // Occurs when looking for a property or the end of the current object.
  // The parser reads a token and if it finds a STRING it transitions to
  // PS_COLON (as that is the start of a property). Alternatively if
  // the token is ENDOBJ it pops the stack.
  PS_PROPERTY_OR_ENDOBJ,
  // Occurs when looking for the colon that separates a key and value in a
  // property. When it reads the COLON token it transitions to PS_PROPERTY_VALUE.
  PS_COLON,
  // Occurs when looking for the value of a property and can accept the value
  // tokens STRING, NUMBER, BOOL, NULL, STARTOBJ or STARTLST. After the 
  // value has been read (complex for nested objects and lists) it transitions
  // to PS_COMMA_OR_ENDOBJ. Note that if the value is STARTOBJ or STARTLST it
  // pushes PS_COMMA_OR_ENDOBJ onto the stack before transitioning. For the
  // STARTOBJ token it will then transition to PS_PROPERTY_OR_ENDOBJ. For the
  // STARTLST token it will then transition to PS_START_VALUE_OR_ENDLST.
  PS_PROPERTY_VALUE,
  // Occurs after an object property has been read and the parser is looking
  // for comma separating properties or the end of the object. If the next token
  // is a COMMA then it transitions to PS_PROPERTY, otherwise the next token
  // should be ENDOBJ in which case it pops the stack.
  PS_COMMA_OR_ENDOBJ,
  // Occurs after a previous property has been read and a separating comma has
  // been found implying another property to come. When it reads the STRING token
  // it transitions to PS_COLON.
  PS_PROPERTY,
  // Occurs when reading a value from a list.
  // The parser reads a token and if it finds a value it transitions to 
  // PS_COMMA_OR_ENDLST. Alternatively if the TOKEN is ENDLST it pops the stack.
  // Note that if the value is STARTOBJ or STARTLST it pushes the state 
  // PS_COMMA_OR_ENDLST on the stack before transitioning. If the token was 
  // STARTOBJ it will transition to PS_PROPERTY_OR_ENDOBJ. If the token was
  // STARTLST it will transition to PS_VALUE_OR_ENDLST.
  PS_VALUE_OR_ENDLST,
  // Occurs after reading a value from a list.
  // The parser reads a token and if it finds a COMMA it transitions to 
  // PS_LIST_VALUE. Alternatively it should find a ENDLST and will then
  // pop the stack.
  PS_COMMA_OR_ENDLST,
  // Occurs after finding a COMMA in a list implying another value to come.
  // The parser reads a value and then will transition to PS_COMMA_OR_ENDLST.
  // If the value is STARTOBJ or STARTLST then PS_COMMA_OR_ENDLST will be
  // pushed onto the stack before the transition. STARTOBJ will cause a transition
  // to PS_PROPERTY_OR_ENDOBJ and STARTLST will transition to 
  // PS_VALUE_OR_ENDLST.
  PS_LIST_VALUE
};
typedef enum parser_state PS_EN;

enum tokenizer_state {
  TS_FIND_TOKEN,
  TS_FOUND_TOKEN,
  TS_IN_PROGRESS
};
typedef enum tokenizer_state TS_EN;

enum number_state {
  NS_BEGIN,
  NS_LEADING_MINUS,
  NS_LEADING_ZERO,
  NS_LEADING_DIGITS,
  NS_DECIMAL_PLACE,
  NS_DECIMAL_DIGITS,
  NS_EXPONENT_CHAR,
  NS_EXPONENT_SIGN,
  NS_EXPONENT_DIGITS
};
typedef enum number_state NS_EN;

enum string_state {
  SS_NORMAL,
  SS_ESCAPE,
  SS_HEX_NUM
};
typedef enum string_state SS_EN;

enum token_type {
  TOK_ILLEGAL,
  TOK_STARTOBJ, // {
  TOK_ENDOBJ, // }
  TOK_STARTLST, // [
  TOK_ENDLST, // ]
  TOK_COLON, // :
  TOK_COMMA, // ,
  TOK_NULL, // null
  TOK_TRUE, // true
  TOK_FALSE, // false
  TOK_NUMBER,
  TOK_STRING
};
typedef enum token_type TT_EN;


typedef struct history {
  int used;
  int length;
  PS_EN *states;
} HISTORY_T;

typedef struct token {
  TS_EN state;
  TT_EN type;
  bool value_bool;
  NS_EN num_state;
  long double value_number;
  SS_EN str_state;
  STR_T *value_string;
} TOKEN_T;

struct json_reader {
  PS_EN state;
  RBTREE_T *prior_states;
  BMSTR_T *landmark;
  STR_T *buf;
  void *user_data;
  JSONRD_CALLBACKS_T callbacks;
  HISTORY_T stack;
  TOKEN_T token;
};


static void error(JSONRD_T *jsonrd, char* fmt, ...) {
  va_list argp;
  if (jsonrd->callbacks.error) {
    va_start(argp, fmt);
    jsonrd->callbacks.error(jsonrd->user_data, fmt, argp);
    va_end(argp);
  }
  jsonrd->state = PS_ERROR;
}

static void push_state(JSONRD_T *jsonrd, PS_EN state) {
  HISTORY_T *stack;
  stack = &(jsonrd->stack);
  if (stack->used >= stack->length) {
    if (stack->length == 0) {
      stack->length = 4;
      stack->states = mm_malloc(sizeof(PS_EN) * stack->length);
    } else {
      stack->length *= 2;
      stack->states = mm_realloc(stack->states, sizeof(PS_EN) * stack->length);
    }
  }
  stack->states[stack->used++] = state;
}

static PS_EN pop_state(JSONRD_T *jsonrd) {
  HISTORY_T *stack;
  stack = &(jsonrd->stack);
  if (stack->used == 0) {
    die("No states on stack!");
    return PS_ERROR; // should not get here
  } else {
    stack->used--;
    return stack->states[stack->used];
  }
}

static int find_landmark(JSONRD_T *jsonrd, const char *chunk, size_t size) {
  int len, pos;

  len = str_len(jsonrd->buf);
  // check for previous partial matches
  if (len > 0) {
    // append up to landmark size -1 characters to the buffer
    str_append(jsonrd->buf, chunk, (size < bmstr_length(jsonrd->landmark) ? size : bmstr_length(jsonrd->landmark) - 1));
    // now look for the landmark
    pos = bmstr_substring(jsonrd->landmark, str_internal(jsonrd->buf), str_len(jsonrd->buf));
    if (pos >= 0) { //match
      pos = pos + bmstr_length(jsonrd->landmark) - len;
      jsonrd->state = PS_READ_VAR;
      return pos;
    } else { //possible partial match
      pos = -(pos + 1);
      if (pos < len) {
        // apparently the chunk wasn't large enough to actually test everything
        // in the buffer. As there's nothing more to test we'll just delete the
        // bits that definately don't match at the front of the buffer
        if (pos > 0) str_delete(jsonrd->buf, 0, pos);
        return size;
      } else {
        // possible partial match, but we don't care because we'll find it when
        // we look at the chunk
        str_clear(jsonrd->buf);
      }
    }
  }
  pos = bmstr_substring(jsonrd->landmark, chunk, size);
  if (pos >= 0) {
    pos = pos + bmstr_length(jsonrd->landmark);
    jsonrd->state = PS_READ_VAR;
    return pos;
  }
  pos = -(pos + 1);
  if (pos < size) {
    str_append(jsonrd->buf, chunk+pos, size - pos);  
  }
  return size;
}

static int read_var(JSONRD_T *jsonrd, const char *chunk, size_t size) {
  int i, start;
  i = 0;
  //check if we've already started reading the var name
  if (str_len(jsonrd->buf) == 0) {
    // not yet started so skip over blanks
    for (; i < size; i++) {
      if (!isblank(chunk[i])) break;
    }
  }
  start = i;
  for (; i < size; i++) {
    if (isspace(chunk[i])) break;
  }
  str_append(jsonrd->buf, chunk+start, i - start);
  if (i < size) {
    jsonrd->state = PS_FIND_START;
  }
  return i;
}

static int find_start(JSONRD_T *jsonrd, const char *chunk, size_t size) {
  int i;
  for (i = 0; i < size; i++) {
    if (chunk[i] == '{') {
      push_state(jsonrd, PS_FIND_LANDMARK);
      jsonrd->state = PS_PROPERTY_OR_ENDOBJ;
      jsonrd->token.state = TS_FOUND_TOKEN; // causes the token data to be reset on next_token call
      if (jsonrd->callbacks.start_data) {
        jsonrd->callbacks.start_data(jsonrd->user_data, str_internal(jsonrd->buf), str_len(jsonrd->buf));
      }
      if (jsonrd->callbacks.start_object) {
        jsonrd->callbacks.start_object(jsonrd->user_data);
      }
      return i + 1;
    }
  }
  return size;
}

static inline int process_keyword_token(JSONRD_T *jsonrd, const char *chunk, size_t size,
    size_t offset, char *want_str, size_t want_len) {
  size_t buf_size;
  buf_size = str_len(jsonrd->buf);
  if ((buf_size + size - offset) >= want_len) {
    jsonrd->token.state = TS_FOUND_TOKEN;
    str_append(jsonrd->buf, chunk+offset, want_len - buf_size);
    if (str_ncmp(jsonrd->buf, want_str, want_len) == 0) {
      return offset + (want_len - buf_size);
    } else {
      jsonrd->token.type = TOK_ILLEGAL;
      return offset + 1;
    }
  } else {
    str_append(jsonrd->buf, chunk+offset, size - offset);
    return size;
  }
}

static inline int process_number_token(JSONRD_T *jsonrd, const char *chunk, size_t size, size_t offset) {
  int i;
  for (i = offset; i < size; i++) {
    switch (jsonrd->token.num_state) {
      case NS_BEGIN:
        if (chunk[i] == '-') {
          jsonrd->token.num_state = NS_LEADING_MINUS;
        } else if (chunk[i] == '0') {
          jsonrd->token.num_state = NS_LEADING_ZERO;
        } else if (chunk[i] > '0' && chunk[i] <= '9') {
          jsonrd->token.num_state = NS_LEADING_DIGITS;
        } else {
          jsonrd->token.type = TOK_ILLEGAL;
          jsonrd->token.state = TS_FOUND_TOKEN;
          return i + 1;
        }
        break;
      case NS_LEADING_MINUS:
        if (chunk[i] == '0') {
          jsonrd->token.num_state = NS_LEADING_ZERO;
        } else if (chunk[i] > '0' && chunk[i] <= '9') {
          jsonrd->token.num_state = NS_LEADING_DIGITS;
        } else {
          jsonrd->token.type = TOK_ILLEGAL;
          jsonrd->token.state = TS_FOUND_TOKEN;
          return i + 1;
        }
        break;
      case NS_LEADING_ZERO:
        if (chunk[i] == '.') {
          jsonrd->token.num_state = NS_DECIMAL_PLACE;
        } else if (chunk[i] == 'e' || chunk[i] == 'E') {
          jsonrd->token.num_state = NS_EXPONENT_CHAR;
        } else {
          jsonrd->token.value_number = strtold(str_internal(jsonrd->buf), NULL);
          jsonrd->token.state = TS_FOUND_TOKEN;
          return i;
        }
        break;
      case NS_LEADING_DIGITS:
        if (chunk[i] >= '0' && chunk[i] <= '9') {
          // keep current state
        } else if (chunk[i] == '.') {
          jsonrd->token.num_state = NS_DECIMAL_PLACE;
        } else if (chunk[i] == 'e' || chunk[i] == 'E') {
          jsonrd->token.num_state = NS_EXPONENT_CHAR;
        } else {
          jsonrd->token.value_number = strtold(str_internal(jsonrd->buf), NULL);
          jsonrd->token.state = TS_FOUND_TOKEN;
          return i;
        }
        break;
      case NS_DECIMAL_PLACE:
        if (chunk[i] >= '0' && chunk[i] <= '9') {
          jsonrd->token.num_state = NS_DECIMAL_DIGITS;
        } else {
          jsonrd->token.type = TOK_ILLEGAL;
          jsonrd->token.state = TS_FOUND_TOKEN;
          return i + 1;
        }
        break;
      case NS_DECIMAL_DIGITS:
        if (chunk[i] >= '0' && chunk[i] <= '9') {
          // keep current state
        } else if (chunk[i] == 'e' || chunk[i] == 'E') {
          jsonrd->token.num_state = NS_EXPONENT_CHAR;
        } else {
          jsonrd->token.value_number = strtold(str_internal(jsonrd->buf), NULL);
          jsonrd->token.state = TS_FOUND_TOKEN;
          return i;
        }
        break;
      case NS_EXPONENT_CHAR:
        if (chunk[i] == '+' || chunk[i] == '-') {
          jsonrd->token.num_state = NS_EXPONENT_SIGN;
        } else if (chunk[i] >= '0' && chunk[i] <= '9') {
          jsonrd->token.num_state = NS_EXPONENT_DIGITS;
        } else {
          jsonrd->token.type = TOK_ILLEGAL;
          jsonrd->token.state = TS_FOUND_TOKEN;
          return i + 1;
        }
        break;
      case NS_EXPONENT_SIGN:
        if (chunk[i] >= '0' && chunk[i] <= '9') {
          jsonrd->token.num_state = NS_EXPONENT_DIGITS;
        } else {
          jsonrd->token.type = TOK_ILLEGAL;
          jsonrd->token.state = TS_FOUND_TOKEN;
          return i + 1;
        }
        break;
      case NS_EXPONENT_DIGITS:
        if (chunk[i] >= '0' && chunk[i] <= '9') {
          // keep current state
        } else {
          jsonrd->token.value_number = strtold(str_internal(jsonrd->buf), NULL);
          jsonrd->token.state = TS_FOUND_TOKEN;
          return i;
        }
        break;
      default:
        die("Illegal state");
        return size;
    }
    str_append(jsonrd->buf, chunk+i, 1);
  }
  return size;
}

static inline int process_string_token(JSONRD_T *jsonrd, const char *chunk, size_t size, size_t offset) {
  int i, bytes, bytes_needed;
  int32_t code_point;
  STR_T *str;
  str = jsonrd->token.value_string;
  // check to see if there are any incomplete UTF-8 code units
  if (str_len(jsonrd->buf) > 0 && jsonrd->token.str_state == SS_NORMAL) { // complete the code unit
    // count the bytes needed for the complete code unit
    unicode_from_string(str_internal(jsonrd->buf), str_len(jsonrd->buf), &bytes);
    bytes_needed = bytes - str_len(jsonrd->buf);
    if ((size - offset) >= bytes_needed) { // got enough bytes to calculate the code point
      str_append(jsonrd->buf, chunk+offset, bytes_needed);
      code_point = unicode_from_string(str_internal(jsonrd->buf), str_len(jsonrd->buf), &bytes);
      if (code_point < 0) { // bad UTF-8
        jsonrd->token.type = TOK_ILLEGAL;
        jsonrd->token.state = TS_FOUND_TOKEN;
        return offset + bytes_needed;
      }
      str_append(str, str_internal(jsonrd->buf), str_len(jsonrd->buf));
      str_clear(jsonrd->buf);
      offset += bytes_needed;
    } else { // incomplete code unit, need to buffer it
      str_append(jsonrd->buf, chunk+offset, size - offset);
      return size;
    }
  }
  // loop over buffer
  for (i = offset; i < size; i += bytes) {
    // check for valid UTF-8
    code_point = unicode_from_string(chunk+i, size - i, &bytes);
    // characters out of the ASCII range are not involved in ending
    // the string or character escapes so they can be handled first
    if (code_point > 0x7F) { // non-ASCII, complete UTF-8 code unit
      if (jsonrd->token.str_state != SS_NORMAL) { // only ASCII allowed for escape
        jsonrd->token.type = TOK_ILLEGAL;
        jsonrd->token.state = TS_FOUND_TOKEN;
        return i + bytes;
      }
      str_append(str, chunk+i, bytes);
    } else if (code_point == -2) { // incomplete code unit, need to buffer it
      if (jsonrd->token.str_state != SS_NORMAL) { // only ASCII allowed for escape
        jsonrd->token.type = TOK_ILLEGAL;
        jsonrd->token.state = TS_FOUND_TOKEN;
        return i + bytes;        
      }
      str_clear(jsonrd->buf);
      str_append(jsonrd->buf, chunk+i, size - i);
      return size;
    } else if (code_point < 0) { // error bad UTF-8!
      jsonrd->token.type = TOK_ILLEGAL;
      jsonrd->token.state = TS_FOUND_TOKEN;
      return i + bytes;
    } 
    // now we handle the ASCII characters
    switch (jsonrd->token.str_state) {
      case SS_NORMAL:
        if (chunk[i] == '"') { // End of string
          jsonrd->token.state = TS_FOUND_TOKEN;
          return i + 1;
        } else if (chunk[i] == '\\') {
          jsonrd->token.str_state = SS_ESCAPE;
        } else {
          str_append(str, chunk+i, 1);
        }
        break;
      case SS_ESCAPE:
        if (chunk[i] == '"') {
          str_append(str, "\"", 1);
        } else if (chunk[i] == '\\') {
          str_append(str, "\\", 1);
        } else if (chunk[i] == '/') {
          str_append(str, "/", 1);
        } else if (chunk[i] == 'b') {
          str_append(str, "\b", 1);
        } else if (chunk[i] == 'f') {
          str_append(str, "\f", 1);
        } else if (chunk[i] == 'n') {
          str_append(str, "\n", 1);
        } else if (chunk[i] == 'r') {
          str_append(str, "\r", 1);
        } else if (chunk[i] == 't') {
          str_append(str, "\t", 1);
        } else if (chunk[i] == 'u') {
          jsonrd->token.str_state = SS_HEX_NUM;
          str_clear(jsonrd->buf);
          break;
        } else {
          jsonrd->token.type = TOK_ILLEGAL;
          jsonrd->token.state = TS_FOUND_TOKEN;
          return i + 1;
        }
        jsonrd->token.str_state = SS_NORMAL;
        break;
      case SS_HEX_NUM:
        if ((chunk[i] >= 'a' && chunk[i] <= 'f') || 
            (chunk[i] >= 'A' && chunk[i] <= 'F') || 
            (chunk[i] >= '0' && chunk[i] <= '9')) {
          str_append(jsonrd->buf, chunk+i, 1);
        } else {
          jsonrd->token.type = TOK_ILLEGAL;
          jsonrd->token.state = TS_FOUND_TOKEN;
          return i + 1;
        }
        if (str_len(jsonrd->buf) == 4) {
          code_point = strtoll(str_internal(jsonrd->buf), NULL, 16);
          str_clear(jsonrd->buf);
          str_append_code(str, code_point);
          jsonrd->token.str_state = SS_NORMAL;
        }
        break;
    }
  }
  return size;
}

static inline TT_EN guess_token(char c) {
  switch (c) {
    case '{':
      return TOK_STARTOBJ;
    case '}':
      return TOK_ENDOBJ;
    case '[':
      return TOK_STARTLST;
    case ']':
      return TOK_ENDLST;
    case ':':
      return TOK_COLON;
    case ',':
      return TOK_COMMA;
    case 't':
      return TOK_TRUE;
      break;
    case 'f':
      return TOK_FALSE;
      break;
    case 'n':
      return TOK_NULL;
      break;
    case '"':
      return TOK_STRING;
    case '0':
    case '1':
    case '2':
    case '3':
    case '4':
    case '5':
    case '6':
    case '7':
    case '8':
    case '9':
    case '-':
      return TOK_NUMBER;
    default:
      return TOK_ILLEGAL;
  }
}

static int next_token(JSONRD_T *jsonrd, const char *chunk, size_t size) {
  int i;
  // reset state
  if (jsonrd->token.state == TS_FOUND_TOKEN) {
    str_clear(jsonrd->buf);
    jsonrd->token.state = TS_FIND_TOKEN;
    jsonrd->token.type = TOK_ILLEGAL;
    jsonrd->token.value_bool = false;
    jsonrd->token.num_state = NS_BEGIN;
    jsonrd->token.value_number = 0;
    jsonrd->token.str_state = SS_NORMAL;
    str_clear(jsonrd->token.value_string);
  }
  i = 0;
  if (jsonrd->token.state == TS_FIND_TOKEN) {
    // skip whitespace
    for (; i < size; i++) {
      if (!isspace(chunk[i])) break;
    }
    if (i == size) { // only whitespace
      jsonrd->token.state = TS_FIND_TOKEN;
      return size; 
    }
    // try to determine the type of token by the first letter
    jsonrd->token.type = guess_token(chunk[i]);
    jsonrd->token.state = TS_IN_PROGRESS;
    switch (jsonrd->token.type) {
      case TOK_TRUE:
        jsonrd->token.value_bool = true;
      case TOK_FALSE:
      case TOK_NULL:
        break;
      case TOK_STRING:
        jsonrd->token.str_state = SS_NORMAL;
        i++;
        break;
      case TOK_NUMBER:
        jsonrd->token.num_state = NS_BEGIN;
        break;
      default:
        jsonrd->token.state = TS_FOUND_TOKEN;
        return i+1;
    }
  }
  // in progress
  assert(jsonrd->token.state = TS_IN_PROGRESS);
  switch (jsonrd->token.type) {
    case TOK_TRUE:
      return process_keyword_token(jsonrd, chunk, size, i, "true", 4);
    case TOK_FALSE:
      return process_keyword_token(jsonrd, chunk, size, i, "false", 5);
    case TOK_NULL:
      return process_keyword_token(jsonrd, chunk, size, i, "null", 4);
    case TOK_NUMBER:
      return process_number_token(jsonrd, chunk, size, i);
    case TOK_STRING:
      return process_string_token(jsonrd, chunk, size, i);
    default:
      die("Bad state");
      return size; // should not get here
  }
}

static void expect_property_or_endobj(JSONRD_T *jsonrd) {
  if (jsonrd->token.type == TOK_STRING) {
    STR_T *property;
    property = jsonrd->token.value_string;
    if (jsonrd->callbacks.start_property) {
      jsonrd->callbacks.start_property(jsonrd->user_data, str_internal(property), str_len(property));
    }
    jsonrd->state = PS_COLON;
  } else if (jsonrd->token.type == TOK_ENDOBJ) {
    if (jsonrd->callbacks.end_object) {
      jsonrd->callbacks.end_object(jsonrd->user_data);
    }
    jsonrd->state = pop_state(jsonrd);
    if (jsonrd->state == PS_COMMA_OR_ENDOBJ) {
      if (jsonrd->callbacks.end_property) {
        jsonrd->callbacks.end_property(jsonrd->user_data);
      }
    } else if (jsonrd->state == PS_FIND_LANDMARK) {
      if (jsonrd->callbacks.end_data) {
        jsonrd->callbacks.end_data(jsonrd->user_data);
      }
    }
  } else {
    error(jsonrd, "Expected a object property or the end of the object.");
  }
}
static void expect_colon(JSONRD_T *jsonrd) {
  if (jsonrd->token.type == TOK_COLON) {
    jsonrd->state = PS_PROPERTY_VALUE;
  } else {
    error(jsonrd, "Expected a colon.");
  }
}
static void expect_property_value(JSONRD_T *jsonrd) {
  switch (jsonrd->token.type) {
    case TOK_STRING:
      if (jsonrd->callbacks.atom_string) {
        STR_T *value;
        value = jsonrd->token.value_string;
        jsonrd->callbacks.atom_string(jsonrd->user_data, str_internal(value), str_len(value));
      }
      goto atom_value;
    case TOK_NUMBER:
      if (jsonrd->callbacks.atom_number) {
        jsonrd->callbacks.atom_number(jsonrd->user_data, jsonrd->token.value_number);
      }
      goto atom_value;
    case TOK_TRUE:
    case TOK_FALSE:
      if (jsonrd->callbacks.atom_bool) {
        jsonrd->callbacks.atom_bool(jsonrd->user_data, jsonrd->token.value_bool);
      }
      goto atom_value;
    case TOK_NULL:
      if (jsonrd->callbacks.atom_null) {
        jsonrd->callbacks.atom_null(jsonrd->user_data);
      }
atom_value:
      jsonrd->state = PS_COMMA_OR_ENDOBJ;
      if (jsonrd->callbacks.end_property) {
        jsonrd->callbacks.end_property(jsonrd->user_data);
      }
      break;
    case TOK_STARTOBJ:
      if (jsonrd->callbacks.start_object) {
        jsonrd->callbacks.start_object(jsonrd->user_data);
      }
      push_state(jsonrd, PS_COMMA_OR_ENDOBJ);
      jsonrd->state = PS_PROPERTY_OR_ENDOBJ;
      break;
    case TOK_STARTLST:
      if (jsonrd->callbacks.start_list) {
        jsonrd->callbacks.start_list(jsonrd->user_data);
      }
      push_state(jsonrd, PS_COMMA_OR_ENDOBJ);
      jsonrd->state = PS_VALUE_OR_ENDLST;
      break;
    default:
      error(jsonrd, "Expected a property value.");
  }
}
static void expect_comma_or_endobj(JSONRD_T *jsonrd) {
  if (jsonrd->token.type == TOK_COMMA) {
    jsonrd->state = PS_PROPERTY;
  } else if (jsonrd->token.type == TOK_ENDOBJ) {
    if (jsonrd->callbacks.end_object) {
      jsonrd->callbacks.end_object(jsonrd->user_data);
    }
    jsonrd->state = pop_state(jsonrd);
    if (jsonrd->state == PS_COMMA_OR_ENDOBJ) {
      if (jsonrd->callbacks.end_property) {
        jsonrd->callbacks.end_property(jsonrd->user_data);
      }
    } else if (jsonrd->state == PS_FIND_LANDMARK) {
      if (jsonrd->callbacks.end_data) {
        jsonrd->callbacks.end_data(jsonrd->user_data);
      }
    }

  } else {
    error(jsonrd, "Expected a comma or the end of the object.");
  }
}
static void expect_property(JSONRD_T *jsonrd) {
  if (jsonrd->token.type == TOK_STRING) {
    STR_T *property;
    property = jsonrd->token.value_string;
    if (jsonrd->callbacks.start_property) {
      jsonrd->callbacks.start_property(jsonrd->user_data, str_internal(property), str_len(property));
    }
    jsonrd->state = PS_COLON;
  } else {
    error(jsonrd, "Expected a property.");
  }
}
static void expect_value_or_endlst(JSONRD_T *jsonrd) {
  switch (jsonrd->token.type) {
    case TOK_STRING:
      if (jsonrd->callbacks.atom_string) {
        STR_T *value;
        value = jsonrd->token.value_string;
        jsonrd->callbacks.atom_string(jsonrd->user_data, str_internal(value), str_len(value));
      }
      goto atom_value;
    case TOK_NUMBER:
      if (jsonrd->callbacks.atom_number) {
        jsonrd->callbacks.atom_number(jsonrd->user_data, jsonrd->token.value_number);
      }
      goto atom_value;
    case TOK_TRUE:
    case TOK_FALSE:
      if (jsonrd->callbacks.atom_bool) {
        jsonrd->callbacks.atom_bool(jsonrd->user_data, jsonrd->token.value_bool);
      }
      goto atom_value;
    case TOK_NULL:
      if (jsonrd->callbacks.atom_null) {
        jsonrd->callbacks.atom_null(jsonrd->user_data);
      }
atom_value:
      jsonrd->state = PS_COMMA_OR_ENDLST;
      break;
    case TOK_STARTOBJ:
      if (jsonrd->callbacks.start_object) {
        jsonrd->callbacks.start_object(jsonrd->user_data);
      }
      push_state(jsonrd, PS_COMMA_OR_ENDLST);
      jsonrd->state = PS_PROPERTY_OR_ENDOBJ;
      break;
    case TOK_STARTLST:
      if (jsonrd->callbacks.start_list) {
        jsonrd->callbacks.start_list(jsonrd->user_data);
      }
      push_state(jsonrd, PS_COMMA_OR_ENDLST);
      jsonrd->state = PS_VALUE_OR_ENDLST;
      break;
    case TOK_ENDLST:
      if (jsonrd->callbacks.end_list) {
        jsonrd->callbacks.end_list(jsonrd->user_data);
      }
      jsonrd->state = pop_state(jsonrd);
      if (jsonrd->state == PS_COMMA_OR_ENDOBJ && jsonrd->callbacks.end_property) {
        jsonrd->callbacks.end_property(jsonrd->user_data);
      }
      break;
    default:
      error(jsonrd, "Expected a list value or the end of the list.");
  }
}
static void expect_comma_or_endlst(JSONRD_T *jsonrd) {
  if (jsonrd->token.type == TOK_COMMA) {
    jsonrd->state = PS_LIST_VALUE;
  } else if (jsonrd->token.type == TOK_ENDLST) {
    if (jsonrd->callbacks.end_list) {
      jsonrd->callbacks.end_list(jsonrd->user_data);
    }
    jsonrd->state = pop_state(jsonrd);
    if (jsonrd->state == PS_COMMA_OR_ENDOBJ && jsonrd->callbacks.end_property) {
      jsonrd->callbacks.end_property(jsonrd->user_data);
    }
  } else {
    error(jsonrd, "Expected a comma or the end of the list.");
  }
}
static void expect_list_value(JSONRD_T *jsonrd) {
  switch (jsonrd->token.type) {
    case TOK_STRING:
      if (jsonrd->callbacks.atom_string) {
        STR_T *value;
        value = jsonrd->token.value_string;
        jsonrd->callbacks.atom_string(jsonrd->user_data, str_internal(value), str_len(value));
      }
      goto atom_value;
    case TOK_NUMBER:
      if (jsonrd->callbacks.atom_number) {
        jsonrd->callbacks.atom_number(jsonrd->user_data, jsonrd->token.value_number);
      }
      goto atom_value;
    case TOK_TRUE:
    case TOK_FALSE:
      if (jsonrd->callbacks.atom_bool) {
        jsonrd->callbacks.atom_bool(jsonrd->user_data, jsonrd->token.value_bool);
      }
      goto atom_value;
    case TOK_NULL:
      if (jsonrd->callbacks.atom_null) {
        jsonrd->callbacks.atom_null(jsonrd->user_data);
      }
atom_value:
      jsonrd->state = PS_COMMA_OR_ENDLST;
      break;
    case TOK_STARTOBJ:
      if (jsonrd->callbacks.start_object) {
        jsonrd->callbacks.start_object(jsonrd->user_data);
      }
      push_state(jsonrd, PS_COMMA_OR_ENDLST);
      jsonrd->state = PS_PROPERTY_OR_ENDOBJ;
      break;
    case TOK_STARTLST:
      if (jsonrd->callbacks.start_list) {
        jsonrd->callbacks.start_list(jsonrd->user_data);
      }
      push_state(jsonrd, PS_COMMA_OR_ENDLST);
      jsonrd->state = PS_VALUE_OR_ENDLST;
      break;
    default:
      error(jsonrd, "Expected a list value.");
  }
}

/*
 * Dispatches the data based on the state.
 */
static int dispatch(JSONRD_T *jsonrd, const char *chunk, size_t size) {
  int consumed;
  if (jsonrd->state <= PS_FIND_START) {
    switch (jsonrd->state) {
      case PS_FIND_LANDMARK:
        consumed = find_landmark(jsonrd, chunk, size);
        break;
      case PS_READ_VAR:
        consumed = read_var(jsonrd, chunk, size);
        break;
      case PS_FIND_START:
        consumed = find_start(jsonrd, chunk, size);
        break;
      case PS_ERROR:
        consumed = size;
        break;
      default:
        consumed = 0;
        die("Bad state");
    }
  } else {
    consumed = next_token(jsonrd, chunk, size);
    if (jsonrd->token.state == TS_FOUND_TOKEN) {
      switch (jsonrd->state) {
        case PS_PROPERTY_OR_ENDOBJ:
          expect_property_or_endobj(jsonrd);
          break;
        case PS_COLON:
          expect_colon(jsonrd);
          break;
        case PS_PROPERTY_VALUE:
          expect_property_value(jsonrd);
          break;
        case PS_COMMA_OR_ENDOBJ:
          expect_comma_or_endobj(jsonrd);
          break;
        case PS_PROPERTY:
          expect_property(jsonrd);
          break;
        case PS_VALUE_OR_ENDLST:
          expect_value_or_endlst(jsonrd);
          break;
        case PS_COMMA_OR_ENDLST:
          expect_comma_or_endlst(jsonrd);
          break;
        case PS_LIST_VALUE:
          expect_list_value(jsonrd);
          break;
        default:
          die("Bad state");
      }
    }
  }
  assert(consumed >= 0);
  assert(consumed <= size);
  return consumed;
}

/*
 * Checks for infinite loops. Every parsing state must either consume
 * some data or change the state to one that hasn't been used at this
 * position. As there are a finite number of states this ensures that
 * parsing will stop at some point or be detected by this function.
 */
static bool loop_check(JSONRD_T *jsonrd, PS_EN prior_state, int consumed) {
  RBTREE_T *prior_states;
  PS_EN new_state;
  bool is_new_state;
  prior_states = jsonrd->prior_states;
  if (consumed == 0) {
    new_state = jsonrd->state;
    if (rbtree_size(prior_states) == 0) {
      if (prior_state == new_state) return true;
      rbtree_put(prior_states, &prior_state, NULL);
      rbtree_put(prior_states, &new_state, NULL);
    } else {
      rbtree_lookup(prior_states, &new_state, true, &is_new_state);
      if (!is_new_state) return true;
    }
  } else {
    rbtree_clear(prior_states);
  }
  return false;
}

/*****************************************************************************
 * Creates a JSON reader that can extract data sections marked with the
 * landmark @JSON_VAR
 ****************************************************************************/
JSONRD_T* jsonrd_create(JSONRD_CALLBACKS_T *callbacks, void *user_data) {
  JSONRD_T *jsonrd;
  jsonrd = mm_malloc(sizeof(JSONRD_T));
  memset(jsonrd, 0, sizeof(JSONRD_T));
  jsonrd->state = PS_FIND_LANDMARK;
  jsonrd->prior_states = rbtree_create(rbtree_intcmp, rbtree_intcpy, free, NULL, NULL);
  jsonrd->landmark = bmstr_create("@JSON_VAR ");
  jsonrd->buf = str_create(0);
  jsonrd->user_data = user_data;
  // setup callbacks
  jsonrd->callbacks.error = callbacks->error;
  jsonrd->callbacks.start_data = callbacks->start_data;
  jsonrd->callbacks.end_data = callbacks->end_data;
  jsonrd->callbacks.start_property = callbacks->start_property;
  jsonrd->callbacks.end_property = callbacks->end_property;
  jsonrd->callbacks.start_object = callbacks->start_object;
  jsonrd->callbacks.end_object = callbacks->end_object;
  jsonrd->callbacks.start_list = callbacks->start_list;
  jsonrd->callbacks.end_list = callbacks->end_list;
  jsonrd->callbacks.atom_null = callbacks->atom_null;
  jsonrd->callbacks.atom_bool = callbacks->atom_bool;
  jsonrd->callbacks.atom_number = callbacks->atom_number;
  jsonrd->callbacks.atom_string = callbacks->atom_string;
  // setup state stack
  jsonrd->stack.used = 0;
  jsonrd->stack.length = 4;
  jsonrd->stack.states = mm_malloc(sizeof(PS_EN) * jsonrd->stack.length);
  // setup token
  jsonrd->token.state = TS_FIND_TOKEN;
  jsonrd->token.type = TOK_STARTOBJ;
  jsonrd->token.value_bool = false;
  jsonrd->token.num_state = NS_BEGIN;
  jsonrd->token.value_number = 0;
  jsonrd->token.str_state = SS_NORMAL;
  jsonrd->token.value_string = str_create(0);
  return jsonrd;
}

/*****************************************************************************
 * Destroys a JSON reader.
 ****************************************************************************/
void jsonrd_destroy(JSONRD_T *jsonrd) {
  str_destroy(jsonrd->token.value_string, false);
  free(jsonrd->stack.states);
  str_destroy(jsonrd->buf, false);
  bmstr_destroy(jsonrd->landmark);
  rbtree_destroy(jsonrd->prior_states);
  memset(jsonrd, 0, sizeof(JSONRD_T));
  free(jsonrd);
}

/*****************************************************************************
 * Updates the JSON reader with a chunk of text to parse.
 ****************************************************************************/
void jsonrd_update(JSONRD_T *jsonrd, const char *chunk, size_t size, bool end) {
  assert(jsonrd != NULL);
  assert(size >= 0);
  int offset, consumed;
  PS_EN prior_state;
  rbtree_clear(jsonrd->prior_states);
  if (size > 0) {
    assert(chunk != NULL);
    offset = 0;
    while (offset < size) {
      prior_state = jsonrd->state;
      consumed = dispatch(jsonrd, chunk+offset, size - offset);
      offset += consumed;
      // check for parser problems
      if (loop_check(jsonrd, prior_state, consumed)) die("Infinite loop detected.");
      assert(offset <= size);
    }
  }
  if (end) {
    if (jsonrd->state != PS_FIND_LANDMARK && jsonrd->state != PS_ERROR) {
      error(jsonrd, "Missing end of data");
    }
  }
}

