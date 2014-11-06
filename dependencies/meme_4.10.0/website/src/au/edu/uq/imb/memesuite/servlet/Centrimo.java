package au.edu.uq.imb.memesuite.servlet;

import au.edu.uq.imb.memesuite.data.*;
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


public class Centrimo extends SubmitJob<Centrimo.Data> {
  private HTMLTemplate tmplMain;
  private HTMLTemplate tmplVerify;
  private ComponentHeader header;
  private ComponentMotifs motifs;
  private ComponentSequences sequences;
  private ComponentSequences comparative;
  private ComponentBfile background;
  private ComponentJobDetails jobDetails;
  private ComponentAdvancedOptions advBtn;
  private ComponentSubmitReset submitReset;
  private ComponentFooter footer;

  private static Logger logger = Logger.getLogger("au.edu.uq.imb.memesuite.web.centrimo");
  
  protected class Data extends SubmitJob.JobData {
    public String email;
    public String description;
    public boolean local;
    public boolean compare;
    public SequenceDataSource sequences;
    public SequenceDataSource comparative;
    public MotifInfo motifs;
    public String strands;
    public double minScore;
    public boolean optScore;
    public Integer maxRegion;
    public double evalueThreshold;
    public boolean storeIds;
    public Background background;

    @Override
    public void outputJson(JsonWr out) throws IOException {
      out.startObject();
      out.property("local", local);
      out.property("compare", compare);
      out.property("sequences", sequences);
      if (comparative != null) out.property("comparative", comparative);
      out.property("motifs", motifs);
      out.property("strands", strands);
      out.property("minScore", minScore);
      out.property("optScore", optScore);
      if (maxRegion != null) out.property("maxRegion", maxRegion);
      out.property("evalueThreshold", evalueThreshold);
      out.property("storeIds", storeIds);
      out.property("background", background);
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
  //    centrimo_webservice [options] <sequences file> <motif databases>
  //
  //      Options:
  //        -local            compute the enrichment of all regions;
  //                           default: central regions only
  //        -score <score>    minimum score counted as hit
  //        -optsc            search for optimized score above the threshold given by
  //                           '-score' argument. Slow computation due to multiple tests
  //        -ethresh <evalue> minimum E-value to report
  //        -maxreg <region>  maximum region size to test
  //        -neg <file>       plot a negative set of sequences against the default set
  //                          and test each window with Fisher's Exact Test
  //        -upmotifs <file>  uploaded motifs
  //        -bfile <file>     background file (0-order)
  //        -norc             don't scan with the reverse complement motif
  //        -sep              scan separately with reverse complement motif;
  //                          (requires --norc)
  //        -flip             allow 'fliping' of sequences causing rc matches to appear
  //                           'reflected' around center
  //        -noseq            don't store sequence ids in the output
  //        -help             brief help message
  
      StringBuilder args = new StringBuilder();
      if (local) addArgs(args, "-local");
      addArgs(args, "-score", minScore);
      if (optScore) addArgs(args, "-optsc");
      addArgs(args, "-ethresh", evalueThreshold);
      if (maxRegion != null) addArgs(args, "-maxreg", maxRegion);
      if (compare) {
        addArgs(args, "-neg", comparative.getName());
      }
      if (motifs instanceof MotifDataSource) {
        addArgs(args, "-upmotifs", ((MotifDataSource) motifs).getName());
      }
      if (background.getSource() == Background.Source.FILE) {
        addArgs(args, "-bfile", background.getBfile().getName());
      }
      if (strands.equals("given")) {
        addArgs(args, "-norc");
      } else if (strands.equals("both_separately")) {
        addArgs(args, "-sep");
	addArgs(args, "-norc");
      } else if (strands.equals("both_flip")) {
        addArgs(args, "-flip");
      }
      if (!storeIds) addArgs(args, "-noseq");
      addArgs(args, sequences.getName());
      if (motifs instanceof MotifDB) {
        for (MotifDBFile file : ((MotifDB) motifs).getMotifFiles()) {
          addArgs(args, file.getFileName());
        }
      }
      return args.toString();
    }
  
    @Override
    public List<DataSource> files() {
      ArrayList<DataSource> list = new ArrayList<DataSource>();
      if (sequences != null) list.add(sequences);
      if (comparative != null) list.add(comparative);
      if (motifs != null && motifs instanceof MotifDataSource)  {
        list.add((MotifDataSource) motifs);
      }
      if (background.getSource() == Background.Source.FILE)
        list.add(background.getBfile());
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
      if (comparative != null) {
        if (!comparative.getFile().delete()) {
          logger.log(Level.WARNING, "Unable to delete temporary file " +
              comparative.getFile());
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

  public Centrimo() {
    super("CENTRIMO", "CentriMo");
  }

  @Override
  public void init() throws ServletException {
    super.init();
    // load the templates
    tmplMain = cache.loadAndCache("/WEB-INF/templates/centrimo.tmpl");
    tmplVerify = cache.loadAndCache("/WEB-INF/templates/centrimo_verify.tmpl");
    header = new ComponentHeader(cache, msp.getVersion(), tmplMain.getSubtemplate("header"));
    motifs = new ComponentMotifs(context, tmplMain.getSubtemplate("motifs"));
    sequences = new ComponentSequences(context, tmplMain.getSubtemplate("sequences"));
    comparative = new ComponentSequences(context, tmplMain.getSubtemplate("comparative"));
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
        sequences.getHelp(), background.getHelp(), jobDetails.getHelp(),
        advBtn.getHelp(), submitReset.getHelp(), footer.getHelp()});
    main.set("header", header.getComponent());
    main.set("sequences", sequences.getComponent());
    main.set("comparative", comparative.getComponent());
    main.set("motifs", motifs.getComponent());
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
    FileCoord.Name comparativeName = namer.createName("comparative_sequences.fa");
    FileCoord.Name motifsName = namer.createName("motifs.meme");
    FileCoord.Name backgroundName = namer.createName("background");
    namer.createName("description");
    namer.createName("address_file");
    Data data = new Data();
    // get the job details
    data.email = jobDetails.getEmail(request, feedback);
    data.description = jobDetails.getDescription(request);
    // get important configuration
    data.local = paramBool(request, "local");
    data.compare = paramBool(request, "compare");
    // get the sequences
    data.sequences = (SequenceDataSource)sequences.getSequences(sequencesName, request, feedback);
    if (data.compare) {
      data.comparative = (SequenceDataSource)comparative.getSequences(comparativeName, request, feedback);
    } else {
      data.comparative = null;
    }
    // get the motifs
    data.motifs =  motifs.getMotifs(motifsName, request, feedback);
    // get advanced options
    data.strands = paramChoice(request, "strands", "both", "given", "both_separately", "both_flip");
    data.minScore = paramNumber(feedback, "minimum score", request, "min_score", null, null, 5.0);
    data.optScore = paramBool(request, "opt_score");
    if (paramBool(request, "use_max_region")) {
      data.maxRegion = paramInteger(feedback, "maximum region size", request, "max_region", 1, null, 200);
    } else {
      data.maxRegion = null;
    }
    data.evalueThreshold = paramNumber(feedback, "evalue threshold", request, "evalue_threshold", 0.0, null, 10.0);
    data.storeIds = paramBool(request, "store_ids");
    data.background = background.getBfile(backgroundName, request, feedback);
    return data;
  }
}

