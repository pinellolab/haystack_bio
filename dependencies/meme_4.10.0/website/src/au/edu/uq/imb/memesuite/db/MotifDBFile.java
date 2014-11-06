package au.edu.uq.imb.memesuite.db;

import au.edu.uq.imb.memesuite.data.Alphabet;
import au.edu.uq.imb.memesuite.data.MotifInfo;
import au.edu.uq.imb.memesuite.util.JsonWr;

import java.io.IOException;

public class MotifDBFile implements MotifInfo {
  private long id;
  private String fileName;
  private Alphabet alphabet;
  private int motifCount;
  private int totalCols;
  private int minCols;
  private int maxCols;
  private double avgCols;
  private double stdDCols;

  public MotifDBFile(long id, String fileName,
      Alphabet alphabet, int motifCount, int totalCols,
      int minCols, int maxCols, double avgCols, double stdDCols) {
    this.id = id;
    this.fileName = fileName;
    this.alphabet = alphabet;
    this.motifCount = motifCount;
    this.totalCols = totalCols;
    this.minCols = minCols;
    this.maxCols = maxCols;
    this.avgCols = avgCols;
    this.stdDCols = stdDCols;
  }

  public long getID() {
    return this.id;
  }

  public String getFileName() {
    return this.fileName;
  }

  public Alphabet getAlphabet() {
    return this.alphabet;
  }

  public int getMotifCount() {
    return this.motifCount;
  }

  public int getTotalCols() {
    return this.totalCols;
  }

  public int getMinCols() {
    return this.minCols;
  }

  public int getMaxCols() {
    return this.maxCols;
  }

  public double getAverageCols() {
    return this.avgCols;
  }

  public double getStandardDeviationCols() {
    return this.stdDCols;
  }

  public String toString() {
    return "MotifListingFile[ Alphabet:" + this.alphabet +
      ", Count:" + this.getMotifCount() + ", Min-Columns:" + this.getMinCols() +
      ", Max-Columns:" + this.getMaxCols() + ", Average-Columns:" + this.getAverageCols() + " ]";
  }

  @Override
  public void outputJson(JsonWr out) throws IOException {
    out.startObject();
    out.property("alphabet", getAlphabet().name());
    out.property("count", getMotifCount());
    out.property("min", getMinCols());
    out.property("max", getMaxCols());
    out.property("avg", getAverageCols());
    out.property("total", getTotalCols());
    out.endObject();
  }
}
