package au.edu.uq.imb.memesuite.updatedb;

import au.edu.uq.imb.memesuite.data.Alphabet;
import au.edu.uq.imb.memesuite.db.SQL;
import au.edu.uq.imb.memesuite.util.MultiSourceStatus;
import au.edu.uq.imb.memesuite.util.Progress;
import org.sqlite.SQLiteDataSource;
import org.xeustechnologies.jtar.TarEntry;
import org.xeustechnologies.jtar.TarInputStream;

import java.io.*;
import java.sql.Connection;
import java.sql.PreparedStatement;
import java.sql.ResultSet;
import java.sql.SQLException;
import java.util.Enumeration;
import java.util.List;
import java.util.concurrent.Callable;
import java.util.concurrent.locks.ReadWriteLock;
import java.util.concurrent.locks.ReentrantReadWriteLock;
import java.util.logging.Level;
import java.util.logging.Logger;
import java.util.regex.Matcher;
import java.util.regex.Pattern;
import java.util.zip.GZIPInputStream;
import java.util.zip.ZipEntry;
import java.util.zip.ZipFile;
import java.util.zip.ZipInputStream;

import static au.edu.uq.imb.memesuite.db.SQL.*;
import static au.edu.uq.imb.memesuite.db.SQL.INSERT_SEQUENCE_FILE;

/**
 * Processes a downloaded sequence file to decompress it and generate statistics.
 */
public abstract class SequenceProcessor implements Callable<Void> {
  private SQLiteDataSource dataSource;
  private ReadWriteLock dbLock;
  private File binDir;
  private File dbDir;
  private MultiSourceStatus multiStatus;
  private Progress progress;

  private static Pattern statsPattern = Pattern.compile("^(\\d+) (\\d+) (\\d+) (\\d+\\.\\d+) (\\d+)$");
  private static Logger logger = Logger.getLogger("au.edu.uq.imb.memesuite.updatedb.processor");
  private static final int BUFFER_SIZE = 8192; // 8KB

  public SequenceProcessor(SQLiteDataSource dataSource,
      ReentrantReadWriteLock dbLock, File binDir, File dbDir,
      MultiSourceStatus multiStatus) {
    this.dataSource = dataSource;
    this.dbLock = dbLock;
    this.binDir = binDir;
    this.dbDir = dbDir;
    this.multiStatus = multiStatus;
    progress = null; // created in call
  }

  private enum FastaFileType {
    ZIP,
    GZIP_TAR,
    GZIP,
    PLAIN,
    UNKNOWN
  }

  public static class FastaGetMarkovProgressMonitor extends Thread {
    private Progress progress;
    private InputStream stderr;

    public FastaGetMarkovProgressMonitor(InputStream stderr, Progress progress) {
      this.progress = progress;
      this.stderr = stderr;
    }

    public void run() {
      BufferedReader in = null;
      try {
        in = new BufferedReader(new InputStreamReader(stderr));
        String line;
        while ((line = in.readLine()) != null) {
          if (progress != null) {
            try {
              progress.setTaskPartDone(Long.parseLong(line));
            } catch (NumberFormatException e) {/* ignore */}
          }
        }
        in.close();
      } catch (IOException e) {
        // ignore
      } finally {
        if (in != null) {
          try {
            in.close();
          } catch (IOException e) {
            //ignore
          }
        }
      }
    }

  }
  
  public static class FastaGetMarkovSummary {
    public final long count;
    public final long minLen;
    public final long maxLen;
    public final long totalLen;
    public final double avgLen;
    public FastaGetMarkovSummary(long count, long minLen, long maxLen, long totalLen, double avgLen) {
      this.count = count;
      this.minLen = minLen;
      this.maxLen = maxLen;
      this.totalLen = totalLen;
      this.avgLen = avgLen;
    }
  }

  public static class FastaGetMarkovSummaryMonitor extends Thread {
    private InputStream stdout;

    private FastaGetMarkovSummary summary = null;

    public FastaGetMarkovSummaryMonitor(InputStream stdout) {
      this.stdout = stdout;
    }

