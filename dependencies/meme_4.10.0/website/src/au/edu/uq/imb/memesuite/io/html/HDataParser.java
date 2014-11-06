package au.edu.uq.imb.memesuite.io.html;

import au.edu.uq.imb.memesuite.io.json.JsonParser;
import au.edu.uq.imb.memesuite.util.BMString;

import java.io.File;
import java.io.FileInputStream;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.io.IOException;
import java.io.Reader;
import java.nio.BufferUnderflowException;
import java.nio.CharBuffer;
import java.util.TreeMap;
import java.util.regex.*;

public class HDataParser {
  // tags
  private TreeMap<String,TagInfo> tags;
  // HTML states
  private final ReadyState READY = new ReadyState();
  private final TagNameState TAG_NAME = new TagNameState();
  private final InTagState IN_TAG = new InTagState();
  private final AttrNameState ATTR_NAME = new AttrNameState();
  private final AttrEqualsState ATTR_EQUALS = new AttrEqualsState();
  private final AttrValueState ATTR_VALUE = new AttrValueState();
  private final SingleQuoteState SINGLE_QUOTE = new SingleQuoteState();
  private final DoubleQuoteState DOUBLE_QUOTE = new DoubleQuoteState();
  private final NoQuoteState NO_QUOTE = new NoQuoteState();
  private final SkipState SKIP = new SkipState();
  private final CommentState COMMENT = new CommentState();
  private final ScriptState SCRIPT = new ScriptState();
  private final VarNameState VAR_NAME = new VarNameState();
  private final FindStartobjState FIND_STARTOBJ = new FindStartobjState();
  private final JsonState JSON = new JsonState();
  // value handlers
  private final InputType VALUE_INPUT_TYPE = new InputType();
  private final InputName VALUE_INPUT_NAME = new InputName();
  private final InputValue VALUE_INPUT_VALUE = new InputValue();
  // state
  private HTMLState state;
  private ValueHandler valueHandler;
  private TagInfo tag;
  private boolean leadingSlash;
  private boolean trailingSlash;
  private boolean escapeChar;
  private boolean isAttrStart;
  private String attrName;
  private StringBuilder store;
  // attribute vars
  private boolean inputTypeIsHidden;
  private String inputName;
  private String inputValue;
  // json vars
  private String jsonVarName;
  private JsonParser jsonData;
  //handlers
  private HDataHandler handler;

  private static String[] tagNames = {
    "a", "abbr", "acronym", "address", "applet", "area", "article", "aside", "audio", "b", "base",
    "basefont", "bdi", "bdo", "big", "blockquote", "body", "br", "button", "canvas", "caption",
    "center", "cite", "code", "col", "colgroup", "command", "datalist", "dd", "del", "details", "dfn", "dialog", "dir",
    "div", "dl", "dt", "em", "embed", "fieldset", "figcaption", "figure", "font", "footer", "form", "frame", "frameset",
    "h1", "h2", "h3", "h4", "h5", "h6", "head", "header", "hgroup", "hr", "html", "i", "iframe",
    "img", "ins", "isindex", "kbd", "keygen", "label", "legend", "li", "link", "main", "map", "mark",
    "menu", "meta", "meter", "nav", "noframes", "noscript", "object", "ol", "optgroup",
    "option", "output", "p", "param", "pre", "progress", "q", "rp", "rt", "ruby", "s", "samp", "section", "select", "small",
    "source", "span", "strike", "strong", "sub", "summary", "sup", "table", "tbody", "td",
    "textarea", "tfoot", "th", "thead", "time", "title", "tr", "track", "tt", "u", "ul",
    "var", "video", "wbr", "xmp"
  };
  private static final int MAX_TAG_LEN;

  static {
    int len = 0;
    for (int i = 0; i < tagNames.length; i++) {
      if (tagNames[i].length() > len) len = tagNames[i].length();
    }
    MAX_TAG_LEN = len + 1;
  }

