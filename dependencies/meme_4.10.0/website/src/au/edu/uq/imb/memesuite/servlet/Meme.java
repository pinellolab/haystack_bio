package au.edu.uq.imb.memesuite.servlet;

import au.edu.uq.imb.memesuite.data.Alphabet;
import au.edu.uq.imb.memesuite.data.Background;
import au.edu.uq.imb.memesuite.data.NamedFileDataSource;
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

public class Meme extends SubmitJob<Meme.Data> {
  private HTMLTemplate tmplMain;
  private HTMLTemplate tmplVerify;
  private ComponentHeader header;
  private ComponentSequences sequences;
  private ComponentSequences control;
  private ComponentBfile background;
  private ComponentJobDetails jobDetails;
  private ComponentAdvancedOptions advancedOptions;
  private ComponentSubmitReset submitReset;
  private ComponentFooter footer;

  protected class Data extends SubmitJob.JobData {
    public String email;
    public String description;
    public SequenceDataSource posSeq;
    public SequenceDataSource negSeq;
    public Background background;
    public String mode;
    public int nMotifs;
    public int minWidth;
    public int maxWidth;
    public Integer minSites;
    public Integer maxSites;
    public boolean shuffle;
    public boolean palindrome;
    public boolean norc;

    @Override
    public void outputJson(JsonWr out) throws IOException {
      out.startObject();
      out.property("posSeq", posSeq);
      if (negSeq != null) out.property("negSeq", negSeq);
      out.property("background", background);
      out.property("mode", mode);
      out.property("nMotifs", nMotifs);
      out.property("minWidth", minWidth);
      out.property("maxWidth", maxWidth);
      if (minSites != null) out.property("minSites", minSites);
      if (maxSites != null) out.property("maxSites", maxSites);
      out.property("shuffle", shuffle);
      out.property("palindrome", palindrome);
      out.property("norc", norc);
      out.endObject();
    }

    public String email() {
      return email;
    }
  
    public String description() {
      return description;
    }

    @Override
    public String emailTemplate() {
      return tmplVerify.getSubtemplate("message").toString();
    }
  
    public String cmd() {
      StringBuilder args = new StringBuilder();
      addArgs(args, "-alpha", argAlpha(posSeq.guessAlphabet()),
          "-mod", mode, "-nmotifs", nMotifs, "-minw", minWidth,
          "-maxw", maxWidth);
      if (minSites != null) addArgs(args, "-minsites", minSites);
      if (maxSites != null) addArgs(args, "-maxsites", maxSites);
      if (background.getSource() == Background.Source.FILE) {
        addArgs(args, "-bfile", background.getBfile().getName());
      }
      if (negSeq != null) addArgs(args, "-neg", negSeq.getName());
      if (norc) addArgs(args, "-norevcomp");
      if (palindrome) addArgs(args, "-pal");
      if (shuffle) addArgs(args, "-shuffle");
      addArgs(args, posSeq.getName());
      return args.toString();
    }
  
    public List<DataSource> files() {
      List<DataSource> sources = new ArrayList<DataSource>();
      if (posSeq != null) sources.add(posSeq);
      if (negSeq != null) sources.add(negSeq);
      if (background.getSource() == Background.Source.FILE) {
        sources.add(background.getBfile());
      }
      return sources;
    }
  
    public void cleanUp() {
      if (posSeq != null) posSeq.getFile().delete();
      if (negSeq != null) negSeq.getFile().delete();
      if (background.getSource() == Background.Source.FILE) {
        background.getBfile().getFile().delete();
      }
    }
  }

  public Meme() {
    super("MEME", "MEME");
  }

