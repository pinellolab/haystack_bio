package au.edu.uq.imb.memesuite.servlet;

import au.edu.uq.imb.memesuite.data.*;
import au.edu.uq.imb.memesuite.db.SequenceDB;
import au.edu.uq.imb.memesuite.servlet.util.*;
import au.edu.uq.imb.memesuite.template.*;
import au.edu.uq.imb.memesuite.util.FileCoord;
import au.edu.uq.imb.memesuite.util.JsonWr;

import java.io.*;
import java.util.*;
import java.util.logging.Level;
import java.util.logging.Logger;

import javax.activation.DataSource;
import javax.servlet.*;
import javax.servlet.http.*;

import static au.edu.uq.imb.memesuite.servlet.util.WebUtils.escapeOpal;

public class Mast extends SubmitJob<Mast.Data> {
  private HTMLTemplate tmplMain;
  private HTMLTemplate tmplVerify;
  private ComponentHeader header;
  private ComponentMotifs motifs;
  private ComponentSequences sequences;
  private ComponentJobDetails jobDetails;
  private ComponentAdvancedOptions advBtn;
  private ComponentSubmitReset submitReset;
  private ComponentFooter footer;

  private static Logger logger = Logger.getLogger("au.edu.uq.imb.memesuite.web.mast");
  
  protected class Data extends SubmitJob.JobData {
    public String email;
    public String description;
    public MotifDataSource motifs;
    public SequenceInfo sequences;
    public String strands;
    public boolean translateDNA;
    public boolean useSeqComp;
    public boolean scaleM;
    public double seqEvalue;
    public Double motifEvalue;

    @Override
    public void outputJson(JsonWr out) throws IOException {
      out.startObject();
      out.property("motifs", motifs);
      out.property("sequences", sequences);
      out.property("translate_dna", translateDNA);
      out.property("strands", strands);
      out.property("use_seq_comp", useSeqComp);
      out.property("scale_m", scaleM);
      out.property("seq_evalue", seqEvalue);
      out.property("motif_evalue", motifEvalue);
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
  
  //    mast_webservice [options] <motifs file> [<sequence database>]
  //
  //      Options:
  //        -dna              search nucleotide database with protein motifs
  //        -seqp             Scale motif display threshold by sequence length
  //        -comp             Use individual sequence composition in E-value and p-value calculation
  //        -sep              treat the rc strand as a seperate sequence; not compatible with -norc
  //        -norc             don't process the rc strand; not compatible with -sep
  //        -ev <thresh>      display sequences with evalue below this threshold
  //        -mev <thresh>     Ignore motifs with evalue above this threshold
  //        -upload_db <file> uploaded sequence database
  //        -df <name>        Name to show for sequence database; underscores are converted to spaces
  //        -help             brief help message
  
  
      StringBuilder args = new StringBuilder();
      if (translateDNA) addArgs(args, "-dna");
      if (scaleM) addArgs(args, "-seqp");
      if (useSeqComp) addArgs(args, "-comp");
      if (strands.equals("separate")) addArgs(args, "-sep");
      if (strands.equals("ignore")) addArgs(args, "-norc");
      addArgs(args, "-ev", seqEvalue);
      if (motifEvalue != null) addArgs(args, "-mev", motifEvalue);
      addArgs(args, "-nseqs", sequences.getSequenceCount());
      if (sequences instanceof SequenceDB) {
        addArgs(args, "-df", escapeOpal(((SequenceDB) sequences).getSequenceName()));
      } else {
        addArgs(args, "-upload_db", ((SequenceDataSource) sequences).getName());
      }
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

  public Mast() {
    super("MAST", "MAST");
  }

  @Override
  public void init() throws ServletException {
    super.init();
    // load the templates
    tmplMain = cache.loadAndCache("/WEB-INF/templates/mast.tmpl");
    tmplVerify = cache.loadAndCache("/WEB-INF/templates/mast_verify.tmpl");
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
    main.set("motifs", motifs.getComponent(request.getParameter("motifs_embed")));
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
    EnumSet<Alphabet> alpha = null;
    // setup default file names
    FileCoord namer = new FileCoord();
    FileCoord.Name sequencesName = namer.createName("sequences.fa");
    FileCoord.Name motifsName = namer.createName("motifs.meme");
    namer.createName("description");
    namer.createName("address_file");
    Data data = new Data();
    // translate dna
    data.translateDNA = WebUtils.paramBool(request, "translate_dna");
    // get the job details
    data.email = jobDetails.getEmail(request, feedback);
    data.description = jobDetails.getDescription(request);
    // get the motifs
    data.motifs =  (MotifDataSource)motifs.getMotifs(motifsName, request, feedback);
    if (data.motifs != null) {
      if (data.translateDNA) {
        if (data.motifs.getAlphabet() != Alphabet.PROTEIN) {
          feedback.whine("The translate DNA option is set so protein motifs were expected.");
        }
        alpha = EnumSet.of(Alphabet.DNA, Alphabet.RNA);
      } else {
        alpha = EnumSet.of(data.motifs.getAlphabet());
      }
    }
    // get the sequences
    data.sequences = sequences.getSequences(alpha, sequencesName, request, feedback);
    // Options
    // strand handling
    data.strands = WebUtils.paramChoice(request, "strands", "combine", "separate", "ignore");
    // sequence E-value
    data.seqEvalue = WebUtils.paramNumber(feedback, "sequence E-value", request, "seq_evalue", 0.0, null, 10.0);
    // use sequence composition
    data.useSeqComp = WebUtils.paramBool(request, "use_seq_comp");
    // scale motif display threshold
    data.scaleM = WebUtils.paramBool(request, "scale_m");
    // filter motifs by E-value
    if (WebUtils.paramBool(request, "motif_evalue_enable")) {
      data.motifEvalue = WebUtils.paramNumber(feedback, "motif E-value", request, "motif_evalue", 0.0, null, 0.05);
    } else {
      data.motifEvalue = null;
    }
    return data;
  }
}
