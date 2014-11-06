package au.edu.uq.imb.memesuite.db;


import au.edu.uq.imb.memesuite.data.Alphabet;

import java.util.EnumSet;

public class Listing {
  private long id;
  private String name;
  private String description;
  private EnumSet<Alphabet> alphabets;

  public Listing(long id, String name, String description, EnumSet<Alphabet> alphabets) {
    this.id = id;
    this.name = name;
    this.description = description;
    this.alphabets = alphabets;
  }

  public long getID() {
    return this.id;  
  }

  public String getName() {
    return this.name;
  }

  public String getDescription() {
    return this.description;
  }

  public EnumSet<Alphabet> getAlphabets() {
    return this.alphabets;
  }
}
