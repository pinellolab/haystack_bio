package au.edu.uq.imb.memesuite.data;

import au.edu.uq.imb.memesuite.util.JsonWr;
import au.edu.uq.imb.memesuite.util.SampleStats;

import java.io.IOException;

/**
 * Calculate statistics on the sequences
 */
public final class SequenceStats implements SequenceInfo {
  private boolean frozen;
  private SampleStats stats;
  private long seqLength;
  private long[] lnum;
  private Alphabet alphabetGuess;

  /**
   * Class constructor
   */
  public SequenceStats() {
    this.stats = new SampleStats(true);
    this.seqLength = 0;
    this.lnum = new long[27];
  }

  /**
   * Process the sequence and update the statistics
   * @param sequence the complete sequence
   * @see #addSeqPart(CharSequence)
   * @see #endSeq()
   */
  public void addSeq(CharSequence sequence) {
    this.addSeqPart(sequence);
    this.endSeq();
  }

  /**
   * Process the sequence and update the statistics
   * @param data a raw UTF-8 encoded buffer containing the complete sequence
   * @param start the start index of the sequence in the buffer
   * @param length the length of the sequence in the buffer
   * @see #addSeqPart(byte[], int, int)
   * @see #endSeq()
   */
  public void addSeq(byte[] data, int start, int length) {
    this.addSeqPart(data, start, length);
    this.endSeq();
  }

  /**
   * Add part of a sequence to be processed.
   * When the full sequence has been process with this method endSeq should be called.
   * @param sequencePart The characters that comprise part of a sequence
   * @see #endSeq()
   */
  public void addSeqPart(CharSequence sequencePart) {
    if (this.frozen) throw new IllegalStateException("SequenceStats is frozen.");
    this.seqLength += sequencePart.length();
    for (int i = 0; i < sequencePart.length(); i++) {
      char l = sequencePart.charAt(i);
      if (l >= 'A' && l <= 'Z') {
        this.lnum[l - 'A']++;
      } else if (l >= 'a' && l <= 'z') {
        this.lnum[l - 'a']++;
      } else {
        this.lnum[26]++;
      }
    }
  }

  /**
   * Add part of a sequence to be processed.
   * When the full sequence has been process with this method endSeq should be called.
   * @param data a raw UTF-8 encoded buffer containing part of a sequence
   * @param start the start index of the part of the sequence in the buffer
   * @param length the length of the sequence part in the buffer
   * @see #endSeq()
   */
  public void addSeqPart(byte[] data, int start, int length) {
    if (this.frozen) throw new IllegalStateException("SequenceStats is frozen.");
    this.seqLength += length;
    int end = start + length;
    for (int i = start; i < end; i++) {
      byte l = data[i];
      if (l >= 'A' && l <= 'Z') {
        this.lnum[l - 'A']++;
      } else if (l >= 'a' && l <= 'z') {
        this.lnum[l - 'a']++;
      } else {
        this.lnum[26]++;
      }
    }
  }

  /**
   * Complete processing of a sequence and update the statistics to include the sequence.
   */
  public void endSeq() {
    if (this.frozen) throw new IllegalStateException("SequenceStats is frozen.");
    this.stats.update(this.seqLength);
    this.seqLength = 0;
  }

  /**
   * Stop further changes.
   * This allows the sequenceStat object to be returned without making
   * a copy as it becomes immutable.
   */
  public void freeze() {
    if (!this.frozen) {
      this.frozen = true;
      this.alphabetGuess = Alphabet.guess(this.lnum);
    }
  }

  /**
   * Return the total count of sequences
   * @return The total count of sequences
   */
  public long getSequenceCount() {
    return this.stats.getCount();
  }

  /**
   * Return the total length of the sequence data
   * @return The total length of the sequence data
   */
  public long getTotalLength() {
    return this.stats.getTotal().longValueExact();
  }

  /**
   * Return the length of the smallest sequence seen or 0 when none have been seen.
   * @return The length of the smallest sequence
   */
  public long getMinLength() {
    return (long)this.stats.getSmallest();
  }

  /**
   * Return the length of the largest sequence seen or 0 when none have been seen.
   * @return The length of the largest sequence
   */
  public long getMaxLength() {
    return (long)this.stats.getLargest();
  }

  /**
   * Returns the average of all the sequence lengths seen or 0 when none have
   * been seen.
   * @return The average sequence length
   */
  public double getAverageLength() {
    return this.stats.getMean();
  }

  /**
   * Returns the standard deviation of all the sequence lengths seen or 0 when
   * less than 2 sequence lengths have been seen.
   * @return The standard deviation of all sequence lengths
   */
  public double getStandardDeviationLength() {
    return this.stats.getStandardDeviation();
  }

  /**
   * Try to guess the alphabet of the sequences by looking at the character frequencies.
   *
   * @return The most likely alphabet of the sequences
   */
  public Alphabet guessAlphabet() {
    if (this.frozen) return this.alphabetGuess;
    return Alphabet.guess(this.lnum);
  }

  @Override
  public void outputJson(JsonWr out) throws IOException {
    out.startObject();
    out.property("type", "sequence");
    out.property("alphabet", guessAlphabet().name());
    out.property("count", getSequenceCount());
    out.property("min", getMinLength());
    out.property("max", getMaxLength());
    out.property("avg", getAverageLength());
    out.property("total", getTotalLength());
    out.endObject();
  }
}
