package au.edu.uq.imb.memesuite.servlet.util;

import au.edu.uq.imb.memesuite.data.MemeSuiteProperties;
import au.edu.uq.imb.memesuite.data.NamedFileDataSource;
import au.edu.uq.imb.memesuite.util.FileCoord;
import edu.sdsc.nbcr.opal.AppServiceLocator;
import edu.sdsc.nbcr.opal.AppServicePortType;
import org.apache.axis.SimpleTargetedChain;
import org.apache.axis.client.AxisClient;
import org.apache.axis.configuration.SimpleProvider;
import org.globus.axis.gsi.GSIConstants;
import org.globus.axis.transport.HTTPSSender;
import org.globus.axis.util.Util;
import org.globus.gsi.gssapi.auth.IdentityAuthorization;
import org.gridforum.jgss.ExtendedGSSCredential;
import org.gridforum.jgss.ExtendedGSSManager;
import org.ietf.jgss.GSSCredential;
import org.ietf.jgss.GSSException;

import javax.servlet.ServletException;
import javax.servlet.http.HttpServletRequest;
import javax.servlet.http.HttpServletResponse;
import javax.servlet.http.Part;
import javax.xml.rpc.ServiceException;
import javax.xml.rpc.Stub;
import java.io.*;
import java.text.DateFormat;
import java.text.SimpleDateFormat;
import java.util.*;
import java.util.logging.Level;
import java.util.logging.Logger;
import java.util.regex.Matcher;
import java.util.regex.Pattern;


public class WebUtils {
  private static final int DEFAULT_BUFFER_SIZE = 10240; // 10KB.
  private static Logger logger = Logger.getLogger("au.edu.uq.imb.memesuite.web");
  /*
   * Email addresses are annoying complex as the local
   * component can be practically anything. For that reason
   * I've chosen to only check the host part. 
   *
   * Note that this does not support the IP address literal format for the
   * domain but surely no legitimate user would use that?
   *
   * From Wikipedia http://en.wikipedia.org/wiki/Email_address
   * The local part of the email address must be no more than 64 characters.
   * The domain part of the email address must be no more than 253 characters.
   * The total length must be no more than 254 characters.
   *
   * From Wikipedia http://en.wikipedia.org/wiki/Hostname
   * Host names are a series of labels concatenated with dots.
   * Each label must be between 1 and 63 characters long.
   * The entire host name has a maximum of 255 characters.
   * ASCII 'a' through 'z' (case insensitive) and digits '0' through '9' and the
   * hyphen '-' are the only allowed characters.
   * The labels may not begin or end with a hyphen.
   *
   * With the rise of Internationalized domain names we may have to change this
   * http://en.wikipedia.org/wiki/Internationalized_domain_name
   */
  private static Pattern emailSplit = Pattern.compile(
      "^.{1,64}@((?:[A-Za-z0-9](?:[A-Za-z0-9\\-]{0,61}[A-Za-z0-9])?\\.)+" + 
      "[A-Za-z0-9](?:[A-Za-z0-9\\-]{0,61}[A-Za-z0-9])?)$");

  /**
   * Close a Closable and ignore IO exceptions.
   * For use in finally blocks where Exceptions should be avoided.
   */
  public static void closeQuietly(Closeable resource) {
    try {
      if (resource != null) resource.close();
    } catch (IOException e) { 
      // ignore
    }
  }

