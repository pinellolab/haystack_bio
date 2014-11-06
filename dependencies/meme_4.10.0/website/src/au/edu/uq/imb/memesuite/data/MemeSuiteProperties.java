package au.edu.uq.imb.memesuite.data;

import java.io.File;
import java.io.IOException;
import java.io.InputStream;
import java.net.MalformedURLException;
import java.net.URL;
import java.util.Properties;
import java.util.concurrent.TimeUnit;
import java.util.prefs.BackingStoreException;
import java.util.regex.Pattern;

/**
 * Stores the configuration values needed to run the MEME Suite.
 */
public class MemeSuiteProperties {
  private String version;
  private String websiteUrl;
  private String servicesBase;
  private String serverDN;
  private boolean httpsInUse;
  private String siteContact;
  private String developerContact;
  private File binDir;
  private File etcDir;
  private File dbDir;
  private String sendmail;
  private int jobExpiry;
  
  public MemeSuiteProperties() throws BackingStoreException {
    // load the properties file
    Properties config = new Properties();
    InputStream inStream = Thread.currentThread().getContextClassLoader().
        getResourceAsStream("MemeSuite.properties");
    if (inStream == null) {
      throw new BackingStoreException("The properties file " +
        "\"MemeSuite.properties\" is required but it " +
        "could not be found in the classpath.");
    }
    try {
      config.load(inStream);
    } catch (IOException e) {
      throw new BackingStoreException(e);
    } catch (IllegalArgumentException e) {
      throw new BackingStoreException(e);
    }
    // get the version property
    if ((version = config.getProperty("version")) == null) {
      throw new BackingStoreException("The MEME Suite version string is " +
        "required. It should be listed in the file \"MemeSuite.properties\" " +
        "for the entry \"version\".");
    }
    // get the site URL property
    if ((websiteUrl = config.getProperty("site.url")) == null) {
      throw new BackingStoreException("The MEME Suite website URL is " +
        "required. It should be listed in the file \"MemeSuite.properties\" " +
        "for the entry \"site.url\".");
    }

    // get the contact property
    this.siteContact = config.getProperty("site.contact");
    // get the developer contact property
    if ((developerContact = config.getProperty("developer.contact")) == null) {
      throw new BackingStoreException("The MEME Suite developer contact is " +
          "required. It should be listed in the file " +
          "\"MemeSuite.properties\" for the entry \"developer.contact\".");
    }
    String expiry = config.getProperty("expiry");
    if (expiry == null) {
      throw new BackingStoreException("The MEME Suite job expiry time is " +
          "required. It should be listed in the file " +
          "\"MemeSuite.properties\" for the entry \"expiry\".");
    }
    jobExpiry = Integer.parseInt(expiry, 10);

    // get the bin property
    String bin = config.getProperty("bin.dir");
    if (bin == null) {
      throw new BackingStoreException("The MEME Suite bin folder is " +
          "required. It should be listed in the file " +
          "\"MemeSuite.properties\" for the entry \"bin.dir\".");
    }
    this.binDir = new File(bin);
    // get the etc.dir property
    String etc = config.getProperty("etc.dir");
    if (etc == null) {
      throw new BackingStoreException("The MEME Suite etc folder is " +
          "required. It should be listed in the file " +
          "\"MemeSuite.properties\" for the entry \"etc.dir\".");
    }
    this.etcDir = new File(etc);
    // get the db.dir property
    String db = config.getProperty("db.dir");
    if (db == null) {
      throw new BackingStoreException("The MEME Suite etc folder is " +
          "required. It should be listed in the file " +
          "\"MemeSuite.properties\" for the entry \"etc.dir\".");
    }
    this.dbDir = new File(db);
    // get the services property
    if ((servicesBase = config.getProperty("site.services")) == null) {
      throw new BackingStoreException("The URL to the MEME Suite services " +
        "is required. It should be listed in the file " +
        "\"MemeSuite.properties\" for the entry \"site.services\".");
    }
    try {
      URL servicesBaseURL = new URL(servicesBase);
      this.httpsInUse = servicesBaseURL.getProtocol().equalsIgnoreCase("https");
    } catch (MalformedURLException e) {
      throw new BackingStoreException("The URL to the MEME Suite services " +
          "is not correctly formed. It should be listed in the file " +
          "\"MemeSuite.properties\" for the entry \"site.services\".");
    }
    this.serverDN = config.getProperty("site.DN");
    if (this.httpsInUse && this.serverDN == null) {
      throw new BackingStoreException("The server distinguished name (DN) " +
        "is required to enable a secure connection. It " +
        "should be listed in the file \"MemeSuite.properties\" for the " +
        "entry \"serverDN\".");
    }
    this.sendmail = config.getProperty("sendmail");
  }

  public String getVersion() {
    return version;
  }

  public String getSiteURL() {
    return websiteUrl + (!websiteUrl.endsWith("/") ? "/" : "");
  }

  public String getSiteContact() {
    return siteContact;
  }

  public String getDeveloperContact() {
    return developerContact;
  }

  public File getBinDir() {
    return binDir;
  }

  public File getDbDir() {
    return dbDir;
  }

  public File getEtcDir() {
    return etcDir;
  }

  public boolean isHttpsInUse() {
    return httpsInUse;
  }

  public String getServicesBase() {
    return servicesBase;
  }

  public String getServerDN() {
    return serverDN;
  }

  public URL getServiceURL(String serviceName) {
    if (serviceName == null) throw new NullPointerException("Parameter serviceName must not be null");
    try {
      return new URL(servicesBase + (!servicesBase.endsWith("/") ? "/" : "")
          + serviceName + "_" + version);
    } catch (MalformedURLException e) {
      throw new RuntimeException(e);
    }
  }

  public URL getStatisticsURL() {
    try {
      return new URL(servicesBase + (!servicesBase.endsWith("/") ? "/" : "") +
          "../dashboard?command=statistics");
    } catch (MalformedURLException e) {
      throw new RuntimeException(e);
    }
  }

  public String getSendmail() {
    return sendmail;
  }

  public long getExpiryDelay() {
    return TimeUnit.DAYS.toMillis(jobExpiry);
  }
}
