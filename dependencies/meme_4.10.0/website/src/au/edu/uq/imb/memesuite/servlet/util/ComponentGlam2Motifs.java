package au.edu.uq.imb.memesuite.servlet.util;

import au.edu.uq.imb.memesuite.data.Alphabet;
import au.edu.uq.imb.memesuite.data.MotifDataSource;
import au.edu.uq.imb.memesuite.data.MotifStats;
import au.edu.uq.imb.memesuite.template.HTMLSub;
import au.edu.uq.imb.memesuite.template.HTMLTemplate;
import au.edu.uq.imb.memesuite.template.HTMLTemplateCache;
import au.edu.uq.imb.memesuite.util.FileCoord;
import au.edu.uq.imb.memesuite.util.JsonWr;

import javax.servlet.ServletException;
import javax.servlet.http.HttpServletRequest;
import javax.servlet.http.Part;
import java.io.*;
import java.util.EnumSet;
import java.util.logging.Level;
import java.util.logging.Logger;

import static au.edu.uq.imb.memesuite.servlet.util.WebUtils.closeQuietly;
import static au.edu.uq.imb.memesuite.servlet.util.WebUtils.getPartFilename;
import static au.edu.uq.imb.memesuite.servlet.util.WebUtils.paramRequire;

/**
 * Component to allow input of GLAM2 motifs.
 */
public class ComponentGlam2Motifs extends PageComponent {
  private HTMLTemplate tmplGlam2Motifs;
  private String prefix;
  private String registerFn;
  private String fieldName;
  private HTMLTemplate title;
  private HTMLTemplate subtitle;
  private DefaultOption defaultOption;
  private EnumSet<Alphabet> alphabets;

  private static Logger logger = Logger.getLogger("au.edu.uq.imb.memesuite.component.glam2motif");

  public static enum DefaultOption {
    FILE,
    EMBED
  }

  public ComponentGlam2Motifs(HTMLTemplateCache cache, HTMLTemplate info) throws ServletException {
    tmplGlam2Motifs = cache.loadAndCache("/WEB-INF/templates/component_glam2motifs.tmpl");
    prefix = getText(info, "prefix", "motifs");
    fieldName = getText(info, "description", "GLAM2 Motifs");
    registerFn = getText(info, "register", "nop");
    title = getTemplate(info, "title", null);
    subtitle = getTemplate(info, "subtitle", null);
    defaultOption = getEnum(info, "default", DefaultOption.class, DefaultOption.FILE);
    alphabets = getEnums(info, "alphabets", Alphabet.class, EnumSet.allOf(Alphabet.class));
  }

  @Override
  public HTMLSub getComponent() {
    return getComponent(null, null);
  }

  public HTMLSub getComponent(String embedMotifs) {
    return getComponent(embedMotifs, null);
  }

  public HTMLSub getComponent(String embedMotifs, String embedName) {
    DefaultOption defaultOption = this.defaultOption;
    HTMLSub motifs = tmplGlam2Motifs.getSubtemplate("component").toSub();
    motifs.set("prefix", prefix);
    if (title != null) motifs.set("title", title);
    if (subtitle != null) motifs.set("subtitle", subtitle);

    if (embedMotifs != null) {
      motifs.getSub("embed_section").set("prefix", prefix).
          set("data", WebUtils.escapeHTML(embedMotifs));
      if (embedName != null) {
        motifs.getSub("embed_section").set("name", WebUtils.escapeHTML(embedName));
      }
      defaultOption = DefaultOption.EMBED;
    } else {
      motifs.empty("embed_option");
      motifs.empty("embed_section");
    }
    switch (defaultOption) {
      case FILE:
        motifs.getSub("file_option").set("selected", "selected");
        break;
      case EMBED:
        if (embedMotifs != null) motifs.getSub("embed_option").set("selected", "selected");
        break;
    }

    StringWriter buf = new StringWriter();
    JsonWr jsonWr = new JsonWr(buf, 18);
    try {
      jsonWr.start();
      jsonWr.property("field", fieldName);
      jsonWr.property("alphabets");
      jsonWr.startObject();
      for (Alphabet alphabet : this.alphabets)
        jsonWr.property(alphabet.name(), true);
      jsonWr.endObject();
      jsonWr.end();
    } catch (IOException e) {
      // no IO exceptions should occur as this uses a StringBuffer
      throw new Error(e);
    }
    motifs.set("options", buf.toString());
    motifs.set("register_component", registerFn);

    return motifs;
  }

  @Override
  public HTMLSub getHelp() {
    return this.tmplGlam2Motifs.getSubtemplate("help").toSub();
  }

  public MotifDataSource getGlam2Motifs(FileCoord.Name name, HttpServletRequest request,
        FeedbackHandler feedback) throws ServletException, IOException {
    // determine the source
    String source = paramRequire(request, prefix + "_source");
    Part part = request.getPart(prefix + "_" + source);
    if (part == null || part.getSize() == 0) {
      feedback.whine("No " + fieldName + " provided.");
      return null; // no sequences submitted
    }
    if (source.equals("file")) {
      name.setOriginalName(getPartFilename(part));
    } else if (source.equals("embed")) {
      name.setOriginalName(request.getParameter(prefix + "_name"));
    }
    MotifStats statistics = null;
    InputStream in = null;
    File file = null;
    OutputStream out = null;
    boolean success = false;
    try {
      in  = new BufferedInputStream(part.getInputStream());
      file = File.createTempFile("uploaded_motifs_", ".fa");
      file.deleteOnExit();
      out = new BufferedOutputStream(new FileOutputStream(file));
      // copy to a temporary file
      byte[] buffer = new byte[10240]; // 10KB
      int len;
      while ((len = in.read(buffer)) != -1) {
          out.write(buffer, 0, len);
      }
      try {out.close();} finally {out = null;}
      try {in.close();} finally {in = null;}
      statistics = Glam2Validator.validate(file);
      if (statistics == null) {
        feedback.whine("The " + fieldName + " did not pass validation.");
      }
      success = (statistics != null);
    } finally {
      closeQuietly(in);
      closeQuietly(out);
      if (file != null && !success) {
        if (!file.delete()) {
          logger.log(Level.WARNING, "Failed to delete temporary file \"" + file +
              "\". A second attempt will be made at exit.");
        }
      }
    }
    if (success) return new MotifDataSource(file, name, statistics, null);
    return null;

  }
}
