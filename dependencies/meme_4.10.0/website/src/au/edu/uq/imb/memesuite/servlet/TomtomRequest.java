package au.edu.uq.imb.memesuite.servlet;

import au.edu.uq.imb.memesuite.data.MemeSuiteProperties;
import au.edu.uq.imb.memesuite.servlet.util.WebUtils;

import javax.servlet.ServletContext;
import javax.servlet.ServletException;
import javax.servlet.http.HttpServlet;
import javax.servlet.http.HttpServletRequest;
import javax.servlet.http.HttpServletResponse;

import java.io.IOException;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

import static au.edu.uq.imb.memesuite.servlet.ConfigurationLoader.CONFIG_KEY;

/**
 * Handle requests for a 2 motif alignment logo.
 */
public class TomtomRequest extends HttpServlet {
  private MemeSuiteProperties msp;
  @Override
  public void init() throws ServletException {
    super.init();
    // load the common templates
    ServletContext context = this.getServletContext();
    msp = (MemeSuiteProperties)context.getAttribute(CONFIG_KEY);
    if (msp == null) throw new ServletException("Failed to get MEME Suite properties");
  }

  /**
   * Get a number parameter if it has been passed otherwise return null.
   * @param req the servlet request.
   * @param name the parameter name.
   * @return the parameter converted to a number if it is both present and
   * valid, otherwise null.
   */
  protected Double numParam(HttpServletRequest req, String name) {
    String param = req.getParameter(name);
    if (param == null) return null;
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

  /**
   * Require a parameter and check that it matches a pattern
   * @param req the servlet request.
   * @param name the parameter name.
   * @param pattern the pattern the parameter name must match or null if any allowed.
   * @return the parameter.
   * @throws ValidationException
   */
  protected String validParam(HttpServletRequest req, String name, String pattern) throws ValidationException {
    String param;
    param = req.getParameter(name);
    if (param == null) throw new ValidationException("Required parameter " + name + " is not set.");
    if (pattern != null && !param.matches(pattern))
      throw new ValidationException("Parameter " + name + " does not pass validation.");
    return param;
  }

  protected void logoRequest(HttpServletRequest req,
      HttpServletResponse resp) throws ServletException, IOException {
    final String numRe = "[+-]?\\d+\\.?\\d*(?:[eE][+-]?\\d+)?";
    final String bgRe = "^" + numRe + " " + numRe + " " + numRe + " " + numRe + "$";
    try {
      // check that this isn't an unknown request
      if (!"logo".equals(req.getParameter("mode")))
        throw new ValidationException("Unexpected value for parameter mode.");

      // get the version and background
      String version = validParam(req, "version", "^\\d+(?:\\.\\d+(?:\\.\\d+)?)?$");
      String[] bgfq = validParam(req, "background", bgRe).split("\\s");

      // target motif parameters
      String targetId = validParam(req, "target_id", "^\\S+$");
      String targetName = req.getParameter("target_name");
      if (targetName == null) {
        Matcher matcher = Pattern.compile("^t_\\d+_(\\S+)$").matcher(targetId);
        if (matcher.matches()) {
          targetName = matcher.group(1);
        } else {
          targetName = targetId;
        }
      }
      int targetLength = Integer.parseInt(validParam(req, "target_length", "^\\d+$"), 10);
      String targetPspm = validParam(req, "target_pspm", null);
      boolean targetRevComp = validParam(req, "target_rc", "^[01]$").equals("1");

      // query motif parameters
      String queryId = validParam(req, "query_id", "^\\S+$");
      String queryName = req.getParameter("query_name");
      if (queryName == null) {
        Matcher matcher = Pattern.compile("^q_(\\S+)$").matcher(queryId);
        if (matcher.matches()) {
          queryName = matcher.group(1);
        } else {
          queryName = queryId;
        }
      }
      int queryLength = Integer.parseInt(validParam(req, "query_length", "^\\d+$"), 10);
      String queryPspm = validParam(req, "query_pspm", null);
      int queryOffset = Integer.parseInt(validParam(req, "query_offset", "^-?\\d+$"), 10);

      // user selected parameters
      boolean png = validParam(req, "image_type", "^(png|eps)$").equals("png");
      boolean errbars = validParam(req, "error_bars", "^[01]$").equals("1");
      boolean ssc = validParam(req, "small_sample_correction", "^[01]$").equals("1");
      boolean flip = validParam(req, "flip", "^[01]$").equals("1");
      Double width = numParam(req, "image_width");
      Double height = numParam(req, "image_height");

      // calculate derived values
      int shift = (flip ? (queryLength + queryOffset) - targetLength : -queryOffset);
      StringBuilder motifs = new StringBuilder();
      motifs.append("MEME version "); motifs.append(version); motifs.append("\n");
      motifs.append("ALPHABET= ACGT\n");
      motifs.append("strands: + -\n");
      motifs.append("Background letter frequencies (from an unknown source):\n");
      motifs.append(String.format("A %s C %s G %s T %s\n", bgfq[0], bgfq[1], bgfq[2], bgfq[3]));
      motifs.append("MOTIF 1\n");
      motifs.append(targetPspm); motifs.append("\n\n");
      motifs.append("MOTIF 2\n");
      motifs.append(queryPspm);

      // send the logo image
      WebUtils.sendLogo(resp, msp, "TOMTOM", motifs.toString(),
          "1", (flip != targetRevComp), targetName,
          "2", flip, queryName,
          shift, errbars, ssc, png, width, height);

    } catch (ValidationException e) {
      WebUtils.debugParameters(e.getMessage(), req, resp);
    }
  }

  @Override
  protected void doGet(HttpServletRequest req,
      HttpServletResponse resp) throws ServletException, IOException {
    logoRequest(req, resp);
  }

  @Override
  protected void doPost(HttpServletRequest req,
      HttpServletResponse resp) throws ServletException, IOException {
    logoRequest(req, resp);
  }

  private static class ValidationException extends ServletException {
    private ValidationException(String message) { super(message); }
  }
}
