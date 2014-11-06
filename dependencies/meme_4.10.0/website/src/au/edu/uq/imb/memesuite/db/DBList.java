package au.edu.uq.imb.memesuite.db;

import java.io.Closeable;
import java.io.File;
import java.util.EnumSet;
import java.util.Iterator;
import java.sql.*;
import java.util.logging.Level;
import java.util.logging.Logger;

import au.edu.uq.imb.memesuite.data.Alphabet;
import org.sqlite.*;

import static au.edu.uq.imb.memesuite.db.SQL.*;

public class DBList {
  private File db;
  private boolean deleteOnCleanup;
  protected SQLiteDataSource ds;
  private static Logger logger = Logger.getLogger("au.edu.uq.imb.memesuite.web");

  public DBList(File db, boolean deleteOnCleanup) throws ClassNotFoundException {
    Class.forName("org.sqlite.JDBC");
    SQLiteConfig config = new SQLiteConfig();
    config.enforceForeignKeys(true);
    config.setReadOnly(true);
    this.ds = new SQLiteDataSource(config);
    this.ds.setUrl("jdbc:sqlite:" + db);
    this.db = db;
    this.deleteOnCleanup = deleteOnCleanup;
  }

  public DBList(SQLiteDataSource ds) {
    this.ds = ds;
    this.db = null;
    this.deleteOnCleanup = false;
  }

  /**
   * Deletes the database file if the deleteOnCleanup flag is set.
   * Obviously you should only call this if you no-longer need the database!
   */
  public void cleanup() {
    if (deleteOnCleanup) {
      logger.log(Level.INFO, "Deleting unused database file \"" + this.db + "\".");
      if (!db.delete()) {
        logger.log(Level.WARNING, "Failed to delete database file \"" + this.db + "\".");
      }
    }
  }

  /**
   * Return the last modified date of the database.
   * @return the last modified date of the database.
   */
  public long lastModified() throws UnsupportedOperationException {
    if (db == null) throw new UnsupportedOperationException("DBList was initialised without a reference to the file so a last modified date can not be accessed.");
    return db.lastModified();
  }

  /**
   * Count the categories
   * @return the number of categories.
   * @throws SQLException if something goes wrong.
   */
  public int countCategories() throws SQLException {
    int count = 0;
    Connection connection = null;
    PreparedStatement ps = null;
    ResultSet rs = null;
    try {
      connection = ds.getConnection();
      ps = connection.prepareStatement(SQL.COUNT_CATEGORIES);
      rs = ps.executeQuery();
      if (!rs.next()) throw new SQLException("COUNT(*) should always return a row");
      count = rs.getInt(1);
      rs.close(); rs = null;
      ps.close(); ps = null;
      connection.close(); connection = null;
    } finally {
      if (rs != null) {
        try {
          rs.close();
        } catch (SQLException e) { /* ignore */ }
      }
      if (ps != null) {
        try {
          ps.close();
        } catch (SQLException e) { /* ignore */ }
      }
      if (connection != null) {
        try {
          connection.close();
        } catch (SQLException e) { /* ignore */ }
      }
    }
    return count;
  }

  public View<Category> getCategories() {
    return new CategoryView();
  }

  public int countListings() throws SQLException {
    int count = 0;
    Connection connection = null;
    PreparedStatement ps = null;
    ResultSet rs = null;
    try {
      connection = ds.getConnection();
      ps = connection.prepareStatement(SQL.COUNT_ALL_LISTINGS);
      rs = ps.executeQuery();
      if (!rs.next()) throw new SQLException("COUNT(*) should always return a row");
      count = rs.getInt(1);
      rs.close(); rs = null;
      ps.close(); ps = null;
      connection.close(); connection = null;
    } finally {
      if (rs != null) {
        try {
          rs.close();
        } catch (SQLException e) { /* ignore */ }
      }
      if (ps != null) {
        try {
          ps.close();
        } catch (SQLException e) { /* ignore */ }
      }
      if (connection != null) {
        try {
          connection.close();
        } catch (SQLException e) { /* ignore */ }
      }
    }
    return count;
  }

  public int countListings(long categoryId) throws SQLException {
    int count = 0;
    Connection connection = null;
    PreparedStatement ps = null;
    ResultSet rs = null;
    try {
      connection = ds.getConnection();
      ps = connection.prepareStatement(SQL.COUNT_CATEGORY_LISTINGS);
      ps.setLong(1, categoryId);
      rs = ps.executeQuery();
      if (!rs.next()) throw new SQLException("COUNT(*) should always return a row");
      count = rs.getInt(1);
      rs.close(); rs = null;
      ps.close(); ps = null;
      connection.close(); connection = null;
    } finally {
      if (rs != null) {
        try {
          rs.close();
        } catch (SQLException e) { /* ignore */ }
      }
      if (ps != null) {
        try {
          ps.close();
        } catch (SQLException e) { /* ignore */ }
      }
      if (connection != null) {
        try {
          connection.close();
        } catch (SQLException e) { /* ignore */ }
      }
    }
    return count;
  }

  public View<Listing> getListings(long categoryId, boolean shortOnly,
      EnumSet<Alphabet> allowedAlphabets) {
    if (allowedAlphabets == null) throw new NullPointerException(
        "Parameter allowedAlphabets must not be null.");
    if (allowedAlphabets.isEmpty()) throw new IllegalArgumentException(
        "At least one allowed alphabet must be supplied.");
    return new ListingView(categoryId, shortOnly, allowedAlphabets);
  }

