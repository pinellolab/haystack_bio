package au.edu.uq.imb.memesuite.updatedb;

import au.edu.uq.imb.memesuite.data.Alphabet;
import au.edu.uq.imb.memesuite.util.MultiSourceStatus;
import org.apache.commons.net.ftp.*;
import org.jsoup.Jsoup;
import org.jsoup.nodes.Document;
import org.jsoup.nodes.Element;
import org.jsoup.select.Elements;
import org.sqlite.SQLiteDataSource;

import java.io.*;
import java.sql.SQLException;
import java.util.*;
import java.util.concurrent.ExecutionException;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.TimeUnit;
import java.util.concurrent.locks.ReentrantReadWriteLock;
import java.util.logging.Level;
import java.util.logging.Logger;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

/**
 * Query and download available genomes from UCSC's Genome Browser
 */
public class UCSCGenomeUpdater extends SequenceUpdater {
  private static Logger logger = Logger.getLogger("au.edu.uq.imb.memesuite.updatedb.ucsc");
  private static Pattern spacePattern = Pattern.compile("^[\\p{Z}\\s]*$");
  private static final String HOST = "hgdownload.cse.ucsc.edu";
  private static final int UCSC_RETRIEVER_ID = 1;
  private static final int RETRY_COUNT = 3;
  private static final int ERROR_COUNT = 10;

  public UCSCGenomeUpdater(SQLiteDataSource dataSource,
      ReentrantReadWriteLock dbLock, File binDir, File dbDir,
      ExecutorService worker, MultiSourceStatus statusManager) {
    super("UCSC Genome Downloader", dataSource, dbLock, binDir, dbDir, worker, statusManager);
  }

  private static boolean isSpace(String value) {
    return spacePattern.matcher(value).matches();
  }

  /**
   * Converts to title case (leading capital only) and converts to singular by
   * removing a trailing s.
   * @param group the group name to prepare.
   * @return a prepared group name.
   */
  private String prepareGroupName(String group) {
    StringBuilder builder = new StringBuilder(group.toLowerCase());
    builder.setCharAt(0, Character.toTitleCase(group.charAt(0)));
    if (group.endsWith("S") || group.endsWith("s")) {
      builder.deleteCharAt(builder.length()-1);
    }
    return builder.toString();
  }


  protected List<UCSCGenome> scrapeAvailableGenomes() {
    progress.setTask("Querying Avaliable Genomes", 0, -1);
    List<UCSCGenome> genomes = new ArrayList<UCSCGenome>();
    // get a listing of species
    try {
      Document doc = Jsoup.connect("http://genome.ucsc.edu/FAQ/FAQreleases.html").get();
      Elements rows = doc.select("table.descTbl tr");
      String group = null;
      String species = null;
      boolean speciesItalic = false;
      for (int i = 0; i < rows.size(); i++) {
        Element row = rows.get(i);
        Elements rowItems = row.select("td");
        if (rowItems.size() != 5) continue;
        String speciesOrGroup, version, releaseDate, releaseName, status;
        boolean speciesOrGroupItalic;
        speciesOrGroup = rowItems.get(0).text();
        speciesOrGroupItalic = !rowItems.get(0).select("em").isEmpty();
        version = rowItems.get(1).text();
        releaseDate = rowItems.get(2).text();
        releaseName = rowItems.get(3).text();
        status = rowItems.get(4).text();
        if (!isSpace(speciesOrGroup)) {
          if (isSpace(version) &&  isSpace(releaseDate) && isSpace(releaseName) && isSpace(status)) {
            group = prepareGroupName(speciesOrGroup);
            continue;
          } else {
            species = speciesOrGroup;
            speciesItalic = speciesOrGroupItalic;
          }
        }
        if (!status.equals("Available") || group == null || species == null) continue;
        genomes.add(new UCSCGenome(group, species, speciesItalic, version, releaseDate, releaseName));
      }
    } catch (IOException e) {
      logger.log(Level.SEVERE, "Problem reading page for genome listing", e);
    }
    return genomes;
  }

  protected boolean determineFtpSource(FTPClient ftp, UCSCGenome genome) throws IOException {
    progress.setTask("Determining full URL for " + genome, 0, -1);
    SequenceFileFilter filter = new SequenceFileFilter(genome.getSequenceVersion());
    String genomePath = "/goldenPath/" + genome.getSequenceVersion() + "/bigZips/";
    logger.log(Level.INFO, "Looking for " + genome + " sequences at " + genomePath);
    FTPFile sequenceFile = filter.best(ftp.mlistDir(genomePath, filter));
    if (sequenceFile == null) {
      logger.log(Level.WARNING, "Skipping " + genome + " as no sequence found.");
      return false;
    }
    genome.setRemoteInfo(genomePath, sequenceFile.getName(), sequenceFile.getSize());
    logger.log(Level.INFO, "Decided URL of " + genome + " is " + genome.getRemoteUrl());
    return true;
  }