  public static AppServicePortType getOpal(MemeSuiteProperties msp, String serviceName) throws ServletException{
    AppServicePortType appServicePort;
    // setup the Opal connection objects
    AppServiceLocator asl = new AppServiceLocator();
    if (msp.isHttpsInUse()) {
      SimpleProvider provider = new SimpleProvider();
      SimpleTargetedChain c = new SimpleTargetedChain(new HTTPSSender());
      provider.deployTransport("https", c);
      asl.setEngine(new AxisClient(provider));
      Util.registerTransport();
    }
    try {
      appServicePort = asl.getAppServicePort(msp.getServiceURL(serviceName));
    } catch (ServiceException e) {
      throw new ServletException(e);
    }
    if (msp.isHttpsInUse()) {
      // read credentials for the client
      String proxyPath = System.getProperty("X509_USER_PROXY");
      if (proxyPath == null) {
        throw new ServletException("Required system property X509_USER_PROXY not set");
      }
      RandomAccessFile in = null;
      byte[] data;
      try {
        in = new RandomAccessFile(proxyPath, "r");
        long length = in.length();
        if (length > Integer.MAX_VALUE) throw new IOException("File size >= 2 GB");
        data = new byte[(int)length];
        in.readFully(data);
        in.close();
      } catch (IOException e) {
        throw new ServletException("Failed to read X509_USER_PROXY configuration.", e);
      } finally {
        WebUtils.closeQuietly(in);
      }

      GSSCredential proxy;
      ExtendedGSSManager manager = (ExtendedGSSManager) ExtendedGSSManager.getInstance();
      try {
        proxy = manager.createCredential(data,
            ExtendedGSSCredential.IMPEXP_OPAQUE, GSSCredential.DEFAULT_LIFETIME,
            null, // use default mechanism - GSI
            GSSCredential.INITIATE_AND_ACCEPT);
      } catch (GSSException e) {
        throw new ServletException("Can't create proxy credentials.", e);
      }
      // set the GSI specific properties
      IdentityAuthorization auth = new IdentityAuthorization(msp.getServerDN());
      ((Stub) appServicePort)._setProperty(GSIConstants.GSI_AUTHORIZATION, auth);
      ((Stub) appServicePort)._setProperty(GSIConstants.GSI_CREDENTIALS, proxy);
    }
    return appServicePort;
  }

  /**
   * Returns the filename of the Multipart post part or null if it can't be found.
   * @param part The part from the HTTP servlet request
   * @return The file name if avaliable
   */
  public static String getPartFilename(Part part) {
    for (String cd : part.getHeader("content-disposition").split(";")) {
      if (cd.trim().startsWith("filename")) {
        String filename = cd.substring(cd.indexOf('=') + 1).trim().replace("\"", "");
        return filename.substring(filename.lastIndexOf('/') + 1).substring(filename.lastIndexOf('\\') + 1); // MSIE fix.
      }
    }
    return null;
  }


  public static String escapeHTML(String text) {
    return text.replace("&", "&amp;").replace("<", "&lt;").replace(">", "&gt;").replace("\"", "&quot;").replace("'", "&#39;");
  }

