package au.edu.uq.imb.memesuite.util;

/**
 * Implementation of Boyer-Moore's string search algorithm.
 * Adapted from implementation at:
 * http://en.wikipedia.org/wiki/Boyer%E2%80%93Moore_string_search_algorithm
 * This only supports searching with the ASCII character range.
 */
public class BMString {
  public static final int ASCII_SIZE = 128;
  private boolean ignoreCase;
  private char[] needle1;
  private char[] needle2;
  private int[] charTable;
  private int[] offsetTable;

  /**
   * Constructs a Boyer-Moore search string.
   * @param needle The text to search for.
   * @param ignoreCase Should the search string be treated as case sensitive?
   */
  public BMString(CharSequence needle, boolean ignoreCase) {
    this.ignoreCase = ignoreCase;
    this.needle1 = new char[needle.length()];
    if (ignoreCase) this.needle2 = new char[needle.length()];
    for (int i = 0; i < this.needle1.length; i++) {
      char c = needle.charAt(i);
      if (c >= 0 && c < ASCII_SIZE) {
        if (ignoreCase) {
          this.needle1[i] = Character.toLowerCase(c);
          this.needle2[i] = Character.toUpperCase(c);
        } else {
          this.needle1[i] = c;
        }
      } else {
        throw new IllegalArgumentException("Search string contains a character outside the ASCII range.");
      }
    }
    this.charTable = makeCharTable(this.needle1, ignoreCase);
    this.offsetTable = makeOffsetTable(this.needle1);
  }

  /**
   * Find the needle (search string) in the haystack (string to scan) and
   * returns the index of the first appearance of the search string after the
   * given index.
   * @param haystack The string to scan for the search string.
   * @param fromIndex The starting index to scan from.
   * @return The index of the needle in the haystack, otherwise
   * return -(index + 1) of the longest prefix of the needle that matches the
   * end of the haystack.
   * @throws IndexOutOfBoundsException if the fromIndex is negative.
   */
  public int indexIn(CharSequence haystack, int fromIndex) {
    if (fromIndex < 0 || fromIndex > haystack.length()) 
        throw new IndexOutOfBoundsException("The fromIndex must be a index in the haystack");
    if (this.needle1.length == 0) return fromIndex;
    if (this.ignoreCase) {
      int i;
      for (i = fromIndex + this.needle1.length - 1; i < haystack.length();) {
        int j = this.needle1.length - 1;
        char h = haystack.charAt(i);
        while (this.needle1[j] == h || this.needle2[j] == h) {
          if (j == 0) return i;
          --i; --j; h = haystack.charAt(i);
        }
        if (h >= 0 && h < ASCII_SIZE) {
          // within ASCII range
          i += Math.max(offsetTable[this.needle1.length - 1 - j], charTable[h]);
        } else {
          // definitely not in needle as outside ASCII range
          i += this.needle1.length; 
        }
      }
      // try to find a partial match
      int noOverlap = (haystack.length() - 1) + this.needle1.length;
      while (i < noOverlap) {
        int j = this.needle1.length - 1;
        // treat the overhanging portion as a match
        int overhang = i - (haystack.length() - 1);
        j -= overhang;
        i -= overhang;
        // continue testing the overlapping bit as normal
        char h = haystack.charAt(i);
        while (this.needle1[j] == h || this.needle2[j] == h) {
          if (j == 0) return -(i + 1);
          --i; --j; h = haystack.charAt(i);
        }
        // now with the mismatch we can only use the charTable
        // because we don't really know what's past the end
        // so using the suffix table doens't make sense
        if (h >= 0 && h < ASCII_SIZE) {
          // within ASCII range
          i += Math.max(this.needle1.length - j, charTable[h]);
        } else {
          // definitely not in needle as outside ASCII range
          i += this.needle1.length; 
        }
      }
      return -(haystack.length() + 1);
    } else {
      int i;
      for (i = fromIndex + this.needle1.length - 1; i < haystack.length();) {
        int j = this.needle1.length - 1;
        char h = haystack.charAt(i);
        while (this.needle1[j] == h) {
          if (j == 0) return i;
          --i; --j; h = haystack.charAt(i);
        }
        if (h >= 0 && h < ASCII_SIZE) {
          // within ASCII range
          i += Math.max(offsetTable[this.needle1.length - 1 - j], charTable[h]);
        } else {
          // definitely not in needle as outside ASCII range
          i += this.needle1.length; 
        }
      }
      // try to find a partial match
      int noOverlap = (haystack.length() - 1) + this.needle1.length;
      while (i < noOverlap) {
        int j = this.needle1.length - 1;
        // treat the overhanging portion as a match
        int overhang = i - (haystack.length() - 1);
        j -= overhang;
        i -= overhang;
        // continue testing the overlapping bit as normal
        char h = haystack.charAt(i);
        while (this.needle1[j] == h) {
          if (j == 0) return -(i + 1);
          --i; --j; h = haystack.charAt(i);
        }
        // now with the mismatch we can only use the charTable
        // because we don't really know what's past the end
        // so using the suffix table doens't make sense
        if (h >= 0 && h < ASCII_SIZE) {
          // within ASCII range
          i += Math.max(this.needle1.length - j, charTable[h]);
        } else {
          // definitely not in needle as outside ASCII range
          i += this.needle1.length; 
        }
      }
      return -(haystack.length() + 1);
    }
  }

