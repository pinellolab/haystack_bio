package au.edu.uq.imb.memesuite.servlet;

import java.io.*;
import java.nio.CharBuffer;
import java.util.*;
import java.util.regex.*;

import javax.activation.DataHandler;
import javax.activation.DataSource;
import javax.activation.FileDataSource;
import javax.servlet.*;
import javax.servlet.http.*;

import au.edu.uq.imb.memesuite.data.*;
import au.edu.uq.imb.memesuite.db.GomoDB;
import au.edu.uq.imb.memesuite.db.MotifDB;
import au.edu.uq.imb.memesuite.db.SequenceDB;
import au.edu.uq.imb.memesuite.io.json.JsonHandler;
import au.edu.uq.imb.memesuite.io.json.JsonParser;
import au.edu.uq.imb.memesuite.template.HTMLSub;
import au.edu.uq.imb.memesuite.template.HTMLTemplate;
import au.edu.uq.imb.memesuite.template.HTMLTemplateCache;
import au.edu.uq.imb.memesuite.servlet.util.FeedbackHandler;
import au.edu.uq.imb.memesuite.servlet.util.WebUtils;
import au.edu.uq.imb.memesuite.util.JsonWr;
import edu.sdsc.nbcr.opal.AppServicePortType;
import edu.sdsc.nbcr.opal.InputFileType;
import edu.sdsc.nbcr.opal.JobInputType;
import edu.sdsc.nbcr.opal.JobSubOutputType;
import org.apache.commons.io.IOUtils;

import static au.edu.uq.imb.memesuite.servlet.ConfigurationLoader.CONFIG_KEY;
import static au.edu.uq.imb.memesuite.servlet.ConfigurationLoader.CACHE_KEY;

public abstract class SubmitJob<E extends SubmitJob.JobData> extends HttpServlet {
  private String serviceName;
  private String programName;
  private HTMLTemplate tmplWhine;
  private HTMLTemplate tmplVerify;
  private HTMLTemplate tmplEmail;
  private AppServicePortType appServicePort;
  protected MemeSuiteProperties msp;
  protected HTMLTemplateCache cache;
  protected ServletContext context;
  
  public abstract class JobData implements JsonWr.JsonValue {
    public abstract String email();
    public abstract String description();
    public abstract String emailTemplate();
    public abstract List<DataSource> files();
    public abstract String cmd();
    public abstract void cleanUp();
  }

  public SubmitJob(String serviceName, String programName) {
    this.serviceName = serviceName;
    this.programName = programName;
  }

  /**
   * This pattern checks that a argument should pass through Opal ok.
   * Opal appears to split arguments based on spaces and ignore most
   * standard space escaping techniques like quotes or backslash escaping.
   * Sometimes Opal accepts the existence of brackets but it seems to depend on
   * the job manager and EBI's configuration breaks when you give it brackets.
   * Additionally '%' symbols seem to break DRMAA as used by NBCR so '%' is out also.
   * I recommend using a modified URL Encoding when there's a possibility that
   * the input data could contain spaces or other odd characters.
   */
  protected static Pattern validArg = Pattern.compile("^[^\\s\\(\\)%]*$");

  /**
   * Append one or more arguments to a command string for passing to Opal.
   */
  protected static void addArgs(StringBuilder cmd, Object ... args) {
    for (Object objArg : args) {
      String arg = String.valueOf(objArg);
      if (!validArg.matcher(arg).matches()) {
        throw new IllegalArgumentException("Argument contains characters " +
            "that can't be passed to Opal (like spaces).");
      }
      if (cmd.length() > 0) cmd.append(" ");
      cmd.append(arg);
    }
  }

  protected static void addArgs(List<String> cmd, Object ... args) {
    for (Object objArg : args) {
      String arg = String.valueOf(objArg);
      if (!validArg.matcher(arg).matches()) {
        throw new IllegalArgumentException("Argument contains characters " +
            "that can't be passed to Opal (like spaces).");
      }
      cmd.add(arg);
    }
  }