  /**
   * Construct a parser to extract data from a HTML file.
   * It can extract hidden input fields and marked JSON sections.
   */
  public HDataParser() {
    // tag lookup
    this.tags = new TreeMap<String,TagInfo>(String.CASE_INSENSITIVE_ORDER);
    for (String tagName : tagNames) {
      this.tags.put(tagName, new TagInfo(tagName, false, false, false));
    }
    this.tags.put("script", new TagInfo("script", true, false, true));
    this.tags.put("style", new TagInfo("style", true, false, false));
    this.tags.put("input", new TagInfo("input", false, true, false));
    // state
    this.state = this.READY;
    this.valueHandler = null;
    this.tag = null;
    this.leadingSlash = false;
    this.trailingSlash = false;
    this.escapeChar = false;
    this.isAttrStart = false;
    this.escapeChar = false;
    this.attrName = null;
    this.store = new StringBuilder();
    // attribute vars
    this.inputTypeIsHidden = false;
    this.inputName = null;
    this.inputValue = null;
    // json vars
    this.jsonVarName = null;
    this.jsonData = new JsonParser();
    // handler
    this.handler = new HDataHandlerAdapter();
    this.jsonData.setHandler(this.handler);
  }

  /**
   *  Set the callbacks.
   *  @param handler the object implementing the callbacks.
   */
  public void setHandler(HDataHandler handler) {
    this.handler = (handler == null ? new HDataHandlerAdapter() : handler);
    this.jsonData.setHandler(this.handler);
  }

  /**
   * Process the content of the buffer.
   * @param buf The buffer to process.
   */
  public void process(CharBuffer buf) {
    HTMLState beforeState;
    while (buf.hasRemaining()) {
      beforeState = this.state;
      this.state.process(buf);
      if (this.state == beforeState) break;
    }
  }

  /**
   * Parse the content of the reader.
   * @param in The reader to parse.
   * @throws IOException if an IO error occurs.
   */
  public void parse(Reader in) throws IOException {
    CharBuffer buf = CharBuffer.allocate(4096);
    try {
      while (in.read(buf) != -1) {
        buf.flip();
        this.process(buf);
        buf.compact();
      }
      //System.out.println();
      in.close();
      in = null;
    } finally {
      if (in != null) {
        try {
          in.close();
        } catch (IOException e) {
          //ignore
        }
      }
    }
  }

  /**
   * Parse the content of the input stream and extract the data sections.
   * @param input The input stream to be parsed. The encoding is assumed
   * to be UTF-8.
   * @throws IOException if an IO error occurs.
   */
  public void parse(InputStream input) throws IOException {
    this.parse(new InputStreamReader(input, "UTF-8"));
  }

  /**
   * Parse the content of the input file and extract the data sections.
   * @param inputFile the input file to be parsed. The encoding is assumed
   * to be UTF-8.
   * @throws IOException if an IO error occurs.
   */
  public void parse(File inputFile) throws IOException {
    this.parse(new FileInputStream(inputFile));
  }

  /**
   * Parse the content of the input file and extract the data sections.
   * @param inputFileName the name of the input file to be parsed. The 
   * encoding is assumed to be UTF-8.
   * @throws IOException if an IO error occurs.
   */
  public void parse(String inputFileName) throws IOException {
    this.parse(new FileInputStream(inputFileName));
  }

  public static void parse(Reader inReader, HDataHandler handler) throws IOException {
    HDataParser parser = new HDataParser();
    parser.setHandler(handler);
    parser.parse(inReader);
  }

  public static void parse(InputStream inStream, HDataHandler handler) throws IOException {
    HDataParser parser = new HDataParser();
    parser.setHandler(handler);
    parser.parse(inStream);
  }

  public static void parse(File inFile, HDataHandler handler) throws IOException {
    HDataParser parser = new HDataParser();
    parser.setHandler(handler);
    parser.parse(inFile);
  }

  public static void parse(String inFilename, HDataHandler handler) throws IOException {
    HDataParser parser = new HDataParser();
    parser.setHandler(handler);
    parser.parse(inFilename);
  }

  /*
   * Fire the value handler callbacks for an attribute without
   * a stated value. The value is assumed to be the attribute name.
   */
  private void emptyAttributeValue() {
    if (this.valueHandler != null) {
      this.valueHandler.start();
      this.valueHandler.process(CharBuffer.wrap(this.attrName));
      this.valueHandler.end();
    }
  }

  /*
   * Enumeration of tag open/close status
   */
  private static enum TagType {
    OPEN,
    CLOSE,
    SELF_CLOSE
  }

  /*
   * Store stats about a tag.
   */
  private static class TagInfo {
    private String name;
    private boolean skip;
    private boolean input;
    private boolean script;
    private TagType lastSeen;
    private int opened;
    private int closed;
    private int selfClosed;
    private BMString closePattern;

