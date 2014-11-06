package au.edu.uq.imb.memesuite.servlet.util;

import au.edu.uq.imb.memesuite.data.*;
import au.edu.uq.imb.memesuite.db.Category;
import au.edu.uq.imb.memesuite.db.DBList;
import au.edu.uq.imb.memesuite.db.SequenceDBList;
import au.edu.uq.imb.memesuite.db.SequenceDB;
import au.edu.uq.imb.memesuite.io.fasta.FastaException;
import au.edu.uq.imb.memesuite.io.fasta.FastaParser;
import au.edu.uq.imb.memesuite.template.HTMLSub;
import au.edu.uq.imb.memesuite.template.HTMLTemplate;
import au.edu.uq.imb.memesuite.template.HTMLTemplateCache;
import au.edu.uq.imb.memesuite.util.FileCoord;
import au.edu.uq.imb.memesuite.util.JsonWr;

import javax.servlet.ServletContext;
import javax.servlet.ServletException;
import javax.servlet.http.HttpServletRequest;
import javax.servlet.http.Part;
import java.io.*;
import java.sql.SQLException;
import java.util.EnumSet;
import java.util.Iterator;

import static au.edu.uq.imb.memesuite.servlet.util.WebUtils.*;
import static au.edu.uq.imb.memesuite.servlet.ConfigurationLoader.CACHE_KEY;
import static au.edu.uq.imb.memesuite.servlet.ConfigurationLoader.SEQUENCE_DB_KEY;


/**
 * A component that is used for inputting FASTA sequences.
 */
public class ComponentSequences extends PageComponent {
  private ServletContext context;
  private HTMLTemplate tmplSequences;
  private EnumSet<Alphabet> alphabets;
  private String prefix;
  private String fieldName; // for feedback
  private HTMLTemplate title;
  private HTMLTemplate subtitle;
  private boolean loadDBs;
  private String registerFn;
  private DefaultOption defaultOption;
  // sequence options
  private boolean weights;
  private boolean mask;
  private boolean ambigs;
  private boolean gaps;
  private Integer maxNameLen;
  private Integer maxDescLen;
  private Integer minSeqLen;
  private Integer maxSeqLen;
  private Integer minSeqCount;
  private Integer maxSeqCount;
  private Integer maxSeqTotal;

  /**
   * Enum of the initial selection state.
   */
  public static enum DefaultOption {
    TEXT,
    FILE,
    EMBED,
    DATABASE
  }

  /**
   * Iterate over the categories and produce a HTMLSub for each
   * from the specified template
   */
  private static class CategoryTemplate implements Iterable<HTMLSub> {
    private DBList db;
    private HTMLTemplate template;
    private boolean selectFirst;
    boolean selectable;
    boolean identifiable;
    boolean nameable;

    public CategoryTemplate(DBList db, HTMLTemplate template, boolean selectFirst) {
      this.db = db;
      this.template = template;
      this.selectFirst = selectFirst;
      selectable = template.containsSubtemplate("selected");
      identifiable = template.containsSubtemplate("id");
      nameable = template.containsSubtemplate("name");
    }

    @Override
    public Iterator<HTMLSub> iterator() {
      return new CategoryTemplateIterator();
    }

    private class CategoryTemplateIterator implements Iterator<HTMLSub> {
      private DBList.View<Category> view;
      private boolean first;

      public CategoryTemplateIterator() {
        view = db.getCategories();
        first = true;
      }

      @Override
      public boolean hasNext() {
        return view.hasNext();
      }

      @Override
      public HTMLSub next() {
        HTMLSub category = template.toSub();
        Category data = view.next();
        if (data != null) {
          if (selectFirst && first && selectable) {
            category.set("selected", "selected");
          }
          if (identifiable) category.set("id", data.getID());
          if (nameable) category.set("name", data.getName());
        }
        first = false;
        return category;
      }

      @Override
      public void remove() {
        throw new UnsupportedOperationException("Remove does not make sense here.");
      }
    }
  }

