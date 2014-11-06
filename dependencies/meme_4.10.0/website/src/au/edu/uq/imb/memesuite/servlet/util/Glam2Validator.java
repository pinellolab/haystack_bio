package au.edu.uq.imb.memesuite.servlet.util;

import au.edu.uq.imb.memesuite.data.Alphabet;
import au.edu.uq.imb.memesuite.data.MotifStats;
import au.edu.uq.imb.memesuite.io.html.HDataHandlerAdapter;
import au.edu.uq.imb.memesuite.io.html.HDataParser;

import java.io.*;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

/**
 * Validates a GLAM2 motif, like the MotifValidator
 */
public class Glam2Validator {
  protected Glam2Validator() {

  }

  public static MotifStats validateAsHTML(InputStream in) throws IOException {
    HTMLGlam2Handler handler = new HTMLGlam2Handler();
    HDataParser.parse(in, handler);
    return handler.getStats();
  }

  protected static class HTMLGlam2Handler extends HDataHandlerAdapter {
    private MotifStats stats;
    private boolean error;
    private String currentId;
    private Integer currentAlnLength;
    private Integer currentMotifLength;
    private Integer currentSites;

    private static final Pattern POS_INT = Pattern.compile("^\\d+$");
    private static final Pattern WORD = Pattern.compile("\\S+");
    private static final Pattern ALN_NAME = Pattern.compile("^aln(\\d+)$");
    private static final Pattern PSPM_NAME = Pattern.compile("^pspm(\\d+)$");
    private static final Pattern ALN_LINE = Pattern.compile(
        "^[^\\s]+\\s+\\d+\\s([A-Z\\.]+)\\s+\\d+\\s+\\+\\s+\\d+\\.\\d*$");
    private static final Pattern PSPM_LINE1 = Pattern.compile(
        "^letter-probability matrix:\\s+alength=\\s+(\\d+)\\s+w=\\s+(\\d+)\\s+nsites=\\s+(\\d+)");
    private static final Pattern PSPM_NUM = Pattern.compile("^(?:1(?:\\.0)?|0\\.\\d+)");

    public HTMLGlam2Handler() {
      this.stats = new MotifStats();
      this.error = false;
    }

    private boolean checkPosInt(String value, int minValue) {
      if (!POS_INT.matcher(value).matches()) {
        return true;
      }
      try {
        int iValue = Integer.parseInt(value, 10);
        if (iValue < minValue) return true;
      } catch (NumberFormatException e) {
        return true;
      }
      return false;
    }

    private boolean checkNum(String value, Double minValue, Double maxValue) {
      try {
        double dValue = Double.parseDouble(value);
        if (minValue != null && dValue < minValue) return true;
        if (maxValue != null && dValue > maxValue) return true;
      } catch (NumberFormatException e) {
        return true;
      }
      return false;
    }

    private void updateCurrentId(String id) {
      if (currentId == null) {
        currentId = id;
        currentAlnLength = null;
        currentMotifLength = null;
        currentSites = null;
      } else if (!currentId.equals(id)) {
        stats.addMotif(currentMotifLength);
        currentId = id;
        currentAlnLength = null;
        currentMotifLength = null;
        currentSites = null;
      }
    }

    private boolean parseAlnValue(String id, String value) {
      String[] lines = value.split("\\n");
      if (lines.length < 4) return true;
      if (currentSites == null) {
        currentSites = lines.length - 3;
      } else if (currentSites != (lines.length - 3)) {
        return true;
      }
      String alignedRegions = lines[1].trim();
      if (currentAlnLength == null) currentAlnLength = alignedRegions.length();
      else if (currentAlnLength != alignedRegions.length()) return true;

      String motifRegions = alignedRegions.replace(".", "");
      if (currentMotifLength == null) currentMotifLength = motifRegions.length();
      else if (currentMotifLength != motifRegions.length()) return true;

      for (int i = 2; i < lines.length - 1; i++) {
        Matcher m = ALN_LINE.matcher(lines[i]);
        if (!m.matches()) return true;
        if (currentAlnLength != m.group(1).length()) return true; // should all be the same length!
      }
      return false;
    }

