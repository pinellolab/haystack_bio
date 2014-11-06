package au.edu.uq.imb.memesuite.servlet.util;

import au.edu.uq.imb.memesuite.template.HTMLSub;
import au.edu.uq.imb.memesuite.template.HTMLTemplate;
import au.edu.uq.imb.memesuite.template.HTMLTemplateCache;

import javax.servlet.ServletException;

/**
 * Form component that can open and close a following div element as well
 * as check if the contents of that div have changed from the default.
 */
public class ComponentAdvancedOptions extends PageComponent {
  private HTMLTemplate tmplAdvBtn;
  private String id;
  private String title;
  private String changeFn;
  private String resetFn;

  public ComponentAdvancedOptions(HTMLTemplateCache cache) throws ServletException {
    tmplAdvBtn = cache.loadAndCache("/WEB-INF/templates/component_adv_btn.tmpl");
    id = "options_btn";
    title = null;
    changeFn = "options_changed";
    resetFn = "options_reset";
  }

  public ComponentAdvancedOptions(HTMLTemplateCache cache, HTMLTemplate info) throws ServletException {
    tmplAdvBtn = cache.loadAndCache("/WEB-INF/templates/component_adv_btn.tmpl");
    id = getText(info, "prefix", "options_btn");
    title = getText(info, "title", null);
    changeFn = getText(info, "change_fn", "options_changed");
    resetFn = getText(info, "reset_fn", "options_reset");
  }

  public HTMLSub getComponent() {
    HTMLSub sub = tmplAdvBtn.getSubtemplate("component").toSub();
    sub.set("id", id);
    sub.set("title", title);
    sub.set("change_fn", changeFn);
    sub.set("reset_fn", resetFn);
    return sub;
  }

  public HTMLSub getHelp() {
    return tmplAdvBtn.getSubtemplate("help").toSub();
  }
}
