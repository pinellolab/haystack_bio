package au.edu.uq.imb.memesuite.util;

import java.util.regex.Matcher;
import java.util.regex.Pattern;

/**
 * This utility class stores a semantic version as defined at
 * <a href="http://semver.org/">http://semver.org/</a>.
 */
public class SemanticVersion implements Comparable<SemanticVersion> {
  private int[] version;
  private String prerel;
  private String metadata;

  private static final String versionRE = "(\\d+)(?:\\.(\\d+)(?:\\.(\\d+))?)?";
  private static final String tokenRE = "[1-9a-zA-Z-][0-9a-zA-Z-]*";
  private static final String tokensRE = tokenRE+"(?:\\."+tokenRE+")*";
  private static final String prerelRE = "(?:-("+tokensRE+"))?";
  private static final String metadataRE = "(?:\\+("+tokensRE+"))?";
  private static Pattern tokensPattern = Pattern.compile("^" + tokenRE + "$");
  private static Pattern versionPattern = Pattern.compile("^\\s*" + versionRE + prerelRE + metadataRE + "\\s*$");

  public SemanticVersion(int major, int minor, int patch) {
    this(major, minor, patch, null, null);
  }

  public SemanticVersion(int major, int minor, int patch, String prerelease, String metadata) {
    if (major < 0 || minor < 0 || patch < 0) {
      throw new  IllegalArgumentException("The version numbers are not allowed to be negative");
    }
    if (prerelease != null && !tokensPattern.matcher(prerelease).matches()) {
      throw new IllegalArgumentException(
          "The pre-release string must either be null or one or more dot separated identifiers.");
    }
    if (metadata != null && !tokensPattern.matcher(metadata).matches()) {
      throw new IllegalArgumentException(
          "The metadata must be either null or one or more dot separated identifiers.");
    }
    version = new int[]{major, minor, patch};
    this.prerel = prerelease;
    this.metadata = metadata;
  }

  public boolean isCompatible(SemanticVersion other) {
    return this.version[0] == other.version[0];
  }

  @Override
  public int compareTo(SemanticVersion other) {
    if (this.version[0] < other.version[0]) {
      return -1;
    } else if (this.version[0] > other.version[0]) {
      return 1;
    } else {
      if (this.version[1] < other.version[1]) {
        return -1;
      } else if (this.version[1] > other.version[1]) {
        return 1;
      } else {
        if (this.version[2] < other.version[2]) {
          return -1;
        } else if (this.version[2] > other.version[2]) {
          return 1;
        } else {
          return 0;
        }
      }
    }
  }

  @Override
  public boolean equals(Object other) {
    if (other instanceof SemanticVersion) {
      SemanticVersion o = (SemanticVersion)other;
      return (this.version[0] == o.version[0]) &&
          (this.version[1] == o.version[1]) &&
          (this.version[2] == o.version[2]);
    }
    return false;
  }

  @Override
  public String toString() {
    return "" + version[0] + "." + version[1] + "." + version[2]
        + (prerel != null ? "-" + prerel : "")
        + (metadata != null ? "+" + metadata : "");
  }

  public static SemanticVersion parse(String versionString) {
    int major, minor = 0, patch = 0;
    Matcher matcher = versionPattern.matcher(versionString);
    if (matcher.matches()) {
      major = Integer.parseInt(matcher.group(1), 10);
      if (matcher.group(2) != null) minor = Integer.parseInt(matcher.group(2), 10);
      if (matcher.group(3) != null) patch = Integer.parseInt(matcher.group(3), 10);
      return new SemanticVersion(major, minor, patch, matcher.group(4), matcher.group(5));
    } else {
      throw new IllegalArgumentException("Invalid version string");
    }
  }
}
