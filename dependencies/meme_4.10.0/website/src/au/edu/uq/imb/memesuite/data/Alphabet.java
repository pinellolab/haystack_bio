package au.edu.uq.imb.memesuite.data;

public enum Alphabet {
  RNA("RNA"), DNA("DNA"), PROTEIN("Protein");
  
  private final String name;

  private Alphabet(String name) {
    this.name = name;
  }

  public String toString() {
    return this.name;
  }

  public int toInt() {
    return (1 << this.ordinal());
  }

  // just some indexes of letters
  private static final int A = 'A' - 'A';
  private static final int B = 'B' - 'A';
  private static final int C = 'C' - 'A';
  private static final int D = 'D' - 'A';
  private static final int E = 'E' - 'A';
  private static final int F = 'F' - 'A';
  private static final int G = 'G' - 'A';
  private static final int H = 'H' - 'A';
  private static final int I = 'I' - 'A';
  private static final int J = 'J' - 'A';
  private static final int K = 'K' - 'A';
  private static final int L = 'L' - 'A';
  private static final int M = 'M' - 'A';
  private static final int N = 'N' - 'A';
  private static final int O = 'O' - 'A';
  private static final int P = 'P' - 'A';
  private static final int Q = 'Q' - 'A';
  private static final int R = 'R' - 'A';
  private static final int S = 'S' - 'A';
  private static final int T = 'T' - 'A';
  private static final int U = 'U' - 'A';
  private static final int V = 'V' - 'A';
  private static final int W = 'W' - 'A';
  private static final int X = 'X' - 'A';
  private static final int Y = 'Y' - 'A';
  private static final int Z = 'Z' - 'A';
  /**
   * Guesses the alphabet from an array of counts of the letters A to Z.
   * The letter A is assumed to be at position 0 in the array, B at position 1,
   * C at position 2, and so forth until Z at position 25.
   * The array must be at least 26 long. The array is not modified.
   * @param counts counts of the letters A to Z as present in the sequences
   * @return A guess of the alphabet based on a frequency analysis
   */
  public static Alphabet guess(long[] counts) {
    if (counts.length < 26) {
      throw new IllegalArgumentException("Expected at least 26 entries in " +
          "the counts array.");
    }
    // check for letters that are not in DNA
    if (counts[E] > 0 || counts[F] > 0 || counts[I] > 0 || counts[J] > 0 || 
        counts[L] > 0 || counts[O] > 0 || counts[P] > 0 || counts[Q] > 0 ||
        counts[X] > 0 || counts[Z] > 0) {
      return PROTEIN;
    }
    // now sum up the counts of A,C,G,T,U and N. They should vastly outnumber
    // any other characters if it is DNA or RNA so we'll set the threshold at
    // 90%
    long countAll = 0;
    for (int i = 0; i < 26; i++) countAll += counts[i];
    long countACGTUN = counts[A] + counts[C] + counts[G] + counts[T] + counts[U] + counts[N];
    if (((double)countACGTUN / (double)countAll) > 0.9) {
      if (counts[U] > 0 && counts[T] == 0) {
        return RNA;
      } else {
        return DNA;
      }
    }
    return PROTEIN;
  }

  /**
   * Convert from a bit representation to the Enum value.
   */
  public static Alphabet fromInt(int alphaBit) {
    int pos = Integer.numberOfTrailingZeros(alphaBit);
    return Alphabet.values()[pos];
  }
}
