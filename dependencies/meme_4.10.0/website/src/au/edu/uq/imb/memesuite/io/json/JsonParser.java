package au.edu.uq.imb.memesuite.io.json;

import java.nio.CharBuffer;
import java.nio.BufferUnderflowException;
import java.util.ArrayDeque;
import java.util.Deque;
import java.util.regex.Pattern;
import java.util.regex.Matcher;

public class JsonParser {
  private String name;
  private Tokenizer tokenizer;
  private JsonState state;
  private Deque<JsonState> stateStack;
  private JsonHandler handler;
  private StringBuilder store;
  // Tokenizer states
  private final GeneralTokenizer GENERAL = new GeneralTokenizer();
  private final NumberTokenizer NUMBER = new NumberTokenizer();
  private final StringTokenizerMain STRING = new StringTokenizerMain();
  private final StringTokenizerEscape STRING_ESCAPE = new StringTokenizerEscape();
  private final StringTokenizerHex STRING_HEX = new StringTokenizerHex();
  // JSON states
  private final IdleState IDLE = new IdleState();
  private final ErrorState ERROR = new ErrorState();
  private final StartobjState STARTOBJ = new StartobjState();
  private final PropertyOrEndobjState PROPERTY_OR_ENDOBJ = new PropertyOrEndobjState();
  private final ColonState COLON = new ColonState();
  private final PropertyValueState PROPERTY_VALUE = new PropertyValueState();
  private final CommaOrEndobjState COMMA_OR_ENDOBJ = new CommaOrEndobjState();
  private final PropertyState PROPERTY = new PropertyState();
  private final ValueOrEndlstState VALUE_OR_ENDLST = new ValueOrEndlstState();
  private final CommaOrEndlstState COMMA_OR_ENDLST = new CommaOrEndlstState();
  private final ListValueState LIST_VALUE = new ListValueState();

  /*
   * Construct a JsonParser object
   */
  public JsonParser() {
    this.name = "";
    this.tokenizer = this.GENERAL;
    this.state = this.IDLE;
    this.stateStack = new ArrayDeque<JsonState>();
    this.handler = new JsonHandlerAdapter();
    this.store = new StringBuilder();
  }

  /**
   * set a handler.
   * @param handler the event handler.
   */
  public void setHandler(JsonHandler handler) {
    this.handler = (handler == null ? new JsonHandlerAdapter() : handler);
  }

  /**
   * Resets the state ready to receive a new JSON structure.
   * @param name the name of the JSON section
   */
  public void begin(String name) {
    this.name = name;
    this.tokenizer = this.GENERAL;
    this.state = this.STARTOBJ;
    this.stateStack.clear();
  }

  /**
   * Returns true if in the idle state which only happens before anything
   * has been parsed and after a complete JSON structure has been parsed.
   * @return true if in the idle state.
   */
  public boolean isComplete() {
    return (this.state == this.IDLE);
  }

  /**
   * Returns true if in the error state.
   * @return true if in the error state.
   */
  public boolean hasError() {
    return (this.state == this.ERROR);
  }

  /**
   * Process the token and return false if the end of the JSON is reached or
   * an error state occurs, otherwise return true.
   * @param token the token to process
   * @return true if more tokens are needed or false if for some reason 
   * (either error or end of JSON structure) they are not.
   */
  public boolean process(JsonToken token) {
    this.state.process(token);
    return (this.state != this.ERROR && this.state != this.IDLE);
  }

  /**
   * Process the buffer and return false if the end of the JSON is reached or
   * an error state occurs, otherwise return true.
   */
  public boolean process(CharBuffer data) {
    Tokenizer before;
    if (this.state == this.ERROR || this.state == this.IDLE) return false;
    while (data.hasRemaining()) {
      before = this.tokenizer;
      this.tokenizer.process(data);
      if (this.tokenizer == before) break;
    }
    return (this.state != this.ERROR && this.state != this.IDLE);
  }



  /*
   * Interface for the tokenizers.
   */
  private interface Tokenizer {
    void process(CharBuffer data);
  }

