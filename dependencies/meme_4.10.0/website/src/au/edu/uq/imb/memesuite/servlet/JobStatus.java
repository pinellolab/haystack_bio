package au.edu.uq.imb.memesuite.servlet;

import au.edu.uq.imb.memesuite.data.MemeSuiteProperties;
import au.edu.uq.imb.memesuite.servlet.util.WebUtils;
import au.edu.uq.imb.memesuite.template.HTMLSub;
import au.edu.uq.imb.memesuite.template.HTMLTemplate;
import au.edu.uq.imb.memesuite.template.HTMLTemplateCache;

import edu.sdsc.nbcr.opal.AppServicePortType;
import edu.sdsc.nbcr.opal.StatusOutputType;
import org.globus.gram.GramJob;

import javax.servlet.ServletContext;
import javax.servlet.ServletException;
import javax.servlet.http.HttpServlet;
import javax.servlet.http.HttpServletRequest;
import javax.servlet.http.HttpServletResponse;
import java.io.IOException;
import java.io.PrintWriter;
import java.net.HttpURLConnection;
import java.net.URL;
import java.net.URLConnection;
import java.rmi.RemoteException;
import java.util.Map;
import java.util.TreeMap;
import java.util.logging.Level;
import java.util.logging.Logger;

import static au.edu.uq.imb.memesuite.servlet.ConfigurationLoader.CONFIG_KEY;
import static au.edu.uq.imb.memesuite.servlet.ConfigurationLoader.CACHE_KEY;

/**
 * Displays the status of a job
 */
public class JobStatus extends HttpServlet {
  private HTMLTemplate tmplStatus;
  private MemeSuiteProperties msp;
  private Map<String, HTMLTemplate> reports;

  private static Logger logger = Logger.getLogger("au.edu.uq.imb.memesuite.web.status");

  @Override
  public void init() throws ServletException {
    ServletContext context = this.getServletContext();
    msp = (MemeSuiteProperties)context.getAttribute(CONFIG_KEY);
    if (msp == null) throw new ServletException("Failed to get MEME Suite properties");
    HTMLTemplateCache cache = (HTMLTemplateCache)context.getAttribute(CACHE_KEY);
    tmplStatus = cache.loadAndCache("/WEB-INF/templates/status.tmpl");
    reports = new TreeMap<String, HTMLTemplate>();
    reports.put("AME", cache.loadAndCache("/WEB-INF/templates/ame_verify.tmpl"));
    reports.put("CENTRIMO", cache.loadAndCache("/WEB-INF/templates/centrimo_verify.tmpl"));
    reports.put("DREME", cache.loadAndCache("/WEB-INF/templates/dreme_verify.tmpl"));
    reports.put("FIMO", cache.loadAndCache("/WEB-INF/templates/fimo_verify.tmpl"));
    reports.put("GLAM2", cache.loadAndCache("/WEB-INF/templates/glam2_verify.tmpl"));
    reports.put("GLAM2SCAN", cache.loadAndCache("/WEB-INF/templates/glam2scan_verify.tmpl"));
    reports.put("GOMO", cache.loadAndCache("/WEB-INF/templates/gomo_verify.tmpl"));
    reports.put("MAST", cache.loadAndCache("/WEB-INF/templates/mast_verify.tmpl"));
    reports.put("MCAST", cache.loadAndCache("/WEB-INF/templates/mcast_verify.tmpl"));
    reports.put("MEME", cache.loadAndCache("/WEB-INF/templates/meme_verify.tmpl"));
    reports.put("MEMECHIP", cache.loadAndCache("/WEB-INF/templates/meme-chip_verify.tmpl"));
    reports.put("SPAMO", cache.loadAndCache("/WEB-INF/templates/spamo_verify.tmpl"));
    reports.put("TOMTOM", cache.loadAndCache("/WEB-INF/templates/tomtom_verify.tmpl"));
  }

  protected static boolean urlExists(String url) {
    logger.log(Level.INFO, "Checking URL \"" + url + "\"");
    url = url.replaceFirst("https", "http");
    try {
      URLConnection connection = new URL(url).openConnection();
      if (connection instanceof HttpURLConnection) {
        HttpURLConnection http = (HttpURLConnection)connection;
        http.setRequestMethod("HEAD");
        int responseCode = http.getResponseCode();
        logger.log(Level.INFO, "Got response code " + responseCode + " for URL \"" + url + "\"");
        return responseCode == 200;
      } else {
        logger.log(Level.WARNING, "Unable to cast URLConnection to HttpURLConnection!");
        return false;
      }
    } catch (IOException e) {
      logger.log(Level.WARNING, "Failed to check URL \"" + url + "\"", e);
      return false;
    }
  }

