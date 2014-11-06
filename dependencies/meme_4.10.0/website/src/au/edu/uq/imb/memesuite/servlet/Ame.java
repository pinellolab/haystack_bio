package au.edu.uq.imb.memesuite.servlet;

import au.edu.uq.imb.memesuite.data.Background;
import au.edu.uq.imb.memesuite.data.MotifDataSource;
import au.edu.uq.imb.memesuite.data.MotifInfo;
import au.edu.uq.imb.memesuite.data.SequenceDataSource;
import au.edu.uq.imb.memesuite.db.MotifDB;
import au.edu.uq.imb.memesuite.db.MotifDBFile;
import au.edu.uq.imb.memesuite.servlet.util.*;
import au.edu.uq.imb.memesuite.template.HTMLSub;
import au.edu.uq.imb.memesuite.template.HTMLTemplate;
import au.edu.uq.imb.memesuite.util.FileCoord;
import au.edu.uq.imb.memesuite.util.JsonWr;

import java.io.*;
import java.util.*;
import java.util.logging.Level;
import java.util.logging.Logger;

import javax.activation.DataSource;
import javax.servlet.*;
import javax.servlet.http.*;

import static au.edu.uq.imb.memesuite.servlet.util.WebUtils.*;


public class Ame extends SubmitJob<Ame.Data> {
  private HTMLTemplate tmplMain;
  private HTMLTemplate tmplVerify;
  private ComponentHeader header;
  private ComponentSequences sequences;
  private ComponentSequences control;
  private ComponentBfile background;
  private ComponentMotifs motifs;
  private ComponentJobDetails jobDetails;
  private ComponentAdvancedOptions advBtn;
  private ComponentSubmitReset submitReset;
  private ComponentFooter footer;

  private static Logger logger = Logger.getLogger("au.edu.uq.imb.memesuite.web.mast");
  
  protected class Data extends SubmitJob.JobData {
    public String email;
    public String description;
    public String method;
    public String scoring;
    public Double pvalue_threshold;
    public Double pwm_threshold;
    public Double pvalue_report_threshold;
    public SequenceDataSource sequences;
    public SequenceDataSource control;
    public Background background;
    public MotifInfo motifs;

    @Override
    public void outputJson(JsonWr out) throws IOException {
      out.startObject();
      out.property("sequences", sequences);
      if (control != null) out.property("control", control);
      out.property("motifs", motifs);
      out.property("background", background);
      out.property("method", method);
      out.property("scoring", scoring);
      if (pvalue_threshold != null) out.property("pvalue_threshold", pvalue_threshold);
      if (pwm_threshold != null) out.property("pwm_threshold", pwm_threshold);
      if (pvalue_report_threshold != null) out.property("pvalue_report_threshold", pvalue_report_threshold);
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

    @Override
    public String cmd() {
      StringBuilder args = new StringBuilder();
      if (control != null) {
        addArgs(args, "-control", control.getName());
      }
      addArgs(args, "-method", method);
      addArgs(args, "-scoring", scoring);
      if (pvalue_threshold != null) {
        addArgs(args, "-pvalue-threshold", pvalue_threshold);
      }
      if (pwm_threshold != null) {
        addArgs(args, "-pwm-threshold", pwm_threshold);
      }
      if (pvalue_report_threshold != null) {
        addArgs(args, "-pvalue-report-threshold", pvalue_report_threshold);
      }
      if (background.getSource() == Background.Source.FILE) {
        addArgs(args, "-bgformat", 2);
        addArgs(args, "-bgfile", background.getBfile().getName());
      } else if (background.getSource() == Background.Source.UNIFORM) {
        addArgs(args, "-bgformat", 0); // Use uniform background
      } else if (background.getSource() == Background.Source.MEME) {
        addArgs(args, "-bgformat", 1); // Use background in motif file
      }
      addArgs(args, sequences.getName());
      if (motifs instanceof MotifDataSource) {
        addArgs(args, ((MotifDataSource) motifs).getName());
      } else if (motifs instanceof MotifDB){
        for (MotifDBFile file : ((MotifDB) motifs).getMotifFiles()) {
          addArgs(args, "db/" + file.getFileName());
        }
      }
      return args.toString();
    }
  
    @Override
    public List<DataSource> files() {
      ArrayList<DataSource> list = new ArrayList<DataSource>();
      list.add(sequences);
      if (control != null) list.add(control);
      if (motifs instanceof MotifDataSource) {
        list.add((MotifDataSource) motifs);
      }
      if (background.getSource() == Background.Source.FILE) {
        list.add(background.getBfile());
      }
      return list;
    }
  
    @Override
    public void cleanUp() {
      if (sequences != null) {
        if (!sequences.getFile().delete()) {
          logger.log(Level.WARNING, "Unable to delete temporary file " +
              sequences.getFile());
        }
      }
      if (control != null) {
        if (!control.getFile().delete()) {
          logger.log(Level.WARNING, "Unable to delete temporary file " +
              control.getFile());
        }
      }
      if (motifs != null && motifs instanceof MotifDataSource) {
        if (!((MotifDataSource) motifs).getFile().delete()) {
          logger.log(Level.WARNING, "Unable to delete temporary file " +
              ((MotifDataSource) motifs).getFile());
        }
      }
      if (background.getSource() == Background.Source.FILE) {
        if (!background.getBfile().getFile().delete()) {
          logger.log(Level.WARNING, "Unable to delete temporary file " +
              background.getBfile().getFile());
        }
      }
    }
  }