    public TagInfo(String name, boolean skip, boolean input, boolean script) {
      this.name = name;
      this.skip = skip;
      this.input = input;
      this.script = script;
      this.lastSeen = null;
      this.opened = 0;
      this.closed = 0;
      this.selfClosed = 0;
      this.closePattern = null;
    }
    public String getName() {
      return this.name;
    }
    public boolean hasHTMLContent() {
      return !skip;
    }
    public boolean isInputTag() {
      return input;
    }
    public boolean isScriptTag() {
      return script;
    }
    public TagType getLastSeen() {
      return lastSeen;
    }
    public void lastOpen() {
      this.lastSeen = TagType.OPEN;
      this.opened++;
    }
    public void lastClose() {
      this.lastSeen = TagType.CLOSE;
      this.closed++;
    }
    public void lastSelfClose() {
      this.lastSeen = TagType.SELF_CLOSE;
      this.selfClosed++;
    }
    public BMString getCloseString() {
      if (this.closePattern == null) {
        this.closePattern = new BMString("</" + this.name, true);
      }
      return this.closePattern;
    }
  }

  /*
   * Process the input data under the constraints defined by the state.
   */
  private interface HTMLState {
    void process(CharBuffer data);
  }

  /*
   * Search for a tag.
   */
  private class ReadyState implements HTMLState {
    public void process(CharBuffer data) {
      if (!data.hasRemaining()) throw new BufferUnderflowException();
      //look for the start of a tag
      while (data.hasRemaining()) {
        if (data.get() == '<') {
          HDataParser.this.state = HDataParser.this.TAG_NAME;
          return;
        }
      }
    }
  }

  /*
   * Read a tag name
   */
  private class TagNameState implements HTMLState {
    public void process(CharBuffer data) {
      if (!data.hasRemaining()) throw new BufferUnderflowException();
      // check for a comment
      if (data.remaining() >= 3 && data.charAt(0) == '!' && 
          data.charAt(1) == '-' && data.charAt(2) == '-') {
        data.position(data.position() + 3);
        HDataParser.this.state = HDataParser.this.COMMENT;
        return;
      }
      // see how many of the leading characters might be part of a tag name
      int check = Math.min(MAX_TAG_LEN, data.remaining());
      int i;
    outer:
      for (i = 0; i < check; i++) {
        switch (data.charAt(i)) {
          case '<':
          case '>':
          case ' ':
          case '\f':
          case '\n':
          case '\r':
          case '\t':
          case '\013'://vertical tab
            break outer;
        }
      }
      // check if we found anything
      if (i == MAX_TAG_LEN) {
        // we have enough data to know for sure so its not a tag, 
        // maybe it's some formula or something?
        // Consume everything, and go back to the ready state.
        data.position(data.limit());
        HDataParser.this.state = HDataParser.this.READY;
        return;
      } else if (i == check) {
        // try for more data
        return;
      }
      int start = 0;
      int end = i;
      // check for leading or trailing slashes
      if (start < end && data.charAt(start) == '/') {
        HDataParser.this.leadingSlash = true;
        start++;
      }
      if (start < end && data.charAt(end-1) == '/') {
        end--;
      }
      // grab the tag name
      String tagName = data.subSequence(start, end).toString();
      //skip over the data we've used
      data.position(data.position() + end);
      // look up the tag
      HDataParser.this.tag = HDataParser.this.tags.get(tagName);
      if (HDataParser.this.tag == null) {
        HDataParser.this.state = HDataParser.this.READY;
      } else {
        HDataParser.this.state = HDataParser.this.IN_TAG;
      }
    }
  }