    public void run() {
      BufferedReader in = null;
      try {
        in = new BufferedReader(new InputStreamReader(stdout));
        String line;
        while ((line = in.readLine()) != null) {
          Matcher m = statsPattern.matcher(line);
          if (m.matches()) {
            try {
              synchronized (this) {
                summary = new FastaGetMarkovSummary(
                    Long.parseLong(m.group(1), 10), // sequence count
                    Long.parseLong(m.group(2), 10), // shortest sequence length
                    Long.parseLong(m.group(3), 10), // longest sequence length
                    Long.parseLong(m.group(5), 10), // total summed lengths
                    Double.parseDouble(m.group(4))  // average sequence length
                );
              }
            } catch (NumberFormatException e) {
              // ignore
            }
          }
        }
        in.close();
        in = null;
      } catch (IOException e) {
        // ignore
      } finally {
        if (in != null) {
          try {
            in.close();
          } catch (IOException e) {
            //ignore
          }
        }
      }
    }

    public synchronized boolean summaryFound() {
      return summary != null;
    }

    public synchronized FastaGetMarkovSummary getSummary() {
      return summary;
    }
  }

  public static class SequenceKeys {
    public final long categoryId;
    public final long listingId;
    public final long fileId;
    public SequenceKeys(long categoryId, long listingId, long fileId) {
      this.categoryId = categoryId;
      this.listingId = listingId;
      this.fileId = fileId;
    }
  }

  private boolean isNameZip(File file) {
    return file.getName().endsWith(".zip");
  }

  private boolean isNameGzipTar(File file) {
    return file.getName().endsWith(".tar.gz");
  }

  private boolean isNameGzip(File file) {
    return file.getName().endsWith(".gz");
  }

  private boolean isNamePlainFasta(File file) {
    return file.getName().endsWith(".fa") || file.getName().endsWith(".fna") ||
        file.getName().endsWith(".faa") || file.getName().endsWith(".seq");
  }

  private boolean checkBytesMatch(RandomAccessFile raf, long fileOffset,
      int[] magicNumber) throws IOException {
    raf.seek(fileOffset);
    for (int magic_val : magicNumber) {
      int file_val = raf.read();
      if (file_val == -1) return false;
      if (magic_val != file_val) return false;
    }
    return true;
  }

  private FastaFileType guessFileTypeFromSignature(File file) {
    final int[] GZIP_SIG = {0x1F, 0x8B, 0x08};
    final int[] ZIP_SIG = {0x50, 0x4B, 0x03, 0x04};
    RandomAccessFile raf = null;
    FastaFileType type = FastaFileType.UNKNOWN;
    try {
      raf = new RandomAccessFile(file, "r");
      if (checkBytesMatch(raf, 0l, ZIP_SIG)) {
        type = FastaFileType.ZIP;
      } else if (checkBytesMatch(raf, 0l, GZIP_SIG)) {
        type = FastaFileType.GZIP;
      }
      raf.close();
    } catch (IOException e) {
      return FastaFileType.UNKNOWN;
    } finally {
      if (raf != null) {
        try {
          raf.close();
        } catch (IOException e) { /* ignore */ }
      }
    }
    if (type == FastaFileType.UNKNOWN) {
      // try to determine if it is plain text by scanning for NUL
      BufferedReader in = null;
      try {
        boolean seenNul = false;
        in = new BufferedReader(new InputStreamReader(new FileInputStream(file), "UTF-8"));
        for (int i = 0; i < 1000; i++) {
          int val = in.read();
          if (val == -1) break;
          if (val == 0) {
            seenNul = true;
            break;
          }
        }
        if (!seenNul) type = FastaFileType.PLAIN;
        in.close();
      } catch (IOException e) {
        return FastaFileType.UNKNOWN;
      } finally {
        if (in != null) {
          try {
            in.close();
          } catch (IOException e) { /* ignore */ }
        }
      }
    }
    return type;
  }

  private FastaFileType guessFileType(File file) {
    if (isNameZip(file)) return FastaFileType.ZIP;
    if (isNameGzipTar(file)) return FastaFileType.GZIP_TAR;
    if (isNameGzip(file)) return FastaFileType.GZIP;
    if (isNamePlainFasta(file)) return FastaFileType.PLAIN;
    return guessFileTypeFromSignature(file);
  }

  private String ext(Alphabet alphabet) {
    switch (alphabet) {
      case RNA:
        return ".fra";
      case DNA:
        return ".fna";
      case PROTEIN:
        return ".faa";
    }
    return ".fa";
  }

