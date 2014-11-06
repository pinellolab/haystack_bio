package au.edu.uq.imb.memesuite.servlet;

import au.edu.uq.imb.memesuite.data.Alphabet;
import au.edu.uq.imb.memesuite.db.*;
import au.edu.uq.imb.memesuite.db.DBList.View;
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
import java.util.EnumSet;

import static au.edu.uq.imb.memesuite.servlet.ConfigurationLoader.CACHE_KEY;
import static au.edu.uq.imb.memesuite.servlet.ConfigurationLoader.SEQUENCE_DB_KEY;

/**
 * Display the available sequence databases.
 */
public class ShowSequenceDBs extends HttpServlet {
  private ServletContext context;
  private HTMLTemplate template;
  private HTMLTemplate categoryLiTemplate;
  private HTMLTemplate categoryTemplate;
  private HTMLTemplate listingTemplate;
  private HTMLTemplate versionTemplate;

  public ShowSequenceDBs() { }

  @Override
  public void init() throws ServletException {
    this.context = this.getServletContext();
    HTMLTemplateCache cache = (HTMLTemplateCache)context.getAttribute(CACHE_KEY);
    template = cache.loadAndCache("/WEB-INF/templates/show_sequence_dbs.tmpl");
    categoryLiTemplate = template.getSubtemplate("content").getSubtemplate("category_li");
    categoryTemplate = template.getSubtemplate("content").getSubtemplate("category");
    listingTemplate = categoryTemplate.getSubtemplate("listing");
    versionTemplate = listingTemplate.getSubtemplate("version");
  }