  /*
   * Read all attributes in a tag
   */
  private class InTagState implements HTMLState {
    public void process(CharBuffer data) {
      if (!data.hasRemaining()) throw new BufferUnderflowException();
      while (true) {
        // skip whitespace
        while (data.hasRemaining()) {
          if (!Character.isWhitespace(data.get())) {
            data.position(data.position()-1);
            break;
          }
        }
        if (!data.hasRemaining()) return; // all space, need more data
        char c = data.get();
        if (c == '/') {
          HDataParser.this.trailingSlash = true;
          continue;
        } else if (c == '>' || c == '<') {
          if (c == '<') data.position(data.position()-1);
          // update the tallies
          if (HDataParser.this.leadingSlash) {
            HDataParser.this.tag.lastClose();
          } else if (HDataParser.this.trailingSlash) {
            HDataParser.this.tag.lastSelfClose();
          } else {
            HDataParser.this.tag.lastOpen();
          }
          // check for required values for callback
          if (HDataParser.this.tag.isInputTag() &&
              HDataParser.this.inputTypeIsHidden &&
              HDataParser.this.inputName != null &&
              HDataParser.this.inputValue != null) {
            //hidden input callback
            HDataParser.this.handler.htmlHiddenData(HDataParser.this.inputName,
                HDataParser.this.inputValue);
          }
          // reset vars
          HDataParser.this.leadingSlash = false;
          HDataParser.this.trailingSlash = false;
          HDataParser.this.inputName = null;
          HDataParser.this.inputValue = null;
          HDataParser.this.inputTypeIsHidden = false;
          // check if we need to ignore stuff
          TagInfo tag = HDataParser.this.tag;
          if (tag.getLastSeen() == TagType.OPEN && !tag.hasHTMLContent()) {
            if (tag.isScriptTag()) {
              HDataParser.this.state = HDataParser.this.SCRIPT;
              //System.out.println("State SCRIPT");
            } else {
              HDataParser.this.state = HDataParser.this.SKIP;
              //System.out.println("State SKIP");
            }
          } else {
            HDataParser.this.tag = null;
            HDataParser.this.state = HDataParser.this.READY;
            //System.out.println("State READY");
          }
        } else {
          data.position(data.position()-1);
          HDataParser.this.trailingSlash = false;
          HDataParser.this.isAttrStart = true;
          HDataParser.this.state = HDataParser.this.ATTR_NAME;
          //System.out.println("State ATTR_NAME");
        }
        break;
      }
    }
  }

  /*
   * Search for an attribute name
   */
  private class AttrNameState implements HTMLState {
    private Pattern attrNamePattern = Pattern.compile(
        "^([^=<>/\\s]{0,6})[^=<>/\\s]*([=<>/\\s])?");
    public void process(CharBuffer data) {
      if (!data.hasRemaining()) throw new BufferUnderflowException();
      if (HDataParser.this.tag.isInputTag() && data.remaining() <= 5) {
        // Insufficient data to detect all the key attributes: "type", "name"
        // and "value" so we must get more data first.
        return;
      }
      HDataParser.this.valueHandler = null;
      Matcher m = attrNamePattern.matcher(data);
      if (!m.find()) throw new Error("Impossible!");
      if (HDataParser.this.tag.isInputTag() && HDataParser.this.isAttrStart) {
        HDataParser.this.attrName = m.group(1);
        if (HDataParser.this.attrName.equalsIgnoreCase("name")) {
          HDataParser.this.valueHandler = HDataParser.this.VALUE_INPUT_NAME;
        } else if (HDataParser.this.attrName.equalsIgnoreCase("type")) {
          HDataParser.this.valueHandler = HDataParser.this.VALUE_INPUT_TYPE;
        } else if (HDataParser.this.attrName.equalsIgnoreCase("value")) {
          HDataParser.this.valueHandler = HDataParser.this.VALUE_INPUT_VALUE;
        }
      }
      String after = m.group(2);
      if (after == null) {
        // Attribute continues to end of buffer.
        // As we previously ensured that we had at least 6 remaining then it
        // can't be one of the attributes we're interested in.
        HDataParser.this.isAttrStart = false;
        data.position(data.limit());
      } else {
        data.position(data.position() + m.start(2));
        HDataParser.this.state = HDataParser.this.ATTR_EQUALS;
        //System.out.println("State ATTR_EQUALS");
      }
    }
  }

  /*
   * Look for the equals sign between a attribute name and a attribute value.
   */
  private class AttrEqualsState implements HTMLState {
    public void process(CharBuffer data) {
      if (!data.hasRemaining()) throw new BufferUnderflowException();
      // skip whitespace
      while (data.hasRemaining()) {
        if (!Character.isWhitespace(data.get())) {
          data.position(data.position()-1);//unget
          break;
        }
      }
      if (!data.hasRemaining()) return; // all space, need more data
      char c = data.get();
      if (c == '=') {
        HDataParser.this.state = HDataParser.this.ATTR_VALUE;
        //System.out.println("State ATTR_VALUE");
      } else {
        data.position(data.position()-1);//unget
        // No equals sign, so value is equal to the attribute name
        HDataParser.this.emptyAttributeValue();
        // go back to looking for attributes
        HDataParser.this.state = HDataParser.this.IN_TAG;
        //System.out.println("State IN_TAG");
      }
    }
  }

