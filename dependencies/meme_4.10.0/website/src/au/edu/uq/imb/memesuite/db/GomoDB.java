package au.edu.uq.imb.memesuite.db;

import au.edu.uq.imb.memesuite.util.JsonWr;

import java.io.IOException;
import java.util.Collections;
import java.util.List;

/**
 * A gomo database entry.
 */
public class GomoDB implements JsonWr.JsonValue {
  private long listingId;
  private String name;
  private String description;
  private int promoterStart;
  private int promoterStop;
  private String goMapFileName;
  private String sequenceFileName;
  private String backgroundFileName;
  private List<GomoDBSecondary> secondaries;

  public GomoDB(long listingId, String name, String description,
      int promoterStart, int promoterStop,
      String goMapFileName, String sequenceFileName, String backgroundFileName,
      List<GomoDBSecondary> secondaries) {
    this.listingId = listingId;
    this.name = name;
    this.description = description;
    this.promoterStart = promoterStart;
    this.promoterStop = promoterStop;
    this.goMapFileName = goMapFileName;
    this.sequenceFileName = sequenceFileName;
    this.backgroundFileName = backgroundFileName;
    this.secondaries = Collections.unmodifiableList(secondaries);
  }

  public long getListingId() {
    return listingId;
  }

  public String getName() {
    return name;
  }

  public String getDescription() {
    return description;
  }

  public int getPromoterStart() {
    return promoterStart;
  }

  public int getPromoterStop() {
    return promoterStop;
  }

  public String getGoMapFileName() {
    return goMapFileName;
  }

  public String getSequenceFileName() {
    return sequenceFileName;
  }

  public String getBackgroundFileName() {
    return backgroundFileName;
  }

  public List<GomoDBSecondary> getSecondaries() {
    return secondaries;
  }

  @Override
  public void outputJson(JsonWr out) throws IOException {
    out.startObject();
    out.property("db_name", getName());
    out.property("db_description", getDescription());
    out.endObject();
  }
}
