package au.edu.uq.imb.memesuite.servlet;

import au.edu.uq.imb.memesuite.data.Alphabet;
import au.edu.uq.imb.memesuite.data.SequenceDataSource;
import au.edu.uq.imb.memesuite.servlet.util.*;
import au.edu.uq.imb.memesuite.template.HTMLSub;
import au.edu.uq.imb.memesuite.template.HTMLTemplate;
import au.edu.uq.imb.memesuite.util.FileCoord;
import au.edu.uq.imb.memesuite.util.JsonWr;

import java.io.*;
import java.util.*;

import javax.activation.DataSource;
import javax.servlet.*;
import javax.servlet.http.*;

import static au.edu.uq.imb.memesuite.servlet.util.WebUtils.*;


public class Glam2 extends SubmitJob<Glam2.Data> {
  private HTMLTemplate tmplMain;
  private HTMLTemplate tmplVerify;
  private ComponentHeader header;
  private ComponentSequences sequences;
  private ComponentJobDetails jobDetails;
  private ComponentAdvancedOptions advancedOptions;
  private ComponentSubmitReset submitReset;
  private ComponentFooter footer;

  
  protected class Data extends SubmitJob.JobData {
    public String email;
    public String description;
    public SequenceDataSource seq;
    public int minAlignedSeqs;
    public int initialAlignedCols;
    public int minAlignedCols;
    public int maxAlignedCols;
    public double deletePseudo;
    public double noDeletePseudo;
    public double insertPseudo;
    public double noInsertPseudo;
    public int replicates;
    public int iterations;
    public boolean norc;
    public boolean shuffle;
    public boolean embed;

    @Override
    public void outputJson(JsonWr out) throws IOException {
      out.startObject();
      out.property("seq", seq);
      out.property("minAlignedSeqs", minAlignedSeqs);
      out.property("initialAlignedCols", initialAlignedCols);
      out.property("minAlignedCols", minAlignedCols);
      out.property("maxAlignedCols", maxAlignedCols);
      out.property("deletePseudo", deletePseudo);
      out.property("noDeletePseudo", noDeletePseudo);
      out.property("insertPseudo", insertPseudo);
      out.property("noInsertPseudo", noInsertPseudo);
      out.property("replicates", replicates);
      out.property("iterations", iterations);
      out.property("norc", norc);
      out.property("shuffle", shuffle);
      out.property("embed", embed);
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
      addArgs(args, "-alpha", argAlpha(seq.guessAlphabet()),
          "-min_seqs", minAlignedSeqs,
          "-min_cols", minAlignedCols,
          "-max_cols", maxAlignedCols,
          "-initial_cols", initialAlignedCols,
          "-runs", replicates,
          "-run_no_impr", iterations,
          "-del_pseudo", deletePseudo,
          "-no_del_pseudo", noDeletePseudo,
          "-ins_pseudo", insertPseudo,
          "-no_ins_pseudo", noInsertPseudo);
      if (!norc && seq.guessAlphabet() != Alphabet.PROTEIN) {
        addArgs(args, "-rev_comp");
      }
      if (embed) addArgs(args, "-embed");
      addArgs(args, seq.getName());
      return args.toString();
    }

    @Override
    public List<DataSource> files() {
      List<DataSource> sources = new ArrayList<DataSource>();
      if (seq != null) sources.add(seq);
      return sources;
    }

    @Override
    public void cleanUp() {
      if (seq != null) seq.getFile().delete();
    }
  }

  public Glam2() {
    super("GLAM2", "GLAM2");
  }

  @Override
  public void init() throws ServletException {
    super.init();
    // load the template
    this.tmplMain = cache.loadAndCache("/WEB-INF/templates/glam2.tmpl");
    this.tmplVerify = cache.loadAndCache("/WEB-INF/templates/glam2_verify.tmpl");
    header = new ComponentHeader(cache, msp.getVersion(), tmplMain.getSubtemplate("header"));
    sequences = new ComponentSequences(context, tmplMain.getSubtemplate("sequences"));
    jobDetails = new ComponentJobDetails(cache);
    advancedOptions = new ComponentAdvancedOptions(cache);
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
    HTMLSub main = this.tmplMain.toSub();
    main.set("help", new HTMLSub[]{header.getHelp(), sequences.getHelp(),
        jobDetails.getHelp(), advancedOptions.getHelp(), submitReset.getHelp(),
        footer.getHelp()});
    main.set("header", header.getComponent());
    main.set("sequences", sequences.getComponent(
        request.getParameter("sequences_embed"),
        request.getParameter("sequences_name")));
    main.set("job_details", jobDetails.getComponent());
    main.set("advanced_options", advancedOptions.getComponent());
    main.set("submit_reset", submitReset.getComponent());
    main.set("footer", footer.getComponent());
    // set values from a GLAM2 rerun action
    main.set("min_seqs", WebUtils.paramOptInteger(request, "min_seqs", 2, null, 2));
    main.set("initial_cols", WebUtils.paramOptInteger(request, "initial_cols", 2, 300, 20));
    main.set("min_cols", WebUtils.paramOptInteger(request, "min_cols", 2, 300, 2));
    main.set("max_cols", WebUtils.paramOptInteger(request, "max_cols", 2, 300, 50));
    main.set("pseudo_del", WebUtils.paramOptNumber(request, "pseudo_del", 0.0, null, 0.1));
    main.set("pseudo_nodel", WebUtils.paramOptNumber(request, "pseudo_nodel", 0.0, null, 2.0));
    main.set("pseudo_ins", WebUtils.paramOptNumber(request, "pseudo_ins", 0.0, null, 0.02));
    main.set("pseudo_noins", WebUtils.paramOptNumber(request, "pseudo_noins", 0.0, null, 1.0));
    main.set("replicates", WebUtils.paramOptInteger(request, "replicates", 1, 100, 10));
    main.set("max_iter", WebUtils.paramOptInteger(request, "max_iter", 1, 1000000, 2000));
    if (WebUtils.paramBool(request, "norc")) main.set("norc_checked", "checked");
    // output
    response.setContentType("text/html");
    main.output(response.getWriter());
  }