  /*
   * Parse a general JSON token.
   */
  private class GeneralTokenizer implements Tokenizer {
    /*
     * Guess the token type from the first character.
     */
    private JsonType guess_type(char c) {
      switch (c) {
        case '{': return JsonType.STARTOBJ;
        case '}': return JsonType.ENDOBJ;
        case '[': return JsonType.STARTLST;
        case ']': return JsonType.ENDLST;
        case ':': return JsonType.COLON;
        case ',': return JsonType.COMMA;
        case 't': return JsonType.TRUE;
        case 'f': return JsonType.FALSE;
        case 'n': return JsonType.NULL;
        case '"': return JsonType.STRING;
        case '0': case '1': case '2': case '3': case '4': case '5': case '6':
        case '7': case '8': case '9': case '-': return JsonType.NUMBER;
        default: return JsonType.ILLEGAL;
      }
    }
    /*
     * Check if the word (followed by a seperator) exists at the
     * start of the buffer.
     */
    private boolean hasWord(CharSequence data, String word) {
      if (data.length() <= word.length()) throw new BufferUnderflowException();
      for (int i = 0; i < word.length(); i++) {
        if (data.charAt(i) != word.charAt(i)) return false;
      }
      char c = data.charAt(word.length());
      if (Character.isWhitespace(c) || c == ',' || c == ']' || c == '}') {
        return true;
      } else {
        return false;
      }
    }
    public void process(CharBuffer data) {
      while (data.hasRemaining()) {
        // skip whitespace
        while (data.hasRemaining()) {
          if (!Character.isWhitespace(data.get())) {
            data.position(data.position()-1);//unget
            break;
          }
        }
        if (!data.hasRemaining()) return;
        // guess the token type
        JsonType type = guess_type(data.charAt(0));
        String word = null;
        switch (type) {
          case ILLEGAL: case STARTOBJ: case ENDOBJ: case STARTLST:
          case ENDLST: case COLON: case COMMA: data.get(); break;
          case NULL: word = "null"; break;
          case TRUE: word = "true"; break;
          case FALSE: word = "false"; break;
          case NUMBER: JsonParser.this.tokenizer = JsonParser.this.NUMBER; return;
          case STRING: 
            data.get(); 
            JsonParser.this.store.setLength(0);
            JsonParser.this.tokenizer = JsonParser.this.STRING;
            return;
          default: throw new Error("Unknown token type");
        }
        if (word != null) {
          if (data.remaining() <= word.length()) return;
          if (!hasWord(data, word)) {
            type = JsonType.ILLEGAL;
          } else {
            // skip over the word
            data.position(data.position() + word.length());
          }
        }
        // process token and exit if no more tokens needed
        if (!JsonParser.this.process(new JsonToken(type))) return;
      }
    }
  }

  /*
   * Parse a JSON number token.
   */
  private class NumberTokenizer implements Tokenizer {
    private Pattern numberEnd = Pattern.compile("[\\s,\\]\\}]");
    private Pattern numberPattern = Pattern.compile(
        "^(-?(?:0|[1-9]\\d*)(\\.\\d+)?(?:[eE][\\+\\-]?\\d+)?)[\\s,\\]\\}]");
    public void process(CharBuffer data) {
      // look to see if there is something that could end a number in the buffer
      if (numberEnd.matcher(data).find()) {
        Matcher m = numberPattern.matcher(data);
        if (m.find()) {
          JsonToken token;
          // found the complete number! Read it!
          if (m.group(2) == null) {
            long value = Long.parseLong(m.group(1));
            token = new JsonToken(value);
          } else {
            double value = Double.parseDouble(m.group(1));
            token = new JsonToken(value);
          }
          // skip over the number
          data.position(data.position() + m.end(1));
          // set to general tokenizer
          JsonParser.this.tokenizer = JsonParser.this.GENERAL;
          // process token
          JsonParser.this.process(token);
          return;
        } 
      } else if (data.position() > 0) {
        return; // buffer not full, try for more data
      }
      // could not read a full number token so report an illegal token.
      JsonParser.this.process(new JsonToken(JsonType.ILLEGAL));
    }
  }

  /*
   * Parse a JSON String token.
   */
  private class StringTokenizerMain implements Tokenizer {
    public void process(CharBuffer data) {
      while (data.hasRemaining()) {
        char c = data.get();
        if (c == '"') {
          // set to general tokenizer
          JsonParser.this.tokenizer = JsonParser.this.GENERAL;
          // process string token
          JsonParser.this.process(new JsonToken(JsonParser.this.store.toString()));
          return;
        } else if (c == '\\') {
          JsonParser.this.tokenizer = JsonParser.this.STRING_ESCAPE;
        } else {
          JsonParser.this.store.append(c);
        }
      }
    }
  }

  /*
   * Parse an escape character within a String token.
   */
  private class StringTokenizerEscape implements Tokenizer {
    public void process(CharBuffer data) {
      if (data.hasRemaining()) {
        char c = data.get();
        char out;
        switch (c) {
          case '"': case '\\': case '/': out = c; break;
          case 'b': out = '\b'; break;
          case 'f': out = '\f'; break;
          case 'n': out = '\n'; break;
          case 'r': out = '\r'; break;
          case 't': out = '\t'; break;
          case 'u':
            JsonParser.this.tokenizer = JsonParser.this.STRING_HEX;
            return;
          default:
            // not an allowed escape character so report an illegal token
            JsonParser.this.process(new JsonToken(JsonType.ILLEGAL));
            return;
        }
        JsonParser.this.store.append(out);
        JsonParser.this.tokenizer = JsonParser.this.STRING;
      }
    }
  }

