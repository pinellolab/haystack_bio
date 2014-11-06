package au.edu.uq.imb.memesuite.updatedb;

import au.edu.uq.imb.memesuite.data.Alphabet;
import au.edu.uq.imb.memesuite.util.JsonWr;
import au.edu.uq.imb.memesuite.util.MultiSourceStatus;
import org.apache.commons.net.ftp.FTPClient;
import org.apache.commons.net.ftp.FTPFile;
import org.sqlite.SQLiteDataSource;

import java.io.ByteArrayOutputStream;
import java.io.File;
import java.io.IOException;
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
 * Downloads genomes from GenBank in the specified groupings.
 */
public class GenbankUpdater extends SequenceUpdater {
  private String[] groups;
  private int retain;
  private static Logger logger = Logger.getLogger("au.edu.uq.imb.memesuite.updatedb.genbank");
  private static final String HOST = "ftp.ncbi.nih.gov";
  private static final int GENBANK_RETRIEVER_ID = 3;
  private static Pattern releaseNumPattern = Pattern.compile("^\\s*(\\d+)\\s*$");
  private static final int RETRY_COUNT = 3;
  private static final int ERROR_COUNT = 10;

  public GenbankUpdater(SQLiteDataSource dataSource,
      ReentrantReadWriteLock dbLock, File binDir, File dbDir,
      ExecutorService worker, MultiSourceStatus multiStatus) {
    super("GenBank Genome Downloader", dataSource, dbLock, binDir, dbDir, worker, multiStatus);

    Properties conf = loadConf(GenbankUpdater.class, dbDir, "GenbankUpdater.properties");
    groups = conf.getProperty("groups", "Bacteria Fungi").trim().split("\\s+");
    retain = getConfInt(conf, "retain", 1, null, 1);
  }

  protected int queryReleaseNumber(FTPClient ftp) throws IOException {
    final String RELEASE_NUMBER_FILE = "/genbank/GB_Release_Number";
    FTPFile file = ftp.mlistFile(RELEASE_NUMBER_FILE);
    if (file == null) throw new IOException("Could not find the release number file \"" +
        RELEASE_NUMBER_FILE + "\".");
    if (file.isDirectory()) throw new IOException("The release number file \"" +
        RELEASE_NUMBER_FILE + "\" turned out to be a directory.");
    if (file.getSize() > 20) throw new IOException("The release number file \"" +
        RELEASE_NUMBER_FILE + "\" was larger than expected.");
    ByteArrayOutputStream storage = new ByteArrayOutputStream((int)file.getSize());
    if (!ftp.retrieveFile(RELEASE_NUMBER_FILE, storage)) {
      throw new IOException("Failed to retrieve the release number file \"" + RELEASE_NUMBER_FILE + "\".");
    }
    int releaseNumber;
    Matcher m = releaseNumPattern.matcher(storage.toString("UTF-8"));
    if (m.matches()) {
      try {
        releaseNumber = Integer.parseInt(m.group(1), 10);
      } catch (NumberFormatException e) {
        throw new IOException("Could not convert the release number into an " +
            "integer! Text was \"" + storage + "\"");
      }
    } else {
      throw new IOException("The release number file \"" + RELEASE_NUMBER_FILE +
          "\" did not contain a number! Text was \"" + storage + "\"");
    }
    return releaseNumber;
  }

  protected List<GenbankGenome> queryAvailableGenomes(FTPClient ftp) throws IOException {
    Pattern dna = Pattern.compile("^.*\\.fna$");
    Pattern protein = Pattern.compile("^.*\\.faa$");
    progress.setTask("Querying Avaliable Genomes", 0, -1);
    int releaseNumber = queryReleaseNumber(ftp);
    logger.log(Level.INFO, "Got release number " + releaseNumber);
    List<GenbankGenome> genomes = new ArrayList<GenbankGenome>();
    for (String group : groups) {
      logger.log(Level.INFO, "Looking for class " + group);
      String genomesPath = "/genbank/genomes/" + group;
      FTPFile[] genomeDirs = ftp.listDirectories(genomesPath);
      for (FTPFile genomeDir : genomeDirs) {
        if (!genomeDir.isDirectory() || genomeDir.getName().equals(".") || genomeDir.getName().equals("..")) continue;
        String id = genomeDir.getName();
        String genomePath = genomesPath + "/" + id + "/";
        genomes.add(new GenbankGenome(releaseNumber, group, id, Alphabet.DNA, genomePath, dna));
        genomes.add(new GenbankGenome(releaseNumber, group, id, Alphabet.PROTEIN, genomePath, protein));
      }
    }
    return genomes;
  }