  @Override
  protected Data checkParameters(FeedbackHandler feedback,
      HttpServletRequest request) throws IOException, ServletException {
    // setup default file names
    FileCoord namer = new FileCoord();
    FileCoord.Name posSeqName = namer.createName("sequences.fa");
    namer.createName("description");
    namer.createName("address_file");
    // create the job data
    Data data =  new Data();
    boolean error = true;
    try {
      // get the email
      data.email = jobDetails.getEmail(request, feedback);
      // get the description
      data.description = jobDetails.getDescription(request);
      // get the positive sequences
      data.seq = (SequenceDataSource)sequences.getSequences(posSeqName, request, feedback);
      // get the minimum aligned sequences
      data.minAlignedSeqs = paramInteger(feedback, "minimum aligned sequences",
          request, "min_seqs", 2, null, 2);
      // get the initial number of aligned columns
      data.initialAlignedCols = paramInteger(feedback, "initial aligned columns",
          request, "initial_cols", 2, 300, 0);
      // get the minimum number of aligned columns
      data.minAlignedCols = paramInteger(feedback, "minimum aligned columns",
          request, "min_cols", 2, 300, 0);
      // get the maximum number of aligned columns
      data.maxAlignedCols = paramInteger(feedback, "maximum aligned columns",
          request, "max_cols", 2, 300, 0);
      // check min is smaller than max
      if (data.minAlignedCols != 0 && data.maxAlignedCols != 0 && data.minAlignedCols > data.maxAlignedCols) {
        feedback.whine("The minimum number of aligned columns must be &le; " + 
            "the maximum number of aligned columns.");
      }
      // check initial is in range
      if (data.initialAlignedCols != 0) {
        if (data.minAlignedCols != 0 && data.minAlignedCols > data.initialAlignedCols) {
          feedback.whine("The initial number of aligned columns must be &ge; " +
              "the minimum number of aligned columns.");
        }
        if (data.maxAlignedCols != 0 && data.maxAlignedCols < data.initialAlignedCols) {
          feedback.whine("The initial number of aligned columns must be &le; " +
              "the maximum number of aligned columns.");
        }
      }
      // get the delete pseudocount
      data.deletePseudo = paramNumber(feedback, "pseudocount for delete",
          request, "pseudo_del", 0.0, null, 0.1);
      // get the no-delete pseudocount
      data.noDeletePseudo = paramNumber(feedback, "pseudocount for no-delete",
          request, "pseudo_nodel", 0.0, null, 2.0);
      // get the insert pseudocount
      data.insertPseudo = paramNumber(feedback, "pseudocount for insert",
          request, "pseudo_ins", 0.0, null, 0.02);
      // get the no-insert pseudocount
      data.noInsertPseudo = paramNumber(feedback, "pseudocount for no-insert",
          request, "pseudo_noins", 0.0, null, 1.0);
      // get the number of replicates
      data.replicates = paramInteger(feedback, "number of alignment replicates",
          request, "replicates", 1, 100, 10);
      // get the number of iterations without improvement
      data.iterations = paramInteger(feedback, "number of iterations without improvement",
          request, "max_iter", 1, 1000000, 2000);
      // get the flags
      data.norc = paramBool(request, "norc");
      data.shuffle = paramBool(request, "shuffle");
      data.embed = paramBool(request, "embed");
      error = false;
    } finally {
      if (error) data.cleanUp();
    }
    return data;
  }

  private static String argAlpha(Alphabet alph) {
    if (alph == null) throw new IllegalArgumentException("Alphabet should not be null");
    switch (alph) {
      case DNA:
      case RNA:
        return "DNA";
      case PROTEIN:
        return "PROTEIN";
      default:
        throw new Error("Unknown alphabet");
    }
  }
}