  /*
   * Parse a hexadecimal unicode sequence within a String token.
   */
  private class StringTokenizerHex implements Tokenizer {
    private Pattern hexPattern = Pattern.compile("^[a-fA-F0-9]{4}");
    public void process(CharBuffer data) {
      if (data.remaining() < 4) return; // need more data
      Matcher m = hexPattern.matcher(data);
      if (m.find()) {
        JsonParser.this.store.appendCodePoint(Integer.parseInt(m.group(), 16));
        data.position(data.position() + 4);
        JsonParser.this.tokenizer = JsonParser.this.STRING;
        return;
      }
      // could not read a full 4 hex digits so report an illegal token.
      JsonParser.this.process(new JsonToken(JsonType.ILLEGAL));
    }
  }

  /*
   * JSON parsing state
   */
  private interface JsonState {
    void process(JsonToken token);
  }

  private class IdleState implements JsonState {
    public void process(JsonToken token) {
      //do nothing
    }
  }

  private class ErrorState extends IdleState {}

  private class StartobjState implements JsonState {
    public void process(JsonToken token) {
      if (token.getType() == JsonType.STARTOBJ) {
        JsonParser.this.stateStack.addFirst(JsonParser.this.IDLE);
        JsonParser.this.state = PROPERTY_OR_ENDOBJ;
        JsonParser.this.handler.jsonStartData(JsonParser.this.name);
        JsonParser.this.handler.jsonStartObject();
      } else {
        // parsing failure!
        JsonParser.this.stateStack.addFirst(JsonParser.this.state);
        JsonParser.this.state = JsonParser.this.ERROR;
        JsonParser.this.handler.jsonError();
      }
    }
  }

  private class PropertyOrEndobjState implements JsonState {
    public void process(JsonToken token) {
      if (token.getType() == JsonType.STRING) {
        JsonParser.this.handler.jsonStartProperty(token.getString());
        JsonParser.this.state = JsonParser.this.COLON;
      } else if (token.getType() == JsonType.ENDOBJ) {
        JsonParser.this.handler.jsonEndObject();
        JsonParser.this.state = JsonParser.this.stateStack.removeFirst();
        if (JsonParser.this.state == JsonParser.this.COMMA_OR_ENDOBJ) {
          JsonParser.this.handler.jsonEndProperty();
        } else if (JsonParser.this.state == JsonParser.this.IDLE) {
          JsonParser.this.handler.jsonEndData();
        }
      } else {
        // parsing failure!
        JsonParser.this.stateStack.addFirst(JsonParser.this.state);
        JsonParser.this.state = JsonParser.this.ERROR;
        JsonParser.this.handler.jsonError();
      }
    }
  }

  private class ColonState implements JsonState {
    public void process(JsonToken token) {
      if (token.getType() == JsonType.COLON) {
        JsonParser.this.state = JsonParser.this.PROPERTY_VALUE;
      } else {
        // parsing failure!
        JsonParser.this.stateStack.addFirst(JsonParser.this.state);
        JsonParser.this.state = JsonParser.this.ERROR;
        JsonParser.this.handler.jsonError();
      }
    }
  }

  private class PropertyValueState implements JsonState {
    public void process(JsonToken token) {
      switch (token.getType()) {
        case STRING:
        case NUMBER:
        case TRUE:
        case FALSE:
        case NULL:
          JsonParser.this.state = JsonParser.this.COMMA_OR_ENDOBJ;
          JsonParser.this.handler.jsonValue(token.getValue());
          JsonParser.this.handler.jsonEndProperty();
          break;
        case STARTOBJ:
          JsonParser.this.stateStack.addFirst(JsonParser.this.COMMA_OR_ENDOBJ);
          JsonParser.this.state = JsonParser.this.PROPERTY_OR_ENDOBJ;
          JsonParser.this.handler.jsonStartObject();
          break;
        case STARTLST:
          JsonParser.this.stateStack.addFirst(JsonParser.this.COMMA_OR_ENDOBJ);
          JsonParser.this.state = JsonParser.this.VALUE_OR_ENDLST;
          JsonParser.this.handler.jsonStartList();
          break;
        default:
          // parsing failure!
          JsonParser.this.stateStack.addFirst(JsonParser.this.state);
          JsonParser.this.state = JsonParser.this.ERROR;
          JsonParser.this.handler.jsonError();
      }
    }
  }

