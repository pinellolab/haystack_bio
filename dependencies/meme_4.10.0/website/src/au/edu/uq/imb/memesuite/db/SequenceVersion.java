package au.edu.uq.imb.memesuite.db;

import au.edu.uq.imb.memesuite.data.Alphabet;

import java.util.*;

/**
 * Object to store information related to a sequence database version.
 */
public class SequenceVersion {
  private EnumMap<Alphabet,SequenceDB> sequenceFileMap;
  private long edition;
  private String version;

  public SequenceVersion(long edition, String version, Collection<SequenceDB> sequenceDBs) {
    this.sequenceFileMap = new EnumMap<Alphabet, SequenceDB>(Alphabet.class);
    this.edition = edition;
    this.version = version;
    for (SequenceDB file : sequenceDBs) {
      sequenceFileMap.put(file.guessAlphabet(), file);
    }
  }

  public Set<Alphabet> getAlphabets() {
    return sequenceFileMap.keySet();
  }

  public long getEdition() {
    return edition;
  }

  public String getVersion() {
    return version;
  }

  public SequenceDB getSequenceFile(Alphabet alphabet) {
    return sequenceFileMap.get(alphabet);
  }


}
