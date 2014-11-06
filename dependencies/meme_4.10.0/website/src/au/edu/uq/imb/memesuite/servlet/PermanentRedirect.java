package au.edu.uq.imb.memesuite.servlet;

import javax.servlet.ServletException;
import javax.servlet.http.HttpServlet;
import javax.servlet.http.HttpServletRequest;
import javax.servlet.http.HttpServletResponse;
import java.io.IOException;
import java.util.Enumeration;

/**
 * Redirects a page.
 */
public class PermanentRedirect extends HttpServlet {
  @Override
  protected void doGet(HttpServletRequest req,
      HttpServletResponse resp) throws ServletException, IOException {
    String redirectURL = getInitParameter("RedirectURL");
    if (redirectURL == null) {
      throw new ServletException("Missing init parameter RedirectURL!");
    }
    resp.setStatus(HttpServletResponse.SC_MOVED_PERMANENTLY);
    resp.setHeader("Location", req.getContextPath() + resp.encodeRedirectURL(redirectURL));
    resp.flushBuffer();
  }
}
