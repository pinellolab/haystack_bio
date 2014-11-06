package au.edu.uq.imb.memesuite.servlet;

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

public class Spamo extends SubmitJob<Spamo.Data> {
  private HTMLTemplate tmplMain;
  private HTMLTemplate tmplVerify;
  private ComponentHeader header;
  private ComponentMotifs primary;
  private ComponentMotifs secondaries;
  private ComponentSequences sequences;
  private ComponentJobDetails jobDetails;
  private ComponentAdvancedOptions advBtn;
  private ComponentSubmitReset submitReset;
  private ComponentFooter footer;

  private static Logger logger = Logger.getLogger("au.edu.uq.imb.memesuite.web.spamo");
  
  protected class Data extends SubmitJob.JobData {
    public String email;
    public String description;
    public SequenceDataSource sequences;
    public MotifDataSource primary;
    public MotifInfo secondaries;
    public boolean dumpseqs;
    public Integer margin;

    @Override
    public void outputJson(JsonWr out) throws IOException {
      out.startObject();
      out.property("sequences", sequences);
      out.property("primary", primary);
      out.property("secondaries", secondaries);
      out.property("dumpseqs", dumpseqs);
      if (margin != null) out.property("margin", margin);
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
  //    spamo_webservice [options] <sequences file> <primary motif> <secondary db patterns>
  //
  //      Options:
  //        -uploaded <file>  file containing uploaded secondary motif database
  //        -margin <margin>  margin parameter passed to spamo
  //        -dumpseqs         dump the sequence matches to a file for each significant primary/secondary
  //        -help             brief help message
      StringBuilder args = new StringBuilder();
      if (secondaries instanceof MotifDataSource) {
        addArgs(args, "-uploaded", ((MotifDataSource) secondaries).getName());
      }
      if (margin != null) addArgs(args, "-margin", margin);
      if (dumpseqs) addArgs(args, "-dumpseqs");
      addArgs(args, sequences.getName());
      addArgs(args, primary.getName());
      if (secondaries instanceof MotifDB) {
        for (MotifDBFile file : ((MotifDB) secondaries).getMotifFiles()) {
          addArgs(args, file.getFileName());
        }
      }
      return args.toString();
    }
  
    @Override
    public List<DataSource> files() {
      ArrayList<DataSource> list = new ArrayList<DataSource>();
      if (sequences != null) list.add(sequences);
      if (primary != null) list.add(primary);
      if (secondaries != null && secondaries instanceof MotifDataSource) {
        list.add((MotifDataSource) secondaries);
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
      if (primary != null) {
        if (!primary.getFile().delete()) {
          logger.log(Level.WARNING, "Unable to delete temporary file " +
              primary.getFile());
        }
      }
      if (secondaries != null && secondaries instanceof MotifDataSource) {
        if (!((MotifDataSource) secondaries).getFile().delete()) {
          logger.log(Level.WARNING, "Unable to delete temporary file " +
              ((MotifDataSource) secondaries).getFile());
        }
      }
    }
  }

  public Spamo() {
    super("SPAMO", "SpaMo");
  }

  @Override
  public void init() throws ServletException {
    super.init();
    // load the templates
    tmplMain = cache.loadAndCache("/WEB-INF/templates/spamo.tmpl");
    tmplVerify = cache.loadAndCache("/WEB-INF/templates/spamo_verify.tmpl");
    header = new ComponentHeader(cache, msp.getVersion(), tmplMain.getSubtemplate("header"));
    sequences = new ComponentSequences(context, tmplMain.getSubtemplate("sequences"));
    primary = new ComponentMotifs(context, tmplMain.getSubtemplate("primary"));
    secondaries = new ComponentMotifs(context, tmplMain.getSubtemplate("secondaries"));
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
    main.set("help", new HTMLSub[]{header.getHelp(), primary.getHelp(),
        sequences.getHelp(), jobDetails.getHelp(), advBtn.getHelp(),
        submitReset.getHelp(), footer.getHelp()});
    main.set("header", header.getComponent());
    main.set("sequences", sequences.getComponent());
    main.set("primary", primary.getComponent(request.getParameter("motifs_embed")));
    main.set("secondaries", secondaries.getComponent());
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
    FileCoord.Name primaryName = namer.createName("primary.meme");
    FileCoord.Name secondariesName = namer.createName("secondaries.meme");
    namer.createName("description");
    namer.createName("address_file");
    Data data = new Data();
    // get the job details
    data.email = jobDetails.getEmail(request, feedback);
    data.description = jobDetails.getDescription(request);
    // get the motifs
    data.primary =  (MotifDataSource)primary.getMotifs(primaryName, request, feedback);
    data.secondaries = secondaries.getMotifs(secondariesName, request, feedback);
    // get the sequences
    data.sequences = (SequenceDataSource)sequences.getSequences(sequencesName, request, feedback);
    // get the options
    data.dumpseqs = paramBool(request, "dumpseqs");
    data.margin = null; //TODO
    return data;
  }

}