  @Override
  public void init() throws ServletException {
    // load the common templates
    context = this.getServletContext();
    msp = (MemeSuiteProperties)context.getAttribute(CONFIG_KEY);
    if (msp == null) throw new ServletException("Failed to get MEME Suite properties");
    cache = (HTMLTemplateCache)context.getAttribute(CACHE_KEY);
    this.tmplWhine = cache.loadAndCache("/WEB-INF/templates/whine.tmpl");
    this.tmplVerify = cache.loadAndCache("/WEB-INF/templates/verify.tmpl");
    this.tmplEmail = cache.loadAndCache("/WEB-INF/templates/email.tmpl");
    appServicePort = WebUtils.getOpal(msp, serviceName);
  }

  protected abstract void displayForm(HttpServletRequest request,
      HttpServletResponse response) throws IOException;
  protected abstract E checkParameters(FeedbackHandler feedback,
      HttpServletRequest request) throws  IOException, ServletException;
  public abstract String title();
  public abstract String subtitle();
  public abstract String logoPath();
  public abstract String logoAltText();

  private static class MessageLayoutReader implements JsonHandler {
    enum State {
      START,
      WRAPPER_OBJECT,
      ITEMS_PROPERTY,
      ITEMS_LIST,
      ITEM
    }
    private State state = State.START;
    private Deque<Object> stack = new LinkedList<Object>();
    private String property;
    private List<Map<String,Object>> items;

    public List<Map<String,Object>> getItems() {
      return items;
    }

    @SuppressWarnings("unchecked")
    private void value(Object value) {
      Object top = stack.peek();
      if (top instanceof Map) {
        ((Map<String,Object>) top).put(property, value);
      } else if (top instanceof List) {
        ((List<Object>) top).add(value);
      }
    }

    @SuppressWarnings("unchecked")
    private Map<String,Object> item() {
      return (Map<String,Object>)stack.getFirst();
    }

    @Override
    public void jsonStartData(String name) {
      state = State.START;
      items = new ArrayList<Map<String, Object>>();
    }

    @Override
    public void jsonEndData() {
      if (state != State.START) {
        throw new IllegalStateException("Expected " + State.START +
            " but got " + state + ".");
      }
    }

    @Override
    public void jsonStartObject() {
      switch (state) {
        case START:
          state = State.WRAPPER_OBJECT;
          break;
        case ITEMS_LIST:
          state = State.ITEM;
          stack.clear();
          stack.push(new TreeMap<String, Object>());
          break;
        case ITEM:
          TreeMap<String, Object> value = new TreeMap<String, Object>();
          value(value);
          stack.push(value);
          break;
        case WRAPPER_OBJECT:
        case ITEMS_PROPERTY:
          throw new IllegalStateException("Unexpected state for start object " + state);
      }
    }

    @Override
    public void jsonEndObject() {
      switch (state) {
        case WRAPPER_OBJECT:
          state = State.START;
          break;
        case ITEM:
          if (stack.size() > 1) {
            stack.pop();
          } else {
            items.add(item());
            state = State.ITEMS_LIST;
          }
          break;
        case START:
        case ITEMS_PROPERTY:
        case ITEMS_LIST:
          throw new IllegalStateException("Unexpected state for end object " + state);
      }
    }

    @Override
    public void jsonStartList() {
      switch (state) {
        case ITEMS_PROPERTY:
          state = State.ITEMS_LIST;
          break;
        case ITEM:
          List<Object> value = new ArrayList<Object>();
          value(value);
          stack.push(value);
          break;
        case START:
        case WRAPPER_OBJECT:
        case ITEMS_LIST:
          throw new IllegalStateException("Unexpected state for start list " + state);
      }
    }

    @Override
    public void jsonEndList() {
      switch (state) {
        case ITEMS_LIST:
          state = State.ITEMS_PROPERTY;
          break;
        case ITEM:
          stack.pop();
          break;
        case START:
        case WRAPPER_OBJECT:
        case ITEMS_PROPERTY:
          throw new IllegalStateException("Unexpected state for end list " + state);
      }
    }

    @Override
    public void jsonStartProperty(String name) {
      switch (state) {
        case WRAPPER_OBJECT:
          if (name.equals("items")) {
            state = State.ITEMS_PROPERTY;
          }
          break;
        case ITEM:
          property = name;
          break;
      }
    }

    @Override
    public void jsonEndProperty() {
      switch (state) {
        case ITEMS_PROPERTY:
          state = State.WRAPPER_OBJECT;
          break;
      }
    }

