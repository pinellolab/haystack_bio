package au.edu.uq.imb.memesuite.data;

import au.edu.uq.imb.memesuite.util.FileCoord;
import au.edu.uq.imb.memesuite.util.JsonWr;

import java.io.File;
import java.io.IOException;
import javax.activation.FileDataSource;

/**
 * Allows specifing an alternate name for a FileDataSource.
 * This can be helpful if the actual file name is just auto-generated
 * to avoid collisions.
 */
public class NamedFileDataSource extends FileDataSource implements JsonWr.JsonValue {
  protected FileCoord.Name name;

  public NamedFileDataSource(File file, FileCoord.Name name) {
    super(file);
    this.name = name;
  }

  @Override
  public String getName() {
    return this.name.getSafeName();
  }

  /**
   * gets the true un-sanitized source name.
   * @return The real source name
   */
  public String getOriginalName() {
    return this.name.getOriginalName();
  }

  @Override
  public void outputJson(JsonWr out) throws IOException {
    out.startObject();
    out.property("safe-file", getName());
    out.property("orig-file", getOriginalName());
    out.endObject();
  }
}
