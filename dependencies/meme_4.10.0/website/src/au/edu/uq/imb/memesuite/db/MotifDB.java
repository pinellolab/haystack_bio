package au.edu.uq.imb.memesuite.db;

import au.edu.uq.imb.memesuite.data.Alphabet;
import au.edu.uq.imb.memesuite.data.MotifInfo;
import au.edu.uq.imb.memesuite.util.JsonWr;

import java.io.IOException;
import java.util.List;
import java.util.ArrayList;
import java.util.Collections;

public class MotifDB implements MotifInfo {
  private String name;
  private String description;
  private List<MotifDBFile> files;
  private Alphabet alphabet;
  private int motifCount;
  private int totalCols;
  private int minCols;
  private int maxCols;
  private double standardDeviationCols;

  public MotifDB(String name, String description, List<MotifDBFile> files) {
    this.name = name;
    this.description = description;
    this.files = Collections.unmodifiableList(new ArrayList<MotifDBFile>(files));
    alphabet = null;
    motifCount = 0;
    totalCols = 0;
    minCols = (files.isEmpty() ? 0 : Integer.MAX_VALUE);
    maxCols = 0;
    double sumVariance = 0;
    for (MotifDBFile file : files) {
      if (alphabet == null) {
        alphabet = file.getAlphabet();
      } else if (alphabet != file.getAlphabet()) {
        throw new IllegalArgumentException("Expected all files to have the same alphabet.");
      }
      motifCount += file.getMotifCount();
      totalCols += file.getTotalCols();
      if (minCols > file.getMinCols()) minCols = file.getMinCols();
      if (maxCols < file.getMaxCols()) maxCols = file.getMaxCols();
      sumVariance += Math.pow(file.getStandardDeviationCols(), 2);
    }
    standardDeviationCols = Math.pow(sumVariance, 0.5);

  }

  public String getName() {
    return this.name;
  }

  public String getDescription() {
    return this.description;
  }

  public List<MotifDBFile> getMotifFiles() {
    return this.files;
  }

  @Override
  public Alphabet getAlphabet() {
    return alphabet;
  }

  @Override
  public int getMotifCount() {
    return motifCount;
  }

  @Override
  public int getTotalCols() {
    return totalCols;
  }

  @Override
  public int getMinCols() {
    return minCols;
  }

  @Override
  public int getMaxCols() {
    return maxCols;
  }

  @Override
  public double getAverageCols() {
    return (double)totalCols / (double)motifCount;
  }

  @Override
  public double getStandardDeviationCols() {
    return standardDeviationCols;
  }

  @Override
  public void outputJson(JsonWr out) throws IOException {
    out.startObject();
    out.property("source", "db");
    out.property("db_name", getName());
    out.property("db_description", getDescription());
    out.property("alphabet", getAlphabet().name());
    out.property("count", getMotifCount());
    out.property("min", getMinCols());
    out.property("max", getMaxCols());
    out.property("avg", getAverageCols());
    out.property("total", getTotalCols());
    out.endObject();
  }
}

