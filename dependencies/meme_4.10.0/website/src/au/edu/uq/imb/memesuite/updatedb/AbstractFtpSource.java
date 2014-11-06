package au.edu.uq.imb.memesuite.updatedb;

import au.edu.uq.imb.memesuite.util.JsonWr;

import java.io.IOException;

/**
 * Partial implementation of FtpSource
 */
public abstract class AbstractFtpSource implements Source, FtpSource {
  public abstract String getRemoteHost();
  public abstract String getRemoteDir();
  public abstract String getRemoteName();
  public abstract long getRemoteSize();
  public String getRemotePath() {
    return getRemoteDir() + getRemoteName();
  }
  public String getRemoteUrl() {
    return "ftp://" + getRemoteHost() + getRemotePath();
  }
  public String getRemoteExt() {
    String name = getRemoteName();
    int firstDot = name.indexOf('.');
    if (firstDot != -1) {
      return name.substring(firstDot);
    }
    return "";
  }
  public String getLocalName() {
    return getNamePrefix() + getRemoteExt();
  }

  @Override
  public void outputJson(JsonWr out) throws IOException {
    out.value((JsonWr.JsonValue)null);
  }
}
