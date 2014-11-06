package au.edu.uq.imb.memesuite.servlet;

import javax.servlet.ServletException;
import javax.servlet.http.HttpServlet;
import javax.servlet.http.HttpServletRequest;
import javax.servlet.http.HttpServletResponse;
import java.io.IOException;
import java.io.OutputStream;
import java.util.regex.Pattern;

import static au.edu.uq.imb.memesuite.servlet.util.WebUtils.paramRequire;

/**
 * Allows Javascript to create a downloadable ASCII file by posting it to
 * the server with a file name and type. This is restricted to EPS and txt both
 * of which should be (in theory) harmless. Of course that depends on the EPS
 * reader not having bugs...
 */
public class EchoFile extends HttpServlet {
  private static final Pattern namePattern = Pattern.compile("[a-zA-Z0-9_\\.]{1,50}");
  @Override
  protected void doPost(HttpServletRequest req, HttpServletResponse resp)
      throws ServletException, IOException {
    echoRequest(req, resp);
  }
  @Override
  protected void doGet(HttpServletRequest req, HttpServletResponse resp)
      throws ServletException, IOException {
    echoRequest(req, resp);
  }
  protected void echoRequest(HttpServletRequest req, HttpServletResponse resp)
      throws ServletException, IOException {
    String name = paramRequire(req, "name");
    // check that the file name only uses basic ASCII alphanumeric with underscore and dot.
    if (!namePattern.matcher(name).matches()) {
      throw new ServletException("Parameter name had a value that " +
                "included disallowed characters.");
    }
    String type = paramRequire(req, "mime_type");
    // ensure that the type is valid and force the file extension to match
    if (type.equals("application/postscript")) {
      if (!name.regionMatches(true, name.length() - 4, ".eps", 0, 4)) {
        name += ".eps";
      }
    } else if (type.equals("text/plain")) { // text/plain
      if (!name.regionMatches(true, name.length() - 4, ".txt", 0, 4)) {
        name += ".txt";
      }
    } else {
      throw new ServletException("Parameter mime_type specified an unsupported MIME type.");
    }
    // the only limitation on content is the size which is handled by the max post size
    String content = paramRequire(req, "content");
    // convert content to bytes
    byte[] contentBytes = content.getBytes("UTF-8");
    resp.setContentType(type);
    resp.setCharacterEncoding("UTF-8");
    resp.setHeader("Content-Length", String.valueOf(contentBytes.length));
    resp.setHeader("Content-Disposition", "attachment; filename=\"" + name +"\"");
    OutputStream output = null;
    try {
      output = resp.getOutputStream();
      output.write(contentBytes);
      output.close(); output = null;
    } finally {
      if (output != null) {
        try {
          output.close();
        } catch (IOException e) { /* ignore */ }
      }
    }
  }
}
