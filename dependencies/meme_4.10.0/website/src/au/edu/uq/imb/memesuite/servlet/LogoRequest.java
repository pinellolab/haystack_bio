package au.edu.uq.imb.memesuite.servlet;

import au.edu.uq.imb.memesuite.data.MemeSuiteProperties;
import au.edu.uq.imb.memesuite.servlet.util.WebUtils;

import javax.servlet.ServletContext;
import javax.servlet.ServletException;
import javax.servlet.http.HttpServlet;
import javax.servlet.http.HttpServletRequest;
import javax.servlet.http.HttpServletResponse;
import java.io.IOException;

import static au.edu.uq.imb.memesuite.servlet.ConfigurationLoader.CONFIG_KEY;

/**
 * Generates a logo or an alignment of 2 logos.
 */
public class LogoRequest extends HttpServlet {
  private MemeSuiteProperties msp;

  @Override
  public void init() throws ServletException {
    ServletContext context = this.getServletContext();
    msp = (MemeSuiteProperties)context.getAttribute(CONFIG_KEY);
    if (msp == null) throw new ServletException("Failed to get MEME Suite properties");
  }

  protected void logoRequest(HttpServletRequest req,
      HttpServletResponse resp) throws ServletException, IOException {
    String program = req.getParameter("program");
    String motifs = WebUtils.paramRequire(req, "motifs");
    String id1 = WebUtils.paramOption(req, "id1", "1");
    boolean rc1 = WebUtils.paramBool(req, "rc1");
    String label1 = req.getParameter("label1");
    String id2 = req.getParameter("id2");
    boolean rc2 = WebUtils.paramBool(req, "rc2");
    String label2 = req.getParameter("label2");
    int shift = WebUtils.paramOptInteger(req, "shift", -100, 100, 0);
    boolean errbars = WebUtils.paramBool(req, "errbars");
    boolean ssc = WebUtils.paramBool(req, "ssc");
    boolean png = WebUtils.paramBool(req, "png");
    Double width = WebUtils.paramOptNumber(req, "width", 0.0, 100.0, null);
    Double height = WebUtils.paramOptNumber(req, "height", 0.0, 100.0, null);
    // send the logo image
    WebUtils.sendLogo(resp, msp, program, motifs,
        id1, rc1, label1, id2, rc2, label2,
        shift, errbars, ssc, png, width, height);
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
}
