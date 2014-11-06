package au.edu.uq.imb.memesuite.template;

import java.io.*;
import java.nio.charset.Charset;
import java.text.ParseException;
import java.util.*;
import java.util.regex.*;

/**
 * Reads from a template file and creates a tree of immutable HTMLTemplate
 * objects from which mutable HTMLSub objects can be instanced.
 *
 * The template syntax looks like this:
 * <!--{item}-->
 * Hello ${name}!
 * <!--{/item}-->
 *
 * Where "${name}" is shorthand for "<!--{name/}-->" which is shorthand for
 * "<!--{name}--><!--{/name}-->"
 *
 * When subtemplate names are repeated in the same frame of reference only
 * the first instance is accessable by getSubtemplate and all later instances
 * must be empty templates. Any substitutions for that name will be made to all
 * instances.
 *
 * To avoid parsing problems template names must not contain '{', '/' or '}'
 * though it is strongly suggested that you stick to alpha-numeric names.
 * 
 */
public final class HTMLTemplate {
  /** The name given to a subtemplate or empty string for the top-level template */
  private String name;
  /** The complete original template data */
  private String data;
  /** A list of strings and HTMLTemplate objects in the order they must be output */
  private List<Object> parts;
  /** A lookup for all the sub templates */
  private Map<String,HTMLTemplate> subs;

  /*
   * Constructs an empty HTMLTemplate object
   */
  private HTMLTemplate(String name, String data) {
    this.name = name;
    this.data = data;
    this.parts = new LinkedList<Object>();
    this.subs = new TreeMap<String, HTMLTemplate>();
  }

  /**
   * Constructs a HTMLTemplate object from a string.
   * @param data the source of the template
   * @throws ParseException if there is something wrong with the source
   */
  public HTMLTemplate(String data) throws ParseException {
    this("", data);
    // parse the template
    List<SubPos> positions = new ArrayList<SubPos>();
    // find position indicators of different types
    Matcher m;
    m = Pattern.compile("\\$\\{([^\\{\\}/]+)\\}").matcher(this.data);
    while (m.find()) {
      positions.add(new SubPos(m.group(1), m.start(), m.end(), false, false));
    }
    m = Pattern.compile("(?:[^X]|\\b)(XXXX([a-zA-WYZ_]{1,20})XXXX)(?:[^X]|\\b)").matcher(this.data);
    while (m.find()) {
      positions.add(new SubPos(m.group(2), m.start(1), m.end(1), false, false));
    }
    m = Pattern.compile("<!--\\{(/?)([^\\{\\}/]+)(/?)\\}-->").matcher(this.data);
    while (m.find()) {
      boolean slashBefore = m.group(1).equals("/");
      boolean slashAfter = m.group(3).equals("/");
      boolean open = !slashBefore && !slashAfter;
      boolean close = slashBefore && !slashAfter;
      positions.add(new SubPos(m.group(2), m.start(), m.end(), open, close));
    }
    Collections.sort(positions);
    
    Deque<SubTemplate> stack = new LinkedList<SubTemplate>();
    HTMLTemplate current = this;
    int offset = 0;
    for (int i = 0; i < positions.size(); i++) {
      SubPos pos = positions.get(i);
      if (pos.start > offset) {
        current.parts.add(data.substring(offset, pos.start));
      }
      offset = pos.end;
      if (pos.open) {
        SubTemplate sub = new SubTemplate(pos);
        stack.push(sub);
        current = sub.template;
      } else if (pos.close) {
        if (stack.isEmpty()) {
          throw new ParseException("Unmatched close subtemplate tag {" +
              pos.name + "}", pos.start);
        }
        SubTemplate sub = stack.pop();
        if (!sub.pos.name.equals(pos.name)) {
          throw new ParseException("Unexpected close subtemplate tag {" +
              pos.name + "} expected close for {" + sub.pos.name + "}" , pos.start);
        }
        sub.template.data = data.substring(sub.pos.end, pos.start);
        current = (stack.isEmpty() ? this : stack.peek().template);
        HTMLTemplate template = current.subs.get(sub.pos.name);
        if (template == null) {
          template = sub.template;
          current.subs.put(template.name, template);
        } else if (sub.template.parts.size() > 0) {
          throw new ParseException("Non-empty subtemplate with duplicate name {" +
              sub.pos.name + "}", sub.pos.start);
        }
        current.parts.add(template);
      } else {
        // only keep a reference to the first template of this name
        HTMLTemplate template = current.subs.get(pos.name);
        if (template == null) {
          template = new HTMLTemplate(pos.name, "");
          current.subs.put(pos.name, template);
        }
        current.parts.add(template);
      }
    }
    if (!stack.isEmpty()) {
      SubTemplate unmatched = stack.peek();
      throw new ParseException("Unmatched open subtemplate tag {" + 
          unmatched.pos.name + "}", unmatched.pos.start);
    }
    if (offset < data.length()) {
      current.parts.add(data.substring(offset));
    }
  }

  /**
   * Returns the subtemplate's name or empty string if this is the top-level
   * template.
   * @return the template's name.
   */
  public String getName() {
    return this.name;
  }

  /**
   * Returns the string that was used to generate the template.
   * @return the template source.
   */
  public String getSource() {
    return this.data;
  }