  public View<Listing> getListings(long categoryId) {
    return new ListingView(categoryId, false, EnumSet.allOf(Alphabet.class));
  }

  protected ResultSet queryCategories(Connection connection) throws SQLException {
    PreparedStatement ps = connection.prepareStatement(SQL.SELECT_UNSORTED_CATEGORIES);
    return ps.executeQuery();
  }

  protected ResultSet queryListings(Connection conn, long categoryId,
      int allowedAlphabets, boolean shortOnly) throws SQLException {
    PreparedStatement ps = conn.prepareStatement(SELECT_LISTINGS_OF_CATEGORY);
    ps.setLong(1, categoryId);
    return ps.executeQuery();
  }

  public interface View<E> extends Iterator<E>, Closeable {
    public void close();
    public boolean hasError();
    public SQLException getError();
  }

  /**
   * Class for providing an iterator view of a list of objects retrieved from a 
   * database
   */
  protected abstract class AbstractView<E> implements View<E> {
    /** Connection to the database. */
    protected Connection conn;
    /** Result set to be iterated over.
     * Note that other queries may be done to fully resolve the objects. */
    protected ResultSet rset;
    /** The current object. */
    protected E current;
    /** A store for the first error that occurs */
    protected SQLException firstError;

    /**
     * Construct a view.
     * Subclasses should call connect after doing their initialisation.
     */
    public AbstractView() {
    }

    /**
     * Run a query to create a result set for iterating over.
     */
    protected abstract ResultSet query(Connection conn) throws SQLException;

    /**
     * Connect to the database and query the database for the list to iterate over.
     */
    protected void connect() {
      boolean error = true;
      this.conn = null;
      this.firstError = null;
      try {
        this.conn = DBList.this.ds.getConnection();
        this.rset = this.query(this.conn);
        this.loadNext();
        error = false;
      } catch (SQLException e) {
        if (this.firstError == null) this.firstError = e;
        logger.log(Level.WARNING, "Problem accessing db", e);
      } finally {
        if (error && conn != null) {
          try {
            conn.close();
          } catch (SQLException e) {
            //ignore
          }
        }
      }
    }

    /**
     * Read values from a result set to construct a storage object.
     */
    protected abstract E constructData(ResultSet rset) throws SQLException;

    /**
     * Load the next object from the result set.
     */
    protected void loadNext() {
      if (this.conn == null) throw new IllegalStateException("The connection has been closed");
      this.current = null;
      try {
        if (this.rset.next()) {
          this.current = constructData(this.rset);
        } else {
          this.close();
        }
      } catch (SQLException e) {
        logger.log(Level.WARNING, "Problem loading next entry from db", e);
        if (this.firstError == null) this.firstError = e;
      }
    }

    /**
     * Check if there are any more objects to iterate over.
     */
    public boolean hasNext() {
      return (this.current != null);
    }

    /**
     * Returns the next object or null if there are no more objects.
     */
    public E next() {
      if (this.current == null) return null;
      E c = this.current;
      this.loadNext();
      return c;
    }

    /**
     * Unsupported operation.
     */
    public void remove() {
      throw new UnsupportedOperationException("Remove is not supported");
    }

    /**
     * Close the connection to the database.
     */
    public void close() {
      if (this.conn != null) {
        try {
          this.conn.close();
          this.conn = null;
        } catch (SQLException e) {
          logger.log(Level.WARNING, "Problem closing connection to db", e);
          if (this.firstError == null) this.firstError = e;
        }
      }
    }

    /**
     * Check if a error is stored.
     */
    public boolean hasError() {
      return this.firstError != null;
    }

    /**
     * Get the first error.
     */
    public SQLException getError() {
      return this.firstError;
    }

  }

  public class CategoryView extends AbstractView<Category> {

    public CategoryView() {
      connect();
    }

    protected ResultSet query(Connection conn) throws SQLException {
      return queryCategories(conn);
    }

    protected Category constructData(ResultSet rset) throws SQLException {
      long id = rset.getLong(1);
      String name = rset.getString(2);
      return new Category(id, name);
    }
  }

  public class ListingView extends AbstractView<Listing> {
    protected long categoryId;
    protected boolean shortOnly;
    protected int allowedAlphabets;

    public ListingView(long categoryId, boolean shortOnly, EnumSet<Alphabet> allowedAlphabets) {
      this.categoryId = categoryId;
      this.shortOnly = shortOnly;
      this.allowedAlphabets = 0;
      for (Alphabet alph : allowedAlphabets) {
        this.allowedAlphabets |= (1 << alph.ordinal());
      }
      connect();
    }

    protected ResultSet query(Connection conn) throws SQLException {
      return queryListings(conn, categoryId, allowedAlphabets, shortOnly);
    }

    protected Listing constructData(ResultSet rset) throws SQLException {
      long id = rset.getLong(1);
      String name = rset.getString(2);
      String description = rset.getString(3);
      int alphabetsBitset = rset.getInt(4);
      EnumSet<Alphabet> alphabets = EnumSet.noneOf(Alphabet.class);
      int max = Integer.highestOneBit(alphabetsBitset);
      for (int i = 1; i <= max; i *= 2) {
        if ((alphabetsBitset & i) != 0) alphabets.add(Alphabet.fromInt(i));
      }
      return new Listing(id, name, description, alphabets);
    }
  }


}