  /*
   * An interface that defines how values are parsed.
   */
  private interface ValueHandler {
    void start();
    void process(CharBuffer data);
    void end();
  }

  /*
   * Check that the value of the type field is "hidden"
   */
  private class InputType implements ValueHandler {
    private static final String HIDDEN = "hidden";
    private int pos;
    private boolean mismatch;
    public void start() {
      this.pos = 0;
      this.mismatch = false;
    }
    public void process(CharBuffer data) {
      if (this.mismatch) return;
      if ((this.pos + data.remaining()) > HIDDEN.length()) {
        this.mismatch = true;
        return;
      }
      while (data.hasRemaining()) {
        if (Character.toLowerCase(data.get()) != HIDDEN.charAt(this.pos++)) {
          this.mismatch = true;
          return;
        }
      }
    }
    public void end() {
      if (this.pos != HIDDEN.length()) this.mismatch = true;
      HDataParser.this.inputTypeIsHidden = !this.mismatch;
    }
  }

  /*
   * Slurp up the name attribute and set the inputName variable.
   */
  private class InputName implements ValueHandler {
    public void start() {
      HDataParser.this.store.setLength(0);
    }
    public void process(CharBuffer data) {
      HDataParser.this.store.append(data);
    }
    public void end() {
      HDataParser.this.inputName = HDataParser.this.store.toString();
      HDataParser.this.store.setLength(0);
    }
  }

  /*
   * Slurp up the value attribute and set the inputValue variable
   */
  private class InputValue implements ValueHandler {
    public void start() {
      HDataParser.this.store.setLength(0);
    }
    public void process(CharBuffer data) {
      HDataParser.this.store.append(data);
    }
    public void end() {
      HDataParser.this.inputValue = HDataParser.this.store.toString();
      HDataParser.this.store.setLength(0);
    }
  }


  /*
   * Look for the begining of an attribute value and determine how it
   * is quoted.
   */
  private class AttrValueState implements HTMLState {
    public void process(CharBuffer data) {
      if (!data.hasRemaining()) throw new BufferUnderflowException();
      // skip whitespace
      // Only allow this mode if the next item is quoted, otherwise assume
      // they meant no value
      boolean gap = Character.isWhitespace(data.charAt(0));
      while (data.hasRemaining()) {
        if (!Character.isWhitespace(data.get())) {
          data.position(data.position()-1);//unget
          break;
        }
      }
      if (!data.hasRemaining()) return; // all space, need more data
      char c = data.get();
      switch (c) {
        case '<':
        case '/':
        case '>':// attribute didn't have a value after all
          data.position(data.position()-1);//unget
          HDataParser.this.emptyAttributeValue();
          HDataParser.this.state = HDataParser.this.IN_TAG;
          //System.out.println("State IN_TAG");
          break;
        case '"':
          HDataParser.this.escapeChar = false;
          HDataParser.this.state = HDataParser.this.DOUBLE_QUOTE;
          //System.out.println("State DOUBLE_QUOTE");
          if (HDataParser.this.valueHandler != null) {
            HDataParser.this.valueHandler.start();
          }
          break;
        case '\'':
          HDataParser.this.escapeChar = false;
          HDataParser.this.state = HDataParser.this.SINGLE_QUOTE;
          //System.out.println("State SINGLE_QUOTE");
          if (HDataParser.this.valueHandler != null) {
            HDataParser.this.valueHandler.start();
          }
          break;
        default:
          data.position(data.position()-1);//unget
          if (gap) { 
            // not quoted so assume that it's actually another
            // attribute and hence the previous attribute gets empty
            // string as its value
            HDataParser.this.emptyAttributeValue();
            HDataParser.this.state = HDataParser.this.IN_TAG;
            //System.out.println("State IN_TAG");
          } else {
            HDataParser.this.escapeChar = false;
            HDataParser.this.state = HDataParser.this.NO_QUOTE;
            //System.out.println("State NO_QUOTE");
            if (HDataParser.this.valueHandler != null) {
              HDataParser.this.valueHandler.start();
            }
          }
      }
    }
  }

