package au.edu.uq.imb.memesuite.db;

public class Category {
  private long id;
  private String name;

  public Category(long id, String name) {
    this.id = id;
    this.name = name;
  }

  public long getID() {
    return this.id;
  }

  public String getName() {
    return this.name;
  }
}