  /**
   * Find the needle (search string) in the haystack (string to scan) and
   * returns the index of the first appearance of the search string after the
   * given index.
   * @param haystack The string to scan for the search string.
   * @return The index of the needle in the haystack, otherwise
   * return -(index + 1) of the longest prefix of the needle that matches the
   * end of the haystack.
   * @throws IndexOutOfBoundsException if the fromIndex is negative.
   */
  public int indexIn(CharSequence haystack) {
    return this.indexIn(haystack, 0);
  }

  /**
   * Get the length of the search string.
   * @return the length of the needle (search string).
   */
  public int length() {
    return this.needle1.length;
  }

  /**
   * Returns true if ignoring case.
   * @return true if ignoring case.
   */
  public boolean isIgnoringCase() {
    return this.ignoreCase;
  }
  
  public String toString() {
    return new String(this.needle1);
  }

  /*
   * Makes the jump table based on the mismatched character information.
   * If ignoreCase is set then the needle will be lower case already.
   */
  private static int[] makeCharTable(char[] needle, boolean ignoreCase) {
    int[] table = new int[ASCII_SIZE];
    for (int i = 0; i < table.length; ++i) {
      table[i] = needle.length;
    }
    for (int i = 0; i < needle.length - 1; ++i) {
      char c = needle[i];
      if (ignoreCase) {
        table[c] = needle.length - 1 - i;
        table[Character.toUpperCase(c)] = needle.length - 1 - i;
      } else {
        table[c] = needle.length - 1 - i;
      }
    }
    return table;
  }
 
  /*
   * Makes the jump table based on the scan offset which mismatch occurs.
   */
  private static int[] makeOffsetTable(char[] needle) {
    int[] table = new int[needle.length];
    int lastPrefixPosition = needle.length;
    for (int i = needle.length - 1; i >= 0; --i) {
      if (isPrefix(needle, i + 1)) {
        lastPrefixPosition = i + 1;
      }
      table[needle.length - 1 - i] = lastPrefixPosition - i + needle.length - 1;
    }
    for (int i = 0; i < needle.length - 1; ++i) {
      int slen = suffixLength(needle, i);
      table[slen] = needle.length - 1 - i + slen;
    }
    return table;
  }

  /*
   * Is needle[p:end] a prefix of needle?
   */
  private static boolean isPrefix(char[] needle, int p) {
    for (int i = p, j = 0; i < needle.length; ++i, ++j) {
      if (needle[i] != needle[j]) {
        return false;
      }
    }
    return true;
  }
 
  /*
   * Returns the maximum length of the substring ends at p and is a suffix.
   */
  private static int suffixLength(char[] needle, int p) {
    int len = 0;
    for (int i = p, j = needle.length - 1;
         i >= 0 && needle[i] == needle[j]; --i, --j) {
      len += 1;
    }
    return len;
  }

}
