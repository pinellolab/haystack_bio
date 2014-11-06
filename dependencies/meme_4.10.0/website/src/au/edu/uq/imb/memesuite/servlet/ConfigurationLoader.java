package au.edu.uq.imb.memesuite.servlet;

import au.edu.uq.imb.memesuite.data.MemeSuiteProperties;
import au.edu.uq.imb.memesuite.db.DBList;
import au.edu.uq.imb.memesuite.db.GomoDBList;
import au.edu.uq.imb.memesuite.db.MotifDBList;
import au.edu.uq.imb.memesuite.db.SequenceDBList;
import au.edu.uq.imb.memesuite.template.HTMLTemplateCache;

import javax.servlet.ServletContextListener;
import javax.servlet.ServletContextEvent;
import javax.servlet.ServletContext;
import java.io.File;
import java.util.Queue;
import java.util.concurrent.Executors;
import java.util.concurrent.LinkedBlockingQueue;
import java.util.concurrent.ScheduledExecutorService;
import java.util.concurrent.TimeUnit;
import java.util.logging.Level;
import java.util.logging.Logger;
import java.util.prefs.BackingStoreException;

public class ConfigurationLoader implements ServletContextListener {

  private ScheduledExecutorService scheduler;
  private volatile Queue<DBList> delayedCleanup;

  private static Logger logger = Logger.getLogger("au.edu.uq.imb.memesuite.web");
  public static final String CONFIG_KEY = "au.edu.uq.imb.memesuite.config";
  public static final String CACHE_KEY = "au.edu.uq.imb.memesuite.cache";
  public static final String SEQUENCE_DB_KEY = "au.edu.uq.imb.memesuite.sequencedb";
  public static final String MOTIF_DB_KEY = "au.edu.uq.imb.memesuite.motifdb";
  public static final String GOMO_DB_KEY = "au.edu.uq.imb.memesuite.gomodb";

  public ConfigurationLoader() {
    delayedCleanup = new LinkedBlockingQueue<DBList>();
  }

  private SequenceDBList getSequenceDB(MemeSuiteProperties config) {
    SequenceDBList sequenceDB = null;
    File sequencesDir = new File(config.getDbDir(), "fasta_databases");
    File db;
    if (sequencesDir.exists() && sequencesDir.isDirectory()) {
      db = new File(sequencesDir, "fasta_db.sqlite");
    } else {
      db = new File(config.getDbDir(), "fasta_db.sqlite");
    }
    if (db.exists() && db.isFile()) {
      try {
        logger.log(Level.INFO, "Loading sequence databases from \"" + db + "\"");
        sequenceDB = new SequenceDBList(db);
      } catch (ClassNotFoundException e) {
        logger.log(Level.SEVERE, "Error loading sequence database", e);
      }
    } else {
      logger.log(Level.WARNING, "Unable to find sequence database in directories:\n" +
          sequencesDir + "\n" + config.getDbDir());
    }
    return sequenceDB;
  }

  private static File findCsv(MemeSuiteProperties config, String dirName, String csvName) {
    // locations that the csv file could be, in order of preference
    File[] csvLocations = new File[]{
        new File(new File(config.getDbDir(), dirName), csvName),
        new File(config.getDbDir(), csvName),
        new File(config.getEtcDir(), csvName)};
    for (File csv : csvLocations) {
      if (csv.exists()) {
        return csv;
      }
    }
    logger.log(Level.WARNING, "Unable to find " + csvName);
    return null;
  }

  public void contextInitialized(ServletContextEvent sce) {
    try {
      ServletContext context = sce.getServletContext();
      context.setAttribute(CACHE_KEY, new HTMLTemplateCache(context));
      MemeSuiteProperties config;
      try {
        config =  new MemeSuiteProperties();
      } catch (BackingStoreException e) {
        logger.log(Level.SEVERE, "Error loading configuration properties!", e);
        return;
      }
      context.setAttribute(CONFIG_KEY, config);
      context.setAttribute(SEQUENCE_DB_KEY, getSequenceDB(config));
      CsvWatcher watcher = new CsvWatcher(context, config);
      watcher.run();
      // setup a regular check of the motif database
      scheduler = Executors.newSingleThreadScheduledExecutor();
      scheduler.scheduleAtFixedRate(watcher, 5, 5, TimeUnit.MINUTES);
    } catch (Error e) {
      logger.log(Level.SEVERE, "Configuration Initialisation Error!", e);
      throw e;
    } catch (RuntimeException e) {
      logger.log(Level.SEVERE, "Configuration Initialisation RuntimeException!", e);
      throw e;
    }
  }

