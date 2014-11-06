package au.edu.uq.imb.memesuite.servlet;

import au.edu.uq.imb.memesuite.servlet.util.WebUtils;
import au.edu.uq.imb.memesuite.template.HTMLSub;
import au.edu.uq.imb.memesuite.template.HTMLTemplate;
import au.edu.uq.imb.memesuite.template.HTMLTemplateCache;

import javax.servlet.ServletContext;
import javax.servlet.ServletException;
import javax.servlet.http.HttpServlet;
import javax.servlet.http.HttpServletRequest;
import javax.servlet.http.HttpServletResponse;
import java.io.IOException;
import java.io.OutputStream;
import java.util.*;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

import static au.edu.uq.imb.memesuite.servlet.ConfigurationLoader.CACHE_KEY;

/**
 * Replaces the old process_request.cgi script.
 * Only supports the GLAM2 actions as they were the only ones that hadn't been
 * moved to different CGI forms a long time ago.
 */
public class Glam2Request extends HttpServlet {
  private HTMLTemplate tmplRequestRedirect;
  private Map<String,String> translatedParams;

  @Override
  public void init() throws ServletException {
    super.init();
    // load the common templates
    ServletContext context = this.getServletContext();
    HTMLTemplateCache cache = (HTMLTemplateCache)context.getAttribute(CACHE_KEY);
    tmplRequestRedirect = cache.loadAndCache("/WEB-INF/templates/request_redirect.tmpl");
    translatedParams = new TreeMap<String, String>();
    translatedParams.put("-z", "min_seqs");
    translatedParams.put("-w", "initial_cols");
    translatedParams.put("-a", "min_cols");
    translatedParams.put("-b", "max_cols");
    translatedParams.put("-D", "pseudo_del");
    translatedParams.put("-E", "pseudo_nodel");
    translatedParams.put("-I", "pseudo_ins");
    translatedParams.put("-J", "pseudo_noins");
    translatedParams.put("-r", "replicates");
  }

  protected void viewAsText(HttpServletRequest req, HttpServletResponse resp,
      String field) throws ServletException, IOException {
    byte[] alnBytes = WebUtils.paramRequire(req, field).getBytes("UTF-8");
    resp.setContentType("text/plain");
    resp.setCharacterEncoding("UTF-8");
    resp.setHeader("Content-Length", String.valueOf(alnBytes.length));
    OutputStream output = null;
    try {
      output = resp.getOutputStream();
      output.write(alnBytes);
      output.close(); output = null;
    } finally {
      if (output != null) {
        try {
          output.close();
        } catch (IOException e) { /* ignore */ }
      }
    }
  }

  protected void scanAln(HttpServletRequest req, HttpServletResponse resp,
      int index) throws ServletException, IOException {
    HTMLTemplate parameter = tmplRequestRedirect.getSubtemplate("parameter");
    String aln = WebUtils.paramRequire(req, "aln" + index);
    HTMLSub out = tmplRequestRedirect.toSub();
    out.set("tool", "glam2scan");
    List<HTMLSub> parameters = new ArrayList<HTMLSub>();
    parameters.add((parameter.toSub().set("name", "aln_name").
        set("value", "alignment_" + index)));
    parameters.add(parameter.toSub().set("name", "aln_embed").
        set("value", WebUtils.escapeHTML(aln)));
    out.set("parameter", parameters);
    resp.setContentType("text/html");
    out.output(resp.getWriter());
  }

