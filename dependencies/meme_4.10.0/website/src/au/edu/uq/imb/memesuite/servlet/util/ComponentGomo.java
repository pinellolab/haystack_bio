package au.edu.uq.imb.memesuite.servlet.util;

import au.edu.uq.imb.memesuite.db.*;
import au.edu.uq.imb.memesuite.template.HTMLSub;
import au.edu.uq.imb.memesuite.template.HTMLTemplate;
import au.edu.uq.imb.memesuite.template.HTMLTemplateCache;

import javax.servlet.ServletContext;
import javax.servlet.ServletException;
import javax.servlet.http.HttpServletRequest;
import java.sql.SQLException;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

import static au.edu.uq.imb.memesuite.servlet.util.WebUtils.paramRequire;
import static au.edu.uq.imb.memesuite.servlet.ConfigurationLoader.CACHE_KEY;
import static au.edu.uq.imb.memesuite.servlet.ConfigurationLoader.GOMO_DB_KEY;

/**
 * Data entry component for GOMO databases
 */
public class ComponentGomo extends PageComponent {
  private ServletContext context;
  private HTMLTemplate tmplGomo;
  private HTMLTemplate tmplCategory;
  private HTMLTemplate tmplListing;
  private String prefix;
  private HTMLTemplate title;
  private HTMLTemplate subtitle;


  private static final Pattern DB_ID_PATTERN = Pattern.compile("^\\d+$");

  public ComponentGomo(ServletContext context, HTMLTemplate info) throws ServletException {
    this.context = context;
    HTMLTemplateCache cache = (HTMLTemplateCache)context.getAttribute(CACHE_KEY);
    tmplGomo = cache.loadAndCache("/WEB-INF/templates/component_gomo.tmpl");
    prefix = getText(info, "prefix", "motifs");
    title = getTemplate(info, "title", null);
    subtitle = getTemplate(info, "subtitle", null);
    tmplCategory = tmplGomo.getSubtemplate("component").getSubtemplate("category");
    tmplListing = tmplCategory.getSubtemplate("listing");
  }

  @Override
  public HTMLSub getComponent() {
    HTMLSub gomo = tmplGomo.getSubtemplate("component").toSub();
    gomo.set("prefix", prefix);
    if (title != null) gomo.set("title", title);
    if (subtitle != null) gomo.set("subtitle", subtitle);
    GomoDBList db = (GomoDBList)context.getAttribute(GOMO_DB_KEY);
    gomo.set("category", new AllCategories(db));
    return gomo;
  }

  @Override
  public HTMLSub getHelp() {
    return this.tmplGomo.getSubtemplate("help").toSub();
  }

  public GomoDB getGomoSequences(HttpServletRequest request) throws ServletException {
    // determine the source
    String source = paramRequire(request, prefix + "_source");
    Matcher m = DB_ID_PATTERN.matcher(source);
    if (!m.matches()) {
      throw new ServletException("Parameter " + prefix + "_source had a " +
          "value that did not match any of the allowed values.");
    }
    long dbId;
    try {
      dbId = Long.parseLong(source, 10);
    } catch (NumberFormatException e) {
      throw new ServletException(e);
    }
    GomoDBList db = (GomoDBList)context.getAttribute(GOMO_DB_KEY);
    if (db == null) {
      throw new ServletException("Unable to access the gomo database.");
    }
    try {
      return db.getGomoListing(dbId);
    } catch (SQLException e) {
      throw new ServletException(e);
    }
  }

  private class AllCategories implements Iterable<HTMLSub> {
    private GomoDBList db;

    private AllCategories(GomoDBList db) {
      this.db = db;
    }

    @Override
    public java.util.Iterator<HTMLSub> iterator() {
      return new Iterator();
    }

    private class Iterator implements java.util.Iterator<HTMLSub> {
      private DBList.View<Category> categoryView;
      public Iterator() {
        categoryView = db.getCategories();
      }

      @Override
      public boolean hasNext() {
        return categoryView.hasNext();
      }

      @Override
      public HTMLSub next() {
        Category category = categoryView.next();
        HTMLSub out = tmplCategory.toSub();
        out.set("name", category.getName());
        out.set("listing", new AllListingsOfCategory(db, category.getID()));
        return out;
      }

      @Override
      public void remove() {
        throw new UnsupportedOperationException("Remove is not supported");
      }
    }
  }

  private class AllListingsOfCategory implements Iterable<HTMLSub> {
    private GomoDBList db;
    private long id;

    public AllListingsOfCategory(GomoDBList db, long id) {
      this.db = db;
      this.id = id;
    }

    @Override
    public java.util.Iterator<HTMLSub> iterator() {
      return new Iterator();
    }

    private class Iterator implements java.util.Iterator<HTMLSub> {
      private DBList.View<Listing> listingView;

      public Iterator() {
        listingView = db.getListings(id);
      }

      @Override
      public boolean hasNext() {
        return listingView.hasNext();
      }

      @Override
      public HTMLSub next() {
        Listing listing = listingView.next();
        HTMLSub out = tmplListing.toSub();
        out.set("id", listing.getID());
        out.set("name", listing.getName());
        return out;
      }

      @Override
      public void remove() {
        throw new UnsupportedOperationException("Remove is not supported");
      }
    }
  }
}
