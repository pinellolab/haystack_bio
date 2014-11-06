package au.edu.uq.imb.memesuite.util;

import java.io.File;
import java.io.FilenameFilter;
import java.util.Set;
import java.util.TreeSet;
import java.util.regex.Pattern;

/**
 * This implements a file filter that supports glob syntax.
 */
public class GlobFilter implements FilenameFilter {
  private Pattern globPattern;
  public GlobFilter(String glob) {
    // Turn a list of whitespace separated globs into a regular expression.
    // Globs follow the following rules:
    // 1) '\' escapes the next character
    // 2) '?' matches exactly 1 unknown character
    // 3) '*' matches any number of unknown characters
    // 4) [characters] matches one of the characters
    //
    // I am not aware of any standard error handling rules so I'm going to do
    // the following:
    // 1) When '\' is the last character with no following character to
    //    escape it will be ignored.
    // 2) When '[', '*' or '?' is used inside brackets unescaped it will be
    //    treated as if escaped.
    // 3) When a opening bracket '[' is found but there is no closing
    //    bracket ']' then the opening bracket will be treated as if escaped.
    // 4) When a closing bracket ']' is found but there is no opening
    //    bracket '[' then the closing bracket will be treated as if escaped.
    // 5) When a character within brackets '[' ']' is repeated the second
    //    occurence will be ignored.
    //
    StringBuilder re = new StringBuilder(glob.length()*2);
    StringBuilder part = new StringBuilder(glob.length());
    Set<Character> chars = new TreeSet<Character>();
    boolean bracket = false;
    boolean escape = false;
    boolean space = true;
    int pipePos = 0;
    re.append("^(?:");
    for (int i = 0; i < glob.length(); i++) {
      char c = glob.charAt(i);
      if (space) {
        if (Character.isWhitespace(c)) continue;
        space = false;
      }
      if (escape) {
        if (bracket) {
          chars.add(c);
        }
        part.append(c);
        escape = false;
      } else {
        if (c == '\\') {
          escape = true;
        } if (Character.isWhitespace(c)) {
          // finish pattern
          re.append(Pattern.quote(part.toString()));
          re.append("|");
          pipePos = re.length();
          // reset to begin new pattern
          space = true;
          bracket = false;
          escape = false;
          part.setLength(0);
        } else if (bracket) {
          if (c == ']') {
            part.setLength(0);
            for (Character ch : chars) part.append(ch);
            re.append('[');
            re.append(Pattern.quote(part.toString()));
            re.append(']');
            bracket = false;
            part.setLength(0);
          } else {
            chars.add(c);
          }
        } else {
          if (c == '[') {
            re.append(Pattern.quote(part.toString()));
            part.setLength(0);
            part.append('[');
            chars.clear();
            bracket = true;
          } else if (c == '?') {
            re.append(Pattern.quote(part.toString()));
            re.append('.');
            part.setLength(0);
          } else if (c == '*') {
            re.append(Pattern.quote(part.toString()));
            re.append(".*");
            part.setLength(0);
          } else {
            part.append(c);
          }
        }
      }
    }
    re.append(Pattern.quote(part.toString()));
    if (re.length() == pipePos) {
      // remove last pipe character
      re.deleteCharAt(re.length() - 1);
    }
    re.append(")$");
    this.globPattern = Pattern.compile(re.toString());
  }

  public boolean accept(File dir, String name) {
    return this.globPattern.matcher(name).matches();
  }
}

