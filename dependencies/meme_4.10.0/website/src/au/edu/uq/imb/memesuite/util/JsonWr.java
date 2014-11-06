package au.edu.uq.imb.memesuite.util;

import java.io.IOException;
import java.io.StringWriter;
import java.io.Writer;
import java.math.BigInteger;
import java.util.*;
import java.util.concurrent.atomic.AtomicBoolean;
import java.util.concurrent.atomic.AtomicInteger;
import java.util.concurrent.atomic.AtomicLong;
import java.util.regex.Pattern;

/**
 * A simple state-machine based JSON writer that will write one JSON object.
 * Note that it will never attempt to close the underlying writer.
 */
public class JsonWr {
  private Writer out;
  private int minCols;
  private int tabCols;
  private int lineCols;
  private int indent;
  private int column;
  private JsonState state;
  private Deque<JsonState> stack;
  private StringWriter line;

  private static final Pattern numberPattern = Pattern.compile(
      "^-?(?:0|[1-9]\\d*)(?:\\.\\d+)?(?:[eE][+-]?\\d+)?$");

  public interface JsonValue {
    /**
     * Writes out the object as a JSON value.
     * @param out the writer to use.
     */
    public void outputJson(JsonWr out) throws IOException;
  }

  private enum JsonState {
    READY,
    EMPTY_OBJECT,
    OBJECT,
    EMPTY_ARRAY,
    SINGLE_LINE_ARRAY,
    MULTI_LINE_ARRAY,
    PROPERTY,
    DONE
  }

  /**
   * Create a JSON writer.
   * @param out the output writer.
   * @param minCols the minimum indent.
   * @param tabCols the number of columns to indent every level.
   * @param lineCols the number of columns to target per line.
   */
  public JsonWr(Writer out, int minCols, int tabCols, int lineCols) {
    if (out == null) throw new NullPointerException("Parameter out must not be null.");
    this.out = out;
    this.minCols = minCols;
    this.tabCols = tabCols;
    this.lineCols = lineCols;
    this.indent = minCols;
    this.column = 0;
    this.state = JsonState.READY;
    this.stack = new ArrayDeque<JsonState>();
    this.line = new StringWriter(lineCols);
  }

  /**
   * Create a JSON writer.
   * @param out the output writer.
   * @param minCols the minimum indent.
   */
  public JsonWr(Writer out, int minCols) {
    this(out, minCols, 2, 80);
  }

  /**
   * Create a JSON writer.
   * @param out the output writer.
   */
  public JsonWr(Writer out) {
    this(out, 0, 2, 80);
  }

  /**
   * Get the output writer.
   * @return the output writer as passed into the constructor.
   */
  public Writer getWriter() {
    return out;
  }

  /**
   * Enforces that the passed state is one of the allowed states.
   * @param state the state to check.
   * @param allowedStates the set of allowed states.
   * @throws IllegalStateException when the state is not one of the allowed states.
   */
  private static void enforceState(JsonState state, Set<JsonState> allowedStates) {
    if (!allowedStates.contains(state)) {
      throw new IllegalStateException("You can't do that!");
    }
  }

