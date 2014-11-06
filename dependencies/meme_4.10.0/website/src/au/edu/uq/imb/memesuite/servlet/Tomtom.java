package au.edu.uq.imb.memesuite.servlet;

import au.edu.uq.imb.memesuite.data.MotifDataSource;
import au.edu.uq.imb.memesuite.data.MotifInfo;
import au.edu.uq.imb.memesuite.db.MotifDB;
import au.edu.uq.imb.memesuite.db.MotifDBFile;
import au.edu.uq.imb.memesuite.servlet.util.*;
import au.edu.uq.imb.memesuite.template.HTMLSub;
import au.edu.uq.imb.memesuite.template.HTMLTemplate;
import au.edu.uq.imb.memesuite.util.FileCoord;
import au.edu.uq.imb.memesuite.util.JsonWr;

import org.apache.commons.io.FileUtils;

import java.io.*;
import java.util.*;
import java.util.concurrent.TimeUnit;

import javax.activation.DataSource;
import javax.servlet.*;
import javax.servlet.http.*;

import static au.edu.uq.imb.memesuite.servlet.util.WebUtils.*;


public class Tomtom extends SubmitJob<Tomtom.Data> {
  private static final int DEFAULT_BUFFER_SIZE = 10240; // 10KB.
  private static final long TOMTOM_TIMEOUT = TimeUnit.MINUTES.toMillis(1);
  private HTMLTemplate tmplMain;
  private HTMLTemplate tmplVerify;
  private ComponentHeader header;
  private ComponentMotifs query;
  private ComponentMotifs target;
  private ComponentJobDetails jobDetails;
  private ComponentAdvancedOptions advBtn;
  private ComponentSubmitReset submitReset;
  private ComponentFooter footer;
  
  protected class Data extends SubmitJob.JobData {
    public String email;
    public String description;
    public MotifDataSource queryMotifs;
    public MotifInfo targetMotifs;
    public String comparisonFunction;
    public boolean isEvalueThreshold;
    public double threshold;
    public boolean completeScoring;
    public boolean immediateRun;

    @Override
    public void outputJson(JsonWr out) throws IOException {
      out.startObject();
      out.property("queryMotifs", queryMotifs);
      out.property("targetMotifs", targetMotifs);
      out.property("comparisonFunction", comparisonFunction);
      out.property("isEvalueThreshold", isEvalueThreshold);
      out.property("threshold", threshold);
      out.property("completeScoring", completeScoring);
      out.property("immediateRun", immediateRun);
      out.endObject();
    }


    @Override
    public String email() {
      return email;
    }
  
    @Override
    public String description() {
      return description;
    }

    @Override
    public String emailTemplate() {
      return tmplVerify.getSubtemplate("message").toString();
    }

    public List<String> args() {
  //    tomtom_webservice [options] <query motifs> <motif databases>
  //
  //      Options:
  //        -dist (pearson|ed|sandelin)   distance function to use; default pearson
  //        -ev <evalue>                  evalue threshold; default 10; not usable with -qv
  //        -qv <qvalue>                  qvalue threshold; not usable with -ev
  //        -m <name>                     filter query motifs by name (id); repeatable
  //        -mi <index>                   filter query motifs by file order; repeatable
  //        -uptargets <file>             uploaded target motifs
  //        -incomplete_scores            don't included unaligned parts of the motif in scoring
  //        -niced                        run tomtom niced
  //        -help                         brief help message
      List<String> args = new ArrayList<String>();
      addArgs(args, "-dist", comparisonFunction,
          (isEvalueThreshold ? "-ev" : "-qv"), threshold);
      List<ComponentMotifs.Selection> selections = queryMotifs.getSelections();
      if (!immediateRun) {
        for (ComponentMotifs.Selection selection : selections) {
          addArgs(args, (selection.isPosition() ? "-mi" : "-m"), selection.getEntry());
        }
      } else {
        if (selections.size() > 0) {
          ComponentMotifs.Selection selection = selections.get(0);
          addArgs(args, (selection.isPosition() ? "-mi" : "-m"), selection.getEntry());
        } else {
          addArgs(args, "-mi", 1);
        }
      }
      if (targetMotifs instanceof MotifDataSource) {
        addArgs(args, "-uptargets", ((MotifDataSource) targetMotifs).getName());
      }
      if (!completeScoring) addArgs(args, "-incomplete_scores");
      if (immediateRun) addArgs(args, "-niced");
      addArgs(args, queryMotifs.getName());
      if (targetMotifs instanceof MotifDB) {
        for (MotifDBFile dbFile : ((MotifDB) targetMotifs).getMotifFiles()) {
          addArgs(args, dbFile.getFileName());
        }
      }
      return args;
    }
  
