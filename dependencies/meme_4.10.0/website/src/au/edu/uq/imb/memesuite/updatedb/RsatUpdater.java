package au.edu.uq.imb.memesuite.updatedb;

import RSATWS.*;
import au.edu.uq.imb.memesuite.data.Alphabet;
import au.edu.uq.imb.memesuite.util.JsonWr;
import au.edu.uq.imb.memesuite.util.MultiSourceStatus;
import org.sqlite.SQLiteDataSource;

import javax.xml.rpc.ServiceException;
import java.io.*;
import java.rmi.RemoteException;
import java.sql.SQLException;
import java.text.DateFormat;
import java.text.SimpleDateFormat;
import java.util.*;
import java.util.concurrent.ExecutionException;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.TimeUnit;
import java.util.concurrent.locks.ReentrantReadWriteLock;
import java.util.logging.Level;
import java.util.logging.Logger;

/**
 * Retrieve sequences from RSA Tools.
 */
public class RsatUpdater extends SequenceUpdater {
  private int ageMin;
  private int start;
  private int end;
  private int retain;

  private static Logger logger = Logger.getLogger("au.edu.uq.imb.memesuite.updatedb.rsat");
  private static final int RSAT_RETRIEVER_ID = 4;

  public RsatUpdater(SQLiteDataSource dataSource, ReentrantReadWriteLock dbLock,
      File binDir, File dbDir, ExecutorService worker, MultiSourceStatus multiStatus) {
    super("RSAT Upstream Downloader", dataSource, dbLock, binDir, dbDir, worker, multiStatus);

    Properties conf = SequenceUpdater.loadConf(RsatUpdater.class, dbDir, "RsatUpdater.properties");
    this.ageMin = getConfInt(conf, "age.min", 0, null, 30);
    this.start = getConfInt(conf, "start", null, null, -1000);
    this.end = getConfInt(conf, "end", null, null, 200);
    this.retain = getConfInt(conf, "retain", 1, null, 1);
  }

  protected List<RsatSource> queryAvailableOrganisms() throws ServiceException, RemoteException {
    progress.setTask("Querying Avaliable Genomes", 0, -1);
    List<RsatSource> sources = new ArrayList<RsatSource>();
    // RSAT does not give version information anywhere I can find so timestamps are the best I can do
    long timestamp = System.currentTimeMillis();
    RSATWebServicesLocator service = new RSATWebServicesLocator();
    RSATWSPortType proxy = service.getRSATWSPortType();
    SupportedOrganismsRequest request = new SupportedOrganismsRequest();
    request.setOutput("client");
    request.setFormat("tab");
    request.setTaxon("");
    SupportedOrganismsResponse res = proxy.supported_organisms(request);
    String data = res.getClient();
    int pos = 0;
    int nextPos;
    while ((nextPos = data.indexOf('\n', pos)) != -1) {
      sources.add(new RsatSource(data.substring(pos, nextPos), timestamp, start, end));
      pos = nextPos + 1;
    }
    // it seems each actual line is terminated by a newline so the last line is empty
    return sources;
  }

  protected void downloadSequence(RsatSource source) throws ServiceException, IOException {
    progress.setTask("Downloading " + source, 0, -1);
    // create the downloads directory if it does not exist
    File downloadDir = new File(dbDir, "downloads");
    if (downloadDir.exists()) {
      if (!downloadDir.isDirectory()) {
        throw new IOException("Unable to create download directory \"" +
            downloadDir + "\" as a file with that name already exists!");
      }
    } else if (!downloadDir.mkdirs()) {
      throw new IOException("Unable to create download directory \"" +
          downloadDir + "\" as the mkdirs command failed!");
    }
    File target = new File(downloadDir, source.getLocalName());
    logger.log(Level.INFO, "Retrieving " + source + " from RSAT web services to \"" + target + "\".");
    RSATWebServicesLocator service = new RSATWebServicesLocator();
    RSATWSPortType proxy = service.getRSATWSPortType();
    RetrieveSequenceRequest request = new RetrieveSequenceRequest();
    request.setOutput("client");
    request.setOrganism(source.getOrganism());
    request.setAll(1);
    request.setNoorf(1);
    request.setFrom(source.getStart());
    request.setTo(source.getEnd());
    request.setFeattype("");
    request.setType("upstream");
    request.setFormat("fasta");
    request.setLw(50);
    request.setLabel("id,name");
    request.setLabel_sep("");
    request.setNocom(0);
    request.setRepeat(0);
    request.setImp_pos(0);
    RetrieveSequenceResponse response = proxy.retrieve_seq(request);
    Writer out = null;
    try {
      out =  new OutputStreamWriter(new FileOutputStream(target), "UTF-8");
      out.write(response.getClient());
      out.close();
      out = null;
    } finally {
      if (out != null) {
        try {
          out.close();
        } catch (IOException e) { /* ignore */ }
      }
    }
    source.setSourceFile(target);
  }