  @Override
  public void doGet(HttpServletRequest request, HttpServletResponse response)
      throws IOException, ServletException {
    if (request.getParameter("category") != null) {
      outputXmlListingsOfCategory(response, getId(request, "category"),
          isShortOnly(request), allowedAlphabets(request));
    } else if (request.getParameter("listing") != null) {
      outputXmlVersionsOfListing(response, getId(request, "listing"),
          isShortOnly(request), allowedAlphabets(request));
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
    SequenceDBList sequenceDBList = (SequenceDBList)context.getAttribute(SEQUENCE_DB_KEY);
    response.setContentType("text/html");
    HTMLSub out = template.toSub();
    if (sequenceDBList != null) {
      out.getSub("content").set("category_li", new AllCategoryList(sequenceDBList));
      out.getSub("content").set("category", new AllCategories(sequenceDBList));
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

  private boolean isShortOnly(HttpServletRequest request) throws ServletException{
    String value = request.getParameter("short");
    if (value == null) return false;
    boolean shortOnly;
    try {
      shortOnly = Integer.parseInt(value, 2) != 0;
    } catch (NumberFormatException e) {
      throw new ServletException("Parameter 'short' is not a binary value.", e);
    }
    return shortOnly;
  }

  private EnumSet<Alphabet> allowedAlphabets(HttpServletRequest request) throws ServletException {
    String value = request.getParameter("alphabets");
    if (value == null) return EnumSet.allOf(Alphabet.class);
    EnumSet<Alphabet> allowedAlphabets;
    try {
      allowedAlphabets = SQL.intToEnums(Alphabet.class, Integer.parseInt(value, 10));
    } catch (NumberFormatException e) {
      throw new ServletException("Parameter 'alphabets' is not a bitset.", e);
    } catch (IllegalArgumentException e) {
      throw new ServletException("Parameter 'alphabets' is not a bitset", e);
    }
    return allowedAlphabets;
  }

  private void outputXmlListingsOfCategory(HttpServletResponse response,
      long categoryId, boolean shortOnly, EnumSet<Alphabet> allowedAlphabets)
      throws IOException {
    SequenceDBList sequenceDBList = (SequenceDBList)context.getAttribute(SEQUENCE_DB_KEY);
    response.setContentType("application/xml");
    // turn off caching
//    response.setHeader("Cache-Control", "no-cache, no-store, must-revalidate"); // HTTP 1.1.
//    response.setHeader("Pragma", "no-cache"); // HTTP 1.0.
//    response.setDateHeader("Expires", 0); // Proxies.
    View<Listing> listingView = null;
    PrintWriter out = null;
    try {
      listingView = sequenceDBList.getListings(categoryId, shortOnly, allowedAlphabets);
      out = response.getWriter();
      out.println("<?xml version=\"1.0\" encoding=\"UTF-8\"?>");
      out.println("<listings category=\"" + categoryId + "\" short=\"" +
          (shortOnly ? "1" : "0") +"\" alphabets=\"" +
          SQL.enumsToInt(allowedAlphabets) + "\">");
      while (listingView.hasNext()) {
        Listing listing = listingView.next();
        out.println("<l i=\"" + listing.getID() + "\" n=\"" + listing.getName() +
            "\" a=\"" + SQL.enumsToInt(listing.getAlphabets()) + "\"/>");
      }
      out.println("</listings>");
    } finally {
      if (out != null) out.close();
      if (listingView != null) listingView.close();
    }
  }

  private void outputXmlVersionsOfListing(HttpServletResponse response,
      long listingId, boolean shortOnly, EnumSet<Alphabet> allowedAlphabets)
      throws IOException {
    SequenceDBList sequenceDBList = (SequenceDBList)context.getAttribute(SEQUENCE_DB_KEY);
    response.setContentType("application/xml");
    View<SequenceVersion> versionView = null;
    PrintWriter out = null;
    try {
      versionView = sequenceDBList.getVersions(listingId, shortOnly, allowedAlphabets);
      out = response.getWriter();
      out.println("<versions listing=\"" + listingId + "\" short=\"" +
          (shortOnly ? "1" : "0") + "\" alphabets=\"" +
          SQL.enumsToInt(allowedAlphabets) + "\">");
      while (versionView.hasNext()) {
        SequenceVersion version = versionView.next();
        out.println("<v i=\"" + version.getEdition() + "\" n=\"" +
            version.getVersion() + "\" a=\"" +
            SQL.enumsToInt(version.getAlphabets()) + "\"/>");
      }
      out.println("</versions>");
    } finally {
      if (out != null) out.close();
      if (versionView != null) versionView.close();
    }

  }

  private class AllCategoryList implements Iterable<HTMLSub> {
    private SequenceDBList sequenceDBList;

    private AllCategoryList(SequenceDBList sequenceDBList) {
      this.sequenceDBList = sequenceDBList;
    }

    @Override
    public java.util.Iterator<HTMLSub> iterator() {
      return new Iterator();
    }

    private class Iterator implements java.util.Iterator<HTMLSub> {
      private View<Category> categoryView;
      public Iterator() {
        categoryView = sequenceDBList.getCategories();
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
    private SequenceDBList sequenceDBList;

    private AllCategories(SequenceDBList sequenceDBList) {
      this.sequenceDBList = sequenceDBList;
    }

    @Override
    public java.util.Iterator<HTMLSub> iterator() {
      return new Iterator();
    }

    private class Iterator implements java.util.Iterator<HTMLSub> {
      private View<Category> categoryView;
      public Iterator() {
        categoryView = sequenceDBList.getCategories();
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
        out.set("listing", new AllListingsOfCategory(sequenceDBList, category.getID()));
        return out;
      }

      @Override
      public void remove() {
        throw new UnsupportedOperationException("Remove is not supported");
      }
    }
  }

  private class AllListingsOfCategory implements Iterable<HTMLSub> {
    private SequenceDBList sequenceDBList;
    private long id;

    public AllListingsOfCategory(SequenceDBList sequenceDBList, long id) {
      this.sequenceDBList = sequenceDBList;
      this.id = id;
    }

    @Override
    public java.util.Iterator<HTMLSub> iterator() {
      return new Iterator();
    }

    private class Iterator implements java.util.Iterator<HTMLSub> {
      private View<Listing> listingView;

      public Iterator() {
        listingView = sequenceDBList.getListings(id);
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
        out.set("version", new AllVersionsOfListing(sequenceDBList, listing.getID()));
        EnumSet<Alphabet> alphabets = listing.getAlphabets();
        if (!alphabets.contains(Alphabet.RNA)) out.empty("rna");
        if (!alphabets.contains(Alphabet.DNA)) out.empty("dna");
        if (!alphabets.contains(Alphabet.PROTEIN)) out.empty("protein");
        return out;
      }

      @Override
      public void remove() {
        throw new UnsupportedOperationException("Remove is not supported");
      }
    }
  }

  private class AllVersionsOfListing implements Iterable<HTMLSub> {
    private SequenceDBList sequenceDBList;
    private long listingId;

    private AllVersionsOfListing(SequenceDBList sequenceDBList, long listingId) {
      this.sequenceDBList = sequenceDBList;
      this.listingId = listingId;
    }

    @Override
    public java.util.Iterator<HTMLSub> iterator() {
      return new Iterator();
    }

    private class Iterator implements java.util.Iterator<HTMLSub> {
      private View<SequenceVersion> versionView;

      public Iterator() {
        versionView = sequenceDBList.getVersions(listingId);
      }

      @Override
      public boolean hasNext() {
        return versionView.hasNext();
      }

      @Override
      public HTMLSub next() {
        SequenceDB sequenceDB;
        SequenceVersion listing = versionView.next();
        HTMLSub out = versionTemplate.toSub();
        out.set("name", listing.getVersion());
        if ((sequenceDB = listing.getSequenceFile(Alphabet.DNA)) != null) {
          out.getSub("dna").set("description", sequenceDB.getDescription());
        } else {
          out.empty("dna");
        }
        if ((sequenceDB = listing.getSequenceFile(Alphabet.RNA)) != null) {
          out.getSub("rna").set("description", sequenceDB.getDescription());
        } else {
          out.empty("rna");
        }
        if ((sequenceDB = listing.getSequenceFile(Alphabet.PROTEIN)) != null) {
          out.getSub("protein").set("description", sequenceDB.getDescription());
        } else {
          out.empty("protein");
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
