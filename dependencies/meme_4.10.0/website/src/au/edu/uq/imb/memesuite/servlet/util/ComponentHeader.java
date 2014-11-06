package au.edu.uq.imb.memesuite.servlet.util;

import au.edu.uq.imb.memesuite.template.HTMLSub;
import au.edu.uq.imb.memesuite.template.HTMLTemplate;
import au.edu.uq.imb.memesuite.template.HTMLTemplateCache;

import javax.servlet.ServletException;

/**
 * Component to display a page header
 */
public class ComponentHeader extends PageComponent {
  private HTMLTemplate tmplHeader;
  private HTMLTemplate tmplTitle;
  private HTMLTemplate tmplSubtitle;
  private HTMLTemplate tmplLogo;
  private HTMLTemplate tmplAlt;
  private HTMLTemplate tmplBlurb;
  private String version;

  public ComponentHeader(HTMLTemplateCache cache, String version,
      HTMLTemplate fillIn) throws ServletException {
    tmplHeader = cache.loadAndCache("/WEB-INF/templates/component_header.tmpl");
    if (fillIn.containsSubtemplate("title")) {
      tmplTitle = fillIn.getSubtemplate("title");
    }
    if (fillIn.containsSubtemplate("subtitle")) {
      tmplSubtitle = fillIn.getSubtemplate("subtitle");
    }
    if (fillIn.containsSubtemplate("logo")) {
      tmplLogo = fillIn.getSubtemplate("logo");
    }
    if (fillIn.containsSubtemplate("alt")) {
      tmplAlt = fillIn.getSubtemplate("alt");
    }
    if (fillIn.containsSubtemplate("blurb")) {
      tmplBlurb = fillIn.getSubtemplate("blurb");
    }
    this.version = version;
  }

  public HTMLSub getComponent() {
    HTMLSub sub = tmplHeader.getSubtemplate("component").toSub();
    sub.set("version", version);
    sub.set("title", tmplTitle);
    sub.set("subtitle", tmplSubtitle);
    sub.set("logo", tmplLogo);
    sub.set("alt", tmplAlt);
    sub.set("blurb", tmplBlurb);
    return sub;
  }

  public HTMLSub getHelp() {
    return tmplHeader.getSubtemplate("help").toSub();
  }

}
