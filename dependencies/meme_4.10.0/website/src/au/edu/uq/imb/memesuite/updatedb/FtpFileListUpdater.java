package au.edu.uq.imb.memesuite.updatedb;

import au.edu.uq.imb.memesuite.data.Alphabet;
import au.edu.uq.imb.memesuite.db.SQL;
import au.edu.uq.imb.memesuite.util.MultiSourceStatus;
import org.apache.commons.net.ftp.FTPClient;
import org.apache.commons.net.ftp.FTPFile;
import org.sqlite.SQLiteDataSource;

import java.io.*;
import java.sql.Connection;
import java.sql.PreparedStatement;
import java.sql.ResultSet;
import java.sql.SQLException;
import java.text.DateFormat;
import java.util.*;
import java.util.concurrent.ExecutionException;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.locks.ReentrantReadWriteLock;
import java.util.logging.Level;
import java.util.logging.Logger;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

/**
 * Reads sequence sources from a csv file
 */
public class FtpFileListUpdater extends SequenceUpdater {
  private static Logger logger = Logger.getLogger("au.edu.uq.imb.memesuite.updatedb.csv");
  private static Pattern SKIP_LINE_RE = Pattern.compile("^\\s*(#.*)?$");
  private static Pattern SOURCE_RE = Pattern.compile("^(ftp://)?([^/]+)(/.*?)([^/]+)$");
  private static final int FTP_FILE_LIST_RETRIEVER_ID = 6;
  private static final int RETRY_COUNT = 3;
  private int retain;

  public FtpFileListUpdater(SQLiteDataSource dataSource,
      ReentrantReadWriteLock dbLock, File binDir, File dbDir,
      ExecutorService worker, MultiSourceStatus status) {
    super("Miscellaneous Source Downloader", dataSource, dbLock, binDir, dbDir, worker, status);
    Properties conf = loadConf(FtpFileListUpdater.class, dbDir, "FtpFileListUpdater.properties");
    retain = getConfInt(conf, "retain", 1, null, 1);
  }

  private void downloadSource(CsvEntry entry, Alphabet alphabet)
      throws ExecutionException, InterruptedException, SQLException {
    String source;
    switch (alphabet) {
      case DNA: source = entry.dnaUrl; break;
      case PROTEIN: source = entry.proteinUrl; break;
      default: throw new IllegalArgumentException("Only DNA or protein sources accepted");
    }
    if (!source.isEmpty()) {
      Matcher m = SOURCE_RE.matcher(source);
      if (m.matches()) {
        String host = m.group(2);
        String path = m.group(3);
        String name = m.group(4);
        // now attempt downloading it
        FTPClient ftp = null;
        try {
          ftp = new FTPClient();
          if (!loginFtp(ftp, host)) throw new IOException("Unable to login to remote host: " + host);
          FTPFile sourceFile = ftp.mlistFile(path + name);
          if (sourceFile == null) throw new IOException("Unable to find remote file: " + path + name);
          Calendar timestamp = sourceFile.getTimestamp();
          timestamp.getTimeInMillis();
          CsvSource ftpSrc = new CsvSource(entry, alphabet,
              host, path, name, sourceFile.getSize(), sourceFile.getTimestamp());
          if (!sourceExists(ftpSrc, true)) {
            if (!downloadFtpSource(ftp, ftpSrc, true, RETRY_COUNT)) return;
            enqueueSequences(new CsvSequenceProcessor(ftpSrc));
          }
          ftp.logout();
          ftp.disconnect();
        } catch (IOException e) {
          logger.log(Level.SEVERE, "Skipping source: " + source, e);
        } finally {
          if (ftp != null && ftp.isConnected()) {
            try {
              ftp.logout();
            } catch (IOException e) { /* ignore */ }
            try {
              ftp.disconnect();
            } catch (IOException e) { /* ignore */ }
          }
        }
      } else {
        logger.log(Level.WARNING, "Source does not match FTP URL pattern:\n" + source);
      }
    }
  }

