package au.edu.uq.imb.memesuite.servlet;

import au.edu.uq.imb.memesuite.data.MotifDataSource;
import au.edu.uq.imb.memesuite.db.GomoDB;
import au.edu.uq.imb.memesuite.db.GomoDBSecondary;
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


public class Gomo extends SubmitJob<Gomo.Data> {
  private HTMLTemplate tmplMain;
  private HTMLTemplate tmplVerify;
  private ComponentHeader header;
  private ComponentMotifs motifs;
  private ComponentGomo gomoSequences;
  private ComponentJobDetails jobDetails;
  private ComponentAdvancedOptions advBtn;
  private ComponentSubmitReset submitReset;
  private ComponentFooter footer;

  private static Logger logger = Logger.getLogger("au.edu.uq.imb.memesuite.web.gomo");
  
  protected class Data extends SubmitJob.JobData {
    public String email;
    public String description;
    public MotifDataSource motifs;
    public GomoDB gomoSequences;
    public Double threshold;
    public Integer shuffleRounds;
    public boolean multiGenome;

    @Override
    public void outputJson(JsonWr out) throws IOException {
      out.startObject();
      out.property("motifs", motifs);
      out.property("gomoSequences", gomoSequences);
      if (threshold != null) out.property("threshold", threshold);
      if (shuffleRounds != null) out.property("shuffleRounds", shuffleRounds);
      out.property("multiGenome", multiGenome);
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
  //    gomo_webservice [options] <motif> <GO map> (<sequences> <background>)+
  //
  //      Options:
  //        -shuffle_scores <times> shuffle scores
  //        -t <threshold>          q-value threshold
  //        -help                   brief help message
  //
  //      Files present in the server gomo databases can be specified by appending 'db/'
  //      to the file name.
      StringBuilder args = new StringBuilder();
      if (shuffleRounds != null) addArgs(args, "-shuffle_scores", shuffleRounds);
      if (threshold != null) addArgs(args, "-t", threshold);
      addArgs(args, motifs.getName());
      addArgs(args, "db/" + gomoSequences.getGoMapFileName());
      addArgs(args, "db/" + gomoSequences.getSequenceFileName());
      addArgs(args, "db/" + gomoSequences.getBackgroundFileName());
      if (multiGenome) {
        for (GomoDBSecondary secondary : gomoSequences.getSecondaries()) {
          addArgs(args, "db/" + secondary.getSequenceFileName());
          addArgs(args, "db/" + secondary.getBackgroundFileName());
        }
      }
      return args.toString();
    }
  
    @Override
    public List<DataSource> files() {
      ArrayList<DataSource> list = new ArrayList<DataSource>();
      if (motifs != null) list.add(motifs);
      return list;
    }
  
    @Override
    public void cleanUp() {
      if (motifs != null) {
        if (!motifs.getFile().delete()) {
          logger.log(Level.WARNING, "Unable to delete temporary file " +
              motifs.getFile());
        }
      }
    }
  }

  public Gomo() {
    super("GOMO", "GOMo");
  }

  @Override
  public void init() throws ServletException {
    super.init();
    // load the templates
    tmplMain = cache.loadAndCache("/WEB-INF/templates/gomo.tmpl");
    tmplVerify = cache.loadAndCache("/WEB-INF/templates/gomo_verify.tmpl");
    header = new ComponentHeader(cache, msp.getVersion(), tmplMain.getSubtemplate("header"));
    motifs = new ComponentMotifs(context, tmplMain.getSubtemplate("motifs"));
    gomoSequences = new ComponentGomo(context, tmplMain.getSubtemplate("gomo_sequences"));
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
        gomoSequences.getHelp(), jobDetails.getHelp(), advBtn.getHelp(),
        submitReset.getHelp(), footer.getHelp()});
    main.set("header", header.getComponent());
    main.set("motifs", motifs.getComponent(request.getParameter("motifs_embed")));
    main.set("gomo_sequences", gomoSequences.getComponent());
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
    FileCoord.Name motifsName = namer.createName("motifs.meme");
    namer.createName("description");
    namer.createName("address_file");
    Data data = new Data();
    // get the job details
    data.email = jobDetails.getEmail(request, feedback);
    data.description = jobDetails.getDescription(request);
    // get the motifs
    data.motifs =  (MotifDataSource)motifs.getMotifs(motifsName, request, feedback);
    // get the sequences
    data.gomoSequences = gomoSequences.getGomoSequences(request);
    data.threshold = paramNumber(feedback, "q-value threshold", request, "threshold", 0.0, 0.5, 0.05);
    data.shuffleRounds = paramInteger(feedback, "shuffle rounds", request, "shuffle_rounds", 1, null, 10);
    data.multiGenome = paramBool(request, "multi_genome");
    return data;
  }
}

