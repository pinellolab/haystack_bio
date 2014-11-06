package au.edu.uq.imb.memesuite.data;

import au.edu.uq.imb.memesuite.util.JsonWr;

/**
 * Information about a group of sequences.
 */
public interface SequenceInfo extends JsonWr.JsonValue {

  /**
   * Return the alphabet of the sequences.
   * @return the alphabet.
   */
  public Alphabet guessAlphabet();

  /**
   * Return the number of sequences in the source.
   * @return The number of sequences
   */
  public long getSequenceCount();

  /**
   * Return the total number of bases (nucleotides or peptides) in the source.
   * @return The total number of bases
   */
  public long getTotalLength();

  /**
   * Return the length of the shortest sequence in the file.
   * @return The length of the shortest sequence
   */
  public long getMinLength();

  /**
   * Return the length of the longest sequence in the file.
   * @return The length of the longest sequence
   */
  public long getMaxLength();

  /**
   * Return the average sequence length
   * @return The average sequence length
   */
  public double getAverageLength();

  /**
   * Return the standard deviation from the average length
   * @return The standard deviation from the average length
   */
  public double getStandardDeviationLength();
}