  public ComponentSequences(ServletContext context, HTMLTemplate info) throws ServletException {
    this.context = context;
    HTMLTemplateCache cache = (HTMLTemplateCache)context.getAttribute(CACHE_KEY);
    tmplSequences = cache.loadAndCache("/WEB-INF/templates/component_sequences.tmpl");
    prefix = getText(info, "prefix", "sequences");
    fieldName = getText(info, "description", "sequences");
    registerFn = getText(info, "register", "nop");
    title = getTemplate(info, "title", null);
    subtitle = getTemplate(info, "subtitle", null);
    alphabets = getEnums(info, "alphabets", Alphabet.class, EnumSet.allOf(Alphabet.class));
    loadDBs = info.containsSubtemplate("enable_db");
    defaultOption = getEnum(info, "default", DefaultOption.class, DefaultOption.FILE);
    // sequence options
    mask = !info.containsSubtemplate("disable_masking");
    ambigs = !info.containsSubtemplate("disable_ambiguous");
    weights = info.containsSubtemplate("enable_weights");
    gaps = info.containsSubtemplate("enable_gaps");
    maxNameLen = getInt(info, "max_name_len", 50);
    maxDescLen = getInt(info, "max_desc_len", 500);
    minSeqLen = getInt(info, "min_seq_len", 1);
    maxSeqLen = getInt(info, "max_seq_len", null);
    minSeqCount = getInt(info, "min_seq_count", 1);
    maxSeqCount = getInt(info, "max_seq_count", null);
    maxSeqTotal = getInt(info, "max_seq_total", null);
  }

  public HTMLSub getComponent() {
    return getComponent(null, null);
  }

  public HTMLSub getComponent(String embedSeqs) {
    return getComponent(embedSeqs, null);
  }

  public HTMLSub getComponent(String embedSeqs, String embedName) {
    boolean selectFirstDB = false;
    DefaultOption defaultOption = this.defaultOption;
    HTMLSub sequences = tmplSequences.getSubtemplate("component").toSub();
    sequences.set("prefix", prefix);
    if (title != null) sequences.set("title", title);
    if (subtitle != null) sequences.set("subtitle", subtitle);
    if (embedSeqs != null) {
      sequences.getSub("embed_section").set("prefix", prefix).
          set("data", WebUtils.escapeHTML(embedSeqs));
      if (embedName != null) {
        sequences.getSub("embed_section").set("name", WebUtils.escapeHTML(embedName));
      }
      defaultOption = DefaultOption.EMBED;
    } else {
      sequences.empty("embed_option");
      sequences.empty("embed_section");
    }
    switch (defaultOption) {
      case TEXT:
        sequences.getSub("text_option").set("selected", "selected");
        break;
      case FILE:
        sequences.getSub("file_option").set("selected", "selected");
        break;
      case EMBED:
        if (embedSeqs != null) sequences.getSub("embed_option").set("selected", "selected");
        break;
      case DATABASE:
        selectFirstDB = true;
    }
    SequenceDBList db = (SequenceDBList)context.getAttribute(SEQUENCE_DB_KEY);
    if (loadDBs && db != null) {
      HTMLTemplate cat_opt = tmplSequences.getSubtemplate("component").
          getSubtemplate("cat_options").getSubtemplate("cat_option");
      sequences.getSub("cat_options").set("cat_option",
          new CategoryTemplate(db, cat_opt, selectFirstDB));
      sequences.getSub("db_section").set("prefix", prefix);
    } else {
      sequences.empty("cat_options");
      sequences.empty("db_section");
    }

    StringWriter buf = new StringWriter();
    JsonWr jsonWr = new JsonWr(buf, 18);
    try {
      jsonWr.start();
      jsonWr.property("field", fieldName);
      jsonWr.property("alphabets");
      jsonWr.startObject();
      for (Alphabet alphabet : this.alphabets)
        jsonWr.property(alphabet.name(), true);
      jsonWr.endObject();
      jsonWr.property("weights", weights);
      jsonWr.property("mask", mask);
      jsonWr.property("ambigs", ambigs);
      jsonWr.property("gaps", gaps);
      jsonWr.property("max_name_len", maxNameLen);
      jsonWr.property("max_desc_len", maxDescLen);
      jsonWr.property("min_seq_len", minSeqLen);
      jsonWr.property("max_seq_len", maxSeqLen);
      jsonWr.property("min_seq_count", minSeqCount);
      jsonWr.property("max_seq_count", maxSeqCount);
      jsonWr.property("max_seq_total", maxSeqTotal);
      jsonWr.end();
    } catch (IOException e) {
      // no IO exceptions should occur as this uses a StringBuffer
      throw new Error(e);
    }
    sequences.set("options", buf.toString());
    sequences.set("register_component", registerFn);
    return sequences;
  }