    @Override
    public void jsonValue(Object value) {
      if (state == State.ITEM) {
        value(value);
      }
    }

    @Override
    public void jsonError() { }

  }

  private String describeSequence(SequenceInfo sequence) {
    StringBuilder value = new StringBuilder();
    if (sequence.getSequenceCount() > 1) {
      value.append("A set of ");
      value.append(sequence.getSequenceCount());
    } else {
      value.append("One");
    }
    switch (sequence.guessAlphabet()) {
      case DNA:
        value.append(" DNA");
        break;
      case RNA:
        value.append(" RNA");
        break;
      case PROTEIN:
        value.append(" protein");
        break;
    }
    if (sequence.getSequenceCount() > 1) {
      value.append(" sequences, ");
      if (sequence.getMinLength() == sequence.getMaxLength()) {
        value.append("all ");
        value.append(sequence.getMinLength());
        value.append(" in length");
      } else {
        value.append("between ");
        value.append(sequence.getMinLength());
        value.append(" and ");
        value.append(sequence.getMaxLength());
        value.append(" in length (average length ");
        value.append(String.format("%.1f", sequence.getAverageLength()));
        value.append(")");
      }
    } else {
      value.append(" sequence, with a length of ");
      value.append(sequence.getMinLength());
    }
    if (sequence instanceof SequenceDataSource) {
      SequenceDataSource sequenceFile = (SequenceDataSource)sequence;
      if (sequenceFile.getOriginalName() != null) {
        value.append(", from the file <span class=\"file\">");
        value.append(WebUtils.escapeHTML(sequenceFile.getOriginalName()));
        value.append("</span>");
        if (!sequenceFile.getName().equals(sequenceFile.getOriginalName())) {
          value.append(" which was renamed to <span class=\"file\">");
          value.append(sequenceFile.getName());
          value.append("</span>");
        }
      }
    } else if (sequence instanceof SequenceDB) {
      SequenceDB sequenceDB = (SequenceDB)sequence;
      value.append(", from the database <span class=\"file\">");
      value.append(WebUtils.escapeHTML(sequenceDB.getListingName()));
      value.append("</span> version <span class=\"file\">");
      value.append(WebUtils.escapeHTML(sequenceDB.getVersion()));
      value.append("</span>");
    }
    value.append('.');
    return value.toString();
  }

  private String describeMotif(MotifInfo motif) {
    StringBuilder value = new StringBuilder();
    if (motif.getMotifCount() > 1) {
      value.append("A set of ");
      value.append(motif.getMotifCount());
    } else {
      value.append("One");
    }
    switch (motif.getAlphabet()) {
      case DNA:
        value.append(" DNA");
        break;
      case RNA:
        value.append(" RNA");
        break;
      case PROTEIN:
        value.append(" protein");
        break;
    }
    if (motif.getMotifCount() > 1) {
      value.append(" motifs, ");
      if (motif.getMinCols() == motif.getMaxCols()) {
        value.append("all ");
        value.append(motif.getMinCols());
        value.append(" in length");
      } else {
        value.append("between ");
        value.append(motif.getMinCols());
        value.append(" and ");
        value.append(motif.getMaxCols());
        value.append(" in length (average length ");
        value.append(String.format("%.1f", motif.getAverageCols()));
        value.append(")");
      }
    } else {
      value.append(" sequence, with a length of ");
      value.append(motif.getMinCols());
    }
    if (motif instanceof MotifDataSource) {
      MotifDataSource motifFile = (MotifDataSource)motif;
      if (motifFile.getOriginalName() != null) {
        value.append(", from the file <span class=\"file\">");
        value.append(WebUtils.escapeHTML(motifFile.getOriginalName()));
        value.append("</span>");
        if (!motifFile.getName().equals(motifFile.getOriginalName())) {
          value.append(" which was renamed to <span class=\"file\">");
          value.append(motifFile.getName());
          value.append("</span>");
        }
      }
    } else if (motif instanceof MotifDB) {
      MotifDB motifDB = (MotifDB)motif;
      value.append(", from the database <span class=\"file\">");
      value.append(motifDB.getName());
      value.append("</span>");
    }
    value.append('.');
    return value.toString();
  }