  public void markRemovedSourcesObsolete(File[] csvFiles) throws IOException, SQLException {
    Connection conn = null;
    PreparedStatement ps, psCategory, psListing;
    ResultSet rs;
    SeqSourceReader reader = null;
    CsvEntry entry;
    dbLock.writeLock().lock();
    try {
      conn = dataSource.getConnection();
      conn.setAutoCommit(false);
      // initially flag all non-obsolete sources from this retriever
      ps = conn.prepareStatement(SQL.MARK_RETRIEVER_FILES_FLAGGED);
      ps.setInt(1, FTP_FILE_LIST_RETRIEVER_ID);
      ps.executeUpdate();
      ps.close();
      // prepare some SQL statements
      ps = conn.prepareStatement(SQL.MARK_RETRIEVER_LISTING_OK);
      psCategory = conn.prepareStatement(SQL.SELECT_CATEGORY_BY_NAME);
      psListing = conn.prepareStatement(SQL.SELECT_LISTING_BY_NAME);
      // now unflag sources listed in csv files
      String currentCategory = null;
      Long categoryId = null;
      Long listingId;
      for (File csv : csvFiles) {
        progress.setName(csv.getName() + " Downloader");
        reader = new SeqSourceReader(csv);
        while ((entry = reader.next()) != null) {
          // get the category in the db
          if (!entry.category.equals(currentCategory)) {
            currentCategory = entry.category;
            psCategory.setString(1, entry.category);
            rs = psCategory.executeQuery();
            if (rs.next()) {
              categoryId = rs.getLong(1);
            } else {
              categoryId = null;
            }
            rs.close();
          }
          if (categoryId == null) continue;
          // get the listing
          psListing.setLong(1, categoryId);
          psListing.setString(2, entry.name);
          rs = psListing.executeQuery();
          if (rs.next()) {
            listingId = rs.getLong(1);
          } else {
            listingId = null;
          }
          rs.close();
          if (listingId == null) continue;
          // unflag the listing
          ps.setInt(1, FTP_FILE_LIST_RETRIEVER_ID);
          ps.setLong(2, listingId);
          ps.executeUpdate();
        }
        reader.close(); reader = null;
      }
      ps.close();
      psCategory.close();
      psListing.close();
      // mark anything that is still flagged as obsolete
      ps = conn.prepareStatement(SQL.MARK_FLAGGED_RETRIEVER_FILES_OBSOLETE);
      ps.setInt(1, FTP_FILE_LIST_RETRIEVER_ID);
      ps.executeUpdate();
      ps.close();
      // commit the changes
      conn.commit();
      conn.close();
      conn = null;
    } finally {
      if (reader != null) {
        try {
          reader.close();
        } catch (IOException e) { /* ignore */ }
      }
      if (conn != null) {
        try {
          conn.rollback();
        } catch (SQLException e) {/* ignore */}
        try {
          conn.close();
        } catch (SQLException e) {/* ignore */}
      }
      dbLock.writeLock().unlock();
    }
  }

  public void downloadSources(File[] csvFiles)
      throws IOException, ExecutionException, InterruptedException, SQLException {
    SeqSourceReader reader = null;
    CsvEntry entry;
    try {
      // download sources listed in csv files
      for (File csv : csvFiles) {
        progress.setName(csv.getName() + " Downloader");
        reader = new SeqSourceReader(csv);
        while ((entry = reader.next()) != null) {
          logger.log(Level.INFO, "Downloading entry " + entry.name);
          downloadSource(entry, Alphabet.DNA);
          downloadSource(entry, Alphabet.PROTEIN);
          checkWorkerTasks();
        }
        reader.close(); reader = null;
      }
    } finally {
      if (reader != null) {
        try {
          reader.close();
        } catch (IOException e) { /* ignore */ }
      }
    }
  }