  public HTMLSub getHelp() {
    return tmplSequences.getSubtemplate("help").toSub();
  }

  private boolean checkSpec(EnumSet<Alphabet> restrictedAlphabets, SequenceStats stats, FeedbackHandler feedback) {
    EnumSet<Alphabet> allowedAlphabets = EnumSet.copyOf(alphabets);
    if (restrictedAlphabets != null) allowedAlphabets.retainAll(restrictedAlphabets);
    boolean ok = true;
    if (minSeqCount != null && minSeqCount > stats.getSequenceCount()) {
      feedback.whine("Too few sequences for " + fieldName);
      ok = false;
    } else if (maxSeqCount != null && maxSeqCount < stats.getSequenceCount()) {
      feedback.whine("Too many sequences for " + fieldName);
      ok = false;
    }
    if (minSeqLen != null && minSeqLen > stats.getMinLength()) {
      feedback.whine(
          "There are one or more sequences that are too short in the "
          + fieldName);
      ok = false;
    }
    if (maxSeqLen != null && maxSeqLen < stats.getMaxLength()) {
      feedback.whine(
          "There are one or more " + fieldName + " that are too long");
      ok = false;
    }
    if (maxSeqTotal != null && maxSeqTotal < stats.getTotalLength()) {
      feedback.whine(
          "The combined length of all the " + fieldName + " is to long.");
      ok = false;
    }
    if (!allowedAlphabets.contains(stats.guessAlphabet())) {
      feedback.whine(
          "The alphabet of the " + fieldName + " seems to be " +
          stats.guessAlphabet() + " but it is not one of the allowed alphabets");
      ok = false;
    }
    return ok;
  }

