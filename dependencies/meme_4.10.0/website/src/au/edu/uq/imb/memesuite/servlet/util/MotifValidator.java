package au.edu.uq.imb.memesuite.servlet.util;

import java.io.*;
import java.util.*;
import java.util.regex.*;
import javax.xml.parsers.*;

import au.edu.uq.imb.memesuite.data.Alphabet;
import au.edu.uq.imb.memesuite.data.MotifStats;
import au.edu.uq.imb.memesuite.io.html.HDataHandlerAdapter;
import au.edu.uq.imb.memesuite.io.html.HDataParser;
import org.xml.sax.*;
import org.xml.sax.helpers.DefaultHandler;

public class MotifValidator {

  protected MotifValidator() {

  }

  public static MotifStats validateAsXML(InputStream in) throws IOException {
    SAXParserFactory factory;
    SAXParser saxParser;
    try {
      factory = SAXParserFactory.newInstance();
      saxParser = factory.newSAXParser();
    } catch (SAXException e) {
      throw new Error("Unexpected exception", e);
    } catch (javax.xml.parsers.ParserConfigurationException e) {
      throw new Error("Unexpected exception", e);
    }
    XMLMotifHandler handler = new XMLMotifHandler();
    try {
      saxParser.parse(in, handler);
    } catch (SAXException e) {
      return null;
    }
    return handler.getStats();
  }

  protected static class XMLMotifHandler extends DefaultHandler {
    private MotifStats stats;
    private Deque<String> elementStack;
    private boolean isMemeFile;
    private boolean isDremeFile;

    public XMLMotifHandler() {
      this.stats = new MotifStats();
      this.elementStack = new ArrayDeque<String>();
    }

    public void startElement(String uri, String localName, String qName, Attributes attributes) throws SAXException {
      if (elementStack.size() == 0) {
        if (qName.equals("MEME")) {
          isMemeFile = true;
        } else if (qName.equals("dreme")) {
          isDremeFile = true;
        } else {
          throw new SAXException("Expected a MEME or DREME XML file."); 
        }
      } else if (isMemeFile) { // MEME XML file
        if (elementStack.size() == 2) {
          if (qName.equals("alphabet")) {
            if (attributes.getValue("id").equals("nucleotide")) {
              this.stats.setAlphabet(Alphabet.DNA);
            } else {
              this.stats.setAlphabet(Alphabet.PROTEIN);
            }
          } else if (qName.equals("motif")) {
            int motifCols;
            try {
              motifCols = Integer.parseInt(attributes.getValue("width"));
            } catch (NumberFormatException e) {
              throw new SAXException("Expected width to be a number.", e);
            }
            this.stats.addMotif(motifCols);
          }
        }
      } else if (isDremeFile) { // DREME XML file
        if (elementStack.size() == 2) {
          if (qName.equals("background")) {
            if (attributes.getValue("type").equals("dna")) {
              this.stats.setAlphabet(Alphabet.DNA);
            } else {
              this.stats.setAlphabet(Alphabet.RNA);
            }
          } else if (qName.equals("motif")) {
            int motifCols;
            try {
              motifCols = Integer.parseInt(attributes.getValue("length"));
            } catch (NumberFormatException e) {
              throw new SAXException("Expected length to be a number.", e);
            }
            this.stats.addMotif(motifCols);
          }
        }
      }
      elementStack.addFirst(qName);
    }
    
    public void endElement(String uri, String localName, String qName) throws SAXException {
      String expect = elementStack.removeFirst();
      if (!qName.equals(expect)) {
        throw new SAXException("Expected element: " + expect);
      }
    }

    public void warning(SAXParseException e) throws SAXException {
      throw e;
    }

    public void error(SAXParseException e) throws SAXException {
      throw e;
    }

    public MotifStats getStats() {
      this.stats.freeze();
      return this.stats;
    }
  }

  public static MotifStats validateAsHTML(InputStream in) throws IOException {
    HTMLMotifHandler handler = new HTMLMotifHandler();
    HDataParser.parse(in, handler);
    return handler.getStats();
  }

  protected static class HTMLMotifHandler extends HDataHandlerAdapter {
    private MotifStats stats;
    private int[] version;
    private Deque<String> propertyStack;
    private boolean error;

    private static final Pattern pspmNamePattern = Pattern.compile(
        "^pspm(\\d+)$", Pattern.CASE_INSENSITIVE);
    private static final Pattern verLinePattern = Pattern.compile(
            "^\\s*MEME\\s+version\\s+(\\d+(?:\\.\\d+){0,2})\\s*$");
    private static final Pattern versionPattern  = Pattern.compile(
        "^(\\d+)(?:\\.(\\d+)(?:\\.(\\d+))?)?$");
    private static final Pattern motifWidthPattern = Pattern.compile(
        "\\sw\\s*=\\s*(\\d+(?:\\.\\d+)?)\\s");

    public HTMLMotifHandler() {
      this.stats = new MotifStats();
      this.version = new int[]{0, 0, 0};
      this.propertyStack = new ArrayDeque<String>();
      this.error = false;
    }

    private void setVersion(String value) {
      Matcher m = versionPattern.matcher(value);
      if (m.matches()) {
        this.version[0] = Integer.parseInt(m.group(1));
        if (m.group(2) != null && m.group(2).length() > 0) {
          this.version[1] = Integer.parseInt(m.group(2));
        } else {
          this.version[1] = 0;
        }
        if (m.group(3) != null && m.group(3).length() > 0) {
          this.version[2] = Integer.parseInt(m.group(3));
        } else {
          this.version[2] = 0;
        }
      }
    }