  /*
   * Read a quoted value until the end as delineated by the
   * isQuote function. Allows the quote character to be escaped
   * by backslash. Passes any quoted content to the callbacks
   * defined to handle it.
   */
  private abstract class QuoteState implements HTMLState {
    public abstract boolean isQuote(char c);
    public void process(CharBuffer data) {
      if (!data.hasRemaining()) throw new BufferUnderflowException();
      boolean foundClosingQuote = false;
      int start = data.position();
      while (data.hasRemaining()) {
        char c = data.get();
        if (HDataParser.this.escapeChar) {
          HDataParser.this.escapeChar = false;
        } else {
          if (c == '\\') {
            HDataParser.this.escapeChar = true;
          } else if (this.isQuote(c)) {
            foundClosingQuote = true;
            break;
          }
        }
      }
      if (HDataParser.this.valueHandler != null) {
        int end = data.position();
        int limit = data.limit();
        data.position(start).limit(end - (foundClosingQuote ? 1 : 0));
        HDataParser.this.valueHandler.process(data.slice());
        data.limit(limit).position(end);
      }
      if (foundClosingQuote) {
        if (HDataParser.this.valueHandler != null) {
          HDataParser.this.valueHandler.end();
        }
        HDataParser.this.state = HDataParser.this.IN_TAG;
        //System.out.println("State IN_TAG");
      }
    }
  }

  /*
   * Read a single quote value
   */
  private class SingleQuoteState extends QuoteState {
    public boolean isQuote(char c) {
      return c == '\'';
    }
  }


  /*
   * Read a double quote value
   */
  private class DoubleQuoteState extends QuoteState {
    public boolean isQuote(char c) {
      return c == '"';
    }
  }

  /*
   * Read a space delineated value
   */
  private class NoQuoteState extends QuoteState {
    public boolean isQuote(char c) {
      return Character.isWhitespace(c) || c == '/' || c == '>' || c == '<';
    }
  }

  /*
   * Skip until the closing tag is found.
   * This is primararly used to skip <style></style> sections.
   */
  private class SkipState implements HTMLState {
    public void process(CharBuffer data) {
      if (!data.hasRemaining()) throw new BufferUnderflowException();
      BMString closeStr = HDataParser.this.tag.getCloseString();
      while (true) {
        //System.out.println("Finding string " + closeStr.toString() + " in:\n" + data.toString());
        int pos = closeStr.indexIn(data);
        if (pos >= 0) {
          //System.out.println("Full match, skipping " + pos);
          // found a full match, skip over the preceeding data
          data.position(data.position() + pos);
          // we need to know what the next character is to be sure
          // that it's the tag we think it might be
          if (!(data.remaining() > closeStr.length())) return;
          data.position(data.position() + closeStr.length());
          char c = data.get();
          if (Character.isWhitespace(c) || c == '>' || c == '<' || c == '/') {
            data.position(data.position() - 1); //unget
            HDataParser.this.state = HDataParser.this.IN_TAG;
            //System.out.println("State IN_TAG");
            HDataParser.this.leadingSlash = true;
            HDataParser.this.trailingSlash = false;
            return;
          }
        } else {
          //System.out.println("Partial match, skipping " + (-(pos + 1)));
          // only a partial match, skip over anything that doesn't match
          data.position(data.position() + (-(pos + 1)));
          return;
        }
      }
    }
  }

  /*
   * Skip until the closing comment tag is found.
   * This is used to skip commented sections: <!-- comment here -->
   */
  private class CommentState implements HTMLState {
    private BMString commentStr = new BMString("-->", false);
    public void process(CharBuffer data) {
      if (!data.hasRemaining()) throw new BufferUnderflowException();
      int pos = commentStr.indexIn(data);
      if (pos >= 0) {
        // found a full match, skip over the preceeding data and match
        data.position(data.position() + pos + commentStr.length());
        // return to the ready state
        HDataParser.this.state = HDataParser.this.READY;
        //System.out.println("State READY");
      } else {
        // only a partial match, skip over anything that doesn't match
        data.position(data.position() + (-(pos + 1)));
      }
    }
  }