  protected boolean determineFtpSource(FTPClient ftp, GenbankGenome genome) throws IOException {
    progress.setTask("Determining full URLs for " + genome, 0, -1);
    PatternFileFilter filter = new PatternFileFilter(genome.getFtpPattern());
    logger.log(Level.INFO, "Looking for " + genome + " sequences at " + genome.getFtpDir());
    FTPFile[] sequenceFiles = ftp.mlistDir(genome.getFtpDir(), filter);
    if (sequenceFiles.length == 0) {
      logger.log(Level.WARNING, "Skipping " + genome + " as no sequence found.");
      return false;
    }
    for (FTPFile file : sequenceFiles) {
      genome.addRemoteSource(file.getName(), file.getSize());
    }
    return true;
  }

  protected boolean determineFtpSource(FTPClient ftp, GenbankGenome genome, int retryCount) throws IOException, InterruptedException {
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
    int errorCount = 0;
    FTPClient ftp = null;
    // get release number from file ftp://ftp.ncbi.nih.gov/genbank/GB_Release_Number
    try {
      logger.log(Level.INFO, "Starting Genbank update");
      ftp = new FTPClient();
      if (!loginFtp(ftp, HOST)) return null;
      List<GenbankGenome> genomes = queryAvailableGenomes(ftp);
      if (genomes.isEmpty()) {
        logger.log(Level.SEVERE, "Failed parsing the genome releases from the FAQ... No genomes found!");
        return null;
      }
      progress.setTask("Excluding Pre-exisiting Genomes", 0, -1);
      // query the database to see which genomes we already have
      // and remove them so we don't download them again
      Iterator<GenbankGenome> iterator = genomes.iterator();
      while (iterator.hasNext()) {
        GenbankGenome genome = iterator.next();
        if (sourceExists(genome, true)) {
          iterator.remove();
          logger.log(Level.INFO, "Already have " + genome);
        }
      }
      if (genomes.isEmpty()) return null;
      genome_loop:
      for (GenbankGenome genome : genomes) {
        try {
          checkWorkerTasks();
          if (!determineFtpSource(ftp, genome, RETRY_COUNT)) continue;
          checkWorkerTasks();
          for (FtpSource source : genome.getRemoteSources()) {
            if (!downloadFtpSource(ftp, source, true, RETRY_COUNT)) continue genome_loop;
          }
          enqueueSequences(new GenbankSequenceProcessor(genome));
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
      logger.log(Level.INFO, "Finished Genbank update");
    } catch (ExecutionException e) { // only thrown by sequence processor
      cancelWorkerTasks();
      logger.log(Level.SEVERE, "Abandoning GenBank update due to failure to process sequences!", e);
    } catch (SQLException e) {
      logger.log(Level.SEVERE, "Abandoning GenBank update! Already downloaded sequences will still be processed", e);
    } catch (IOException e) {
      logger.log(Level.SEVERE, "Abandoning GenBank update! Already downloaded sequences will still be processed", e);
    } catch (InterruptedException e) {
      logger.log(Level.WARNING, "GenBank update interrupted!");
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
    return null;  //To change body of implemented methods use File | Settings | File Templates.
  }

  private class GenbankSequenceProcessor extends SequenceProcessor {
    private GenbankGenome source;

    private GenbankSequenceProcessor(GenbankGenome source) {
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

  private class GenbankGenome implements Source {
    private int version;
    private String group;
    private String id;
    private Alphabet alphabet;
    private String ftpPath;
    private Pattern[] ftpPattern;
    private List<RemoteSource> ftpSources;
    private List<File> packedFiles;
    private File sequenceFile;
    private File bgFile;
    private long seqCount;
    private long seqMinLen;
    private long seqMaxLen;
    private double seqAvgLen;
    private double seqStdDLen;
    private long seqTotalLen;

    public GenbankGenome(int version, String group, String id,
        Alphabet alphabet, String path, Pattern ftpPattern) {
      this.version = version;
      this.group = group;
      this.id = id;
      this.alphabet = alphabet;
      this.ftpPath = path;
      this.ftpPattern = new Pattern[]{ftpPattern};
      this.ftpSources = new ArrayList<RemoteSource>();
      this.packedFiles = new ArrayList<File>();
    }

    public Pattern[] getFtpPattern() {
      return ftpPattern;
    }

    public String getFtpDir() {
      return ftpPath;
    }

    public void addRemoteSource(String name, long size) {
      packedFiles.add(null);
      ftpSources.add(new RemoteSource(ftpSources.size(), name, size));
    }

    public List<? extends FtpSource> getRemoteSources() {
      return Collections.unmodifiableList(ftpSources);
    }

    public String toString() {
      return "GenBank v" + version + " " + id.replace('_', ' ') + " (" + alphabet + ")";
    }

    @Override
    public int getRetrieverId() {
      return GENBANK_RETRIEVER_ID;
    }

    @Override
    public String getCategoryName() {
      return "GenBank " + group + " Genomes";
    }

    @Override
    public String getListingName() {
      return id.replace('_', ' ');
    }

    @Override
    public String getListingDescription() {
      return "Sequences from GenBank for <i>" + id.replace('_', ' ') + "</i>.";
    }

    @Override
    public Alphabet guessAlphabet() {
      return alphabet;  //To change body of implemented methods use File | Settings | File Templates.
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
      for (int i = 0; i < ftpSources.size(); i++) {
        RemoteSource source = ftpSources.get(i);
        builder.append("<a href=\"").append(source.getRemoteUrl()).append("\">");
        builder.append(source.getRemoteUrl());
        builder.append("</a>");
        if (i < (ftpSources.size() - 2)) {
          builder.append(", ");
        } else if (i == (ftpSources.size() - 2)) {
          builder.append(" and ");
        } else {
          builder.append(".");
        }
      }
      return builder.toString();
    }

    @Override
    public String getNamePrefix() {
      String name = id;
      name = name.replaceAll("[^a-zA-Z0-9\\p{Z}\\s_\\.-]", "");
      name = name.replaceAll("[\\p{Z}\\s]+", "_");
      return "genbank_" + name + "_" + version;
    }

    public List<File> getSourceFiles() {
      return Collections.unmodifiableList(packedFiles);
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
    public void outputJson(JsonWr out) throws IOException {
      out.value((JsonWr.JsonValue)null);
    }

    private class RemoteSource implements FtpSource {
      private int ordinal;
      private String name;
      private long size;

      public RemoteSource(int ordinal, String name, long size) {
        this.ordinal = ordinal;
        this.name = name;
        this.size = size;
      }

      public String toString() {
        return GenbankGenome.this.toString() + " [file " + (ordinal + 1) + " of " + ftpSources.size() + "]";
      }

      @Override
      public String getRemoteHost() {
        return HOST;
      }

      @Override
      public String getRemoteDir() {
        return ftpPath;
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
      public String getRemotePath() {
        return getRemoteDir() + getRemoteName();  //To change body of implemented methods use File | Settings | File Templates.
      }

      @Override
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

      @Override
      public String getLocalName() {
        return getNamePrefix() + "." + (ordinal + 1) + getRemoteExt();
      }

      @Override
      public void setSourceFile(File file) {
        packedFiles.set(ordinal, file);
      }
    }
  }

}