    private void setAlphabet(String value) {
      value = value.trim();
      if (value.equals("ACGU")) {
        this.stats.setAlphabet(Alphabet.RNA);
      } else if (value.equals("ACGT")) {
        this.stats.setAlphabet(Alphabet.DNA);
      } else if (value.equals("ACDEFGHIKLMNPQRSTVWY")) {
        this.stats.setAlphabet(Alphabet.PROTEIN);
      }
    }
    
    private void htmlVersion(String value) {
      Matcher m = verLinePattern.matcher(value);
      if (m.matches()) this.setVersion(m.group(1));
    }

    private void htmlPSPM(String value) {
      Matcher m = motifWidthPattern.matcher(value);
      if (m.find()) {
        this.stats.addMotif(Integer.parseInt(m.group(1)));
      }
    }

    public void htmlHiddenData(String name, String value) {
      if (name == null) throw new NullPointerException("Attribute name is null");
      if (value == null) throw new NullPointerException("Attribute value is null");
      // other fields that are not checked for below are:
      // strands, bgfreq, name, combinedblock, nmotifs
      // motifname#, motifblock#, pssm#, BLOCKS#
      if ("version".equalsIgnoreCase(name)) {
        this.htmlVersion(value);
      } else if ("alphabet".equalsIgnoreCase(name)) {
        this.setAlphabet(value);
      } else if (pspmNamePattern.matcher(name).matches()) {
        this.htmlPSPM(value);
      }
    }

    public void jsonStartProperty(String name) {
      this.propertyStack.addFirst(name);
    }

    public void jsonEndProperty() {
      this.propertyStack.removeFirst();
    }

    private boolean isProperty(String ... stack) {
      if (this.propertyStack.size() != stack.length) return false;
      Iterator<String> iter = this.propertyStack.iterator();
      for (String s : stack) {
        if (!s.equalsIgnoreCase(iter.next())) return false;
      }
      return true;
    }

    public void jsonValue(Object value) {
      if (isProperty("version")) {
        if (value instanceof String) this.setVersion((String)value);
      } else if (isProperty("symbols", "alphabet")) {
        if (value instanceof String) this.setAlphabet((String)value);
      } else if (isProperty("len", "motifs")) {
        if (value instanceof Number) this.stats.addMotif(((Number) value).intValue());
      }
    }

    public void jsonError() {
      this.error = true;
    }

    public MotifStats getStats() {
      if (this.error || this.version[0] < 4 || 
          (this.version[0] == 4 && this.version[1] < 3) ||
          (this.version[0] == 4 && this.version[1] == 3 && this.version[2] < 2)) {
        return null;
      }
      this.stats.freeze();
      return this.stats;
    }
  }

  public static MotifStats validateAsText(InputStream inStream) throws IOException {
    TextMotifParser parser = new TextMotifParser(); 
    BufferedReader in = null;
    try {
      in = new BufferedReader(new InputStreamReader(inStream, "UTF-8"));
      String line;
      while ((line = in.readLine()) != null) {
        parser.update(line);
      }
      in.close();
      in = null;
    } finally {
      if (in != null) {
        try {
          in.close();
        } catch (IOException e) { /* ignore */ }
      }
    }
    return parser.getStats();
  }

  private static class TextMotifParser {
    private MotifStats stats;
    private int[] version;
    private int stage;

    private static final Pattern verLineRe = Pattern.compile(
        "^\\s*MEME\\s+version\\s+(\\d+)(?:\\.(\\d+)(?:\\.(\\d+))?)?");
    private static final Pattern alphabetRe = Pattern.compile(
        "^\\s*ALPHABET\\s*=\\s*(\\S+)");
    private static final Pattern motifWidthRe = Pattern.compile(
        "^\\s*letter-probability\\s+matrix:.*\\s+w\\s*=\\s+(\\d+)");

    public TextMotifParser() {
      this.stats = new MotifStats();
      this.version = new int[]{0, 0, 0};
      this.stage = 0;
    }

    public void update(String line) {
      Matcher m;
      switch (this.stage) {
        case 0:
          m = verLineRe.matcher(line);
          if (m.find()) {
            this.version[0] = Integer.parseInt(m.group(1));
            if (m.group(2) != null && m.group(2).length() > 0) {
              this.version[1] = Integer.parseInt(m.group(2));
            } else {
              this.version[1] = 0;
            }
            if (m.group(3) != null && m.group(3).length() > 0) {
              this.version[2] = Integer.parseInt(m.group(3));
            } else {
              this.version[2] = 0;
            }
            this.stage = 1;
          }
          break;
        case 1:
          m = alphabetRe.matcher(line);
          if (m.find()) {
            String alphStr = m.group(1);
            if (alphStr.equals("ACGU")) {
              this.stats.setAlphabet(Alphabet.RNA);
            } else if (alphStr.equals("ACGT")) {
              this.stats.setAlphabet(Alphabet.DNA);
            } else if (alphStr.equals("ACDEFGHIKLMNPQRSTVWY")) {
              this.stats.setAlphabet(Alphabet.PROTEIN);
            }
            this.stage = 2;
          }
          break;
        case 2:
          m = motifWidthRe.matcher(line);
          if (m.find()) {
            int motifCols = Integer.parseInt(m.group(1));
            this.stats.addMotif(motifCols);
          }
          break;
      }
    }
    
    public MotifStats getStats() {
      if (this.stage != 2) return null;
      this.stats.freeze();
      return this.stats;
    }
  }

  public static MotifStats validate(File file) throws IOException {
    MotifStats stat;
    stat = validateAsHTML(new FileInputStream(file));
    if (stat != null) return stat;
    stat = validateAsXML(new FileInputStream(file));
    if (stat != null) return stat;
    stat = validateAsText(new FileInputStream(file));
    return stat;
  }
}
