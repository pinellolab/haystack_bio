package au.edu.uq.imb.memesuite.updatedb;

import org.apache.commons.net.ftp.FTPFile;
import org.apache.commons.net.ftp.FTPFileFilter;

import java.util.regex.Pattern;

/**
 * Ranks files by their match to a series of regular expression patterns.
 * Will accept files that match any of the patterns but when asked for the
 * best will return the first file that matches the earliest pattern.
 */
public class PatternFileFilter implements FTPFileFilter {
  private Pattern[] patterns;

  public PatternFileFilter(Pattern[] patterns) {
    this.patterns = patterns;
  }

  public int priority(String name) {
    for (int i = 0; i < patterns.length; i++) {
      if (patterns[i].matcher(name).find()) {
        return i + 1;
      }
    }
    return 0;
  }

  public FTPFile best(FTPFile[] ftpFiles) {
    FTPFile best = null;
    int bestPriority = patterns.length + 1;
    for (FTPFile ftpFile : ftpFiles) {
      int priority = priority(ftpFile.getName());
      if (priority == 1) return ftpFile;
      if (priority != 0 && priority < bestPriority) {
        bestPriority = priority;
        best = ftpFile;
      }
    }
    return best;
  }

  @Override
  public boolean accept(FTPFile ftpFile) {
    return ftpFile != null && !ftpFile.isDirectory() && priority(ftpFile.getName()) > 0;
  }
}