  protected void rerunGlam2(HttpServletRequest req, HttpServletResponse resp)
      throws ServletException, IOException {
    HTMLTemplate parameter = tmplRequestRedirect.getSubtemplate("parameter");
    HTMLSub out = tmplRequestRedirect.toSub();
    out.set("tool", "glam2");
    List<HTMLSub> parameters = new ArrayList<HTMLSub>();
    // set the sequences
    String sequences = WebUtils.paramRequire(req, "data");
    parameters.add(parameter.toSub().set("name", "sequences_embed").
        set("value", WebUtils.escapeHTML(sequences)));

    // set the name of the sequences from the original
    String seqFileName = req.getParameter("seq_file");
    if (seqFileName != null) {
      parameters.add(parameter.toSub().set("name", "sequences_name").
          set("value", WebUtils.escapeHTML(seqFileName)));
    }

    // Get the value for -n .
    // Note that 10,000 is the default compiled into the program
    // but the default value on the website is 2,000 so normally this
    // will have the value of 2,000
    int max_iter = WebUtils.paramOptInteger(req, "-n", 1, 1000000, 10000);
    // double it (constraining to 1,000,000) and set
    parameters.add(parameter.toSub().set("name", "max_iter").
        set("value", Math.min(1000000, max_iter * 2)));

    // check for dna which doesn't have both strands as that means the norc switch
    String alphabet = WebUtils.paramChoice(req, "alphabet", "p", "n");
    if (alphabet.equals("n") && req.getParameter("-2") == null) {
      parameters.add(parameter.toSub().set("name", "norc").set("value", "1"));
    }

    // set any parameters that we have values for
    // translate names to updated ones used by the servlet
    Enumeration<String> paramNames = req.getParameterNames();
    while (paramNames.hasMoreElements()) {
      String paramName = paramNames.nextElement();
      if (translatedParams.containsKey(paramName)) {
        String name = translatedParams.get(paramName);
        String value = req.getParameter(paramName);
        if (value == null) continue;
        parameters.add(parameter.toSub().set("name", name).
            set("value", WebUtils.escapeHTML(value)));
      }
    }

    out.set("parameter", parameters);
    resp.setContentType("text/html");
    out.output(resp.getWriter());

  }

  protected void doAction(HttpServletRequest req,
      HttpServletResponse resp) throws ServletException, IOException {
    Pattern scanAlnRE = Pattern.compile("^Scan alignment (\\d+)");
    Pattern viewAlnOrPSPMRE = Pattern.compile("^View (alignment|PSPM) (\\d+)");
    Matcher match;
    String action = req.getParameter("action");
    if (action == null) {
      WebUtils.debugParameters("No action specified", req, resp);
      return;
    }
    if ((match = viewAlnOrPSPMRE.matcher(action)).matches()) {
      viewAsText(req, resp, (match.group(1).equals("PSPM") ? "pspm" : "aln") + Integer.parseInt(match.group(2), 10));
    } else if ((match = scanAlnRE.matcher(action)).matches()) {
      scanAln(req, resp, Integer.parseInt(match.group(1), 10));
    } else if (action.contains("re-run GLAM2")) {
      rerunGlam2(req, resp);
    } else if (
        action.equals("MAST") ||
        action.matches("^MAST PSSM \\d+") ||
        action.matches("^Scan PSSM \\d+") ||
        action.equals("FIMO") ||
        action.matches("^FIMO PSPM \\d+") ||
        action.equals("GOMO") ||
        action.matches("^GOMO PSPM \\d+") ||
        action.startsWith("BLOCKS") ||
        action.matches("^Submit BLOCK \\d+") ||
        action.matches("^View \\w+ \\d+") ||
        action.matches("^TOMTOM PSPM \\d+") ||
        action.matches("^Compare PSPM \\d+") ||
        action.matches("^COMPARE PSPM \\d+") ||
        action.startsWith("View motif summary") ||
        action.contains("MetaMEME") ||
        action.contains("MEME Man Page") ||
        action.contains("MAST Man Page")
    ) {
      WebUtils.debugParameters("Obsolete action!", req, resp);
    } else  {
      WebUtils.debugParameters("Unknown action!", req, resp);
    }
  }

  @Override
  protected void doGet(HttpServletRequest req,
      HttpServletResponse resp) throws ServletException, IOException {
    doAction(req, resp);
  }

  @Override
  protected void doPost(HttpServletRequest req,
      HttpServletResponse resp) throws ServletException, IOException {
    doAction(req, resp);
  }
}
