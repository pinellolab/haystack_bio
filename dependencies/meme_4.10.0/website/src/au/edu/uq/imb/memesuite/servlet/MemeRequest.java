package au.edu.uq.imb.memesuite.servlet;

import au.edu.uq.imb.memesuite.data.MemeSuiteProperties;
import au.edu.uq.imb.memesuite.servlet.util.WebUtils;
import au.edu.uq.imb.memesuite.template.HTMLSub;
import au.edu.uq.imb.memesuite.template.HTMLTemplate;
import au.edu.uq.imb.memesuite.template.HTMLTemplateCache;

import javax.servlet.ServletContext;
import javax.servlet.ServletException;
import javax.servlet.http.HttpServlet;
import javax.servlet.http.HttpServletRequest;
import javax.servlet.http.HttpServletResponse;
import java.io.*;
import java.util.*;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

import static au.edu.uq.imb.memesuite.servlet.ConfigurationLoader.CACHE_KEY;
import static au.edu.uq.imb.memesuite.servlet.ConfigurationLoader.CONFIG_KEY;


/**
 * Implementation of old meme_request.cgi script to maintain backwards
 * compatibility.
 */
public class MemeRequest extends HttpServlet {
  private MemeSuiteProperties msp;
  private HTMLTemplate tmplRequestRedirect;
  @Override
  public void init() throws ServletException {
    super.init();
    // load the common templates
    ServletContext context = this.getServletContext();
    msp = (MemeSuiteProperties)context.getAttribute(CONFIG_KEY);
    if (msp == null) throw new ServletException("Failed to get MEME Suite properties");
    HTMLTemplateCache cache = (HTMLTemplateCache)context.getAttribute(CACHE_KEY);
    tmplRequestRedirect = cache.loadAndCache("/WEB-INF/templates/request_redirect.tmpl");
  }

  protected void appendMotif(HttpServletRequest req, int motifIndex, StringBuilder builder) {
    String name = req.getParameter("motifname" + motifIndex);
    String pspm = req.getParameter("pspm" + motifIndex);
    String pssm = req.getParameter("pssm" + motifIndex);
    builder.append("MOTIF ");
    if (name != null) { // MEME does not specify the name
      builder.append(name);
      builder.append(" DREME\n");
    } else { // So this must be MEME
      builder.append(motifIndex);
      builder.append(" MEME\n");
    }
    if (pspm != null) {
      builder.append(pspm);
      builder.append("\n");
    }
    if (pssm != null) {
      builder.append(pssm);
      builder.append("\n");
    }
  }

  protected String generateMotifs(HttpServletRequest req, String motif) throws ServletException, IOException {
    String alphabet = WebUtils.paramChoice(req, "alphabet", "ACGT", "ACDEFGHIKLMNPQRSTVWY");
    String strands = WebUtils.paramChoice(req, "strands", "+ -", "+", "both", "single", "none");
    String source = req.getParameter("bgsrc");
    String bg = WebUtils.paramRequire(req, "bgfreq");
    StringBuilder builder = new StringBuilder();
    builder.append("\nMEME version 4\n\n");
    builder.append("ALPHABET= ");
    builder.append(alphabet);
    builder.append("\n\n");
    if (alphabet.equals("ACGT")) {
      if (strands.equals("both") || strands.equals("+ -")) {
        builder.append("strands: + -\n\n");
      } else {
        builder.append("strands: +\n\n");
      }
    }
    builder.append("Background letter frequencies (from ");
    if (source != null) builder.append(source);
    builder.append("):\n");
    builder.append(bg);
    builder.append("\n\n");
    if (motif.equals("all")) {
      int nmotifs;
      try {
        nmotifs = Integer.parseInt(req.getParameter("nmotifs"), 10);
      } catch (NumberFormatException e) {
        throw new ServletException("Parameter nmotifs was not integer", e);
      } catch (NullPointerException e) {
        throw new ServletException("Parameter nmotifs was not set", e);
      }
      for (int i = 1; i <= nmotifs; i++) {
        appendMotif(req, i, builder);
      }
    } else {
      int motifIndex;
      try {
        motifIndex = Integer.parseInt(motif);
      } catch (NumberFormatException e) {
        throw new ServletException("Selected motif index was not an integer.", e);
      }
      appendMotif(req, motifIndex, builder);
    }
    return builder.toString();
  }

  protected void sendRequestRedirect(HttpServletResponse resp, String tool, String motifs) throws IOException {
    HTMLTemplate parameter = tmplRequestRedirect.getSubtemplate("parameter");
    HTMLSub out = tmplRequestRedirect.toSub();
    out.set("tool", tool);
    List<HTMLSub> parameters = new ArrayList<HTMLSub>();
    parameters.add(parameter.toSub().set("name", "motifs_embed").
        set("value", WebUtils.escapeHTML(motifs)));
    out.set("parameter", parameters);
    resp.setContentType("text/html");
    out.output(resp.getWriter());
  }