  /**
   * Write out a quoted and properly escaped, JSON string.
   * @param out the destination for the string.
   * @param str the string to write.
   * @return the number of columns written.
   * @throws IOException when the underlying writer has problems.
   */
  private static int writeString(Writer out, CharSequence str) throws IOException {
    int cols = 2;
    int start = 0;
    out.append('"');
    for (int offset = 0; offset < str.length();) {
      String substitute = null;
      final int codepoint = Character.codePointAt(str, offset);
      switch (codepoint) {
        case 8: //backspace
          substitute = "\\b";
          cols += 2;
          break;
        case 9: // tab
          substitute = "\\t";
          cols += 2;
          break;
        case 10: // line-feed (newline)
          substitute = "\\n";
          cols += 2;
          break;
        case 12: // form-feed
          substitute = "\\f";
          cols += 2;
          break;
        case 13: // carriage return
          substitute = "\\r";
          cols += 2;
          break;
        case 34: // double quote (quotation mark)
          substitute = "\\\"";
          cols += 2;
          break;
        case 47: // slash (solidus)
          substitute = "\\/";
          cols += 2;
          break;
        case 92: // backslash (reverse solidus)
          substitute = "\\\\";
          cols += 2;
          break;
        default:
          // check if a control character
          // or if line separator (U+2028) or if paragraph separator (U+2029)
          // the latter two are valid JSON but not valid Javascript as Javascript
          // can't have unescaped newline characters in a string.
          if (codepoint <= 0x1F || (codepoint >= 0x7F && codepoint <= 0x9F) ||
              codepoint == 0x2028 || codepoint == 0x2029) {
            substitute = String.format("\\u%.04x", codepoint);
            cols += 6;
          } else {
            cols++;
          }
      }
      if (substitute != null) {
        out.append(str, start, offset);
        out.append(substitute);
        start = (offset += Character.charCount(codepoint));
      } else {
        offset += Character.charCount(codepoint);
      }
    }
    if (start < str.length()) {
      out.append(str, start, str.length());
    }
    out.append('"');
    return cols;
  }

  /**
   * Writes a new line followed by enough spaces to match the indent.
   * @throws IOException if a writing error occurs.
   */
  private void writeNlIndent() throws IOException {
    out.append('\n');
    for (int i = 0; i < indent; i++) out.append(' ');
    column = indent;
  }

  /**
   * The set of states that are allowed to write values.
   */
  private static final Set<JsonState> ALLOWED_VALUE_STATES =
      Collections.unmodifiableSet(EnumSet.of(JsonState.PROPERTY,
          JsonState.EMPTY_ARRAY, JsonState.SINGLE_LINE_ARRAY,
          JsonState.MULTI_LINE_ARRAY));

  /**
   * Sets up everything required to write a value
   * and returns the Writer that should be used to write the value.
   * @param valueLen the length of the value to be written.
   * @return the writer to use to write the value
   * @throws IOException when something goes wrong with one of the writers.
   */
  private Writer writeValueSetup(int valueLen) throws IOException{
    enforceState(state, ALLOWED_VALUE_STATES);
    if (state == JsonState.EMPTY_ARRAY) {
      if ((indent + 1 + valueLen + 2) < lineCols) {
        // clear the line buffer
        line.getBuffer().setLength(0);
        state = JsonState.SINGLE_LINE_ARRAY;
        return line;
      } else {
        out.append('[');
        column += 1;
        writeNlIndent();
      }
    } else if (state == JsonState.SINGLE_LINE_ARRAY) {
      int lineLen = line.getBuffer().length();
      if ((indent + 1 + lineLen + 2 + valueLen + 2) < lineCols) {
        line.append(", ");
        return line;
      } else {
        out.append('[');
        column++;
        writeNlIndent();
        out.append(line.getBuffer());
        column += lineLen;
        state = JsonState.MULTI_LINE_ARRAY;
      }
    }
    if (state == JsonState.MULTI_LINE_ARRAY) {
      out.append(',');
      column++;
      if ((column + 1 + valueLen + 2) < lineCols) {
        out.append(' ');
        column++;
      } else {
        writeNlIndent();
      }
    }
    column += valueLen;
    if (state == JsonState.PROPERTY) {
      state = stack.pop();
    } else {
      state = JsonState.MULTI_LINE_ARRAY;
    }
    return out;
  }

  /**
   * Write out a value that has been converted to a string.
   * @param valueStr the string version of the value.
   * @throws IOException if an output error occurs.
   */
  private void writeValue(CharSequence valueStr) throws IOException{
    Writer valueOut = writeValueSetup(valueStr.length());
    valueOut.append(valueStr);
  }