  private String describeGomo(GomoDB gomo) {
    StringBuilder value = new StringBuilder();
    value.append("The database <span class=\"file\">");
    value.append(gomo.getName());
    value.append("</span> described as ");
    value.append(gomo.getDescription());
    return value.toString();
  }

  private String describeBackground(Background background) {
    switch (background.getSource()) {
      case UNIFORM:
        return "A uniform background.";
      case MEME:
        return "The background specified in the motif input.";
      case FILE:
      {
        NamedFileDataSource file = background.getBfile();
        boolean same_name = file.getName().equals(file.getOriginalName());
        String text = same_name ? "The background from !!SAFE-NAME!!." :
            "The background from !!ORIG-NAME!! which was renamed to !!SAFE-NAME!!.";
        return describeFile(file, text);
      }
      case ORDER_0:
      case ORDER_1:
      case ORDER_2:
      case ORDER_3:
      case ORDER_4:
      {
        StringBuilder value = new StringBuilder();
        value.append("A order-");
        value.append(background.getSource().getGeneratedOrder());
        value.append(" background generated from the supplied sequences.");
        return value.toString();
      }
      default:
        return "";
    }
  }

  private String describeFile(NamedFileDataSource file, String text) {
    StringBuilder value = new StringBuilder();
    String originalFile = "<span class=\"file\">" +
        WebUtils.escapeHTML(file.getOriginalName()) + "</span>";
    String safeFile = "<span class=\"file\">" +
        WebUtils.escapeHTML(file.getName()) + "</span>";
    int i = 0;
    while (i < text.length()) {
      int pos1 = text.indexOf("!!SAFE-NAME!!", i);
      int pos2 = text.indexOf("!!ORIG-NAME!!", i);
      if (pos1 == -1 && pos2 == -1) break;
      boolean isOrig = (pos1 == -1 || (pos2 != -1 && pos2 < pos1));
      int pos = (isOrig ? pos2 : pos1);
      if (i < pos) value.append(text.substring(i, pos));
      value.append(isOrig ? originalFile : safeFile);
      i = pos + 13;
    }
    if (i < text.length()) value.append(text.substring(i));
    return value.toString();
  }

