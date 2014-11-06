package au.edu.uq.imb.memesuite.updatedb;

import java.io.File;

/**
 * Interface containing information about a FTP source.
 */
public interface FtpSource {
  public String getRemoteHost();
  public String getRemoteDir();
  public String getRemoteName();
  public long getRemoteSize();
  public String getRemotePath();
  public String getRemoteUrl();
  public String getLocalName();
  public void setSourceFile(File file);
}
