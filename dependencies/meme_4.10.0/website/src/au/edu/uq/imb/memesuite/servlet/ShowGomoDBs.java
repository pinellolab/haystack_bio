package au.edu.uq.imb.memesuite.servlet;

import au.edu.uq.imb.memesuite.db.*;
import au.edu.uq.imb.memesuite.template.HTMLSub;
import au.edu.uq.imb.memesuite.template.HTMLTemplate;
import au.edu.uq.imb.memesuite.template.HTMLTemplateCache;

import javax.servlet.ServletContext;
import javax.servlet.ServletException;
import javax.servlet.http.HttpServlet;
import javax.servlet.http.HttpServletRequest;
import javax.servlet.http.HttpServletResponse;
import java.io.IOException;
import java.io.PrintWriter;
import java.sql.SQLException;
import java.util.ArrayList;
import java.util.List;
import java.util.logging.Level;
import java.util.logging.Logger;

import static au.edu.uq.imb.memesuite.servlet.ConfigurationLoader.CACHE_KEY;
import static au.edu.uq.imb.memesuite.servlet.ConfigurationLoader.GOMO_DB_KEY;

/**
 * Show the databases available to the GOMO program.
 */
public class ShowGomoDBs extends HttpServlet {
  private ServletContext context;
  private HTMLTemplate template;
  private HTMLTemplate categoryLiTemplate;
  private HTMLTemplate categoryTemplate;
  private HTMLTemplate listingTemplate;
  private HTMLTemplate secondaryTemplate;

  private static Logger logger = Logger.getLogger("au.edu.uq.imb.memesuite.web");

  public ShowGomoDBs() { }

  @Override
  public void init() throws ServletException {
    context = this.getServletContext();
    HTMLTemplateCache cache = (HTMLTemplateCache)context.getAttribute(CACHE_KEY);
    template = cache.loadAndCache("/WEB-INF/templates/show_gomo_dbs.tmpl");
    categoryLiTemplate = template.getSubtemplate("content").getSubtemplate("category_li");
    categoryTemplate = template.getSubtemplate("content").getSubtemplate("category");
    listingTemplate = categoryTemplate.getSubtemplate("listing");
    secondaryTemplate = listingTemplate.getSubtemplate("secondaries").getSubtemplate("secondary");
  }

  @Override
  public void doGet(HttpServletRequest request, HttpServletResponse response)
      throws IOException, ServletException {
    if (request.getParameter("category") != null) {
      outputXmlListingsOfCategory(response, getId(request, "category"));
    } else {
      display(response);
    }
  }

  @Override
  public void doPost(HttpServletRequest request, HttpServletResponse response)
      throws IOException, ServletException {
    display(response);
  }

  private void display(HttpServletResponse response)
      throws IOException, ServletException {
    GomoDBList gomoDBList = (GomoDBList)context.getAttribute(GOMO_DB_KEY);
    response.setContentType("text/html");
    HTMLSub out = template.toSub();
    if (gomoDBList != null) {
      out.getSub("content").set("category_li", new AllCategoryList(gomoDBList));
      out.getSub("content").set("category", new AllCategories(gomoDBList));
    } else {
      out.empty("content");
    }
    out.output(response.getWriter());
  }

  private long getId(HttpServletRequest request, String name) throws ServletException {
    String value = request.getParameter(name);
    if (value == null) {
      throw new ServletException("Parameter '" + name + "' was not set.");
    }
    long id;
    try {
      id = Long.parseLong(value, 10);
    } catch (NumberFormatException e) {
      throw new ServletException("Parameter '" + name + "' is not a integer value.", e);
    }
    return id;
  }

  private void outputXmlListingsOfCategory(HttpServletResponse response, long categoryId)
      throws IOException {
    GomoDBList gomoDBList = (GomoDBList)context.getAttribute(GOMO_DB_KEY);
    response.setContentType("application/xml");
    // turn off caching
//    response.setHeader("Cache-Control", "no-cache, no-store, must-revalidate"); // HTTP 1.1.
//    response.setHeader("Pragma", "no-cache"); // HTTP 1.0.
//    response.setDateHeader("Expires", 0); // Proxies.
    DBList.View<Listing> listingView = null;
    PrintWriter out = null;
    try {
      listingView = gomoDBList.getListings(categoryId);
      out = response.getWriter();
      out.println("<?xml version=\"1.0\" encoding=\"UTF-8\"?>");
      out.println("<listings category=\"" + categoryId + "\">");
      while (listingView.hasNext()) {
        Listing listing = listingView.next();
        out.println("<l i=\"" + listing.getID() + "\" n=\"" + listing.getName() + "\"/>");
      }
      out.println("</listings>");
    } finally {
      if (out != null) out.close();
      if (listingView != null) listingView.close();
    }
  }