  private class CommaOrEndobjState implements JsonState {
    public void process(JsonToken token) {
      if (token.getType() == JsonType.COMMA) {
        JsonParser.this.state = JsonParser.this.PROPERTY;
      } else if (token.getType() == JsonType.ENDOBJ) {
        JsonParser.this.state = JsonParser.this.stateStack.removeFirst();
        JsonParser.this.handler.jsonEndObject();
        if (JsonParser.this.state == JsonParser.this.COMMA_OR_ENDOBJ) {
          JsonParser.this.handler.jsonEndProperty();
        } else if (JsonParser.this.state == JsonParser.this.IDLE) {
          JsonParser.this.handler.jsonEndData();
        }
      } else {
        // parsing failure!
        JsonParser.this.stateStack.addFirst(JsonParser.this.state);
        JsonParser.this.state = JsonParser.this.ERROR;
        JsonParser.this.handler.jsonError();
      }
    }
  }

  private class PropertyState implements JsonState {
    public void process(JsonToken token) {
      if (token.getType() == JsonType.STRING) {
        JsonParser.this.state = JsonParser.this.COLON;
        JsonParser.this.handler.jsonStartProperty(token.getString());
      } else {
        // parsing failure!
        JsonParser.this.stateStack.addFirst(JsonParser.this.state);
        JsonParser.this.state = JsonParser.this.ERROR;
        JsonParser.this.handler.jsonError();
      }
    }
  }

  private class ValueOrEndlstState implements JsonState {
    public void process(JsonToken token) {
      switch (token.getType()) {
        case STRING:
        case NUMBER:
        case TRUE:
        case FALSE:
        case NULL:
          JsonParser.this.state = JsonParser.this.COMMA_OR_ENDLST;
          JsonParser.this.handler.jsonValue(token.getValue());
          break;
        case STARTOBJ:
          JsonParser.this.stateStack.addFirst(JsonParser.this.COMMA_OR_ENDLST);
          JsonParser.this.state = JsonParser.this.PROPERTY_OR_ENDOBJ;
          JsonParser.this.handler.jsonStartObject();
          break;
        case STARTLST:
          JsonParser.this.stateStack.addFirst(JsonParser.this.COMMA_OR_ENDLST);
          JsonParser.this.state = JsonParser.this.VALUE_OR_ENDLST;
          JsonParser.this.handler.jsonStartList();
          break;
        case ENDLST:
          JsonParser.this.state = JsonParser.this.stateStack.removeFirst();
          JsonParser.this.handler.jsonEndList();
          if (JsonParser.this.state == JsonParser.this.COMMA_OR_ENDOBJ) {
            JsonParser.this.handler.jsonEndProperty();
          }
          break;
        default:
          // parsing failure!
          JsonParser.this.stateStack.addFirst(JsonParser.this.state);
          JsonParser.this.state = JsonParser.this.ERROR;
          JsonParser.this.handler.jsonError();
      }
    }
  }

  private class CommaOrEndlstState implements JsonState {
    public void process(JsonToken token) {
      if (token.getType() == JsonType.COMMA) {
        JsonParser.this.state = JsonParser.this.LIST_VALUE;
      } else if (token.getType() == JsonType.ENDLST) {
        JsonParser.this.state = JsonParser.this.stateStack.removeFirst();
        JsonParser.this.handler.jsonEndList();
        if (JsonParser.this.state == JsonParser.this.COMMA_OR_ENDOBJ) {
          JsonParser.this.handler.jsonEndProperty();
        }
      } else {
        // parsing failure!
        JsonParser.this.stateStack.addFirst(JsonParser.this.state);
        JsonParser.this.state = JsonParser.this.ERROR;
        JsonParser.this.handler.jsonError();
      }
    }
  }

  private class ListValueState implements JsonState {
    public void process(JsonToken token) {
      switch (token.getType()) {
        case STRING:
        case NUMBER:
        case TRUE:
        case FALSE:
        case NULL:
          JsonParser.this.state = JsonParser.this.COMMA_OR_ENDLST;
          JsonParser.this.handler.jsonValue(token.getValue());
          break;
        case STARTOBJ:
          JsonParser.this.stateStack.addFirst(JsonParser.this.COMMA_OR_ENDLST);
          JsonParser.this.state = JsonParser.this.PROPERTY_OR_ENDOBJ;
          JsonParser.this.handler.jsonStartObject();
          break;
        case STARTLST:
          JsonParser.this.stateStack.addFirst(JsonParser.this.COMMA_OR_ENDLST);
          JsonParser.this.state = JsonParser.this.VALUE_OR_ENDLST;
          JsonParser.this.handler.jsonStartList();
          break;
        default:
          // parsing failure!
          JsonParser.this.stateStack.addFirst(JsonParser.this.state);
          JsonParser.this.state = JsonParser.this.ERROR;
          JsonParser.this.handler.jsonError();
      }
    }
  }
}
