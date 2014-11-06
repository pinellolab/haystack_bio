package au.edu.uq.imb.memesuite.servlet;

import au.edu.uq.imb.memesuite.data.Alphabet;
import au.edu.uq.imb.memesuite.data.MotifDataSource;
import au.edu.uq.imb.memesuite.data.SequenceDataSource;
import au.edu.uq.imb.memesuite.data.SequenceInfo;
import au.edu.uq.imb.memesuite.db.SequenceDB;
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

public class Mcast extends SubmitJob<Mcast.Data> {
  private HTMLTemplate tmplMain;
  private HTMLTemplate tmplVerify;
  private ComponentHeader header;
  private ComponentMotifs motifs;
  private ComponentSequences sequences;
  private ComponentJobDetails jobDetails;
  private ComponentAdvancedOptions advBtn;
  private ComponentSubmitReset submitReset;
  private ComponentFooter footer;

  private static Logger logger = Logger.getLogger("au.edu.uq.imb.memesuite.web.mcast");
  
  protected class Data extends SubmitJob.JobData {
    public String email;
    public String description;
    public MotifDataSource motifs;
    public SequenceInfo sequences;
    public double motifPv;
    public int maxGap;
    public double outputEv;
    public double pseudocount;

    @Override
    public void outputJson(JsonWr out) throws IOException {
      out.startObject();
      out.property("motifs", motifs);
      out.property("sequences", sequences);
      out.property("motifPv", motifPv);
      out.property("maxGap", maxGap);
      out.property("outputEv", outputEv);
      out.property("pseudocount", pseudocount);
      out.endObject();
    }


    @Override
    public String email() {
      return email;
    }
  
    @Override
    public String description() {
      return description;  // generated code
    }

    @Override
    public String emailTemplate() {
      return tmplVerify.getSubtemplate("message").toString();
    }
  
    @Override
    public String cmd() {
      assert(sequences != null);
      assert(motifs != null);
  //    mcast_webservice [options] <motifs> <sequence db>
  //
  //          Options:
  //            -upseqs <file>        Uploaded sequences
  //            -bgweight <b>         Add b * background frequency to each count in query
  //            -motif_pvthresh <pv>  p-value threshold for motif hits
  //            -max_gap  <gap>       Maximum allowed distance between adjacent hits
  //            -output_evthresh <ev> Print matches with E-values less than E
  //            -help                 brief help message
      StringBuilder args = new StringBuilder();
      if (sequences instanceof SequenceDataSource) {
        addArgs(args, "-upseqs", ((SequenceDataSource) sequences).getName());
      }
      addArgs(args, "-bgweight", pseudocount);
      addArgs(args, "-motif_pvthresh", motifPv);
      addArgs(args, "-max_gap", maxGap);
      addArgs(args, "-output_evthresh", outputEv);
      addArgs(args, motifs.getName());
      if (sequences instanceof SequenceDB) {
        addArgs(args, ((SequenceDB) sequences).getSequenceName());
      }
      return args.toString();
    }
  
    @Override
    public List<DataSource> files() {
      ArrayList<DataSource> list = new ArrayList<DataSource>();
      if (motifs != null) list.add(motifs);
      if (sequences != null && sequences instanceof SequenceDataSource) {
        list.add((SequenceDataSource) sequences);
      }
      return list;
    }
  
    @Override
    public void cleanUp() {
      if (sequences != null && sequences instanceof SequenceDataSource) {
        if (!((SequenceDataSource) sequences).getFile().delete()) {
          logger.log(Level.WARNING, "Unable to delete temporary file " +
              ((SequenceDataSource) sequences).getFile());
        }
      }
      if (motifs != null) {
        if (!motifs.getFile().delete()) {
          logger.log(Level.WARNING, "Unable to delete temporary file " +
              motifs.getFile());
        }
      }
    }
  }

  public Mcast() {
    super("MCAST", "MCAST");
  }

  @Override
  public void init() throws ServletException {
    super.init();
    // load the templates
    tmplMain = cache.loadAndCache("/WEB-INF/templates/mcast.tmpl");
    tmplVerify = cache.loadAndCache("/WEB-INF/templates/mcast_verify.tmpl");
    header = new ComponentHeader(cache, msp.getVersion(), tmplMain.getSubtemplate("header"));
    motifs = new ComponentMotifs(context, tmplMain.getSubtemplate("motifs"));
    sequences = new ComponentSequences(context, tmplMain.getSubtemplate("sequences"));
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
        sequences.getHelp(), jobDetails.getHelp(), advBtn.getHelp(),
        submitReset.getHelp(), footer.getHelp()});
    main.set("header", header.getComponent());
    main.set("motifs", motifs.getComponent());
    main.set("sequences", sequences.getComponent());
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
    FileCoord.Name motifsName = namer.createName("motifs.txt");
    namer.createName("description");
    namer.createName("address_file");
    Data data = new Data();
    // get the job details
    data.email = jobDetails.getEmail(request, feedback);
    data.description = jobDetails.getDescription(request);
    // get the motifs
    data.motifs =  (MotifDataSource)motifs.getMotifs(motifsName, request, feedback);
    // get the sequences
    data.sequences = sequences.getSequences(sequencesName, request, feedback);
    // get the advanced options
    data.pseudocount = WebUtils.paramNumber(feedback, "pseudocount",
        request, "pseudocount", 0.0, null, 4.0);
    data.motifPv = WebUtils.paramNumber(feedback, "motif p-value",
        request, "motif_pv", 0.0, 1.0, 0.0005);
    data.maxGap = WebUtils.paramInteger(feedback, "max gap between hits",
        request, "max_gap", 0, null, 50);
    data.outputEv = WebUtils.paramNumber(feedback, "output E-value threshold",
        request, "output_ev", 0.0, null, 10.0);
    return data;
  }
}