  private class AllCategoryList implements Iterable<HTMLSub> {
    private GomoDBList gomoDBList;

    private AllCategoryList(GomoDBList gomoDBList) {
      this.gomoDBList = gomoDBList;
    }

    @Override
    public java.util.Iterator<HTMLSub> iterator() {
      return new Iterator();
    }

    private class Iterator implements java.util.Iterator<HTMLSub> {
      private DBList.View<Category> categoryView;
      public Iterator() {
        categoryView = gomoDBList.getCategories();
      }

      @Override
      public boolean hasNext() {
        return categoryView.hasNext();
      }

      @Override
      public HTMLSub next() {
        Category category = categoryView.next();
        HTMLSub out = categoryLiTemplate.toSub();
        out.set("id", category.getID());
        out.set("name", category.getName());
        return out;
      }

      @Override
      public void remove() {
        throw new UnsupportedOperationException("Remove is not supported");
      }
    }
  }

  private class AllCategories implements Iterable<HTMLSub> {
    private GomoDBList gomoDBList;

    private AllCategories(GomoDBList gomoDBList) {
      this.gomoDBList = gomoDBList;
    }

    @Override
    public java.util.Iterator<HTMLSub> iterator() {
      return new Iterator();
    }

    private class Iterator implements java.util.Iterator<HTMLSub> {
      private DBList.View<Category> categoryView;
      public Iterator() {
        categoryView = gomoDBList.getCategories();
      }

      @Override
      public boolean hasNext() {
        return categoryView.hasNext();
      }

      @Override
      public HTMLSub next() {
        Category category = categoryView.next();
        HTMLSub out = categoryTemplate.toSub();
        out.set("id", category.getID());
        out.set("name", category.getName());
        out.set("listing", new AllListingsOfCategory(gomoDBList, category.getID()));
        return out;
      }

      @Override
      public void remove() {
        throw new UnsupportedOperationException("Remove is not supported");
      }
    }
  }

  private class AllListingsOfCategory implements Iterable<HTMLSub> {
    private GomoDBList gomoDBList;
    private long id;

    public AllListingsOfCategory(GomoDBList gomoDBList, long id) {
      this.gomoDBList = gomoDBList;
      this.id = id;
    }

    @Override
    public java.util.Iterator<HTMLSub> iterator() {
      return new Iterator();
    }

    private class Iterator implements java.util.Iterator<HTMLSub> {
      private DBList.View<Listing> listingView;

      public Iterator() {
        listingView = gomoDBList.getListings(id);
      }

      @Override
      public boolean hasNext() {
        return listingView.hasNext();
      }

      @Override
      public HTMLSub next() {
        Listing listing = listingView.next();
        HTMLSub out = listingTemplate.toSub();
        out.set("name", listing.getName());
        out.set("description", listing.getDescription());
        try {
          GomoDB gomoDB = gomoDBList.getGomoListing(listing.getID());
          List<GomoDBSecondary> secondaries = gomoDB.getSecondaries();
          if (!secondaries.isEmpty()) {
            List<HTMLSub> list = new ArrayList<HTMLSub>();
            for (GomoDBSecondary secondary : secondaries) {
              list.add(secondaryTemplate.toSub().set("description", secondary.getDescription()));
            }
            out.getSub("secondaries").set("secondary", list);
          } else {
            out.empty("secondaries");
          }
        } catch (SQLException e) {
          out.set("secondaries", e.getMessage());
          logger.log(Level.WARNING, "Failed to query GOMO database",  e);
        }
        return out;
      }

      @Override
      public void remove() {
        throw new UnsupportedOperationException("Remove is not supported");
      }
    }
  }
}
