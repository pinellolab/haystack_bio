package au.edu.uq.imb.memesuite.servlet.util;

import au.edu.uq.imb.memesuite.data.Alphabet;
import au.edu.uq.imb.memesuite.data.Background;
import au.edu.uq.imb.memesuite.data.NamedFileDataSource;
import au.edu.uq.imb.memesuite.template.HTMLSub;
import au.edu.uq.imb.memesuite.template.HTMLTemplate;
import au.edu.uq.imb.memesuite.template.HTMLTemplateCache;
import au.edu.uq.imb.memesuite.util.FileCoord;
import au.edu.uq.imb.memesuite.util.JsonWr;

import javax.servlet.ServletContext;
import javax.servlet.ServletException;
import javax.servlet.http.HttpServletRequest;
import java.io.IOException;
import java.io.StringWriter;
import java.util.EnumSet;

import static au.edu.uq.imb.memesuite.servlet.ConfigurationLoader.CACHE_KEY;
import static au.edu.uq.imb.memesuite.servlet.util.WebUtils.paramFile;
import static au.edu.uq.imb.memesuite.servlet.util.WebUtils.paramRequire;

/**
 * A component for allowing input of a Markov Model Background or the selection
 * of the order of model that should be generated from the sequences.
 */
public class ComponentBfile extends PageComponent {
  private HTMLTemplate tmplBgFile;
  private EnumSet<Alphabet> alphabets;
  private String prefix;
  private String fieldName; // for feedback
  private String registerFn;
  private boolean allowZeroOrderBackgrounds;
  private boolean allowHighOrderBackgrounds;
  private boolean allowMemeBackgrounds;
  private boolean allowUniformBackgrounds;
  private boolean allowUploadBackgrounds;
  private String bg_selected;

  public ComponentBfile(ServletContext context, HTMLTemplate info) throws ServletException {
    HTMLTemplateCache cache = (HTMLTemplateCache)context.getAttribute(CACHE_KEY);
    tmplBgFile = cache.loadAndCache("/WEB-INF/templates/component_bfile.tmpl");
    prefix = getText(info, "prefix", "bfile");
    fieldName = getText(info, "description", "background");
    registerFn = getText(info, "register", "nop");
    alphabets = getEnums(info, "alphabets", Alphabet.class, EnumSet.of(Alphabet.DNA, Alphabet.PROTEIN));
    allowZeroOrderBackgrounds = info.containsSubtemplate("enable_zero_order");
    allowHighOrderBackgrounds = info.containsSubtemplate("enable_high_order");
    allowMemeBackgrounds = info.containsSubtemplate("enable_meme");
    allowUniformBackgrounds = info.containsSubtemplate("enable_uniform");
    allowUploadBackgrounds = info.containsSubtemplate("enable_upload");
    bg_selected = getText(info, "selected", "");
  }

  @Override
  public HTMLSub getComponent() {
    boolean selected = false;
    HTMLSub sub = tmplBgFile.getSubtemplate("component").toSub();
    sub.set("prefix", prefix);
    if (allowZeroOrderBackgrounds) {
      if (bg_selected.equals("zero")) sub.getSub("zero_order_bg").set("select", "selected");
    } else {
      sub.empty("zero_order_bg");
    }
    if (allowHighOrderBackgrounds) {
      if (bg_selected.equals("high")) sub.getSub("higher_order_bg").set("select", "selected");
    } else {
      sub.empty("higher_order_bg");
    }
    if (allowMemeBackgrounds) {
      if (bg_selected.equals("meme")) sub.getSub("meme_bg").set("select", "selected");
    } else {
      sub.empty("meme_bg");
    }
    if (allowUniformBackgrounds) {
      if (bg_selected.equals("uniform")) sub.getSub("uniform_bg").set("select", "selected");
    } else {
      sub.empty("uniform_bg");
    }
    if (allowUploadBackgrounds) {
      if (bg_selected.equals("upload")) sub.getSub("upload_bg").set("select", "selected");
    } else {
      sub.empty("upload_bg");
    }

    //if (bg_selected.equals("")) sub.set("select", "selected");

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
      jsonWr.property("file_max", 1 << 20);
      jsonWr.end();
    } catch (IOException e) {
      // no IO exceptions should occur as this uses a StringBuffer
      throw new Error(e);
    }
    sub.set("options", buf.toString());
    sub.set("register_component", registerFn);
    return sub;  // generated code
  }

  @Override
  public HTMLSub getHelp() {
    return tmplBgFile.getSubtemplate("help").toSub();  // generated code
  }

  public Background getBfile(EnumSet<Alphabet> restrictedAlphabets, FileCoord.Name name,
        HttpServletRequest request, FeedbackHandler feedback) throws ServletException, IOException {
    // first determine if we're using a file, a generated background or the pre-generated background from the NRDB
    String source = paramRequire(request, prefix + "_source");
    Background.Source bgsrc;
    NamedFileDataSource file = null;
    if (source.equals("file")) {
      bgsrc = Background.Source.FILE;
      file = paramFile(request, prefix + "_file", name);
    } else if (source.equals("uniform")) {
      bgsrc = Background.Source.UNIFORM;
    } else if (source.equals("meme")) {
      bgsrc = Background.Source.MEME;
    } else {
      try {
        switch (Integer.parseInt(source, 10)) {
          case 0:
            bgsrc = Background.Source.ORDER_0;
            break;
          case 1:
            bgsrc = Background.Source.ORDER_1;
            break;
          case 2:
            bgsrc = Background.Source.ORDER_2;
            break;
          case 3:
            bgsrc = Background.Source.ORDER_3;
            break;
          case 4:
            bgsrc = Background.Source.ORDER_4;
            break;
          default:
            throw new NumberFormatException("Number is not in range 0 to 4");
        }
      } catch (NumberFormatException e) {
        throw new ServletException(
            "Expected source to be 'file', 'uniform', 'meme', 'nrdb' or a number between 0 and 4", e);
      }
    }
    return new Background(bgsrc, file);
  }

  public Background getBfile(FileCoord.Name name,
          HttpServletRequest request, FeedbackHandler feedback) throws ServletException, IOException {
    return getBfile(null, name, request, feedback);
  }
}
