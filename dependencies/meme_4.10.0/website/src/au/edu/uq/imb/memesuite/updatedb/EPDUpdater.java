package au.edu.uq.imb.memesuite.updatedb;

import au.edu.uq.imb.memesuite.data.Alphabet;
import au.edu.uq.imb.memesuite.util.MultiSourceStatus;
import org.apache.commons.net.ftp.FTPClient;
import org.apache.commons.net.ftp.FTPFile;
import org.apache.commons.net.ftp.FTPFileFilter;
import org.sqlite.SQLiteDataSource;

import java.io.File;
import java.io.IOException;
import java.sql.SQLException;
import java.util.*;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.locks.ReentrantReadWriteLock;
import java.util.logging.Level;
import java.util.logging.Logger;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

/**
 * Downloads the Eukaryotic Promoter Database
 */
public class EPDUpdater extends SequenceUpdater {
  private static final int EPD_RETRIEVER_ID = 5;
  private static Logger logger = Logger.getLogger("au.edu.uq.imb.memesuite.updatedb.epd");
  private static final String HOST = "ftp.epd.unil.ch";
  private static final String DIR = "/epd/views/";
  private static final Pattern NUM_RE = Pattern.compile("^epd(\\d+)\\.seq$");
  private int retain;
  private String category;
  private String listingName;
  private String listingDescription;

  public EPDUpdater(SQLiteDataSource dataSource, ReentrantReadWriteLock dbLock,
      File binDir, File dbDir, ExecutorService worker, MultiSourceStatus status) {
    super("EPD Downloader", dataSource, dbLock, binDir, dbDir, worker, status);

    Properties conf = loadConf(EPDUpdater.class, dbDir, "EPDUpdater.properties");
    retain = getConfInt(conf, "retain", 1, null, 1);
    category = conf.getProperty("category.name", "Non-redundant and Special Databases");
    listingName = conf.getProperty("listing.name", "Eukaryotic Promoter Database");
    listingDescription = conf.getProperty("listing.description",
        "<a href=\"http://epd.vital-it.ch\">Eucaryotic Promoter Database</a>");
  }

  @Override
  public Void call() throws Exception {
    FTPClient ftp = null;
    try {
      ftp = new FTPClient();
      if (!loginFtp(ftp, HOST)) throw new IOException("Unable to login to remote host: " + HOST);
      // find available seq files
      progress.setTask("Querying files");
      FTPFile[] files = ftp.listFiles(DIR, new EpdSeqFileFilter());
      progress.setTask("Processing files");
      logger.log(Level.INFO, "Found " + files.length + " EPD versions available for download.");
      Arrays.sort(files, new EpdSeqFileVersionComparator());
      for (int i = 0; i < retain && i < files.length; i++) {
        progress.setTask("Processing files", i, files.length);
        EpdSource ftpSrc = new EpdSource(HOST, DIR, files[i]);
        if (!sourceExists(ftpSrc, true)) {
          logger.log(Level.INFO, "Downloading: " + ftpSrc.getRemoteUrl());
          if (!downloadFtpSource(ftp, ftpSrc, true)) return null;
          enqueueSequences(new EpdSequenceProcessor(ftpSrc));
        } else {
          logger.log(Level.INFO, "Skipping already downloaded file: " + ftpSrc.getRemoteUrl());
        }

      }
      ftp.logout();
      ftp.disconnect();
    } catch (Exception e) {
      logger.log(Level.SEVERE, "Abandoning update", e);
      throw e;
    } finally {
      if (ftp != null && ftp.isConnected()) {
        try {
          ftp.logout();
        } catch (IOException e) { /* ignore */ }
        try {
          ftp.disconnect();
        } catch (IOException e) { /* ignore */ }
      }
      progress.complete();
    }
    return null;  // generated code
  }

  private static  int version(FTPFile file) throws IllegalArgumentException {
    Matcher m = NUM_RE.matcher(file.getName());
    if (!m.matches()) throw new IllegalArgumentException("Expected Seq File");
    return Integer.parseInt(m.group(1), 10);
  }

  private static class EpdSeqFileFilter implements FTPFileFilter {
    @Override
    public boolean accept(FTPFile ftpFile) {
      return ftpFile.isFile() && NUM_RE.matcher(ftpFile.getName()).matches();
    }
  }