  public void contextDestroyed(ServletContextEvent sce) {
    try {
      ServletContext context = sce.getServletContext();
      HTMLTemplateCache cache = (HTMLTemplateCache)context.getAttribute(CACHE_KEY);
      cache.clear();
      // stop the csv watcher
      scheduler.shutdownNow();
      try {
        scheduler.awaitTermination(1, TimeUnit.MINUTES);
      } catch (InterruptedException e) { /* ignore */ }
      // attempt to delete the generated databases
      MotifDBList motifDBList = (MotifDBList)context.getAttribute(MOTIF_DB_KEY);
      motifDBList.cleanup();
      GomoDBList gomoDBList = (GomoDBList)context.getAttribute(GOMO_DB_KEY);
      gomoDBList.cleanup();
      DBList db;
      while ((db = delayedCleanup.poll()) != null) {
        db.cleanup();
      }
    } catch (Error e) {
      logger.log(Level.SEVERE, "Configuration Cleanup Error!", e);
      throw e;
    } catch (RuntimeException e) {
      logger.log(Level.SEVERE, "Configuration Cleanup RuntimeException!", e);
      throw e;
    }
  }

  public class CsvWatcher implements Runnable {
    private ServletContext context;
    private MemeSuiteProperties config;

    private void updateMotifDb() {
      MotifDBList db = (MotifDBList)context.getAttribute(MOTIF_DB_KEY);
      File currentCsv = (db == null ? null : db.getCSV());
      File foundCsv = findCsv(config, "motif_databases", "motif_db.csv");
      if (foundCsv == null) return;
      if (currentCsv == null || !foundCsv.equals(currentCsv) ||
          db.lastModified() != foundCsv.lastModified()) {
        if (currentCsv != null) logger.log(Level.INFO, "Found a changed motif database csv at " + foundCsv);
        MotifDBList db2;
        try {
           db2 = new MotifDBList(foundCsv, new File(config.getDbDir(), "motif_databases"));
        } catch (Exception e) {
          logger.log(Level.SEVERE, "Error loading motif database", e);
          return;
        }
        context.setAttribute(MOTIF_DB_KEY, db2);
        if (db != null) delayedCleanup.offer(db);
      }
    }

    private void updateGomoDb() {
      GomoDBList db = (GomoDBList)context.getAttribute(GOMO_DB_KEY);
      File currentCsv = (db == null ? null : db.getCSV());
      File foundCsv = findCsv(config, "gomo_databases", "gomo_db.csv");
      if (foundCsv == null) return;
      if (currentCsv == null || !foundCsv.equals(currentCsv) ||
          db.lastModified() != foundCsv.lastModified()) {
        if (currentCsv != null) logger.log(Level.INFO, "Found a changed gomo database csv at " + foundCsv);
        GomoDBList db2;
        try {
           db2 = new GomoDBList(foundCsv);
        } catch (Exception e) {
          logger.log(Level.SEVERE, "Error loading gomo database", e);
          return;
        }
        context.setAttribute(GOMO_DB_KEY, db2);
        if (db != null) delayedCleanup.offer(db);
      }
    }

    public CsvWatcher(ServletContext context, MemeSuiteProperties config) {
      this.context = context;
      this.config = config;
    }

    @Override
    public void run() {
      // as this should only be run every 5 minutes it minimises the possibility
      // that we're deleting a db that is still in use
      DBList db;
      while ((db = delayedCleanup.poll()) != null) {
        db.cleanup();
      }
      updateMotifDb();
      updateGomoDb();
    }
  }
}