  /**
   * Sequences come from 4 sources.
   * They can be typed, uploaded, embedded or be a pre-existing database.
   * Typed, embedded and uploaded sequences need to be preprocessed to calculate
   * statistics and to ensure they are valid.
   *
   * So what should happen if...
   * 1) An expected field is missing
   *    throw an exception and stop processing the request
   * 2) Sequences have not been sent
   *    whine complaining about the missing sequences
   *    return null
   * 3) Sequences contain a syntax error
   *    whine complaining about the syntax error
   *    return null
   * 4) Sequences violate some constraint
   *    whine complaining about the failed constraint
   *    return null
   * 5) Sequences are valid, pass all constraints
   *    return a sequence source
   *
   *
   * @param restrictedAlphabets specifies what alphabets are allowed after
   *                            other settings have restricted them.
   * @param name manages the name of user supplied sequences and avoids clashes.
   * @param request all the information sent to the webserver.
   * @param feedback an interface for providing error messages to the user.
   * @return a sequence source and return null when a source is not available.
   * @throws ServletException if request details are incorrect (like missing form fields).
   * @throws IOException if storing a parsed version of the sequences to file fails.
   */
  public SequenceInfo getSequences(EnumSet<Alphabet> restrictedAlphabets, FileCoord.Name name,
      HttpServletRequest request, FeedbackHandler feedback) throws ServletException, IOException {
    String source = paramRequire(request, prefix + "_source");
    if (source.equals("text") || source.equals("file") || source.equals("embed")) {
      // get the reader from the part and use the file name if possible
      Part part = request.getPart(prefix + "_" + source);
      if (part == null || part.getSize() == 0) {
        feedback.whine("No " + fieldName + " provided.");
        return null; // no sequences submitted
      }
      if (source.equals("file")) {
        name.setOriginalName(getPartFilename(part));
      } else if (source.equals("embed")) {
        name.setOriginalName(request.getParameter(prefix + "_name"));
      }
      SequenceStats statistics = null;
      InputStream in = null;
      File file = null;
      OutputStream out = null;
      boolean success = false;
      try {
        in  = new BufferedInputStream(part.getInputStream());
        file = File.createTempFile("uploaded_sequences_", ".fa");
        file.deleteOnExit();
        out = new BufferedOutputStream(new FileOutputStream(file));
        FeedbackFastaWriter handler = new FeedbackFastaWriter(feedback, fieldName, out);
        FastaParser.parse(in, handler);
        try {out.close();} finally {out = null;}
        try {in.close();} finally {in = null;}
        // check the statistics
        statistics = handler.getStatsRecorder();
        success = checkSpec(restrictedAlphabets, statistics, feedback);
      } catch (FastaException e) {
        // ignore
      } finally {
        closeQuietly(in);
        closeQuietly(out);
        if (file != null && !success) {
          if (!file.delete()) file.deleteOnExit();
        }
      }
      if (success) return new SequenceDataSource(file, name, statistics);
      return null;
    } else { // database
      EnumSet<Alphabet> allowedAlphabets = EnumSet.copyOf(alphabets);
      if (restrictedAlphabets != null) allowedAlphabets.retainAll(restrictedAlphabets);
      long listingId, edition;
      try {
        listingId = Long.parseLong(paramRequire(request, prefix + "_db_listing"), 10);
      } catch (NumberFormatException e) {
        throw new ServletException("Expected the listing ID to be a number", e);
      }
      try {
        edition = Long.parseLong(paramRequire(request, prefix + "_db_version"), 10);
      } catch (NumberFormatException e) {
        throw new ServletException("Expected the version ID to be a number", e);
      }
      // now get a sequence database that matches the listing, version and alphabet
      SequenceDBList db = (SequenceDBList)context.getAttribute(SEQUENCE_DB_KEY);
      if (db == null) {
        throw new ServletException("Unable to access the sequence database.");
      }
      SequenceDB dbFile;
      try {
        dbFile = db.getSequenceFile(listingId, edition, allowedAlphabets);
      } catch (SQLException e) {
        throw new ServletException(e);
      }
      if (dbFile == null) {
        StringBuilder alphabetText = new StringBuilder();
        Alphabet[] alphabetList = allowedAlphabets.toArray(new Alphabet[allowedAlphabets.size()]);
        alphabetText.append(alphabetList[0]);
        for (int i = 1; i < alphabetList.length - 1; i++) {
          alphabetText.append(", ");
          alphabetText.append(alphabetList[i]);
        }
        if (alphabetList.length > 1) {
          alphabetText.append(" or ");
          alphabetText.append(alphabetList[alphabetList.length - 1]);
        }
          feedback.whine("There is no " + alphabetText + " variant of that database for the " + fieldName);
      }
      return dbFile;
    }
  }
  /**
   * Sequences come from 4 sources.
   * They can be typed, uploaded, embedded or be a pre-existing database.
   * Typed, embedded and uploaded sequences need to be preprocessed to calculate
   * statistics and to ensure they are valid.
   *
   * @param name manages the name of user supplied sequences and avoids clashes.
   * @param request all the information sent to the webserver.
   * @param feedback an interface for providing error messages to the user.
   * @return a sequence source and return null when a source is not available.
   * @throws ServletException if request details are incorrect (like missing form fields).
   * @throws IOException if storing a parsed version of the sequences to file fails.
   */
  public SequenceInfo getSequences(FileCoord.Name name, HttpServletRequest request, FeedbackHandler feedback) throws ServletException, IOException {
    return getSequences(null, name, request, feedback);
  }
}
