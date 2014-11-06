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

import javax.activation.DataSource;
import javax.servlet.*;
import javax.servlet.http.*;


public class Glam2scan extends SubmitJob<Glam2scan.Data> {
  private HTMLTemplate tmplMain;
  private HTMLTemplate tmplVerify;
  private ComponentHeader header;
  private ComponentGlam2Motifs glam2Motifs;
  private ComponentSequences sequences;
  private ComponentJobDetails jobDetails;
  private ComponentAdvancedOptions advBtn;
  private ComponentSubmitReset submitReset;
  private ComponentFooter footer;
  
  protected class Data extends SubmitJob.JobData {
    public String email;
    public String description;
    public MotifDataSource motifs;
    public SequenceInfo sequences;
    public int alignments;
    public boolean norc;

    @Override
    public void outputJson(JsonWr out) throws IOException {
      out.startObject();
      out.property("motifs", motifs);
      out.property("sequences", sequences);
      out.property("alignments", alignments);
      out.property("norc", norc);
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
      StringBuilder args = new StringBuilder();
      addArgs(args, "-alpha", motifs.getAlphabet().name());
      addArgs(args, "-aligns", alignments);
      if (sequences instanceof SequenceDataSource) {
        addArgs(args, "-up_seqs", ((SequenceDataSource) sequences).getName());
      }
      if (motifs.getAlphabet() != Alphabet.PROTEIN && !norc) {
        addArgs(args, "-revcomp");
      }
      addArgs(args, motifs.getName());
      if (sequences instanceof SequenceDB) {
        addArgs(args, ((SequenceDB) sequences).getSequenceName());
      }
      return args.toString();
    }
  
    @Override
    public List<DataSource> files() {
      List<DataSource> sources = new ArrayList<DataSource>();
      if (motifs != null) sources.add(motifs);
      if (sequences != null && sequences instanceof SequenceDataSource) {
        sources.add((SequenceDataSource)sequences);
      }
      return sources;
    }
  
    @Override
    public void cleanUp() {
      if (motifs != null) motifs.getFile().delete();
      if (sequences != null && sequences instanceof SequenceDataSource) {
        ((SequenceDataSource) sequences).getFile().delete();
      }
    }
  }

  public Glam2scan() {
    super("GLAM2SCAN", "GLAM2Scan");
  }

  @Override
  public void init() throws ServletException {
    super.init();
    // load the template
    this.tmplMain = cache.loadAndCache("/WEB-INF/templates/glam2scan.tmpl");
    this.tmplVerify = cache.loadAndCache("/WEB-INF/templates/glam2scan_verify.tmpl");
    header = new ComponentHeader(cache, msp.getVersion(), tmplMain.getSubtemplate("header"));
    glam2Motifs = new ComponentGlam2Motifs(cache, tmplMain.getSubtemplate("motifs"));
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
    return msp.getSiteURL() +  tmplVerify.getSubtemplate("logo").toString();
  }

  @Override
  public String logoAltText() {
    return tmplVerify.getSubtemplate("alt").toString();
  }

  @Override
  protected void displayForm(HttpServletRequest request, HttpServletResponse response) throws IOException {
    HTMLSub main = tmplMain.toSub();
    main.set("help", new HTMLSub[]{header.getHelp(), sequences.getHelp(),
        jobDetails.getHelp(), advBtn.getHelp(), submitReset.getHelp(),
        footer.getHelp()});
    main.set("header", header.getComponent());
    main.set("motifs", glam2Motifs.getComponent(
        request.getParameter("aln_embed"),
        request.getParameter("aln_name")));
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
    FileCoord namer = new FileCoord();
    FileCoord.Name glam2MotifsName = namer.createName("motifs.glam2");
    FileCoord.Name sequencesName = namer.createName("sequences.fa");
    namer.createName("description");
    Data data = new Data();
    data.email = jobDetails.getEmail(request, feedback);
    data.description = jobDetails.getDescription(request);
    data.motifs = glam2Motifs.getGlam2Motifs(glam2MotifsName, request, feedback);
    if (data.motifs != null) alpha = EnumSet.of(data.motifs.getAlphabet());
    data.sequences = sequences.getSequences(alpha, sequencesName, request, feedback);
    data.alignments = WebUtils.paramInteger(feedback, "number of alignments to report",
        request, "alignments", 1, 200, 25);
    data.norc = WebUtils.paramBool(request, "norc");
    return data;
  }
}

