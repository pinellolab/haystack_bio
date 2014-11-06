package au.edu.uq.imb.memesuite.db;

import au.edu.uq.imb.memesuite.data.Alphabet;
import au.edu.uq.imb.memesuite.data.SequenceInfo;
import au.edu.uq.imb.memesuite.util.JsonWr;

import java.io.IOException;

/**
 * Information about a sequence database with statistics on the contained sequences.
 */
public class SequenceDB implements SequenceInfo {
  private String listingName;
  private String listingDescription;
  private long id;
  private long edition;
  private String version;
  private Alphabet alphabet;
  private String description;
  private String fileSeq;
  private String fileBg;
  private long sequenceCount;
  private long totalLen;
  private long minLen;
  private long maxLen;
  private double avgLen;
  private double stdDLen;

  public SequenceDB(String listingName, String listingDescription,
      long id, Alphabet alphabet, long edition, String version,
      String description, String fileSeq, String fileBg, long sequenceCount,
      long totalLen, long minLen, long maxLen, double avgLen, double stdDLen) {
    this.listingName = listingName;
    this.listingDescription = listingDescription;
    this.id = id;
    this.alphabet = alphabet;
    this.edition = edition;
    this.version = version;
    this.description = description;
    this.fileSeq = fileSeq;
    this.fileBg = fileBg;
    this.sequenceCount = sequenceCount;
    this.totalLen = totalLen;
    this.minLen = minLen;
    this.maxLen = maxLen;
    this.avgLen = avgLen;
    this.stdDLen = stdDLen;
  }

  public String getListingName() {
    return listingName;
  }

  public String getListingDescription() {
    return listingDescription;
  }

  public long getId() {
    return id;
  }

  public long getEdition() {
    return edition;
  }

  public String getVersion() {
    return version;
  }

  public String getDescription() {
    return description;
  }

  public String getSequenceName() {
    return fileSeq;
  }

  public String getBackgroundName() {
    return fileBg;
  }

  @Override
  public Alphabet guessAlphabet() {
    return alphabet;
  }

  @Override
  public long getSequenceCount() {
    return sequenceCount;
  }

  @Override
  public long getTotalLength() {
    return totalLen;
  }

  @Override
  public long getMinLength() {
    return minLen;
  }

  @Override
  public long getMaxLength() {
    return maxLen;
  }

  @Override
  public double getAverageLength() {
    return avgLen;
  }

  @Override
  public double getStandardDeviationLength() {
    return stdDLen;
  }

  @Override
  public void outputJson(JsonWr out) throws IOException {
    out.startObject();
    out.property("source", "db");
    out.property("db_name", listingName);
    out.property("db_description", listingDescription);
    out.property("alphabet", guessAlphabet().name());
    out.property("count", getSequenceCount());
    out.property("min", getMinLength());
    out.property("max", getMaxLength());
    out.property("avg", getAverageLength());
    out.property("total", getTotalLength());
    out.endObject();
  }
}