    @Override
    public String cmd() {
      List<String> args = args();
      boolean first = true;
      StringBuilder out = new StringBuilder();
      for (String arg : args) {
        if (!first) out.append(' ');
        out.append(arg);
        first = false;
      }
      return out.toString();
    }
  
    @Override
    public List<DataSource> files() {
      List<DataSource> sources = new ArrayList<DataSource>();
      if (queryMotifs != null) sources.add(queryMotifs);
      if (targetMotifs != null && targetMotifs instanceof MotifDataSource) {
        sources.add((MotifDataSource) targetMotifs);
      }
      return sources;
    }
  
    @SuppressWarnings("ResultOfMethodCallIgnored")
    @Override
    public void cleanUp() {
      if (queryMotifs != null) {
        queryMotifs.getFile().delete();
      }
      if (targetMotifs != null && targetMotifs instanceof MotifDataSource) {
        ((MotifDataSource) targetMotifs).getFile().delete();
      }
    }
  }

  public Tomtom() {
    super("TOMTOM", "Tomtom");
  }

  @Override
  public void init() throws ServletException {
    super.init();
    // load the template
    this.tmplMain = cache.loadAndCache("/WEB-INF/templates/tomtom.tmpl");
    this.tmplVerify = cache.loadAndCache("/WEB-INF/templates/tomtom_verify.tmpl");
    header = new ComponentHeader(cache, msp.getVersion(), tmplMain.getSubtemplate("header"));
    query = new ComponentMotifs(context, tmplMain.getSubtemplate("query_motifs"));
    target = new ComponentMotifs(context, tmplMain.getSubtemplate("target_motifs"));
    jobDetails = new ComponentJobDetails(cache);
    advBtn = new ComponentAdvancedOptions(cache);
    submitReset = new ComponentSubmitReset(cache);
    footer = new ComponentFooter(cache, msp);
  }

  @Override
  public String title() {
    return tmplVerify.getSubtemplate("title").toString();
  }

  @Override
  public String subtitle() {
    return tmplVerify.getSubtemplate("subtitle").toString();
  }

  @Override
  public String logoPath() {
    return tmplVerify.getSubtemplate("logo").toString();
  }

  @Override
  public String logoAltText() {
    return tmplVerify.getSubtemplate("alt").toString();
  }

  @Override
  protected void displayForm(HttpServletRequest request, HttpServletResponse response) throws IOException {
    HTMLSub main = tmplMain.toSub();
    main.set("help", new HTMLSub[]{header.getHelp(), query.getHelp(),
        jobDetails.getHelp(), advBtn.getHelp(),
        submitReset.getHelp(), footer.getHelp()});
    main.set("header", header.getComponent());
    main.set("query_motifs", query.getComponent(request.getParameter("motifs_embed")));
    main.set("target_motifs", target.getComponent());
    main.set("job_details", jobDetails.getComponent());
    main.set("advanced_options", advBtn.getComponent());
    main.set("submit_reset", submitReset.getComponent());
    main.set("footer", footer.getComponent());
    response.setContentType("text/html");
    main.output(response.getWriter());
  }

  @Override
  protected Data checkParameters(FeedbackHandler feedback,
      HttpServletRequest request) throws IOException, ServletException {
    FileCoord namer = new FileCoord();
    FileCoord.Name queryName = namer.createName("query_motifs");
    FileCoord.Name targetName = namer.createName("target_motifs");
    boolean error = true; // detect errors
    Data data = new Data();
    try {
      // decide if the job should be sent to Opal
      data.immediateRun = paramBool(request, "instant_run");
      // get the job details
      if (!data.immediateRun) data.email = jobDetails.getEmail(request, feedback);
      data.description = jobDetails.getDescription(request);
      // get the query motifs
      data.queryMotifs =  (MotifDataSource)query.getMotifs(queryName, request, feedback);
      // get the target motifs
      data.targetMotifs =  target.getMotifs(targetName, request, feedback);
      // get the comparison function
      data.comparisonFunction = paramChoice(request, "comparison_function", "pearson", "ed", "sandelin");
      // get the cut-off threshold
      data.isEvalueThreshold = paramBool(request, "thresh_type");
      data.threshold = paramNumber(feedback,
          (data.isEvalueThreshold ? "E-value threshold" : "q-value threshold"),
          request, "thresh", 0.0,
          (data.isEvalueThreshold ? null : 1.0 - Math.ulp(1.0)), //don't allow q-value of 1 because it's silly
          (data.isEvalueThreshold ? 10 : 0.1));
      // should overlaps be ignored or included in scoring?
      data.completeScoring = paramBool(request, "complete_scoring");
      error = false;
    } finally {
      if (error) data.cleanUp();
    }
    return data;
  }