  public Ame() {
    super("AME", "AME");
  }

  @Override
  public void init() throws ServletException {
    super.init();
    // load the templates
    tmplMain = cache.loadAndCache("/WEB-INF/templates/ame.tmpl");
    tmplVerify = cache.loadAndCache("/WEB-INF/templates/ame_verify.tmpl");
    header = new ComponentHeader(cache, msp.getVersion(), tmplMain.getSubtemplate("header"));
    motifs = new ComponentMotifs(context, tmplMain.getSubtemplate("motifs"));
    sequences = new ComponentSequences(context, tmplMain.getSubtemplate("sequences"));
    control = new ComponentSequences(context, tmplMain.getSubtemplate("control"));
    background = new ComponentBfile(context, tmplMain.getSubtemplate("bfile"));
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
    main.set("help", new HTMLSub[]{header.getHelp(), motifs.getHelp(),
        sequences.getHelp(), jobDetails.getHelp(), advBtn.getHelp(), background.getHelp(),
        submitReset.getHelp(), footer.getHelp()});
    main.set("header", header.getComponent());
    main.set("motifs", motifs.getComponent());
    main.set("sequences", sequences.getComponent());
    main.set("control", control.getComponent());
    main.set("bfile", background.getComponent());
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
    // setup default file names
    FileCoord namer = new FileCoord();
    FileCoord.Name sequencesName = namer.createName("sequences.fa");
    FileCoord.Name controlName = namer.createName("control.fa");
    FileCoord.Name motifsName = namer.createName("motifs.meme");
    FileCoord.Name backgroundName = namer.createName("background");
    namer.createName("description");
    namer.createName("address_file");
    Data data = new Data();
    // get the job details
    data.email = jobDetails.getEmail(request, feedback);
    data.description = jobDetails.getDescription(request);
    // get the sequences
    data.sequences = (SequenceDataSource)sequences.getSequences(sequencesName, request, feedback);
    if (!paramBool(request, "generate")) {
      data.control = (SequenceDataSource)control.getSequences(controlName, request, feedback);
    } else {
      data.control = null;
    }
    data.motifs = motifs.getMotifs(motifsName, request, feedback);
    data.method = WebUtils.paramChoice(request, "method", "ranksum", "fisher");
    data.scoring = WebUtils.paramChoice(request, "scoring", "avg", "max", "sum", "totalhits");
    if (data.scoring.equals("totalhits")) {
      data.pvalue_threshold = WebUtils.paramNumber(feedback, "motif match threshold", request, "pvalue_threshold", 0.0, 1.0, 0.0002);
    } else {
      data.pvalue_threshold = null;
    }
    if (data.method.equals("fisher")) {
      data.pwm_threshold = WebUtils.paramNumber(feedback, "sequence match threshold", request, "pwm_threshold", -1e100, +1e100, 1.0);
    } else {
      data.pwm_threshold = null;
    }

    // get advanced options
    data.pvalue_report_threshold = paramNumber(feedback, "p-value report threshold", request, "pvalue_report_threshold", 0.0, 1.0, 0.05);
    data.background = background.getBfile(backgroundName, request, feedback);
    return data;
  }
}