  private void unpackZip(File sourceFile, OutputStream sequenceOut) throws IOException, InterruptedException {
    logger.log(Level.INFO, "Treating \"" + sourceFile + "\" as ZIP.");
    ZipInputStream zis = null;
    ZipEntry entry;
    int read;
    byte[] buffer = new byte[BUFFER_SIZE];
    try {
      // calculate the total output size
      long totalSize = 0;
      ZipFile zipFile = new ZipFile(sourceFile);
      Enumeration<? extends ZipEntry> zipEntries = zipFile.entries();
      while (zipEntries.hasMoreElements()) {
        entry = zipEntries.nextElement();
        if (!entry.isDirectory() && entry.getName().endsWith(".fa")) {
          totalSize += entry.getSize();
        }
      }
      // unzip the files
      progress.setTaskProgress(0, totalSize);
      zis = new ZipInputStream(new FileInputStream(sourceFile));
      while ((entry = zis.getNextEntry()) != null) {
        if (Thread.interrupted()) throw new InterruptedException();
        if (!entry.isDirectory() && entry.getName().endsWith(".fa")) {
          while ((read = zis.read(buffer)) != -1) {
            if (Thread.interrupted()) throw new InterruptedException();
            sequenceOut.write(buffer, 0, read);
            progress.addTaskPartDone(read);
          }
          sequenceOut.write('\n'); // just in case the sequence doesn't end with a newline
        }
      }
      zis.closeEntry();
      zis.close();
      zis = null;
    } finally {
      if (zis != null) {
        try {
          zis.close();
        } catch (IOException e) {
          //ignore
        }
      }
    }
  }

  private void unpackTarGzip(File sourceFile, OutputStream sequenceOut) throws IOException, InterruptedException {
    logger.log(Level.INFO, "Treating \"" + sourceFile + "\" as GZIPed TAR.");
    TarInputStream tis = null;
    TarEntry entry;
    int read;
    byte[] buffer = new byte[BUFFER_SIZE];
    try {
      // untar the files
      progress.setTaskProgress(0, -1);
      tis = new TarInputStream(new BufferedInputStream(new GZIPInputStream(new FileInputStream(sourceFile))));
      while ((entry = tis.getNextEntry()) != null) {
        if (Thread.interrupted()) throw new InterruptedException();
        if (!entry.isDirectory() && entry.getName().endsWith(".fa")) {
          while ((read = tis.read(buffer)) != -1) {
            if (Thread.interrupted()) throw new InterruptedException();
            sequenceOut.write(buffer, 0, read);
            progress.addTaskPartDone(read);
          }
          sequenceOut.write('\n');
        }
      }
      sequenceOut.flush();
      tis.close();
      tis = null;
    } finally {
      if (tis != null) {
        try {
          tis.close();
        } catch (IOException e) {
          // ignore
        }
      }
    }
  }

  private void unpackGzip(File sourceFile, OutputStream sequenceOut) throws IOException, InterruptedException {
    logger.log(Level.INFO, "Treating \"" + sourceFile + "\" as GZIPed FASTA.");
    GZIPInputStream gis = null;
    int read;
    byte[] buffer = new byte[BUFFER_SIZE];
    try {
      progress.setTaskProgress(0, -1);
      gis = new GZIPInputStream(new FileInputStream(sourceFile));
      while ((read = gis.read(buffer)) != -1) {
        if (Thread.interrupted()) throw new InterruptedException();
        sequenceOut.write(buffer, 0, read);
        progress.addTaskPartDone(read);
      }
      gis.close();
      gis = null;
    } finally {
      if (gis != null) {
        try {
          gis.close();
        } catch (IOException e) {
          //ignore
        }
      }
    }
  }

  private void appendFasta(File sourceFile, OutputStream sequenceOut) throws IOException, InterruptedException {
    logger.log(Level.INFO, "Treating \"" + sourceFile + "\" as FASTA.");
    InputStream in = null;
    int read;
    byte[] buffer = new byte[BUFFER_SIZE];
    try {
      progress.setTaskProgress(0, sourceFile.length());
      in = new FileInputStream(sourceFile);
      while ((read = in.read(buffer)) != -1) {
        if (Thread.interrupted()) throw new InterruptedException();
        sequenceOut.write(buffer, 0, read);
        progress.addTaskPartDone(read);
      }
      in.close();
      in = null;
    } finally {
      if (in != null) {
        try {
          in.close();
        } catch (IOException e) {
          //ignore
        }
      }
    }
  }

