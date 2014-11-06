package au.edu.uq.imb.memesuite.servlet.util;

import au.edu.uq.imb.memesuite.data.MemeSuiteProperties;
import au.edu.uq.imb.memesuite.template.HTMLSub;
import au.edu.uq.imb.memesuite.template.HTMLTemplate;
import au.edu.uq.imb.memesuite.template.HTMLTemplateCache;

import javax.servlet.ServletException;

import static au.edu.uq.imb.memesuite.servlet.util.WebUtils.escapeHTML;

/**
 * Component to display the page footer.
 */
public class ComponentFooter extends PageComponent {
  private HTMLTemplate tmplFooter;
  private MemeSuiteProperties msp;

  public ComponentFooter(HTMLTemplateCache cache, MemeSuiteProperties msp) throws ServletException {
    tmplFooter = cache.loadAndCache("/WEB-INF/templates/component_footer.tmpl");
    this.msp = msp;
  }

  public HTMLSub getComponent() {
    HTMLSub sub = tmplFooter.getSubtemplate("component").toSub();
    sub.set("version", msp.getVersion());
    sub.set("help_email", msp.getDeveloperContact());
    sub.set("help_email_enc", escapeHTML(msp.getDeveloperContact()));
    return sub;
  }

  public HTMLSub getHelp() {
    return tmplFooter.getSubtemplate("help").toSub();
  }
}