  protected HTMLSub makeVerificationEmail(HTMLTemplate template, E data,
      String jobID, Date submitted, Date expires) throws ServletException {
    HTMLSub email = template.toSub();
    email.set("id", jobID);
    email.set("service", this.serviceName);
    email.set("site_url", this.msp.getSiteURL());
    email.set("logo", this.logoPath());
    email.set("alt", this.logoAltText());
    email.set("title", this.title());
    email.set("subtitle", this.subtitle());

    // create a list of table rows
    List<HTMLSub> detailRows = new ArrayList<HTMLSub>();
    // get the details row template
    HTMLTemplate rowTemplate = template.getSubtemplate("details");
    boolean rowEven = false;
    // create rows for the description and timestamps
    detailRows.add(rowTemplate.toSub().set("name", "Submitted").
        set("value", submitted.toString()).set("class", "odd"));
    detailRows.add(rowTemplate.toSub().set("name", "Expires").
            set("value", expires.toString()).set("class", "even"));
    String description = data.description();
    if (description != null) {
      detailRows.add(rowTemplate.toSub().set("name", "Description").
              set("value", WebUtils.escapeHTML(description)).
          set("class", "odd preformatted"));
      rowEven = true;
    }
    // parse the JSON describing the verification message
    JsonParser parser = new JsonParser();
    MessageLayoutReader handler = new MessageLayoutReader();
    parser.setHandler(handler);
    parser.begin("verify");
    parser.process(CharBuffer.wrap(data.emailTemplate()));
    if (parser.hasError()) throw new IllegalArgumentException("JSON table contains error");
    if (!parser.isComplete()) throw new IllegalArgumentException("JSON table is not complete");
    List<Map<String,Object>> items = handler.getItems();
    try {
      for (Map<String,Object> item : items) {
        String type = (String)item.get("type");
        String name = (String)item.get("name");
        String value;
        if (type == null) throw new ServletException("No item type specified!");
        if (type.equals("sequences")) {
          SequenceInfo sequence = (SequenceInfo) data.getClass().
              getDeclaredField((String) item.get("key")).get(data);
          if (sequence == null) continue;
          value = describeSequence(sequence);
        } else if (type.equals("motifs")) {
          MotifInfo motif = (MotifInfo) data.getClass().
              getDeclaredField((String) item.get("key")).get(data);
          if (motif == null) continue;
          value = describeMotif(motif);
        } else if (type.equals("gomo")) {
          GomoDB gomo = (GomoDB) data.getClass().
              getDeclaredField((String) item.get("key")).get(data);
          if (gomo == null) continue;
          value = describeGomo(gomo);
        } else if (type.equals("background")) {
          Background background = (Background) data.getClass().
              getDeclaredField((String) item.get("key")).get(data);
          value = describeBackground(background);
        } else if (type.equals("file")) {
          NamedFileDataSource file = (NamedFileDataSource) data.getClass().
              getDeclaredField((String) item.get("key")).get(data);
          if (file == null) continue;
          boolean same_name = file.getName().equals(file.getOriginalName());
          String text = (String) item.get(same_name ? "normal" : "rename");
          value = describeFile(file, text);
        } else if (type.equals("choice")) {
          String choice = (String) data.getClass().
              getDeclaredField((String) item.get("key")).get(data);
          if (choice == null) continue;
          @SuppressWarnings("unchecked")
          Map<String,Object> choices = (Map<String,Object>)item.get("options");
          if (!choices.containsKey(choice)) continue;
          value = (String) choices.get(choice);
        } else if (type.equals("count")) {
          Number count = (Number) data.getClass().
              getDeclaredField((String) item.get("key")).get(data);
          if (count == null) continue;
          long countVal = count.longValue();
          String text;
          if (countVal == 0l) {
            text = (String) item.get("zero");
          } else if (countVal == 1l) {
            text = (String) item.get("one");
          } else {
            text = (String) item.get("any");
          }
          value = text.replace("!!VALUE!!", Long.toString(countVal));
        } else if (type.equals("number")) {
          Number number = (Number) data.getClass().
                        getDeclaredField((String) item.get("key")).get(data);
          if (number == null) continue;
          double numberVal = number.doubleValue();
          String text;
          if (numberVal == 0.0 && item.containsKey("zero")) {
            text = (String) item.get("zero");
          } else if (numberVal == 1.0 && item.containsKey("one")) {
            text = (String) item.get("one");
          } else if (item.containsKey("any")) {
            text = (String) item.get("any");
          } else {
            throw new IllegalArgumentException("Expected template string \"any\" for number " + name);
          }
          value = text.replace("!!VALUE!!", Double.toString(numberVal));
        } else if (type.equals("range")) {
          Number low = (Number) data.getClass().
              getDeclaredField((String) item.get("keyLow")).get(data);
          Number high = (Number) data.getClass().
              getDeclaredField((String) item.get("keyHigh")).get(data);
          if (low == null && high == null) continue;
          if (low != null && high != null) {
            String text = (String) item.get(
                low.doubleValue() == high.doubleValue() ? "same" : "both");
            value = text.replace("!!LOW!!", low.toString()).replace("!!HIGH!!", high.toString());
          } else if (low != null) {
            String text = (String) item.get("low");
            value = text.replace("!!LOW!!", low.toString());
          } else {
            String text = (String) item.get("high");
            value = text.replace("!!HIGH!!", high.toString());
          }
        } else if (type.equals("flag")) {
          Boolean val = (Boolean) data.getClass().getDeclaredField((String) item.get("key")).get(data);
          if (val) {
            if (!item.containsKey("on")) continue;
            value = (String)item.get("on");
          } else {
            if (!item.containsKey("off")) continue;
            value = (String)item.get("off");
          }
        } else {
          throw new ServletException("Unknown item type: " + type);
        }
        HTMLSub row = rowTemplate.toSub().set("name", name).set("value", value).
            set("class", (rowEven ? "even" : "odd"));
        detailRows.add(row);
        rowEven = !rowEven;
      }
    } catch (NoSuchFieldException e) {
      throw new ServletException(e);
    } catch (IllegalAccessException e) {
      throw new ServletException(e);
    } catch (ClassCastException e) {
      throw new ServletException(e);
    }
    email.set("details", detailRows);
    return email;
  }