  protected boolean unpackSequences(Source genome) throws IOException, InterruptedException {
    logger.log(Level.INFO, "Unpacking sequences for " + genome);
    progress.setTask("Unpacking sequences for " + genome, 0, -1);
    File sequenceFile = new File(dbDir, genome.getNamePrefix() + ext(genome.guessAlphabet()));
    OutputStream sequenceOut = null;
    try {
      sequenceOut = new BufferedOutputStream(new FileOutputStream(sequenceFile));
      List<File> sourceFiles = genome.getSourceFiles();
      for (int i = 0; i < sourceFiles.size(); i++) {
        progress.setTask("Unpacking file " + (i + 1) + " of " + sourceFiles.size() + " for " + genome, 0, -1);
        File sourceFile = sourceFiles.get(i);
        FastaFileType type = guessFileType(sourceFile);
        switch (type) {
          case ZIP:
            unpackZip(sourceFile, sequenceOut);
            break;
          case GZIP_TAR:
            unpackTarGzip(sourceFile, sequenceOut);
            break;
          case GZIP:
            unpackGzip(sourceFile, sequenceOut);
            break;
          case PLAIN:
            appendFasta(sourceFile, sequenceOut);
            break;
          case UNKNOWN:
            throw new IOException("Unable to identify file type of source \"" +
                sourceFile + "\".");

        }
        sequenceOut.flush();
        if (!sourceFile.delete()) {
          logger.log(Level.WARNING, "Unable to delete packed sequences \"" + sourceFile + "\"");
        }
      }
      sequenceOut.close();
      sequenceOut = null;
    } finally {
      if (sequenceOut != null) {
        try {
          sequenceOut.close();
        } catch (IOException e) { /* ignored */ }
      }
    }
    genome.setSequenceFile(sequenceFile);
    return true;
  }
  
  public static FastaGetMarkovSummary generateBackground(File binDir, File sequence, File background, Progress progress) throws IOException, InterruptedException {
    String exe = new File(binDir, "fasta-get-markov").getPath();
    ProcessBuilder builder;
    builder = new ProcessBuilder(exe, "-machine", "-m", "1", sequence.getPath(), background.getPath());
    Process fastaGetMarkov = builder.start();
    // we don't want to send anything to the process
    fastaGetMarkov.getOutputStream().close();
    // read the progress messages from stderr
    new FastaGetMarkovProgressMonitor(fastaGetMarkov.getErrorStream(), progress).start();
    // read the summary message from stdout
    FastaGetMarkovSummaryMonitor summaryMonitor = new FastaGetMarkovSummaryMonitor(fastaGetMarkov.getInputStream());
    summaryMonitor.start();
    // wait for process to exit
    try {
      fastaGetMarkov.waitFor();
    } catch (InterruptedException e) {
      // exit ASAP
      fastaGetMarkov.destroy();
      while (true) {
        try {
          fastaGetMarkov.waitFor();
          break;
        } catch (InterruptedException e2) {
          // ignore
        }
      }
      throw e;
    }
    if (fastaGetMarkov.exitValue() != 0) {
      throw new IOException("Command (" + builder.toString() + 
          ") failed with exit status " + fastaGetMarkov.exitValue() );
    }
    summaryMonitor.join();
    if (!summaryMonitor.summaryFound()) {
      throw new IOException("Expected " + exe +
          " to print summary statistics but none was found!");
    }
    return summaryMonitor.getSummary();
  }

  protected void processSequences(Source genome) throws IOException, InterruptedException {
    logger.log(Level.INFO, "Calculating background and statistics of the " +
        "sequences in the file \"" + genome.getSequenceFile() + "\"");
    progress.setTask("Calculating background and statistics for " + genome, 0, genome.getSequenceFile().length());
    String seqs = genome.getSequenceFile().getName();
    File bgFile = new File(dbDir, seqs + ".bfile");
    FastaGetMarkovSummary summary = generateBackground(binDir, genome.getSequenceFile(), bgFile, progress);
    genome.setBgFile(bgFile);
    genome.setStats(summary.count, summary.minLen, summary.maxLen, summary.avgLen, 0.0, summary.totalLen);
  }

