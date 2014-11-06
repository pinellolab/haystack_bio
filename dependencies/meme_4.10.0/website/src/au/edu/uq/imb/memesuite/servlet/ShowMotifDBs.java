package au.edu.uq.imb.memesuite.servlet;

import au.edu.uq.imb.memesuite.data.Alphabet;
import au.edu.uq.imb.memesuite.db.*;
import au.edu.uq.imb.memesuite.servlet.util.WebUtils;
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
import java.util.EnumSet;

import static au.edu.uq.imb.memesuite.servlet.ConfigurationLoader.CACHE_KEY;
import static au.edu.uq.imb.memesuite.servlet.ConfigurationLoader.MOTIF_DB_KEY;
import static au.edu.uq.imb.memesuite.servlet.ConfigurationLoader.SEQUENCE_DB_KEY;


/**
 * Display the available motif databases
 */
public class ShowMotifDBs extends HttpServlet {
  private ServletContext context;
  private HTMLTemplate template;
  private HTMLTemplate categoryTemplate;
  private HTMLTemplate listingTemplate;

  public ShowMotifDBs() { }

  @Override
  public void init() throws ServletException {
    context = this.getServletContext();
    HTMLTemplateCache cache = (HTMLTemplateCache)context.getAttribute(CACHE_KEY);
    template = cache.loadAndCache("/WEB-INF/templates/show_motif_dbs.tmpl");
    categoryTemplate = template.getSubtemplate("content").getSubtemplate("category");
    listingTemplate = categoryTemplate.getSubtemplate("listing");
  }

  @Override
  public void doGet(HttpServletRequest request, HttpServletResponse response)
      throws IOException, ServletException {
    if (request.getParameter("listing") != null) {
      outputXmlListing(response, getId(request, "listing"));
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
    MotifDBList motifDBList = (MotifDBList)context.getAttribute(MOTIF_DB_KEY);
    response.setContentType("text/html");
    HTMLSub out = template.toSub();
    if (motifDBList != null) {
      out.getSub("content").set("category", new AllCategories(motifDBList));
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


  private void outputXmlListing(HttpServletResponse response, long listingId)
      throws IOException {
    MotifDBList motifDBList = (MotifDBList)context.getAttribute(MOTIF_DB_KEY);
    response.setContentType("application/xml");
    PrintWriter out = null;
    try {
      MotifDB motifDB = motifDBList.getMotifListing(listingId);
      out = response.getWriter();
      out.print("<motif_db id=\"" + listingId + "\" alphabet=\"" +
          motifDB.getAlphabet().name() + "\">");
      // name contains HTML, hence can't be escaped or it becomes useless
      out.print("<name><![CDATA[");
      out.print(motifDB.getName());
      out.println("]]></name>");
      // description contains HTML, hence can't be escaped or it becomes useless
      out.print("<description><![CDATA[");
      out.print(motifDB.getDescription());
      out.println("]]></description>");
      for (MotifDBFile file : motifDB.getMotifFiles()) {
        out.println("<file src=\"" +WebUtils.escapeHTML(file.getFileName()) +
            "\" count=\"" + file.getMotifCount() + "\" cols=\"" +
            file.getTotalCols() + "\" min=\"" + file.getMinCols() +
            "\" max=\"" + file.getMaxCols() + "\" />");
      }
      out.println("</motif_db>");
    } catch (SQLException e) {
      throw new IOException(e);
    } finally {
      if (out != null) out.close();
    }

  }

  private class AllCategories implements Iterable<HTMLSub> {
    private MotifDBList motifDBList;

    private AllCategories(MotifDBList motifDBList) {
      this.motifDBList = motifDBList;
    }

    @Override
    public java.util.Iterator<HTMLSub> iterator() {
      return new Iterator();
    }

    private class Iterator implements java.util.Iterator<HTMLSub> {
      private DBList.View<Category> categoryView;
      public Iterator() {
        categoryView = motifDBList.getCategories();
      }

      @Override
      public boolean hasNext() {
        return categoryView.hasNext();
      }

      @Override
      public HTMLSub next() {
        Category category = categoryView.next();
        HTMLSub out = categoryTemplate.toSub();
        out.set("listing", new AllListingsOfCategory(motifDBList, category.getID()));
        return out;
      }

      @Override
      public void remove() {
        throw new UnsupportedOperationException("Remove is not supported");
      }
    }
  }

  private class AllListingsOfCategory implements Iterable<HTMLSub> {
    private MotifDBList motifDBList;
    private long id;

    public AllListingsOfCategory(MotifDBList motifDBList, long id) {
      this.motifDBList = motifDBList;
      this.id = id;
    }

    @Override
    public java.util.Iterator<HTMLSub> iterator() {
      return new Iterator();
    }

    private class Iterator implements java.util.Iterator<HTMLSub> {
      private DBList.View<Listing> listingView;

      public Iterator() {
        listingView = motifDBList.getListings(id);
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
        return out;
      }

      @Override
      public void remove() {
        throw new UnsupportedOperationException("Remove is not supported");
      }
    }
  }

}