  @Override
  public void init() throws ServletException {
    super.init();
    // load the template
    this.tmplMain = cache.loadAndCache("/WEB-INF/templates/meme.tmpl");
    this.tmplVerify = cache.loadAndCache("/WEB-INF/templates/meme_verify.tmpl");
    header = new ComponentHeader(cache, msp.getVersion(), tmplMain.getSubtemplate("header"));
    sequences = new ComponentSequences(context, tmplMain.getSubtemplate("sequences"));
    control = new ComponentSequences(context, tmplMain.getSubtemplate("control"));
    background = new ComponentBfile(context, tmplMain.getSubtemplate("bfile"));
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
        background.getHelp(), jobDetails.getHelp(), advancedOptions.getHelp(),
        submitReset.getHelp(), footer.getHelp()});
    main.set("header", header.getComponent());
    main.set("sequences", sequences.getComponent());
    main.set("control", control.getComponent());
    main.set("bfile", background.getComponent());
    main.set("job_details", jobDetails.getComponent());
    main.set("advanced_options", advancedOptions.getComponent());
    main.set("submit_reset", submitReset.getComponent());
    main.set("footer", footer.getComponent());
    response.setContentType("text/html");
    main.output(response.getWriter());
  }

  @Override
  protected Data checkParameters(FeedbackHandler feedback,
      HttpServletRequest request) throws IOException, ServletException {
    EnumSet<Alphabet> alph = null;
    // setup default file names
    FileCoord namer = new FileCoord();
    FileCoord.Name posSeqName = namer.createName("sequences.fa");
    FileCoord.Name negSeqName = namer.createName("control_sequences.fa");
    FileCoord.Name bfileName = namer.createName("background.bkg");
    namer.createName("description");
    namer.createName("address_file");
    // create the job data
    Data data = new Data();
    boolean error = true;
    try {
      // get the email
      data.email = jobDetails.getEmail(request, feedback);
      // get the description
      data.description = jobDetails.getDescription(request);
      // get the input sequences
      data.posSeq = (SequenceDataSource)sequences.getSequences(posSeqName, request, feedback);
      // get the discriminative sequences
      if (paramBool(request, "discr")) {
        if (data.posSeq != null) {
          alph = (data.posSeq.guessAlphabet() == Alphabet.PROTEIN ?
              EnumSet.of(Alphabet.PROTEIN) : EnumSet.of(Alphabet.DNA, Alphabet.RNA));
        }
        data.negSeq = (SequenceDataSource)control.getSequences(alph, negSeqName, request, feedback);
      }
      // get the site distribution mode
      data.mode = paramChoice(request, "dist", "zoops", "oops", "anr");
      // get the number of motifs
      data.nMotifs = paramInteger(feedback, "maximum motifs",
          request, "nmotifs", 1, null, 3);
      // get the minimum motif width
      data.minWidth = paramInteger(feedback, "minimum width",
          request, "minw", 2, 300, 6);
      // get the maximum motif width
      data.maxWidth = paramInteger(feedback, "maximum width",
          request, "maxw", 2, 300, 50);
      // check min width is smaller than max width
      if (data.minWidth > data.maxWidth) {
        feedback.whine("The minimum width (" + data.minWidth + ") must be " +
            "less than or equal to the maximum motif width (" + data.maxWidth +
            ").");
      }
      // check for optional site bounds
      if (!data.mode.equals("oops")) {
        // check min sites
        if (paramBool(request, "minsites_enable")) {
          data.minSites = paramInteger(feedback, "minimum sites",
              request, "minsites", 2, 600, 2);
        }
        // check max sites
        if (paramBool(request, "maxsites_enable")) {
          data.maxSites = paramInteger(feedback, "maximum sites",
              request, "maxsites", 2, 600, 600);
        }
        // check min sites is smaller than max sites
        if (data.minSites != null && data.maxSites != null && 
            data.minSites > data.maxSites) {
          feedback.whine("The minimum motif sites (" + data.minSites +
              ") must be less than or equal to the maximum motif sites (" +
              data.maxSites + ").");
        }
      }
      // get the background file
      data.background = background.getBfile(bfileName, request, feedback);
      // get the flags
      data.shuffle = paramBool(request, "shuffle");
      data.palindrome = paramBool(request, "pal");
      data.norc = paramBool(request, "norc");
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
        return "dna";
      case PROTEIN:
        return "protein";
      default:
        throw new Error("Unknown alphabet");
    }
  }
}