  /*
   * Script tags get special processing to look for JSON sections.
   * It is assumed that looking for the closing </script> tag is
   * sufficient for knowing where the Javascript ends.
   * No checking is done for comments or Javascript
   * syntax within the script tags so it's potentially possible
   * for this parser to get confused. As we are parsing our own 
   * HTML this shouldn't be a problem as we're not going to do wacky
   * confusing stuff with script tags within comments...
   */
  private class ScriptState implements HTMLState {
    private BMString jsonStr = new BMString("@JSON_VAR ", false);
    public void process(CharBuffer data) {
      if (!data.hasRemaining()) throw new BufferUnderflowException();
      BMString closeScriptStr = HDataParser.this.tag.getCloseString();
      while (true) {
        // look for the end of the script tag
        int closePos = closeScriptStr.indexIn(data);
        // record the limit so we can restore it later
        int limitPos = data.limit();
        // constrain jsonPos to be before the closing script tag (if it exists)
        data.limit(data.position() + (closePos < 0 ? -(closePos + 1) : closePos));
        // search for @JSON_VAR 
        int jsonPos = this.jsonStr.indexIn(data);
        // restore the limit
        data.limit(limitPos);
        // now see if we found a JSON_VAR
        if (jsonPos >= 0) {
          // found a match to the JSON_VAR
          // skip over and change to JSON parsing mode
          data.position(data.position() + jsonPos + this.jsonStr.length());
          HDataParser.this.store.setLength(0);
          HDataParser.this.state = HDataParser.this.VAR_NAME;
          //System.out.println("State VAR_NAME");
          return;
        } else if (data.remaining() < (-(jsonPos + 1) + this.jsonStr.length())) {
          // the data ends with an incomplete @JSON_VAR string and no </script string
          //skip up to the start of the incomplete @JSON_VAR string
          data.position(data.position() + -(jsonPos + 1));
          return;
        } else if (closePos >= 0) {
          // found a full match to the </script string, skip over the preceeding data
          data.position(data.position() + closePos);
          // we need to know what the next character is to be sure
          // that it's the script close tag we think it might be
          if (!(data.remaining() > closeScriptStr.length())) return;
          data.position(data.position() + closeScriptStr.length());
          char c = data.get();
          if (Character.isWhitespace(c) || c == '>' || c == '<' || c == '/') {
            data.position(data.position() - 1); //unget
            HDataParser.this.state = HDataParser.this.IN_TAG;
            //System.out.println("State IN_TAG");
            HDataParser.this.leadingSlash = true;
            HDataParser.this.trailingSlash = false;
            return;
          }
          // otherwise loop around and try again
        } else {
          // only a partial match, skip over anything that doesn't match
          data.position(data.position() + -(closePos + 1));
          return;
        }
      }
    }
  }

  /*
   * Read the name of a specially flagged JSON section within the script tags
   */
  private class VarNameState implements HTMLState {
    public void process(CharBuffer data) {
      if (!data.hasRemaining()) throw new BufferUnderflowException();
      // first remove any leading spaces (not newlines!)
      if (HDataParser.this.store.length() == 0) {
        // skip whitespace
        while (data.hasRemaining()) {
          char c = data.get();
          if (c != ' ' && c != '\t') {
            data.position(data.position()-1);//unget
            break;
          }
        }
        if (!data.hasRemaining()) return; // all space, need more data
      }
      // append to the store
      while (data.hasRemaining()) {
        char c = data.get();
        if (Character.isWhitespace(c)) {
          HDataParser.this.jsonVarName = HDataParser.this.store.toString();
          HDataParser.this.state = HDataParser.this.FIND_STARTOBJ;
          //System.out.println("State FIND_STARTOBJ");
          break;
        }
        HDataParser.this.store.append(c);
      }
    }
  }

  /*
   * Find the start of the JSON
   */
  private class FindStartobjState implements HTMLState {
    public void process(CharBuffer data) {
      if (!data.hasRemaining()) throw new BufferUnderflowException();
      // skip until first '{' which we assume is the start of the JSON data
      while (data.hasRemaining()) {
        if (data.get() == '{') {
          data.position(data.position() - 1); // unget
          HDataParser.this.state = HDataParser.this.JSON;
          //System.out.println("State JSON");
          HDataParser.this.jsonData.begin(HDataParser.this.jsonVarName);
          break;
        }
      }
    }
  }

  /*
   * Process the JSON
   */
  private class JsonState implements HTMLState {
    public void process(CharBuffer data) {
      if (!HDataParser.this.jsonData.process(data)) {
        // either end of JSON or error in JSON
        HDataParser.this.state = HDataParser.this.SCRIPT;
        //System.out.println("State SCRIPT");
      }
    }
  }

}
