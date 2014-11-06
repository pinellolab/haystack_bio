package au.edu.uq.imb.memesuite.data;

import au.edu.uq.imb.memesuite.util.JsonWr;
import au.edu.uq.imb.memesuite.util.SampleStats;

import java.io.IOException;

public class MotifStats implements MotifInfo {
  private boolean frozen;
  private Alphabet alphabet;
  private SampleStats stats;

  public MotifStats() {
    this.frozen = false;
    this.alphabet = null;
    this.stats = new SampleStats(true);
  }

  public void setAlphabet(Alphabet alphabet) {
    if (this.frozen) throw new IllegalStateException("MotifStats is frozen.");
    this.alphabet = alphabet;
  }

  public void addMotif(int cols) {
    if (this.frozen) throw new IllegalStateException("MotifStats is frozen.");
    this.stats.update(cols);
  }

  public void freeze() {
    this.frozen = true;
  }

  public Alphabet getAlphabet() {
    return this.alphabet;
  }

  public int getMotifCount() {
    return (int)this.stats.getCount();
  }

  public int getTotalCols() {
    return this.stats.getTotal().intValue();
  }

  public int getMinCols() {
    return (int)this.stats.getSmallest();
  }

  public int getMaxCols() {
    return (int)this.stats.getLargest();
  }

  public double getAverageCols() {
    return this.stats.getMean();
  }

  public double getStandardDeviationCols() {
    return this.stats.getStandardDeviation();
  }

  public String toString() {
    return "MotifStats[ Alphabet:" + this.alphabet +
      ", Count:" + this.getMotifCount() + ", Min-Columns:" + this.getMinCols() +
      ", Max-Columns:" + this.getMaxCols() + ", Average-Columns:" + this.getAverageCols() + " ]";
  }

  @Override
  public void outputJson(JsonWr out) throws IOException {
    out.startObject();
    out.property("type", "motif");
    out.property("alphabet", getAlphabet().name());
    out.property("count", getMotifCount());
    out.property("min", getMinCols());
    out.property("max", getMaxCols());
    out.property("avg", getAverageCols());
    out.property("total", getTotalCols());
    out.endObject();
  }
}
