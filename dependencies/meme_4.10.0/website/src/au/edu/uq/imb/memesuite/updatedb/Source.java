package au.edu.uq.imb.memesuite.updatedb;

import au.edu.uq.imb.memesuite.data.Alphabet;
import au.edu.uq.imb.memesuite.data.SequenceInfo;

import java.io.File;
import java.util.List;

/**
 * Keeps track of information about a sequence source.
 */
public interface Source extends SequenceInfo {
  public int getRetrieverId();
  public String getCategoryName();
  public String getListingName();
  public String getListingDescription();
  public long getSequenceEdition();
  public String getSequenceVersion();
  public String getSequenceDescription();
  public String getNamePrefix();
  public List<File> getSourceFiles();
  public void setSequenceFile(File file);
  public File getSequenceFile();
  public void setBgFile(File file);
  public File getBgFile();
  public void setStats(long count, long minLen, long maxLen, double avgLen, double stdDLen, long totalLen);
}