  private static String jobStatus(StatusOutputType status) {
    switch (status.getCode()) {
      case GramJob.STATUS_UNSUBMITTED:
      case GramJob.STATUS_PENDING:
      case GramJob.STATUS_STAGE_IN:
        return "pending";
      case GramJob.STATUS_ACTIVE:
      case GramJob.STATUS_STAGE_OUT:
        return "active";
      case GramJob.STATUS_SUSPENDED:
        return "suspended";
      case GramJob.STATUS_FAILED:
        return "failed";
      case GramJob.STATUS_DONE:
        return "done";
      default:
        return "unknown";
    }
  }

  private static String jobUrl(StatusOutputType status) {
    String baseURL = status.getBaseURL().toString();
    return baseURL + (baseURL.endsWith("/") ? "" : "/") + "index.html";
  }

  @Override
  protected void doGet(HttpServletRequest req, HttpServletResponse resp) throws ServletException, IOException {
    String serviceName = req.getParameter("service");
    String jobId = req.getParameter("id");
    if (req.getParameter("xml") == null) {
      generateHtml(serviceName, jobId, resp);
    } else {
      generateXml(serviceName, jobId, resp);
    }
  }

  protected void generateXml(String serviceName, String jobId, HttpServletResponse resp) throws ServletException, IOException {
    StatusOutputType status = null;
    if (reports.containsKey(serviceName) && jobId != null) {
      try {
        AppServicePortType app = WebUtils.getOpal(msp, serviceName);
        status = app.queryStatus(jobId);
      } catch (RemoteException e) { /* ignore */  }
    }
    resp.setContentType("application/xml");
    PrintWriter out = resp.getWriter();
    if (status != null) {
      String url = jobUrl(status);
      out.println("<job>");
      out.print("<status>");
      out.print(jobStatus(status));
      out.println("</status>");
      if (urlExists(url)) {
        out.print("<url>");
        out.print(url);
        out.println("</url>");
      }
      out.println("</job>");
    } else {
      out.println("<job>");
      out.println("<status>unknown</status>");
      out.println("</job>");
    }
  }

  protected void generateHtml(String serviceName, String jobId, HttpServletResponse resp) throws ServletException, IOException {
    HTMLSub page = this.tmplStatus.toSub();
    // look up parameters
    // try to get info for service
    HTMLTemplate serviceInfo = reports.get(serviceName);
    if (serviceInfo != null) {
      // Yay, I've heard about this service!
      // input report information
      page.set("report", serviceInfo.getSubtemplate("message"));
      // setup page header
      page.empty("suite_header");
      page.getSub("program_header").
          set("title", serviceInfo.getSubtemplate("title")).
          set("subtitle", serviceInfo.getSubtemplate("subtitle")).
          set("logo", "../" + serviceInfo.getSubtemplate("logo")).
          set("alt", serviceInfo.getSubtemplate("alt"));
      page.set("service", serviceName);

      // see if we can discover information about the job
      StatusOutputType status = null;
      if (jobId != null) {
        try {
          AppServicePortType app = WebUtils.getOpal(msp, serviceName);
          status = app.queryStatus(jobId);
        } catch (RemoteException e) { /* ignore */  }
      }
      if (status != null) {
        // Yay, I know about this job!
        page.set("id", jobId);
        // set the displayed status message
        page.set("status", jobStatus(status));
        page.set("title", serviceInfo.getSubtemplate("title"));
        // display the output index if it has been created
        String fullURL = jobUrl(status);
        page.set("url", fullURL);
        if (urlExists(fullURL)) {
          page.set("preview", "expanded");
        } else {
          page.set("details", "expanded");
        }
      } else {
        // don't know about this job!
        page.set("status", "expired");
      }
    } else {
      // don't know about this service!
      page.set("report", "{}");
      page.empty("program_header");
      page.set("status", "expired");
    }
    resp.setContentType("text/html");
    page.output(resp.getWriter());
  }
}