    private boolean parsePspmValue(String id, String value) {
      String[] lines = value.split("\\n");
      boolean foundStart = false;
      int alen = 0;
      int width = 0;
      int nsites = 0;
      int rows = 0;
      Matcher m;
      for (String line : lines) {
        line = line.trim();
        if (!foundStart) {
          m = PSPM_LINE1.matcher(line);
          if (m.find()) {
            try {
              alen = Integer.parseInt(m.group(1));
              width = Integer.parseInt(m.group(2));
              nsites = Integer.parseInt(m.group(3));
            } catch (NumberFormatException e) {
              return true;
            }
            if (this.stats.getAlphabet() != null) {
              switch (this.stats.getAlphabet()) {
                case RNA:
                case DNA:
                  if (alen != 4) return true;
                  break;
                case PROTEIN:
                  if (alen != 20) return true;
                  break;
              }
            } else {
              if (alen == 4) {
                this.stats.setAlphabet(Alphabet.DNA);
              } else if (alen == 20) {
                this.stats.setAlphabet(Alphabet.PROTEIN);
              }
            }
            if (currentMotifLength == null) {
              currentMotifLength = width;
            } else if (currentMotifLength != width) {
              return true;
            }
            if (currentSites == null) {
              currentSites = nsites;
            } else if (currentSites != nsites) {
              return true;
            }
            foundStart = true;
          }
          continue;
        }
        if (rows >= width) {
          if (line.length() > 0) return true;
          continue;
        }
        String[] nums = line.split("\\s+");
        if (nums.length != alen) return true;
        for (String num : nums)
          if (!PSPM_NUM.matcher(num).matches()) return true;
      }
      return false;
    }

    public void htmlHiddenData(String name, String value) {
      Matcher m;
      if (name.equals("-o")) { // output dir, no clobber
        error |= !WORD.matcher(value).find();
      } else if (name.equals("-O")) { // output dir, with clobber
        error |= !WORD.matcher(value).find();
      } else if (name.equals("-2")) { // search both strands
        error |= !value.equals("1");
      } else if (name.equals("-z")) { // min sequences in alignment
        error |= checkPosInt(value, 2);
      } else if (name.equals("-a")) { // min aligned columns
        error |= checkPosInt(value, 2);
      } else if (name.equals("-b")) { // max aligned columns
        error |= checkPosInt(value, 2);
      } else if (name.equals("-d")) { // specify a dirichlet mixture file
        error |= !WORD.matcher(value).find();
      } else if (name.equals("-D")) { // deletion pseudocount
        error |= checkNum(value, 0.0, null);
      } else if (name.equals("-E")) { // no-deletion pseudocount
        error |= checkNum(value, 0.0, null);
      } else if (name.equals("-I")) { // insertion pseudocount
        error |= checkNum(value, 0.0, null);
      } else if (name.equals("-J")) { // no-insertion pseudocount
        error |= checkNum(value, 0.0, null);
      } else if (name.equals("-q")) { // weight for generic versus sequence-set-specific residue abundances
        error |= checkNum(value, 0.0, null);
      } else if (name.equals("-r")) { // alignment runs
        error |= checkPosInt(value, 2);
      } else if (name.equals("-n")) { // iterations at best seen
        error |= checkPosInt(value, 2);
      } else if (name.equals("-w")) { // initial aligned cols
        error |= checkPosInt(value, 2);
      } else if (name.equals("-t")) { // initial temperature
        error |= checkNum(value, 0.0, null);
      } else if (name.equals("-c")) { // cooling factor
        error |= checkNum(value, 0.0, null);
      } else if (name.equals("-u")) { // min temperature
        error |= checkNum(value, 0.0, null);
      } else if (name.equals("-m")) { // col sample vs site sample decision probability
        error |= checkNum(value, 0.0, 1.0);
      } else if (name.equals("-x")) { // site sampling algorithm
        error |= (!(value.equals("0") || value.equals("1") || value.equals("2")));
      } else if (name.equals("-s")) { // random seed
        error |= checkNum(value, null, null);
      } else if (name.equals("-p")) { // print extra state info
        // I suspect from reading the code of glam2html that this will
        // consume the value of the next parameter as it's not listed
        // among the flags
        error |= !WORD.matcher(value).find();
      } else if (name.equals("-Q")) { // quiet
        error |= !value.equals("1");
      } else if (name.equals("address")) { // email address
        error |= !WORD.matcher(value).find();
      } else if (name.equals("address_verify")) { // verify email address
        error |= !WORD.matcher(value).find();
      } else if (name.equals("description")) { // description
        error |= value.isEmpty();
      } else if (name.equals("alphabet")) { // alphabet
        // can be n for DNA, p for PROTEIN or a file name, but we don't allow the file name!
        if (value.equals("n")) {
          stats.setAlphabet(Alphabet.DNA);
        } else if (value.equals("p")) {
          stats.setAlphabet(Alphabet.PROTEIN);
        } else {
          error = true;
        }
      } else if (name.equals("seq_file")) { // name of the sequence file
        error |= !WORD.matcher(value).find();
      } else if (name.equals("data")) { // embeded sequences
        error |= !WORD.matcher(value).find();
      } else if ((m = ALN_NAME.matcher(name)).matches()) {
        updateCurrentId(m.group(1));
        error |= parseAlnValue(m.group(1), value);
      } else if ((m = PSPM_NAME.matcher(name)).matches()) {
        updateCurrentId(m.group(1));
        error |= parsePspmValue(m.group(1), value);
      } else {
        // unknown option
        error = true;
      }
    }