  /**
   * Escape an arbitrary string so it passes through Opal's argument parsing as
   * a single argument and doesn't break anything...
   * This converts the string into a series of UTF-8 bytes then proceeds to apply
   * a modified URL encoding to the bytes. Basically only alphanumeric, dash
   * and dot are accepted and everything else is encoded into hexadecimal using
   * underscore as the escape character. That means space ' ' will become "_20".
   *
   */
  public static String escapeOpal(String text) {
    byte[] utf8text;
    try {
      utf8text = text.getBytes("UTF-8");
    } catch (UnsupportedEncodingException e) {
      throw new Error(e); //should never happen
    }
    StringBuilder builder = new StringBuilder();
    for (int i = 0; i < utf8text.length; i++) {
      int c = utf8text[i] & 0xFF;
      if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '-' || c == '.') {
        builder.append((char)c);
      } else {
        builder.append(String.format("_%02x", c));
      }
    }
    return builder.toString();
  }

  /**
   * Reads the parameter and converts it to a boolean value.
   * @param request The servlet request object
   * @param param The get parameter or form field name in a posted form
   * @return <code>true</code> if the parameter value equals "1"
   */
  public static boolean paramBool(HttpServletRequest request, String param) {
    String boolStr = request.getParameter(param);
    return boolStr != null && boolStr.equals("1");
  }

  /**
   * Returns the parameter if it exists but otherwise throws a exception.
   * @param request The servlet request object
   * @param param The parameter to return
   * @return The value of the parameter
   * @throws ServletException If the parameter was not set
   */
  public static String paramRequire(HttpServletRequest request, String param)
      throws ServletException {
    String data = request.getParameter(param);
    if (data == null) throw new ServletException("Required parameter " + 
        param + " was not set.");
    return data;
  }

  /**
   * Returns the parameter if it exists but otherwise returns the default value.
   * @param request The servlet request object.
   * @param param The parameter to return.
   * @param defaultValue The default value to return when the parameter does not exist.
   * @return The value of the parameter or the default value.
   */
  public static String paramOption(HttpServletRequest request,
      String param, String defaultValue) {
    String data = request.getParameter(param);
    if (data == null) return defaultValue;
    return data;
  }

  /**
   * Returns the mapping of the parameters value if it is allowed but otherwise
   * throws a exception.
   * @param request The servlet request object
   * @param param The parameter to check
   * @param allowedValues The valid parameter values as keys with the value to
   * return as values
   * @return The mapping of the parameter value in the allowed values
   * @throws ServletException If the parameter is not set or it does not
   * map to any of the allowed values.
   */
  public static <V> V paramChoice(HttpServletRequest request, String param,
      Map<String,V> allowedValues) throws ServletException {
    String data = paramRequire(request, param);
    if (!allowedValues.containsKey(data)) {
      throw new ServletException("Parameter " + param + " had a value that " +
          "did not match any of the allowed values.");
    }
    return allowedValues.get(data);
  }

  /**
   * Returns the parameters value if it is allowed but otherwise
   * throws a exception.
   * @param request The servlet request object
   * @param param The parameter to check
   * @param allowedValues The valid parameter values
   * @return The mapping of the parameter value in the allowed values
   * @throws ServletException If the parameter is not set or it does not
   * map to any of the allowed values.
   */
  public static String paramChoice(HttpServletRequest request, String param,
      String ... allowedValues) throws ServletException {
    String data = paramRequire(request, param);
    for (int i = 0; i < allowedValues.length; i++) {
      if (allowedValues[i].equals(data)) return allowedValues[i];
    }
    throw new ServletException("Parameter " + param + " had a value that " +
        "did not match any of the allowed values.");
  }

  /**
   * Returns the email address.
   * If the parameters are not set then it throws a ServletException.
   * Otherwise it attempts to validate the email address by checking:
   *  - If the email address has content
   *  - If the email address matches the confirmation field
   *  - If the email address has a @ symbol followed by a host name
   *  - If the host name is found by a nslookup
   * @return The email address
   */
  public static String paramEmail(FeedbackHandler feedback,
      HttpServletRequest request, String paramEmail)
      throws ServletException {
    String email = paramRequire(request, paramEmail).trim();
    if (email.isEmpty()) {
      feedback.whine("You must include a return email address to receive your results.");
    }
    if (email.length() > 254) {
      feedback.whine("The return email address is longer than the standard allows.");
    }
    Matcher m = emailSplit.matcher(email);
    if (!m.matches() || m.group(1).length() > 253) {
      feedback.whine("The return email address doesn't seem to contain a valid domain (everthing after the @).");
      return null;
    }
    String domain = m.group(1);
    // do a lookup on the host to check it exists
    boolean foundDomain = false;
    Pattern checkDomain = Pattern.compile("^" + domain.replace(".", "\\.") + 
        ".*\\s(mail exchanger|internet address)\\s=");
    ProcessBuilder pb = new ProcessBuilder("nslookup", "-q=mx", domain);
    pb.redirectErrorStream(true);
    try {
      Process ps = pb.start();
      BufferedReader in = null;
      try {
        in = new BufferedReader(new InputStreamReader(ps.getInputStream(), "UTF-8"));
        String line;
        while ((line = in.readLine()) != null) {
          if (checkDomain.matcher(line).find()) {
            foundDomain = true;
          }
        }
        in.close();
      } finally {
        closeQuietly(in);
      }
    } catch (IOException e) {
      throw new ServletException("Unable to lookup email domain", e);
    }
    if (!foundDomain) {
      feedback.whine(
        "There is an error in your return email address:<br>\n" +
        "&nbsp;&nbsp;&nbsp;&nbsp; <span style=\"font-family: monospace;\">"
        + escapeHTML(email) + "</span><br>\n" +
        "It is possible that your email address is correct, in which case\n" +
        "the problem may be that your host is behind a firewall and\n" +
        "is consequently not found by the nslookup routines. Consult with\n" +
        "your systems people to see if you have an nslookup-visible email\n" +
        "address.");
      return null;
    }
    return email;
  }

  /**
   * Returns the description or null.
   * @param request The servlet request object
   * @param param The parameter to read as a description
   * @throws ServletException If the parameter is not set
   * @return The description
   */
  public static String paramDescription(HttpServletRequest request,
      String param) throws ServletException {
    String description;
    description = paramRequire(request, param);
    if (description.matches("^\\s*$")) {
      // only whitespace, no point in including it
      description = null;
    } else {
      // convert to unix end of line
      description = description.replace("\r\n", "\n");
    }
    return description;
  }

  /**
   * Returns the parameter value converted to a number.
   * If the parameter is not set then it throws a ServletException.
   * If the parameter value is empty or just whitespace it returns the default
   * value.
   * If the parameter value is not a number or outside the prescribed bounds it
   * calls whine on the feedback handler with a descriptive error message and
   * returns a default value.
   * @param feedback A handler object for feedback intended for the end-user
   * @param request The servlet request object
   * @param param The parameter to read as a number
   * @param description A description of the parameter that identifies it to
   * the end-user
   * @param minValue The exclusive lower bound for the parameter value or null
   * if the value should not have a minimum
   * @param maxValue The maximum value for the parameter value or null if the
   * value should not have a maximum
   * @param defaultValue The value returned when the parameter can not be
   * converted into a number within the supplied bounds
   * @throws ServletException If the parameter is not set
   * @return The number
   */
  public static Double paramNumber(FeedbackHandler feedback, String description,
      HttpServletRequest request, String param,
      Double minValue, Double maxValue, Double defaultValue)
      throws ServletException {
    String data = paramRequire(request, param).trim();
    if (!data.isEmpty()) {
      try {
        double value = Double.parseDouble(data.toUpperCase());
        if (minValue != null && value <= minValue) {
          feedback.whine("The " + description + " (" + value + ") must be &gt; " + minValue + ".");
        } else if (maxValue != null && value > maxValue) {
          feedback.whine("The " + description + " (" + value + ") must be &le; " + maxValue + ".");
        } else {
          return value;
        }
      } catch (NumberFormatException e) {
        feedback.whine("The " + description + " must be a number.");
      }
    }
    return defaultValue;
  }

  /**
   * Returns the parameter value converted to a number.
   * If the parameter is not set then it throws a ServletException.
   * If the parameter value is empty or just whitespace it returns the default
   * value.
   * If the parameter value is not a number or outside the prescribed bounds it
   * calls whine on the feedback handler with a descriptive error message and
   * returns a default value.
   * @param request The servlet request object
   * @param param The parameter to read as a number
   * @param minValue The exclusive lower bound for the parameter value or null
   * if the value should not have a minimum
   * @param maxValue The maximum value for the parameter value or null if the
   * value should not have a maximum
   * @param defaultValue The value returned when the parameter can not be
   * converted into a number within the supplied bounds
   * @return The number
   */
  public static Double paramOptNumber(
      HttpServletRequest request, String param,
      Double minValue, Double maxValue, Double defaultValue) {
    String data = request.getParameter(param);
    if (data == null) return defaultValue;
    data = data.trim();
    if (!data.isEmpty()) {
      try {
        double value = Double.parseDouble(data);
        if ((minValue == null || value > minValue) && (maxValue == null || value < maxValue)) {
          return value;
        }
      } catch (NumberFormatException e) { /* ignore */ }
    }
    return defaultValue;
  }

  /**
   * Returns the parameter value converted to a integer.
   * If the parameter is not set then it throws a ServletException.
   * If the parameter value is empty or just whitespace it returns the default
   * value.
   * If the parameter value is not a integer or outside the prescribed bounds it
   * calls whine on the feedback handler with a descriptive error message and
   * returns a default value.
   * @param feedback A handler object for feedback intended for the end-user
   * @param request The servlet request object
   * @param param The parameter to read as a number
   * @param description A description of the parameter that identifies it to
   * the end-user
   * @param minValue The minimum value for the parameter value or null if the
   * value should not have a minimum
   * @param maxValue The maximum value for the parameter value or null if the
   * value should not have a maximum
   * @param defaultValue The value returned when the parameter can not be
   * converted into a number within the supplied bounds
   * @throws ServletException If the parameter is not set
   * @return The number
   */
  public static Integer paramInteger(FeedbackHandler feedback, String description,
      HttpServletRequest request, String param,
      Integer minValue, Integer maxValue, Integer defaultValue)
      throws ServletException {
    String data = paramRequire(request, param).trim();
    if (!data.isEmpty()) {
      try {
        if (!data.matches("^\\s*[+-]?\\d+\\s*$")) throw new NumberFormatException("Not an integer");
        int value = Integer.parseInt(data.toUpperCase());
        if (minValue != null && value < minValue) {
          feedback.whine("The " + description + " (" + value + ") must be &ge; " + minValue + ".");
        } else if (maxValue != null && value > maxValue) {
          feedback.whine("The " + description + " (" + value + ") must be &le; " + maxValue + ".");
        } else {
          return value;
        }
      } catch (NumberFormatException e) {
        feedback.whine("The " + description + " must be an integer.");
      }
    }
    return defaultValue;
  }

  /**
   * Returns the parameter value converted to a integer.
   * If the parameter is not set or does not meet the requirements the
   * default value is returned.
   * @param request The servlet request object
   * @param param The parameter to read as a number
   * @param minValue The minimum value for the parameter value or null if the
   * value should not have a minimum
   * @param maxValue The maximum value for the parameter value or null if the
   * value should not have a maximum
   * @param defaultValue The value returned when the parameter can not be
   * converted into a number within the supplied bounds
   * @return The number
   */
  public static Integer paramOptInteger(
      HttpServletRequest request, String param,
      Integer minValue, Integer maxValue, Integer defaultValue) {
    String data = request.getParameter(param);
    if (data == null) return defaultValue;
    data = data.trim();
    if (!data.isEmpty()) {
      try {
        if (data.matches("^\\s*[+-]?\\d+\\s*$")) {
          int value = Integer.parseInt(data.toUpperCase());
          if ((minValue == null || value >= minValue) &&
              (maxValue == null || value <= maxValue)) {
            return value;
          }
        }
      } catch (NumberFormatException e) { /* ignore */ }
    }
    return defaultValue;
  }

  public static NamedFileDataSource paramBg(HttpServletRequest request,
      String partFile, FileCoord.Name name) throws ServletException, IOException {
    return paramFile(request, partFile, name);
  }

  /**
   * Reads a file from the end user saves it locally and creates a datasource.
   * <b>Warning</b> - this will only work if you have setup the web.xml to
   * support multipart-config or used the <code>@MultipartConfig</code>
   * annotation on your servlet!
   *
   * @param request The servlet request object
   * @param partFile A form part which contains a file
   * @param name A name coordinated with all the other file names
   */
  public static NamedFileDataSource paramFile(HttpServletRequest request,
      String partFile, FileCoord.Name name) throws ServletException, IOException {
    InputStream in = null;
    OutputStream out = null;
    File file = null;
    boolean error = true;
    try {
      // get the reader from the file part and use the file name if possible
      Part filePart = request.getPart(partFile);
      if (filePart == null || filePart.getSize() == 0) {
        return null;
      }
      name.setOriginalName(getPartFilename(filePart));
      // copy to a temporary file
      in = filePart.getInputStream();
      file = File.createTempFile("uploaded_", ".tmp");
      file.deleteOnExit();
      out = new FileOutputStream(file);
      byte[] buffer = new byte[10240]; // 10KB
      int len;
      while ((len = in.read(buffer)) != -1) {
          out.write(buffer, 0, len);
      }
      out.close();
      in.close();
      error = false;
    } finally {
      closeQuietly(out);
      closeQuietly(in);
      if (error && file != null) file.delete();
    }
    return new NamedFileDataSource(file, name);
  }

  /**
   * Writes the contents of a file as a response.
   * @param resp the response to write the file to.
   * @param file the file to output.
   * @param name the name of the file.
   * @param type the mime type of the file.
   * @throws IOException when something goes wrong.
   */
  public static void outputFile(HttpServletResponse resp, File file,
      String name, String type) throws IOException {
    resp.reset();
    resp.setBufferSize(DEFAULT_BUFFER_SIZE);
    resp.setContentType(type);
    resp.setHeader("Content-Length", String.valueOf(file.length()));
    resp.setHeader("Content-Disposition", "attachment; filename=\"" + name +"\"");
    // Prepare streams.
    BufferedInputStream input = null;
    BufferedOutputStream output = null;
    try {
      // Open streams.
      input = new BufferedInputStream(new FileInputStream(file), DEFAULT_BUFFER_SIZE);
      output = new BufferedOutputStream(resp.getOutputStream(), DEFAULT_BUFFER_SIZE);

      // Write file contents to response.
      byte[] buffer = new byte[DEFAULT_BUFFER_SIZE];
      int length;
      while ((length = input.read(buffer)) > 0) {
          output.write(buffer, 0, length);
      }
      output.close(); output = null;
      input.close(); input = null;
    } finally {
      closeQuietly(output);
      closeQuietly(input);
    }
  }

  /**
   * Reads all data from a source and writes it to standard output.
   * @param in the source to read; normally the output of a commandline program.
   * @throws IOException when something goes wrong.
   */
  public static void copyToStdOut(InputStream in) throws IOException {
    byte[] buffer = new byte[100];
    int read;
    try {
      while ((read = in.read(buffer)) != -1) {
        System.out.write(buffer, 0, read);
      }
      in.close(); in = null;
    } finally {
      closeQuietly(in);
    }
  }

  public static void copyToStdOutAsync(final InputStream in) {
    Thread thread = new Thread() {
      public void run() {
        try {
          copyToStdOut(in);
        } catch (IOException e) {/* ignore */}
      }
    };
    thread.setDaemon(true);
    thread.start();
  }

  /**
   * Generate the fine-print text for a logo, given a program name and if small
   * sample correction is enabled.
   * @param program the name of the program to include in the fineprint.
   * @param ssc if small sample correction is enabled.
   * @return a fineprint string.
   */
  private static String logoFinePrint(String program, boolean ssc) {
    StringBuilder sb = new StringBuilder();
    if (program != null) sb.append(program);
    else sb.append("CEQLOGO");
    if (ssc) {
      sb.append(" (with SSC) ");
    } else {
      sb.append(" (no SSC) ");
    }
    DateFormat dateFormat = new SimpleDateFormat("dd.MM.yyyy HH:mm");
    sb.append(dateFormat.format(new Date()));
    return sb.toString();
  }

  /**
   * Convert a motif, or a pair of motifs, into a logo and send to the client.
   * @param resp the servlet response.
   * @param msp the MEME suite properties, which includes the bin directory
   *            used to find ceqlogo.
   * @param program the name of the program to put in the fine text.
   * @param motifs the motifs in MEME format.
   * @param id1 the position or id of the top motif in the logo.
   * @param rc1 should the top motif in the logo be reverse complemented.
   * @param label1 what label should be shown above the top motif in the logo.
   * @param id2 the position or id of the bottom motif in the logo (or null if no bottom motif).
   * @param rc2 should the bottom logo in the motif be reverse complemented.
   * @param label2 what label should be shown below the bottom motif in the logo
   * @param shift how far should the top motif be shifted relative to the bottom motif.
   * @param errbars should error bars be displayed.
   * @param ssc should small sample correction be applied?
   * @param png should a png or eps be output.
   * @param width optionally specify the width of the logo.
   * @param height optionally specify the height of the logo.
   * @throws java.io.IOException when a IO problem occurs.
   * @throws ServletException when a program problem occurs.
   */
  public static void sendLogo(
      HttpServletResponse resp, MemeSuiteProperties msp,
      String program, String motifs,
      String id1, boolean rc1, String label1,
      String id2, boolean rc2, String label2,
      int shift, boolean errbars, boolean ssc,
      boolean png, Double width, Double height)
      throws IOException, ServletException {
    File motifFile = null, imgFile = null;
    try {
      // create input and output temporary files
      motifFile = File.createTempFile("motifs_", ".meme");
      motifFile.deleteOnExit();
      imgFile = File.createTempFile("logo_", (png ? ".png" : ".eps"));
      imgFile.deleteOnExit();
      // save the motif text to a temporary file
      BufferedWriter writer = null;
      try {
        writer = new BufferedWriter(new FileWriter(motifFile));
        writer.write(motifs);
        writer.close(); writer = null;
      } finally {
        if (writer != null) { try { writer.close(); } catch (IOException e) { /* ignore */ } }
      }
      List<String> cmd = new ArrayList<String>();
      // get the full path to ceqlogo
      File ceqlogoPath = new File(msp.getBinDir(), "ceqlogo");
      // set program name
      cmd.add(ceqlogoPath.toString());
      // set descriptive fine text
      cmd.add("-d"); cmd.add(logoFinePrint(program, ssc));
      // set the title to the label for the top motif
      if (label1 != null) { cmd.add("-t"); cmd.add(label1); }
      // set the x-axis label to the label for the bottom motif
      if (id2 != null && label2 != null) { cmd.add("-x"); cmd.add(label2); }
      // enable error bars
      if (errbars) cmd.add("-E");
      // enable ssc
      if (ssc) cmd.add("-S");
      // set the motif width
      if (width != null) { cmd.add("-w"); cmd.add(width.toString()); }
      // set the motif height
      if (height != null) { cmd.add("-h"); cmd.add(height.toString()); }
      // set the format
      if (png) { cmd.add("-f"); cmd.add("PNG"); }
      // preferentially select motifs by their position in the file
      cmd.add("-l");
      // set the input file
      cmd.add("-i"); cmd.add(motifFile.toString());
      // load the top motif
      cmd.add("-m"); cmd.add(id1);
      // reverse complement the top motif if requested
      if (rc1) cmd.add("-r");
      // shift the first motif if requested
      if (shift != 0) { cmd.add("-s"); cmd.add(Integer.toString(shift)); }
      // load the bottom motif
      if (id2 != null) {
        cmd.add("-m"); cmd.add(id2);
        // reverse complement the bottom motif if requested
        if (rc2) cmd.add("-r");
      }
      // set the output file
      cmd.add("-o"); cmd.add(imgFile.toString());
      // run ceqlogo
      ProcessBuilder pb = new ProcessBuilder(cmd);
      pb.redirectErrorStream(true);
      Process ceqlogo = pb.start();
      // send standard output to console
      copyToStdOut(ceqlogo.getInputStream());
      // the program should have finished but wait anyway
      try {
        ceqlogo.waitFor();
      } catch (InterruptedException e) {
        throw new ServletException(e);
      }
      if (ceqlogo.exitValue() != 0) {
        throw new ServletException("Ceqlogo returned non-zero exit status");
      }
      outputFile(resp, imgFile, "logo." + (png ? "png" : "eps"),
          (png ? "image/png" : "application/postscript"));
    } finally {
      if (motifFile != null) {
        if (!motifFile.delete()) logger.log(Level.WARNING, "Failed to delete " + motifFile);
      }
      if (imgFile != null) {
        if (!imgFile.delete()) logger.log(Level.WARNING, "Failed to delete " + imgFile);
      }
    }
  }

  public static void debugParameters(String error, HttpServletRequest req, HttpServletResponse resp)
      throws ServletException, IOException {
    String stars = "*******************************************************************************";
    String dashes = "-------------------------------------------------------------------------------";
    resp.setContentType("text/plain");
    PrintWriter out = resp.getWriter();
    out.println(error);
    Enumeration<String> paramNames = req.getParameterNames();
    while (paramNames.hasMoreElements()) {
      String paramName = paramNames.nextElement();
      String[] paramValues = req.getParameterValues(paramName);
      out.println(stars);
      out.println(paramName);
      for (String paramValue : paramValues) {
        out.println(dashes);
        out.println(paramValue);
      }
    }
    out.println(stars);
  }

  public static void sendmail(MemeSuiteProperties msp, String address, String from, String subject, String message) {
    String sendmailPath = msp.getSendmail();
    if (sendmailPath == null || sendmailPath.isEmpty()) return; // sendmail is not available
    File sendmailExe = new File(sendmailPath);
    if (!sendmailExe.exists() || !sendmailExe.canExecute()) {
      logger.log(Level.INFO, "Sending of mail was skipped because the configured sendmail program either does not exist or is not executable");
      return;
    }
    try {
      ProcessBuilder pb = new ProcessBuilder(msp.getSendmail(), "-t");
      pb.redirectErrorStream(true);
      Process sendmail = pb.start();
      copyToStdOutAsync(sendmail.getInputStream());
      Writer out = null;
      try {
        out = new BufferedWriter(new OutputStreamWriter(sendmail.getOutputStream(), "UTF-8"));
        out.write("To: "); out.write(address);
        out.write("\nFrom: "); out.write(from);
        out.write("\nReply-to: "); out.write(from);
        out.write("\nSubject: "); out.write(subject);
        out.write("\nContent-type: text/html\n\n");
        out.write(message);
        out.close();
        out = null;
      } finally {
        if (out != null) {
          try {
            out.close();
          } catch (IOException e) {/* ignore */}
        }
      }
      if (sendmail.waitFor() != 0) {
        logger.log(Level.SEVERE, "Program " + sendmailPath +
            " had non-zero exit value " + sendmail.exitValue() + ".");
      }
    } catch (Exception e) {
      logger.log(Level.SEVERE, "Sending of mail failed", e);
    }
  }
}