  /**
   * Write out the start of a new object or array.
   * @param newState either object or array state.
   * @throws IOException if an output error occurs.
   */
  private void writeStart(JsonState newState) throws IOException{
    enforceState(state, ALLOWED_VALUE_STATES);
    if (state != JsonState.PROPERTY) {
      if (state != JsonState.MULTI_LINE_ARRAY) {
        out.append('[');
        column++;
        writeNlIndent();
      }
      if (state == JsonState.SINGLE_LINE_ARRAY) {
        out.append(line.getBuffer());
        column += line.getBuffer().length();
      }
      if (state != JsonState.EMPTY_ARRAY) {
        out.append(", ");
        column += 2;
      }
      stack.push(JsonState.MULTI_LINE_ARRAY);
      if ((column + 1) >= lineCols) writeNlIndent();
    }
    state = newState;
    indent += tabCols;
  }

  /**
   * The set of states that are allowed to begin writing.
   */
  private static final Set<JsonState> ALLOWED_BEGIN_STATES =
      Collections.unmodifiableSet(EnumSet.of(JsonState.READY));

  /**
   * Begin writing the outermost object.
   * @throws IOException if an output error occurs.
   */
  public void start() throws IOException {
    enforceState(state, ALLOWED_BEGIN_STATES);
    out.append('{');
    indent = minCols + tabCols;
    column = minCols;
    state = JsonState.EMPTY_OBJECT;
    stack.push(JsonState.DONE);
  }

  /**
   * Finishes writing the outermost object
   * @throws IOException if an output error occurs.
   */
  public void end() throws IOException {
    if ((state != JsonState.EMPTY_OBJECT && state != JsonState.OBJECT) || stack.peek() != JsonState.DONE) {
      throw new IllegalStateException("Not in correct state to end JSON object");
    }
    indent -= tabCols;
    if (state != JsonState.EMPTY_OBJECT) writeNlIndent();
    out.append('}');
    column++;
    state = stack.pop();
  }

  /**
   * The set of states that are allowed to add properties.
   */
  private Set<JsonState> ALLOWED_PROPERTY_STATES =
      Collections.unmodifiableSet(EnumSet.of(JsonState.EMPTY_OBJECT, JsonState.OBJECT));

  /**
   * Write out a property.
   * @param property the property name.
   * @throws IOException if an output error occurs.
   */
  public void property(CharSequence property) throws IOException {
    if (property == null) throw new NullPointerException("Property should not be null");
    enforceState(state, ALLOWED_PROPERTY_STATES);
    if (state != JsonState.EMPTY_OBJECT) out.append(',');
    writeNlIndent();
    column += writeString(out, property);
    out.append(": ");
    column += 2;
    stack.push(JsonState.OBJECT);
    state = JsonState.PROPERTY;
  }

  /**
   * Write out the start of an object.
   * @throws IOException if an output error occurs.
   */
  public void startObject() throws IOException {
    writeStart(JsonState.EMPTY_OBJECT);
    out.append('{');
    column++;
  }

  /**
   * The set of allowed states to end an object.
   */
  private static final Set<JsonState> ALLOWED_END_OBJECT_STATES =
      Collections.unmodifiableSet(EnumSet.of(JsonState.EMPTY_OBJECT, JsonState.OBJECT));

  /**
   * End an object.
   * @throws IOException if an output error occurs.
   */
  public void endObject() throws IOException {
    enforceState(state, ALLOWED_END_OBJECT_STATES);
    if (stack.peek() == JsonState.DONE) throw new IllegalStateException("Must call end to end JSON object");
    indent -= tabCols;
    if (state != JsonState.EMPTY_OBJECT) writeNlIndent();
    out.append('}');
    column++;
    state = stack.pop();
  }

  /**
   * Start an array.
   * @throws IOException if an output error occurs.
   */
  public void startArray() throws IOException {
    writeStart(JsonState.EMPTY_ARRAY);
    // note we don't write the [ yet
  }