    public MotifStats getStats() {
      if (error || stats.getAlphabet() == null || stats.getMotifCount() == 0) return null;
      updateCurrentId(null);
      this.stats.freeze();
      return stats;
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
        } catch(IOException e) { /* ignore */ }
      }
    }
    return parser.getStats();
  }

  private static class TextMotifParser {
    private MotifStats stats;
    private int stage;
    private boolean error;

    private static final Pattern cmdLineRe = Pattern.compile(
        "^glam2\\s.*([pn])\\s+[^\\s]+\\s*$");
    private static final Pattern motifStartRe = Pattern.compile(
        "^Score:\\s+\\d+\\.\\d+\\s+Columns:\\s+(\\d+)\\s+Sequences:\\s+\\d+\\s*$");

    public TextMotifParser() {
      stats = new MotifStats();
      stage = 0;
      error = false;
    }

    public void update(String line) {
      Matcher m;
      switch (stage) {
        case 0:
          if ((m = cmdLineRe.matcher(line)).matches()) {
            if (m.group(1).equals("n")) {
              stats.setAlphabet(Alphabet.DNA);
            } else {
              stats.setAlphabet(Alphabet.PROTEIN);
            }
            stage = 1;
          }
          break;
        case 1:
        case 2:
          if ((m = motifStartRe.matcher(line)).matches()) {
            int cols;
            try {
              cols = Integer.parseInt(m.group(1));
            } catch (NumberFormatException e) {
              error = true;
              return;
            }
            stats.addMotif(cols);
            stage = 2;
          }
      }
    }

    public MotifStats getStats() {
      if (error || stage != 2) return null;
      stats.freeze();
      return stats;
    }
  }

  public static MotifStats validate(File file) throws IOException {
    MotifStats stat;
    stat = validateAsHTML(new FileInputStream(file));
    if (stat != null) return stat;
    stat = validateAsText(new FileInputStream(file));
    return stat;
  }
}