  protected SequenceKeys recordSequences(Source genome) throws SQLException {
    Long categoryId = null, listingId = null;
    long fileId;
    logger.log(Level.INFO, "Creating database record for " + genome);
    progress.setTask("Creating database record for " + genome, 0, -1);
    Connection connection = null;
    dbLock.writeLock().lock();
    try {
      PreparedStatement ps;
      ResultSet rs;
      connection = dataSource.getConnection();
      connection.setAutoCommit(false);
      // get or create the category
      ps = connection.prepareStatement(SELECT_CATEGORY_BY_NAME);
      ps.setString(1, genome.getCategoryName());
      rs = ps.executeQuery();
      if (rs.next()) {
        categoryId = rs.getLong(1);
      }
      rs.close();
      ps.close();
      if (categoryId == null) {
        ps = connection.prepareStatement(INSERT_CATEGORY);
        ps.setString(1, genome.getCategoryName());
        ps.executeUpdate();
        rs = ps.getGeneratedKeys();
        if (!rs.next()) throw new SQLException("Failed to get generated keys");
        categoryId = rs.getLong(1);
        rs.close();
        ps.close();
      }
      // get or create the listing
      ps = connection.prepareStatement(SELECT_LISTING_BY_NAME);
      ps.setLong(1, categoryId);
      ps.setString(2, genome.getListingName());
      rs = ps.executeQuery();
      if (rs.next()) {
        listingId = rs.getLong(1);
      }
      rs.close();
      ps.close();
      if (listingId == null) {
        ps = connection.prepareStatement(INSERT_LISTING);
        ps.setLong(1, categoryId);
        ps.setString(2, genome.getListingName());
        ps.setString(3, genome.getListingDescription());
        ps.executeUpdate();
        rs = ps.getGeneratedKeys();
        if (!rs.next()) throw new SQLException("Failed to get generated keys");
        listingId = rs.getLong(1);
        rs.close();
        ps.close();
      }
      // create the sequence file entry
      ps = connection.prepareStatement(INSERT_SEQUENCE_FILE);
      ps.setInt(1, genome.getRetrieverId());
      ps.setLong(2, listingId);
      ps.setInt(3, genome.guessAlphabet().toInt());
      ps.setLong(4, genome.getSequenceEdition());
      ps.setString(5, genome.getSequenceVersion());
      ps.setString(6, genome.getSequenceDescription());
      ps.setString(7, genome.getSequenceFile().getName());
      ps.setString(8, genome.getBgFile().getName());
      ps.setLong(9, genome.getSequenceCount());
      ps.setLong(10, genome.getTotalLength());
      ps.setLong(11, genome.getMinLength());
      ps.setLong(12, genome.getMaxLength());
      ps.setDouble(13, genome.getAverageLength());
      ps.setDouble(14, genome.getStandardDeviationLength());
      ps.executeUpdate();
      rs = ps.getGeneratedKeys();
      if (!rs.next()) throw new SQLException("Failed to get generated keys");
      fileId = rs.getLong(1);
      rs.close();
      ps.close();
      connection.commit();
      connection.close();
      connection = null;
    } catch (SQLException e) {
      logger.throwing("SequenceProcessor", "recordSequences", e);
      throw e;
    } finally {
      if (connection != null) {
        try { connection.rollback(); } catch (SQLException e) { /* ignore */ }
        try { connection.close(); } catch (SQLException e) { /* ignore */ }
      }
      dbLock.writeLock().unlock();
    }
    return new SequenceKeys(categoryId, listingId, fileId);
  }

  public void obsoleteOldEditions(long listingId, Alphabet alphabet, int keep) throws SQLException {
    Connection conn = null;
    try {
      conn = dataSource.getConnection();
      conn.setAutoCommit(false);
      PreparedStatement ps = conn.prepareStatement(SQL.MARK_OLD_LISTING_FILES_OBSOLETE);
      ps.setLong(1, listingId);
      ps.setInt(2, alphabet.toInt());
      ps.setLong(3, listingId);
      ps.setInt(4, alphabet.toInt());
      ps.setInt(5, keep);
      ps.executeUpdate();
      ps.close();
      conn.commit();
      conn.close();
      conn = null;
    } catch (SQLException e) {
      logger.throwing("SequenceProcessor", "obsoleteOldEditions", e);
      throw e;
    } finally {
      if (conn != null) {
        try { conn.rollback(); } catch (SQLException e) { /* ignore */ }
        try { conn.close(); } catch (SQLException e) { /* ignore */ }
      }
    }
  }

  public abstract void process() throws IOException, SQLException, InterruptedException;

  public Void call() throws IOException, SQLException, InterruptedException {
    progress = new Progress(multiStatus, "Sequence Processor");
    try {
      process();
    } finally {
      progress.complete();
    }
    return null;
  }

}