  /**
   * Creates a temporary results directory.
   * @return result dir.
   */
  private File createResultDir() {
    File tempDir = new File(System.getProperty("java.io.tmpdir"));
    String fileNamePart = "TOMTOM_" + msp.getVersion() + "_" + System.currentTimeMillis() + "-";
    final int TEMP_DIR_ATTEMPTS = 10000;
    for (int i = 0; i < TEMP_DIR_ATTEMPTS; i++) {
      File resultDir = new File(tempDir, fileNamePart + i);
      if (resultDir.mkdir()) {
        return resultDir;
      }
    }
    throw new IllegalStateException("Failed to create TOMTOM result directory");
  }

  private void storeSource(File dir, DataSource source) throws IOException{
    InputStream in = null;
    OutputStream out = null;
    try {
      in = source.getInputStream();
      out = new FileOutputStream(new File(dir, source.getName()));
      byte[] buffer = new byte[10240]; // 10KB
      int len;
      while ((len = in.read(buffer)) != -1) {
        out.write(buffer, 0, len);
      }
      in.close(); in = null;
      out.close(); out = null;
    } finally {
      closeQuietly(in);
      closeQuietly(out);
    }
  }

  private void storeString(File dir, String fileName, String value) throws IOException {
    OutputStream out = null;
    try {
      out = new FileOutputStream(new File(dir, fileName));
      out.write(value.getBytes("UTF-8"));
      out.close(); out = null;
    } finally {
      closeQuietly(out);
    }
  }

  private static class Waiter extends Thread {
    private final Process process;
    public Waiter(Process process) {
      this.process = process;
    }
    public void run() {
      try { process.waitFor(); } catch (InterruptedException e) { /* just exit */ }
    }
  }

  @Override
  protected void submitOpalJob(Data data, HttpServletResponse response)
      throws ServletException, IOException {
    if (!data.immediateRun) {
      super.submitOpalJob(data, response);
    } else {
      // skip sending to opal, just run tomtom_webservice and redirect
      File resultDir = null;
      try {
        resultDir = createResultDir();
        // store the data files needed by the webservice
        for (DataSource source : data.files()) storeSource(resultDir, source);
        if (data.description != null) storeString(resultDir, "description", data.description());
        storeString(resultDir, "address_file", "meme@sdsc.edu");
        // run the script
        List<String> tomtom_args = data.args();
        tomtom_args.add(0, new File(msp.getBinDir(), "tomtom_webservice").toString());
        ProcessBuilder pb = new ProcessBuilder();
        pb.directory(resultDir); // set the running directory
        pb.command(tomtom_args); // set the command
        Process tomtom = pb.start();
        // setup a thread to consume any messages and print them to console
        StreamCopier.toSystemOut(tomtom.getInputStream(), true);
        StreamCopier.toSystemErr(tomtom.getErrorStream(), true);
        // setup a thread to wait for the process so we can wait for it (until a timeout)
        Waiter waiter = new Waiter(tomtom);
        waiter.start();
        Integer exitValue = null;
        try {
          waiter.join(TOMTOM_TIMEOUT);
          try { exitValue = tomtom.exitValue(); } catch (IllegalThreadStateException e) { /* ignore */ }
        } catch (InterruptedException e) {
          throw new ServletException(e);
        } finally {
          tomtom.destroy();
        }
        // now check that it worked.
        if (exitValue == null) {
          throw new ServletException("Tomtom did not complete within time limit for unqueued jobs.");
        } else if (exitValue != 0) {
          throw new ServletException("Tomtom returned non-zero exit status.");
        }
        // check that a tomtom.html output was created
        File result = new File(resultDir, "tomtom.html");
        if (!result.isFile()) {
          throw new ServletException("Tomtom did not create a tomtom.html file");
        }
        // send the file as output
        response.reset();
        response.setBufferSize(DEFAULT_BUFFER_SIZE);
        response.setContentType("text/html");
        response.setHeader("Content-Length", String.valueOf(result.length()));
        // Prepare streams.
        BufferedInputStream input = null;
        BufferedOutputStream output = null;
        try {
          // Open streams.
          input = new BufferedInputStream(new FileInputStream(result), DEFAULT_BUFFER_SIZE);
          output = new BufferedOutputStream(response.getOutputStream(), DEFAULT_BUFFER_SIZE);
          // Write file contents
          byte[] buffer = new byte[DEFAULT_BUFFER_SIZE];
          int length;
          while ((length = input.read(buffer)) > 0) {
              output.write(buffer, 0, length);
          }
          output.close(); output = null;
          input.close(); input = null;
        } finally {
          closeQuietly(output);
          closeQuietly(input);
        }
      } finally {
        if (resultDir != null) FileUtils.deleteDirectory(resultDir);
      }
    }
  }
}