  /**
   * Get a string parameter if it has been passed otherwise return a default value.
   * @param req the servlet request.
   * @param name the parameter name.
   * @param whichMotif the motif position or null/empty string if unneeded.
   * @param defaultValue the default value to return when missing.
   * @return the parameter or the default value when it is missing.
   */
  protected String strParam(HttpServletRequest req, String name,
      String whichMotif, String defaultValue) {
    String param = null;
    if (whichMotif != null && !whichMotif.isEmpty()) {
      param = req.getParameter(name + "_" + whichMotif);
    }
    if (param == null) param = req.getParameter(name);
    if (param == null) param = defaultValue;
    return param;
  }

  /**
   * Get a boolean parameter if it has been passed otherwise return a default value.
   * @param req the servlet request.
   * @param name the parameter name.
   * @param whichMotif the motif position or null/empty string if unneeded.
   * @param defaultValue the default value to return when missing.
   * @return the parameter converted to a boolean if it is present,
   * otherwise the default value.
   */
  protected boolean boolParam(HttpServletRequest req, String name,
      String whichMotif, boolean defaultValue) {
    return Boolean.parseBoolean(strParam(req, name, whichMotif, Boolean.toString(defaultValue)));
  }

  /**
   * Get a number parameter if it has been passed otherwise return null.
   * @param req the servlet request.
   * @param name the parameter name.
   * @param whichMotif the motif position or null/empty string if unneeded.
   * @return the parameter converted to a number if it is both present and
   * valid, otherwise null.
   */
  protected Double numParam(HttpServletRequest req, String name, String whichMotif) {
    String param = strParam(req, name, whichMotif, "");
    Pattern numStrPat =  Pattern.compile("^\\s*(\\d+(?:\\.\\d+)?)\\s*$");
    Matcher numStrMat = numStrPat.matcher(param);
    if (numStrMat.matches()) {
      try {
        return Double.valueOf(numStrMat.group(1));
      } catch (NumberFormatException e) {
        return null;
      }
    } else {
      return null;
    }
  }

  protected void dispatchRequest(HttpServletRequest req, HttpServletResponse resp) throws ServletException, IOException {
    String program = req.getParameter("program");
    String whichMotif = req.getParameter("motif");
    if (program == null || whichMotif == null) {
      // search for the program name and action from the name of the submit button
      // as in old MEME output.
      Pattern buttonNameRe = Pattern.compile("^do_(MAST|FIMO|GOMO|BLOCKS|TOMTOM|LOGO)_(all|\\d+)$");
      Enumeration<String> paramNames = req.getParameterNames();
      while (paramNames.hasMoreElements()) {
        String paramName = paramNames.nextElement();
        Matcher matcher = buttonNameRe.matcher(paramName);
        if (matcher.matches()) {
          program = matcher.group(1);
          whichMotif = matcher.group(2);
          break;
        }
      }
      if (program == null || whichMotif == null) {
        WebUtils.debugParameters("Unable to determine program and motif selection", req, resp);
        return;
      }
    }
    String motifs =  generateMotifs(req, whichMotif);
    if (program.equals("MAST")) {
      sendRequestRedirect(resp, "mast", motifs);
    } else if (program.equals("FIMO")) {
      sendRequestRedirect(resp, "fimo", motifs);
    } else if (program.equals("GOMO")) {
      sendRequestRedirect(resp, "gomo", motifs);
    } else if (program.equals("TOMTOM")) {
      sendRequestRedirect(resp, "tomtom", motifs);
    } else if (program.equals("SPAMO")) {
      sendRequestRedirect(resp, "spamo", motifs);
    } else if (program.equals("BLOCKS")) {
      WebUtils.debugParameters("Submitting to Blocks is not implemented!", req, resp);
    } else if (program.equals("LOGO")) {
      boolean ssc = boolParam(req, "logossc", whichMotif, false);
      boolean png = strParam(req, "logoformat", whichMotif, "png").equals("png");
      boolean rc = boolParam(req, "logorc", whichMotif, false);
      Double width = numParam(req, "logowidth", whichMotif);
      Double height = numParam(req, "logoheight", whichMotif);
      WebUtils.sendLogo(resp, msp, "MEME", motifs,
          "1", rc, null,
          null, false, null,
          0, ssc, ssc, png, width, height);
    } else {
      WebUtils.debugParameters("Program not recognised", req, resp);
    }
  }

  @Override
  protected void doPost(HttpServletRequest req,
      HttpServletResponse resp) throws ServletException, IOException {
    dispatchRequest(req, resp);
  }

  @Override
  protected void doGet(HttpServletRequest req,
      HttpServletResponse resp) throws ServletException, IOException {
    dispatchRequest(req, resp);
  }
}