  /**
   * The set of states allowed to end an array.
   */
  private static final Set<JsonState> ALLOWED_END_ARRAY_STATES =
      Collections.unmodifiableSet(EnumSet.of(JsonState.EMPTY_ARRAY,
          JsonState.SINGLE_LINE_ARRAY, JsonState.MULTI_LINE_ARRAY));

  /**
   * End an array.
   * @throws IOException if an output error occurs.
   */
  public void endArray() throws IOException {
    enforceState(state, ALLOWED_END_ARRAY_STATES);
    indent -= tabCols;
    if (state == JsonState.MULTI_LINE_ARRAY) {
      writeNlIndent();
    } else {
      int lineLen = (state == JsonState.SINGLE_LINE_ARRAY ? line.getBuffer().length() : 0);
      if ((column + 1 + lineLen + 2) >= lineCols) {
        writeNlIndent();
      }
      out.append('[');
      column++;
    }
    if (state == JsonState.SINGLE_LINE_ARRAY) {
      out.append(line.getBuffer());
      column += line.getBuffer().length();
    }
    out.append(']');
    column++;
    state = stack.pop();
  }
  /**
   * Write out a null.
   * @throws IOException if an error occurs.
   */
  public void value() throws IOException {
    writeValue("null");
  }

  /**
   * Write out a string value or null.
   * @param value the value to write.
   * @throws IOException if an error occurs.
   */
  public void value(CharSequence value) throws IOException {
    if (value == null) {
      value();
      return;
    }
    // measure the string
    int valueLen;
    try {
      valueLen = writeString(new Writer() {
        public void write(char[] chars, int i, int i2) throws IOException {}
        public void flush() throws IOException {}
        public void close() throws IOException {}
      }, value);
    } catch (IOException e) {
      throw new Error("No IO occurs, this should not happen!");
    }
    Writer valueOut = writeValueSetup(valueLen);
    writeString(valueOut, value);
  }

  /**
   * Write out a long value.
   * @param value the value to write.
   * @throws IOException if an error occurs.
   */
  public void value(long value) throws IOException {
    writeValue(Long.toString(value));
  }

  /**
   * Write out a double value.
   * @param value the value to write.
   * @throws IOException if an error occurs.
   */
  public void value(double value) throws IOException {
    if (Double.isNaN(value) || Double.isInfinite(value)) {
      throw new IllegalArgumentException("Can not represent infinity or NaN in JSON!");
    }
    writeValue(Double.toString(value));
  }

  /**
   * Write out a Number value (or null).
   * @param value the value to write.
   * @throws IOException if an error occurs.
   * @throws IllegalArgumentException when the number can not be represented in JSON (eg NaN or Inf)
   */
  public void value(Number value) throws IOException {
    if (value == null) {
      value();
      return;
    }
    String text = value.toString();
    if (numberPattern.matcher(text).matches()) {
      writeValue(text);
    } else {
      throw new IllegalArgumentException("Can not represent the Number " + text + " in JSON!");
    }
  }

  /**
   * Write out a boolean value.
   * @param value the value to write.
   * @throws IOException if an error occurs.
   */
  public void value(boolean value) throws IOException {
    writeValue(Boolean.toString(value));
  }

  /**
   * Write out a object.
   * @param value the object to write as JSON.
   * @throws IOException if an output error occurs.
   */
  public void value(JsonValue value) throws IOException {
    if (value == null) {
      writeValue("null");
      return;
    }
    value.outputJson(this);
  }

  /**
   * Write out an array of CharSequence values.
   * @param values the values to write.
   * @throws IOException if an error occurs
   */
  public void array(CharSequence[] values) throws IOException {
    startArray();
    for (CharSequence value : values) {
      value(value);
    }
    endArray();
  }

  /**
   * Write out an array of String values.
   * @param values the values to write.
   * @throws IOException if a output error occurs.
   */
  public void array(String[] values) throws IOException {
    startArray();
    for (String value : values) {
      value(value);
    }
    endArray();
  }