  @Override
  public Void call() throws Exception {
    try {
      logger.log(Level.INFO, "Starting RSAT update");
      List<RsatSource> genomes = queryAvailableOrganisms();
      if (genomes.isEmpty()) {
        logger.log(Level.SEVERE, "Failed parsing the genome releases from the FAQ... No genomes found!");
        return null;
      }
      progress.setTask("Excluding Pre-exisiting Genomes", 0, -1);
      // query the database to see which organisms we already have
      // and remove them so we don't download them again
      long updateDelayMillis = TimeUnit.DAYS.toMillis(ageMin);
      Iterator<RsatSource> iterator = genomes.iterator();
      while (iterator.hasNext()) {
        RsatSource genome = iterator.next();
        if (sourceExists(genome, true, false, updateDelayMillis)) {
          iterator.remove();
          logger.log(Level.INFO, "Already updated " + genome + " within " + ageMin + " day(s) ago.");
        }
      }
      if (genomes.isEmpty()) return null;

      for (RsatSource source : genomes) {
        if (Thread.currentThread().isInterrupted()) throw new InterruptedException();
        checkWorkerTasks();
        downloadSequence(source);
        enqueueSequences(new RsatSequenceProcessor(source));
      }
      progress.complete();
      waitForWorkerTasks();
      logger.log(Level.INFO, "Finished RSAT update");
    } catch (ExecutionException e) { // only thrown by sequence processor
      cancelWorkerTasks();
      logger.log(Level.SEVERE, "Abandoning RSAT update due to failure to process sequences!", e);
    } catch (ServiceException e) {
      logger.log(Level.SEVERE, "Abandoning RSAT update!", e);
    } catch (SQLException e) {
      logger.log(Level.SEVERE, "Abandoning RSAT update!", e);
    } catch (IOException e) {
      logger.log(Level.SEVERE, "Abandoning RSAT update!", e);
    } catch (InterruptedException e) {
      logger.log(Level.WARNING, "RSAT update interrupted!");
    } catch (RuntimeException e) {
      logger.log(Level.SEVERE, "RuntimeException!", e);
      throw e;
    } catch (Error e) {
      logger.log(Level.SEVERE, "Error!", e);
      throw e;
    } finally {
      progress.complete();
    }
    return null;
  }

  private class RsatSequenceProcessor extends SequenceProcessor {
    private RsatSource source;

    public RsatSequenceProcessor(RsatSource source) {
      super(dataSource, dbLock, binDir, dbDir, status);
      this.source = source;
    }

    private void moveSequence(RsatSource source) throws IOException {
      File sourceFile = source.getSourceFiles().get(0);
      File sequenceFile = new File(dbDir, source.getLocalName());
      if (!sourceFile.renameTo(sequenceFile)) {
        throw new IOException("Failed to rename \"" + sourceFile +
            "\" to \"" + sequenceFile + "\"");
      }
      source.setSequenceFile(sequenceFile);
    }

    @Override
    public void process() throws IOException, SQLException, InterruptedException {
      moveSequence(source);
      processSequences(source);
      obsoleteOldEditions(recordSequences(source).listingId, source.guessAlphabet(), retain);
    }
  }

  private static class RsatSource implements Source {
    private String organism;
    private int start;
    private int end;
    private long timestamp;
    private File sourceFile;
    private File sequenceFile;
    private File bgFile;
    private long seqCount;
    private long seqMinLen;
    private long seqMaxLen;
    private double seqAvgLen;
    private double seqStdDLen;
    private long seqTotalLen;

    public RsatSource(String organism, long timestamp, int start, int end) {
      this.organism = organism;
      this.timestamp = timestamp;
      this.start = start;
      this.end = end;
    }

    public String toString() {
      return "RSAT " + organism.replace('_', ' ') + " (Upstream " + start + " to " + end + ")";
    }

    public String getOrganism() {
      return organism;
    }

    public int getStart() {
      return start;
    }

    public int getEnd() {
      return end;
    }

    @Override
    public int getRetrieverId() {
      return RSAT_RETRIEVER_ID;
    }

    @Override
    public String getCategoryName() {
      return "Upstream Sequences";
    }

    @Override
    public String getListingName() {
      return organism.replace('_', ' ');
    }

    @Override
    public String getListingDescription() {
      String name = organism.replace('_', ' ');
      return "Upstream sequences (" + start + " to " + end + ") for <i>" + name + "</i>";
    }

    @Override
    public Alphabet guessAlphabet() {
      return Alphabet.DNA;
    }

    @Override
    public long getSequenceEdition() {
      return timestamp;
    }

    @Override
    public String getSequenceVersion() {
      return DateFormat.getDateInstance(DateFormat.MEDIUM).format(new Date(timestamp));
    }

    @Override
    public String getSequenceDescription() {
      String when = DateFormat.getDateInstance(DateFormat.MEDIUM).format(new Date(timestamp));
      return "Sequences from Regulatory Sequence Analysis Tools " +
          "retrieve_seq webservice retrieved on " + when + ".";
    }

    @Override
    public String getNamePrefix() {
      SimpleDateFormat df = new SimpleDateFormat("yyyy-MM-dd");
      String when = df.format(new Date(timestamp));
      return "rsat_" + organism + "_" + when;
    }

    public void setSourceFile(File file) {
      this.sourceFile = file;
    }

    @Override
    public List<File> getSourceFiles() {
      return Collections.singletonList(sourceFile);
    }

    public String getLocalName() {
      return getNamePrefix() + ".fna";
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
  }
}
