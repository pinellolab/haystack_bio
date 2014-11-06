package au.edu.uq.imb.memesuite.data;

import au.edu.uq.imb.memesuite.util.JsonWr;

import java.io.IOException;

/**
 * Stores the results of a background selection
 */
public class Background implements JsonWr.JsonValue {
  public enum Source {
    ORDER_0(0),
    ORDER_1(1),
    ORDER_2(2),
    ORDER_3(3),
    ORDER_4(4),
    FILE(-1),
    UNIFORM(-2),
    MEME(-3);

    private int modelOrder;

    private Source(int modelOrder) {
      this.modelOrder = modelOrder;
    }

    public boolean isGeneratedModel() {
      return this.modelOrder >= 0;
    }

    public int getGeneratedOrder() {
      return this.modelOrder;
    }
  }
  private Source source;
  private NamedFileDataSource bfile;

  public Background(Source source, NamedFileDataSource bfile) {
    this.source = source;
    this.bfile = bfile;
  }

  public Source getSource() {
    return this.source;
  }

  public NamedFileDataSource getBfile() {
    if (this.source != Source.FILE) throw new IllegalStateException(
        "Can't get the file of a background that does not have one!");
    return this.bfile;
  }

  @Override
  public void outputJson(JsonWr out) throws IOException {
    out.startObject();
    out.property("source", source.name());
    if (source.isGeneratedModel()) out.property("order", source.getGeneratedOrder());
    if (source == Source.FILE) out.property("file", bfile);
    out.endObject();
  }
}