  /**
   * Write out an array of long values.
   * @param values the values to write.
   * @throws IOException if a output error occurs.
   */
  public void array(long[] values) throws IOException {
    startArray();
    for (long value : values) {
      value(value);
    }
    endArray();
  }

  /**
   * Write out an array of double values.
   * @param values the values to write.
   * @throws IOException if a output error occurs.
   */
  public void array(double[] values) throws IOException {
    startArray();
    for (double value : values) {
      value(value);
    }
    endArray();
  }

  /**
   * Write out an array of boolean values.
   * @param values the values to write.
   * @throws IOException if a output error occurs.
   */
  public void array(boolean[] values) throws IOException {
    startArray();
    for (boolean value : values) {
      value(value);
    }
    endArray();
  }

  /**
   * Write out an array of JsonValue objects.
   * @param values the values to write.
   * @throws IOException
   */
  public void array(JsonValue[] values) throws IOException {
    startArray();
    for (JsonValue value : values) {
      value(value);
    }
    endArray();
  }

  /**
   * Write out an array of JsonValue objects
   * @param values
   * @throws IOException
   */
  public void array(Iterable<JsonValue> values) throws IOException {
    startArray();
    for (JsonValue value: values) {
      value(value);
    }
    endArray();
  }

  /**
   * Write out all the values in an iterable converting
   * to strings when they can't be converted to one of
   * the other formats.
   * @param values the values to write.
   * @throws IOException if a output error occurs.
   */
  public void arrayAny(Iterable values) throws IOException{
    startArray();
    for (Object value : values) {
      if (value != null) {
        if (value instanceof Number) {
          if (value instanceof Long || value instanceof Integer ||
              value instanceof Short || value instanceof Byte ||
              value instanceof BigInteger || value instanceof AtomicInteger ||
              value instanceof AtomicLong) {
            value(((Number)value).longValue());
          } else {
            value(((Number)value).doubleValue());
          }
        } else if (value instanceof Boolean) {
          value((Boolean)value);
        } else if (value instanceof AtomicBoolean) {
          value(((AtomicBoolean)value).get());
        } else if (value instanceof CharSequence) {
          value((CharSequence)value);
        } else if (value instanceof JsonValue) {
          value((JsonValue)value);
        } else {
          value(value.toString());
        }
      } else {
        value((CharSequence)null);
      }
    }
    endArray();
  }

  /**
   * Write out a string valued property.
   * @param property the property name.
   * @param value the property value.
   * @throws IOException if an output error occurs.
   */
  public void property(CharSequence property, CharSequence value) throws IOException {
    property(property);
    value(value);
  }

  /**
   * Write out a long valued property.
   * @param property the property name.
   * @param value the property value.
   * @throws IOException if an output error occurs.
   */
  public void property(CharSequence property, long value) throws IOException {
    property(property);
    value(value);
  }

  public void property(CharSequence property, double value) throws IOException {
    property(property);
    value(value);
  }

  public void property(CharSequence property, Number value) throws IOException {
    property(property);
    value(value);
  }

  public void property(CharSequence property, boolean value) throws IOException {
    property(property);
    value(value);
  }

  public void property(CharSequence property, JsonValue value) throws  IOException {
    property(property);
    value(value);
  }

  public void property(CharSequence property, CharSequence[] values) throws IOException {
    property(property);
    array(values);
  }

  public void property(CharSequence property, String[] values) throws IOException {
    property(property);
    array(values);
  }

  public void property(CharSequence property, long[] values) throws IOException {
    property(property);
    array(values);
  }

  public void property(CharSequence property, double[] values) throws IOException {
    property(property);
    array(values);
  }

  public void property(CharSequence property, boolean[] values) throws IOException {
    property(property);
    array(values);
  }

  public void property(CharSequence property, JsonValue[] values) throws IOException {
    property(property);
    array(values);
  }

  public void propertyAny(CharSequence property, Iterable values) throws IOException {
    property(property);
    arrayAny(values);
  }

}
