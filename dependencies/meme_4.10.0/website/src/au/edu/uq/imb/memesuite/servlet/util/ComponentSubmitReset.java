package au.edu.uq.imb.memesuite.servlet.util;

import au.edu.uq.imb.memesuite.template.HTMLSub;
import au.edu.uq.imb.memesuite.template.HTMLTemplate;
import au.edu.uq.imb.memesuite.template.HTMLTemplateCache;

import javax.servlet.ServletException;

/**
 * Component containing the submit and reset buttons.
 */
public class ComponentSubmitReset extends PageComponent {
  private HTMLTemplate tmplSubmitReset;
  private String submitTitle;
  private String resetTitle;

  public ComponentSubmitReset(HTMLTemplateCache cache) throws ServletException {
    tmplSubmitReset = cache.loadAndCache("/WEB-INF/templates/component_submit_reset.tmpl");
    submitTitle = "Start Search";
    resetTitle = "Clear Input";
  }

  public HTMLSub getComponent() {
    HTMLSub sub = tmplSubmitReset.getSubtemplate("component").toSub();
    sub.set("submit_title", submitTitle);
    sub.set("reset_title", resetTitle);
    return sub;
  }

  public HTMLSub getHelp() {
    return tmplSubmitReset.getSubtemplate("help").toSub();
  }
}
