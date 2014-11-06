package au.edu.uq.imb.memesuite.template;

import java.io.IOException;
import java.lang.ref.WeakReference;
import java.text.ParseException;
import java.util.Iterator;
import java.util.TreeMap;
import java.util.Map;

import javax.servlet.ServletContext;
import javax.servlet.ServletException;

public class HTMLTemplateCache  {
  private ServletContext context;
  private Map<String,WeakReference<HTMLTemplate>> cache;


  /**
   * Constructor taking a servlet context which allows it
   * to find files relative to the web application.
   */
  public HTMLTemplateCache(ServletContext context) {
    this.context = context;
    this.cache = new TreeMap<String,WeakReference<HTMLTemplate>>();
  }

  /**
   * Cleans up references to deallocated templates.
   */
  public void clean() {
    Iterator<Map.Entry<String,WeakReference<HTMLTemplate>>> iter;
    iter = this.cache.entrySet().iterator();
    while (iter.hasNext()) {
      if (iter.next().getValue().get() == null) iter.remove();
    }
  }

  /**
   * Clears the cache.
   */
  public void clear() {
    this.cache.clear();
  }

  /**
   * Returns a HTMLTemplate from the cache.
   */
  public HTMLTemplate loadAndCache(String resourcePath) throws ServletException {
    WeakReference<HTMLTemplate> weakEntry = cache.get(resourcePath);
    HTMLTemplate template = (weakEntry != null ? weakEntry.get() : null);
    if (template == null) {
      try {
        template = HTMLTemplate.load(this.context.getResourceAsStream(resourcePath));
      } catch (IOException e) {
        throw new ServletException("Unable to read template file", e);
      } catch (ParseException e) {
        throw new ServletException("Template file contains errors.", e);
      }
      weakEntry = new WeakReference<HTMLTemplate>(template);
      cache.put(resourcePath, weakEntry);
      this.clean();
      return template;
    }
    return template;
  }
}
