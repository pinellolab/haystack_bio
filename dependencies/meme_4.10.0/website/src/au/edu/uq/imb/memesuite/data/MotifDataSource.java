package au.edu.uq.imb.memesuite.data;

import au.edu.uq.imb.memesuite.servlet.util.ComponentMotifs;
import au.edu.uq.imb.memesuite.util.FileCoord;
import au.edu.uq.imb.memesuite.util.JsonWr;

import java.io.File;
import java.io.IOException;
import java.util.*;

/**
 * A datasource containing motifs with statistics on the motifs.
 */
public class MotifDataSource extends NamedFileDataSource implements MotifInfo {
  private MotifStats statistics;
  private List<ComponentMotifs.Selection> selections;

  public MotifDataSource(File file, FileCoord.Name name, MotifStats statistics,
      List<ComponentMotifs.Selection> selections) {
    super(file, name);
    this.statistics = statistics;
    if (selections != null)
      this.selections = Collections.unmodifiableList(new ArrayList<ComponentMotifs.Selection>(selections));
    else
      this.selections = Collections.emptyList();
  }

  @Override
  public Alphabet getAlphabet() {
    return statistics.getAlphabet();
  }

  @Override
  public int getMotifCount() {
    return statistics.getMotifCount();
  }

  @Override
  public int getTotalCols() {
    return statistics.getTotalCols();
  }

  @Override
  public int getMinCols() {
    return statistics.getMinCols();
  }

  @Override
  public int getMaxCols() {
    return statistics.getMaxCols();
  }

  @Override
  public double getAverageCols() {
    return statistics.getAverageCols();
  }

  @Override
  public double getStandardDeviationCols() {
    return statistics.getStandardDeviationCols();
  }

  public List<ComponentMotifs.Selection> getSelections() {
    return selections;
  }

  @Override
  public void outputJson(JsonWr out) throws IOException {
    out.startObject();
    if (getOriginalName() != null) {
      out.property("source", "file");
      out.property("safe-file", getName());
      out.property("orig-file", getOriginalName());
    } else {
      out.property("source", "text");
    }
    out.property("alphabet", getAlphabet().name());
    out.property("count", getMotifCount());
    out.property("min", getMinCols());
    out.property("max", getMaxCols());
    out.property("avg", getAverageCols());
    out.property("total", getTotalCols());
    out.endObject();
  }
}