  /**
   * Returns a list containing objects of type String or HTMLTemplate
   * that describe the complete template.
   * @return list of component Strings and HTMLTemplates
   */
  public List<Object> getParts() {
    return Collections.unmodifiableList(this.parts);
  }

  /**
   * Checks if the named subtemplate exists in this template.
   * @param name the name of the subtemplate
   * @return true iff the named subtemplate exists
   */
  public boolean containsSubtemplate(String name) {
    return this.subs.containsKey(name);
  }

  /**
   * Gets the named subtemplate.
   * @param name the name of the subtemplate
   * @return the subtemplate with the given name
   * @throws IllegalArgumentException if the subtemplate doesn't exist
   */
  public HTMLTemplate getSubtemplate(String name) {
    HTMLTemplate subtemplate = this.subs.get(name);
    if (subtemplate == null) {
      throw new IllegalArgumentException("Unknown subtemplate {" + name + "}");
    }
    return subtemplate;
  }

  /**
   * Convenience function for creating a HTMLSub object which allows 
   * substituting in other values for the subtemplates.
   * @return a mutable HTMLSub object wrapped around this template.
   */
  public HTMLSub toSub() {
    return new HTMLSub(this);
  }

  /**
   * Writes out the string components of the template and subtemplates.
   * @param out the output writer used
   * @throws IOException if problems occur while writing
   */
  public void output(Writer out) throws IOException {
    for (int i = 0; i < this.parts.size(); i++) {
      Object part = this.parts.get(i);
      if (part instanceof HTMLTemplate) {
        ((HTMLTemplate)part).output(out);
      } else { // string
        out.write((String)part);
      }
    }
  }

  /**
   * Return the HTML as a string
   * @return the HTML
   */
  public String toString() {
    StringWriter out = new StringWriter();
    try {
      this.output(out);
    } catch (IOException e) {
      // this should not be possible
      throw new Error(e);
    }
    return out.toString();
  }

  /**
   * Loads the HTMLTemplate from the stream and closes the stream.
   * @param stream the input stream to read the template from
   * @return a HTMLTemplate constructed from the stream.
   * @throws IOException if there is a problem reading the stream
   * @throws ParseException if there is a problem constructing the HTMLTemplate
   */
  public static HTMLTemplate load(InputStream stream) throws IOException, ParseException {
    Reader reader = null;
    try {
      Charset cs = Charset.forName("UTF-8");
      reader = new BufferedReader(new InputStreamReader(stream, cs));
      StringBuilder builder = new StringBuilder();
      char[] buffer = new char[100];
      int read;
      while ((read = reader.read(buffer, 0, buffer.length)) > 0) {
        builder.append(buffer, 0, read);
      }
      reader.close();
      reader = null;
      return new HTMLTemplate(builder.toString());
    } finally {
      if (reader != null) {
        try {
          reader.close();
        } catch (IOException e) {
          //ignore
        }
      }
      if (stream != null) {
        try {
          stream.close();
        } catch (IOException e) {
          //ignore
        }
      }
    }
  }

  /**
   * Loads the HTMLTemplate from the file
   * @param file the file to read the template from
   * @return a HTMLTemplate constructed from the file.
   * @throws IOException if there is a problem reading the file
   * @throws ParseException if there is a problem constructing the HTMLTemplate
   */
  public static HTMLTemplate load(File file) throws IOException, ParseException {
    return HTMLTemplate.load(new FileInputStream(file));
  }

  /**
   * Loads the HTMLTemplate from the path
   * @param path the file to read the template from
   * @return a HTMLTemplate constructed from the file.
   * @throws IOException if there is a problem reading the file
   * @throws ParseException if there is a problem constructing the HTMLTemplate
   */
  public static HTMLTemplate load(String path) throws IOException, ParseException {
    return HTMLTemplate.load(new FileInputStream(path));
  }

  /*
   * This keeps track of the location of a parsing tag
   */
  private static class SubPos implements Comparable<SubPos> {
    public final String name; // tag name
    public final int start; // start offset inclusive
    public final int end; // end offset exclusive
    public final boolean open; // is it a open tag
    public final boolean close; // is it a close tag
    public SubPos(String name, int start, int end, boolean open, boolean close) {
      this.name = name;
      this.start = start;
      this.end = end;
      this.open = open;
      this.close = close;
    }

    public int compareTo(SubPos other) {
      return this.start - other.start;
    }

    /*
     * For debugging print out all the properties
     */
    @Override
    public String toString() {
      StringBuilder strb = new StringBuilder();
      strb.append("SubPos.[name={");
      strb.append(this.name);
      strb.append("},start=");
      strb.append(this.start);
      strb.append(",end=");
      strb.append(this.end);
      strb.append(",open=");
      strb.append(this.open);
      strb.append(",close=");
      strb.append(this.close);
      strb.append("]");
      return strb.toString();
    }
  }

  /*
   * This keeps track of a template along with the location of
   * the tag that started it.
   */
  private static class SubTemplate {
    public final SubPos pos; // starting position in the file
    public final HTMLTemplate template; // the template

    public SubTemplate(SubPos pos) {
      this.pos = pos;
      this.template = new HTMLTemplate(pos.name, "");
    }
  }
}
