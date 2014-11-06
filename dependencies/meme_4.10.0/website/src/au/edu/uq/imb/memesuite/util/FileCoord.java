package au.edu.uq.imb.memesuite.util;

import java.util.*;
import java.util.regex.Pattern;
import java.io.File;
import java.io.IOException;

/**
 * This coordinates the names of the files trying to use the original name where
 * possible but otherwise avoiding file name collisions.
 */
public class FileCoord {
  private List<FileCoord.Name> names;
  private Map<String, FileCoord.Name> lookup;
  private boolean dirty;

  private static Pattern invalidName = Pattern.compile(
      "^(?:\\.|\\.\\.|com[1-9]|lpt[1-9]|con|nul|prn|aux)(\\..*)?$", Pattern.CASE_INSENSITIVE);
  
  public FileCoord() {
    this.names = new ArrayList<FileCoord.Name>();
    this.lookup = new TreeMap<String, FileCoord.Name>();
    this.dirty = true;
  }

  public FileCoord.Name createName(String defaultFileName) {
    if (this.lookup.containsKey(defaultFileName)) {
      throw new IllegalStateException("That default file name has already been used.");
    }
    FileCoord.Name name = this.new Name(defaultFileName);
    this.names.add(name);
    this.lookup.put(defaultFileName, name);
    this.dirty = true;
    return name;
  }

  public FileCoord.Name getName(String defaultFileName) {
    return this.lookup.get(defaultFileName);
  }

  public List<FileCoord.Name> getNames() {
    return Collections.unmodifiableList(this.names);
  }

  /**
   * Removes parts of a file name that may not be safe to use in the
   * file-system or shell.
   * @param name The file name to sanitize 
   * @param defaultName The file name to use when the original is null or
   * nothing is left after sanitizing.
   */
  protected static String sanitizeName(String name, String defaultName) {
    if (name == null) return defaultName;
    // convert spaces to underscores
    String safeName = name.replaceAll("\\s", "_");
    // limit to whitelisted characters
    safeName = safeName.replaceAll("[^a-zA-Z0-9_,\\.\\-]", "");
    // remove doubled up or leading underscores
    safeName = safeName.replaceAll("_{2,}", "_").replaceAll("^_", "");
    // remove doubled up dashes
    safeName = safeName.replaceAll("-{2,}", "-");
    // remove doubled up dots
    safeName = safeName.replaceAll("\\.{2,}", ".");
    // check for a reasonable length
    if (safeName.length() < 4) {
      return defaultName;
    }
    // check for reserved names
    if (invalidName.matcher(safeName).matches()) {
      return defaultName;
    }
    // check that a file could be called that
    try {
      File f = new File(System.getProperty("java.io.tmpdir"), safeName);
      // if the name is not safe this should throw an exception...
      f.getCanonicalPath();
      return safeName;
    } catch (IOException e) {
      return defaultName;
    }
  }

  protected void moveConflicts(Map<String, FileCoord.Name> names, String clearName) {
    FileCoord.Name name = names.get(clearName);
    if (name != null) {
      moveConflicts(names, name.defaultName);
      names.put(name.defaultName, name);
    }
  }

  protected void calculateSafeNames() {
    Map<String, FileCoord.Name> names = new TreeMap<String, FileCoord.Name>();
    for (int i = 0; i < this.names.size(); i++) {
      FileCoord.Name name = this.names.get(i);
      String safeName = sanitizeName(name.originalName, name.defaultName);
      if (names.containsKey(safeName)) {
        moveConflicts(names, name.defaultName);
        names.put(name.defaultName, name);
      } else {
        names.put(safeName, name);
      }
    }
    for (Map.Entry<String, FileCoord.Name> pair : names.entrySet()) {
      pair.getValue().safeName = pair.getKey();
    }
    this.dirty = false;
  }

  public class Name {
    private String defaultName;
    private String originalName;
    private String safeName;

    public Name(String defaultName) {
      this.defaultName = defaultName;
      this.originalName = null;
      this.safeName = null;
    }

    public void setOriginalName(String originalName) {
      FileCoord.this.dirty = true;
      this.originalName = originalName;
    }

    public String getOriginalName() {
      return this.originalName;
    }

    public String getSafeName() {
      if (FileCoord.this.dirty) FileCoord.this.calculateSafeNames();
      return this.safeName;
    }

  }

}
