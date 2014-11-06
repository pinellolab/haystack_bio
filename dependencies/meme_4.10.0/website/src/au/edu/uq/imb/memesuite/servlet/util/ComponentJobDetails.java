package au.edu.uq.imb.memesuite.servlet.util;

import au.edu.uq.imb.memesuite.template.HTMLSub;
import au.edu.uq.imb.memesuite.template.HTMLTemplate;
import au.edu.uq.imb.memesuite.template.HTMLTemplateCache;

import javax.servlet.ServletException;
import javax.servlet.http.HttpServletRequest;

import java.io.*;

import static au.edu.uq.imb.memesuite.servlet.util.WebUtils.paramDescription;
import static au.edu.uq.imb.memesuite.servlet.util.WebUtils.paramEmail;

/**
 * This component contains the email and description fields.
 */
public class ComponentJobDetails extends PageComponent {
  private HTMLTemplate tmplJobDetails;

  public ComponentJobDetails(HTMLTemplateCache cache) throws ServletException {
    tmplJobDetails = cache.loadAndCache("/WEB-INF/templates/component_job_details.tmpl");
  }

  /**
   * Get the template used to input the job email and description.
   * @return the template used to input the job email and description.
   */
  public HTMLSub getComponent() {
    return tmplJobDetails.getSubtemplate("component").toSub();
  }

  /**
   * Get the help popups that are used by the component.
   * @return the template containing the help popups related to the email and description.
   */
  public HTMLSub getHelp() {
    return tmplJobDetails.getSubtemplate("help").toSub();
  }

  /**
   * Get the validated email input from this component.
   * @param request the post from the user.
   * @param feedback a handler for user feedback.
   * @return the email address or null if it was not valid.
   * @throws ServletException if the expected fields are missing.
   */
  public String getEmail(HttpServletRequest request, FeedbackHandler feedback) throws ServletException {
    return paramEmail(feedback, request, "email");
  }

  /**
   * Get the validated description input from this component.
   * @param request the post from the user.
   * @return a handler for user feedback.
   * @throws ServletException if the expected fields are missing.
   */
  public String getDescription(HttpServletRequest request) throws ServletException, IOException {
    return paramDescription(request, "description");
  }
}
