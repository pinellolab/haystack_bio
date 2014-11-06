package au.edu.uq.imb.memesuite.db;

/**
 * GOMo can use secondary sequence files
 */
public class GomoDBSecondary {
  private String description;
  private String sequenceFileName;
  private String backgroundFileName;

  public GomoDBSecondary(String description,
      String sequenceFileName, String backgroundFileName) {
    this.description = description;
    this.sequenceFileName = sequenceFileName;
    this.backgroundFileName = backgroundFileName;
  }

  public String getDescription() {
    return description;
  }

  public String getSequenceFileName() {
    return sequenceFileName;
  }

  public String getBackgroundFileName() {
    return backgroundFileName;
  }
}