  protected void submitOpalJob(E data, HttpServletResponse response)
      throws ServletException, IOException {
    // launch job
    JobInputType in = new JobInputType();
    in.setArgList(data.cmd());

    //this line gives a warning and I have no idea why!
    @SuppressWarnings("unchecked")
    List<DataSource> sources = data.files();
    int fileCount = sources.size() + (data.description() != null ? 1 : 0) + 1;
    InputFileType[] files = new InputFileType[fileCount];
    {
      int i;
      for (i = 0; i < sources.size(); i++) {
        DataSource src = sources.get(i);
        InputFileType file = new InputFileType();
        file.setName(src.getName());
        if (src instanceof FileDataSource) {
          // since this might be a subclass of FileDataSource then get the file and make a new one.
          file.setAttachment(new DataHandler(new FileDataSource(((FileDataSource) src).getFile())));
        } else {
          // Opal currently only supports FileDataSource attachments!
          file.setContents(IOUtils.toByteArray(src.getInputStream()));
        }
        files[i] = file;
      }
      if (data.description() != null) {
        InputFileType file = new InputFileType();
        file.setContents(data.description().getBytes("UTF-8"));
        file.setName("description");
        files[i++] = file;
      }
      {
        InputFileType file = new InputFileType();
        file.setContents(data.email().getBytes("UTF-8"));
        file.setName("address_file");
        files[i] = file;
      }
    }
    in.setInputFile(files);

    // set up a non-blocking call
    JobSubOutputType subOut = this.appServicePort.launchJob(in);

    response.setContentType("text/html");
    HTMLSub verify = this.tmplVerify.toSub();
    verify.set("service", this.serviceName);
    verify.set("id", subOut.getJobID());
    StringWriter json = new StringWriter();
    JsonWr dataOut = new JsonWr(json, 6);
    Date now = new Date();
    Date expiry = new Date(now.getTime() + msp.getExpiryDelay());
    dataOut.start();
    dataOut.property("service", this.serviceName);
    dataOut.property("id", subOut.getJobID());
    dataOut.property("program", this.programName);
    dataOut.property("when", now.getTime());
    dataOut.property("expiry", expiry.getTime());
    if (data.description() != null)
      dataOut.property("description", data.description());
    dataOut.property("inputs", data);
    dataOut.end();
    verify.set("version", this.msp.getVersion());
    verify.set("data", json.toString());
    verify.output(response.getWriter());
    HTMLSub verifyMessage = makeVerificationEmail(this.tmplEmail, data,
        subOut.getJobID(), now, expiry);
    WebUtils.sendmail(msp, data.email(), msp.getSiteContact(),
        this.programName + " Submission Information (job " + subOut.getJobID() + ")",
        verifyMessage.toString());
  }

  @Override
  public void doGet(HttpServletRequest request, HttpServletResponse response)
      throws IOException, ServletException {
    this.displayForm(request, response);
  }

  @Override
  public void doPost(HttpServletRequest request, HttpServletResponse response) 
      throws IOException, ServletException {
    if (request.getParameter("search") != null) {
      SubmitJob.JobFeedback feedback = this.new JobFeedback();
      E data = checkParameters(feedback, request);
      try {
        if (!feedback.hasErrors()) {
          this.submitOpalJob(data, response);
        } else {
          feedback.displayErrors(response);
        }
      } finally {
        data.cleanUp();
      }
    } else {
      this.displayForm(request, response);
    }
  }

  public class JobFeedback implements FeedbackHandler {
    private HTMLTemplate tmplMessage;
    private List<HTMLSub> feedback;

    public JobFeedback() {
      this.tmplMessage = SubmitJob.this.tmplWhine.getSubtemplate("message");
      this.feedback = new ArrayList<HTMLSub>();
    }

    public void whine(String message) {
      feedback.add(this.tmplMessage.toSub().set("text", message));
    }

    public boolean hasErrors() {
      return !feedback.isEmpty(); 
    }

    public void displayErrors(HttpServletResponse response) throws IOException {
      response.setContentType("text/html");
      SubmitJob.this.tmplWhine.toSub().set("message", feedback).output(
          response.getWriter());
    }
  }
}