  private static class EpdSeqFileVersionComparator implements Comparator<FTPFile> {
    @Override
    public int compare(FTPFile file1, FTPFile file2) {
      try {
        return version(file2) - version(file1);
      } catch (IllegalArgumentException e) { /* ignore */ }
      return file1.getName().compareTo(file2.getName()); // fallback
    }
  }



  private class EpdSequenceProcessor extends SequenceProcessor {
    private Source source;

    private EpdSequenceProcessor(Source source) {
      super(dataSource, dbLock, binDir, dbDir, status);
      this.source = source;
    }

    @Override
    public void process() throws IOException, SQLException, InterruptedException {
      if (!unpackSequences(source)) return;
      processSequences(source);
      obsoleteOldEditions(recordSequences(source).listingId, source.guessAlphabet(), retain);
    }
  }

  private class EpdSource extends AbstractFtpSource {
    private final String host;
    private final String dir;
    private final String name;
    private final long size;
    private int version;
    private File downloadedFile;
    // generated files
    private File sequenceFile;
    private File bgFile;
    // sequence properties
    private long count;
    private long minLen;
    private long maxLen;
    private double avgLen;
    private double stdDLen;
    private long totalLen;

    public EpdSource(String host, String dir, FTPFile ftpFile) {
      this.host = host;
      this.dir = dir;
      this.name = ftpFile.getName();
      this.size = ftpFile.getSize();
      this.version = version(ftpFile);
    }

    public String toString() {
      return "EPD v" + version;
    }

    /*
    ** FtpSource methods
     */

    @Override
    public String getRemoteHost() {
      return host;
    }

    @Override
    public String getRemoteDir() {
      return dir;
    }

    @Override
    public String getRemoteName() {
      return name;
    }

    @Override
    public long getRemoteSize() {
      return size;
    }

    @Override
    public void setSourceFile(File file) {
      this.downloadedFile = file;
    }

    /*
    ** Source methods
     */

    @Override
    public int getRetrieverId() {
      return EPD_RETRIEVER_ID;
    }

    @Override
    public String getCategoryName() {
      return category;
    }

    @Override
    public String getListingName() {
      return listingName;
    }

    @Override
    public String getListingDescription() {
      return listingDescription;
    }

    @Override
    public long getSequenceEdition() {
      return version;
    }

    @Override
    public String getSequenceVersion() {
      return Integer.toString(version);
    }

    @Override
    public String getSequenceDescription() {
      StringBuilder builder = new StringBuilder();
      builder.append("Downloaded from ");
      builder.append("<a href=\"").append(this.getRemoteUrl()).append("\">");
      builder.append(this.getRemoteUrl());
      builder.append("</a>.");
      return builder.toString();
    }

    @Override
    public String getNamePrefix() {
      return "epd_" + version;
    }

    @Override
    public List<File> getSourceFiles() {
      return Collections.singletonList(downloadedFile);
    }

    @Override
    public void setSequenceFile(File file) {
      this.sequenceFile = file;
    }

    @Override
    public File getSequenceFile() {
      return sequenceFile;
    }

    @Override
    public void setBgFile(File file) {
      this.bgFile = file;
    }

    @Override
    public File getBgFile() {
      return bgFile;
    }

    @Override
    public void setStats(long count, long minLen, long maxLen, double avgLen,
        double stdDLen, long totalLen) {
      this.count = count;
      this.minLen = minLen;
      this.maxLen = maxLen;
      this.avgLen = avgLen;
      this.stdDLen = stdDLen;
      this.totalLen = totalLen;
    }

    /*
    ** SequenceInfo methods
     */

    @Override
    public Alphabet guessAlphabet() {
      return Alphabet.DNA;
    }

    @Override
    public long getSequenceCount() {
      return count;
    }

    @Override
    public long getTotalLength() {
      return totalLen;
    }

    @Override
    public long getMinLength() {
      return minLen;
    }

    @Override
    public long getMaxLength() {
      return maxLen;
    }

    @Override
    public double getAverageLength() {
      return avgLen;
    }

    @Override
    public double getStandardDeviationLength() {
      return stdDLen;
    }
  }
}