  @Override
  public Void call() throws Exception {
    try {
      // get csv files in conf directory
      File confDir = new File(dbDir, "conf");
      File[] csvFiles = confDir.listFiles(new FileFilter() {
        @Override
        public boolean accept(File file) {
          return file.isFile() && file.getName().endsWith(".csv");
        }
      });
      // mark removed sources as obsolete
      markRemovedSourcesObsolete(csvFiles);
      // download sources listed in csv files
      downloadSources(csvFiles);
      // check that we actually tried to do something
      if (csvFiles.length == 0) {
        logger.log(Level.WARNING, "No csv files for miscellaneous sources in: " + confDir + "\n" +
            "(you probably want to copy in the default CSV files provided in the distribution's etc folder)");
        progress.setTask("No CSV's provided");
      }
    } catch (Exception e) {
      logger.log(Level.SEVERE, "Abandoning update", e);
      throw e;
    } finally {
      progress.complete();
    }
    return null;  // generated code
  }

  private class SeqSourceReader {
    private BufferedReader reader;
    private String category = "Uncategorised";

    public SeqSourceReader(File csv) throws FileNotFoundException, UnsupportedEncodingException {
      reader =  new BufferedReader(new InputStreamReader(new FileInputStream(csv), "UTF-8"));
    }
    public CsvEntry next() throws IOException {
      String line;
      Pattern categoryPattern = Pattern.compile("^-*(.*?)-*$");
      while ((line = reader.readLine()) != null) {
        if (SKIP_LINE_RE.matcher(line).matches()) continue;
        String[] parts = line.split(",");
        if (parts.length < 5) {
          logger.log(Level.WARNING, "Line does not contain enough values to be a category");
          continue;
        }
        String root = parts[0].trim();
        String name = parts[4].trim();
        if (root.length() == 0) {
          // category
          Matcher m = categoryPattern.matcher(name);
          if (m.matches()) {
            category = m.group(1);
          } else {
            category = name;
          }
        } else {
          // listing
          if (parts.length < 7) {
            logger.log(Level.WARNING, "Line does not contain enough values");
            continue;
          }
          String description = parts[5].trim();
          String proteinUrl = parts[6].trim();
          String dnaUrl = parts.length >= 8 ? parts[7].trim() : "";
          return new CsvEntry(category, root, name, description, dnaUrl, proteinUrl);
        }
      }
      return null;
    }
    public void close() throws IOException {
      reader.close();
    }
  }

  private class CsvSequenceProcessor extends SequenceProcessor {
    private CsvSource source;

    private CsvSequenceProcessor(CsvSource source) {
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

  private class CsvSource extends AbstractFtpSource {
    private final CsvEntry entry;
    private final Alphabet alphabet;
    private final String host;
    private final String dir;
    private final String name;
    private final long size;
    private final Calendar timestamp;
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

    public CsvSource(CsvEntry entry, Alphabet alphabet, String host, String dir,
        String name, long size, Calendar timestamp) {
      this.entry = entry;
      this.alphabet = alphabet;
      this.host = host;
      this.dir = dir;
      this.name = name;
      this.size = size;
      this.timestamp = timestamp;
    }

    public String toString() {
      return entry.name + " " + getSequenceVersion();
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
      return FTP_FILE_LIST_RETRIEVER_ID;
    }

    @Override
    public String getCategoryName() {
      return entry.category;
    }

    @Override
    public String getListingName() {
      return entry.name;
    }

    @Override
    public String getListingDescription() {
      return entry.description;
    }

    @Override
    public long getSequenceEdition() {
      return timestamp.getTimeInMillis();
    }

    @Override
    public String getSequenceVersion() {
      return DateFormat.getDateInstance(DateFormat.MEDIUM).format(timestamp.getTime());
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
      return entry.root + "_" + alphabet.name() + "_" + timestamp.getTimeInMillis();
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
      return alphabet;
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

  private static class CsvEntry {
    public final String category;
    public final String root;
    public final String name;
    public final String description;
    public final String dnaUrl;
    public final String proteinUrl;
    public CsvEntry(String category, String root, String name,
        String description, String dnaUrl, String proteinUrl) {
      this.category = category;
      this.root = root;
      this.name = name;
      this.description = description;
      this.dnaUrl = dnaUrl;
      this.proteinUrl = proteinUrl;
    }
  }
}