  protected boolean determineFtpSource(FTPClient ftp, UCSCGenome genome, int retryCount) throws IOException, InterruptedException {
    for (int attempt = 1; attempt <= retryCount; attempt++) {
      try {
        // this could happen if we're retrying
        if (!ftp.isConnected()) {
          // try to recreate the connection
          if (!loginFtp(ftp, HOST)) throw new IOException("Unable to login to " + HOST);
        }
        return determineFtpSource(ftp, genome);
      } catch (IOException e) {
        logger.log(Level.WARNING, "Failed to determine source for " + genome + " attempt " + attempt + " of " + retryCount, e);
        if (attempt == retryCount) throw e;
        if (ftp.isConnected()) {
          try {
            ftp.disconnect();
          } catch (IOException e2) { /* ignore */ }
        }
        Thread.sleep(TimeUnit.SECONDS.toMillis(10));
      }
    }
    return false;
  }

  @Override
  public Void call() throws Exception {
    FTPClient ftp = null;
    int errorCount = 0;
    try {
      logger.log(Level.INFO, "Starting UCSC update");
      List<UCSCGenome> genomes = scrapeAvailableGenomes();
      if (genomes.isEmpty()) {
        logger.log(Level.SEVERE, "Failed parsing the genome releases from the FAQ... No genomes found!");
        return null;
      }
      progress.setTask("Excluding Pre-exisiting Genomes", 0, -1);
      // query the database to see which genomes we already have
      // and remove them so we don't download them again
      Iterator<UCSCGenome> iterator = genomes.iterator();
      while (iterator.hasNext()) {
        UCSCGenome genome = iterator.next();
        if (sourceExists(genome, true)) {
          iterator.remove();
          logger.log(Level.INFO, "Already have " + genome.getNamePrefix());
        }
      }
      if (genomes.isEmpty()) return null;
      ftp = new FTPClient();
      if (!loginFtp(ftp, HOST)) return null;
      for (UCSCGenome genome : genomes) {
        try {
          checkWorkerTasks();
          if (!determineFtpSource(ftp, genome, RETRY_COUNT)) continue;
          checkWorkerTasks();
          if (!downloadFtpSource(ftp, genome, true, RETRY_COUNT)) continue;
          enqueueSequences(new UCSCSequenceProcessor(genome));
        } catch (IOException e) {
          logger.log(Level.WARNING, "Skipped " + genome + " due to ftp errors", e);
          errorCount++;
          if (errorCount >= ERROR_COUNT) throw new IOException("Too many IO Exceptions", e);
        }
      }
      ftp.logout();
      ftp.disconnect();
      progress.complete();
      waitForWorkerTasks();
      logger.log(Level.INFO, "Finished UCSC update");
    } catch (ExecutionException e) { // only thrown by sequence processor
      cancelWorkerTasks();
      logger.log(Level.SEVERE, "Abandoning UCSC update due to failure to process sequences!", e);
    } catch (SQLException e) {
      logger.log(Level.SEVERE, "Abandoning UCSC update! Already downloaded sequences will still be processed", e);
    } catch (IOException e) {
      logger.log(Level.SEVERE, "Abandoning UCSC update!", e);
    } catch (InterruptedException e) {
      logger.log(Level.WARNING, "UCSC update interrupted!");
    } catch (RuntimeException e) {
      logger.log(Level.SEVERE, "RuntimeException!", e);
      throw e;
    } catch (Error e) {
      logger.log(Level.SEVERE, "Error!", e);
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
    return null;
  }

  private class UCSCSequenceProcessor extends SequenceProcessor {
    private UCSCGenome genome;

    private UCSCSequenceProcessor(UCSCGenome genome) {
      super(dataSource, dbLock, binDir, dbDir, status);
      this.genome = genome;
    }

    @Override
    public void process() throws IOException, SQLException, InterruptedException {
      if (!unpackSequences(genome)) return;
      processSequences(genome);
      recordSequences(genome);
    }
  }

  private static class UCSCGenome extends AbstractFtpSource {
    private String group;
    private String species;
    private boolean speciesItalic;
    private String version;
    private String releaseDate;
    private String releaseName;
    private int edition;
    private String ftpDir;
    private String ftpName;
    private long ftpSize;
    private File packedFile;
    private File sequenceFile;
    private File bgFile;
    private long seqCount;
    private long seqMinLen;
    private long seqMaxLen;
    private double seqAvgLen;
    private double seqStdDLen;
    private long seqTotalLen;

    private static Pattern editionPattern = Pattern.compile("^[a-zA-Z]+(\\d+)$");

    public UCSCGenome(String group, String species, boolean speciesItalic, String version, String releaseDate, String releaseName) {
      Matcher m = editionPattern.matcher(version);
      if (!m.matches()) throw new IllegalArgumentException("UCSC version does not match expected pattern");
      this.group = group;
      this.species = species;
      this.speciesItalic = speciesItalic;
      this.version = version;
      this.releaseDate = releaseDate;
      this.releaseName = releaseName;
      this.edition = Integer.parseInt(m.group(1), 10);
    }

    public String toString() {
      return "UCSC " + version;
    }

    @Override
    public int getRetrieverId() {
      return UCSC_RETRIEVER_ID;
    }

    @Override
    public String getCategoryName() {
      return "UCSC " + group + " Genomes";
    }

    @Override
    public String getListingName() {
      return species;
    }

    @Override
    public String getListingDescription() {
      return "Sequences from UCSC Genome Browser for " +
          (speciesItalic ? "<i>" + species + "</i>" : species) + ".";
    }

    @Override
    public Alphabet guessAlphabet() {
      return Alphabet.DNA;
    }

    @Override
    public long getSequenceEdition() {
      return edition;
    }

    @Override
    public String getSequenceVersion() {
      return version;
    }

    @Override
    public String getSequenceDescription() {
      return "Downloaded from <a href=\"" + getRemoteUrl() + "\">" +
          getRemoteUrl() + "</a>. Originally released as " + releaseName + " in " +
          releaseDate + ".";
    }

    public void setRemoteInfo(String ftpDir, String ftpName, long ftpSize) {
      this.ftpDir = ftpDir;
      this.ftpName = ftpName;
      this.ftpSize = ftpSize;
    }

    @Override
    public String getRemoteHost() {
      return HOST;
    }

    @Override
    public String getRemoteDir() {
      return ftpDir;
    }

    @Override
    public String getRemoteName() {
      return ftpName;
    }

    @Override
    public long getRemoteSize() {
      return ftpSize;
    }

    public void setSourceFile(File packedFile) {
      this.packedFile = packedFile;
    }

    public List<File> getSourceFiles() {
      return Collections.singletonList(packedFile);
    }

    public void setSequenceFile(File sequenceFile) {
      this.sequenceFile = sequenceFile;
    }

    public File getSequenceFile() {
      return sequenceFile;
    }

    public void setBgFile(File bgFile) {
      this.bgFile = bgFile;
    }

    public File getBgFile() {
      return bgFile;
    }

    @Override
    public void setStats(long count, long minLen, long maxLen, double avgLen,
        double stdDLen, long totalLen) {
      this.seqCount = count;
      this.seqMinLen = minLen;
      this.seqMaxLen = maxLen;
      this.seqAvgLen = avgLen;
      this.seqStdDLen = stdDLen;
      this.seqTotalLen = totalLen;
    }

    @Override
    public long getSequenceCount() {
      return seqCount;
    }

    @Override
    public long getTotalLength() {
      return seqTotalLen;
    }

    @Override
    public long getMinLength() {
      return seqMinLen;
    }

    @Override
    public long getMaxLength() {
      return seqMaxLen;
    }

    @Override
    public double getAverageLength() {
      return seqAvgLen;
    }

    @Override
    public double getStandardDeviationLength() {
      return seqStdDLen;
    }

    @Override
    public String getNamePrefix() {
      return "ucsc_" + version;
    }
  }

  private static class SequenceFileFilter extends PatternFileFilter {
    public SequenceFileFilter(String version) {
      super(new Pattern[]{
          Pattern.compile(
              "^(?:" +
              "chromFa\\.(?:tar\\.gz|zip)|" +
              Pattern.quote(version) + "\\.(?:fa\\.gz|tar\\.gz|zip)" +
              ")$"
          ),
          Pattern.compile(
              "^(?:" +
              Pattern.quote(version) + "\\.softmask\\.fa\\.gz|" +
              "soft[mM]ask2?\\.(?:fa\\.gz|zip)|"+
              "allFa\\.tar\\.gz|" +
              "GroupFa\\.zip" +
              ")$"
          ),
          Pattern.compile(
              "^[sS]caffoldFa\\.(?:zip|gz)$"
          )
      });
    }
  }
}
