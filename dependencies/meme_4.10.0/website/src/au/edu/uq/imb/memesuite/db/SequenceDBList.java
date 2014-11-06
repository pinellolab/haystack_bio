package au.edu.uq.imb.memesuite.db;

import au.edu.uq.imb.memesuite.data.Alphabet;
import org.sqlite.SQLiteDataSource;

import java.io.File;
import java.sql.Connection;
import java.sql.PreparedStatement;
import java.sql.ResultSet;
import java.sql.SQLException;
import java.util.ArrayList;
import java.util.EnumSet;
import java.util.List;
import java.util.logging.Level;
import java.util.logging.Logger;

import static au.edu.uq.imb.memesuite.db.SQL.*;

/**
 * Access information on sequence databases from the sqlite database.
 */
public class SequenceDBList extends DBList {
  private static Logger logger = Logger.getLogger("au.edu.uq.imb.memesuite.web");

  public SequenceDBList(File db) throws ClassNotFoundException {
    super(db, false);
  }

  public SequenceDBList(SQLiteDataSource ds) {
    super(ds);
  }

  @Override
  protected ResultSet queryCategories(Connection connection) throws SQLException {
    PreparedStatement ps = connection.prepareStatement(SQL.SELECT_SEQUENCE_CATEGORIES);
    return ps.executeQuery();
  }

  @Override
  protected ResultSet queryListings(Connection conn, long categoryId,
      int allowedAlphabets, boolean shortOnly) throws SQLException {
    PreparedStatement ps = conn.prepareStatement(SELECT_SEQUENCE_LISTINGS_OF_CATEGORY);
    ps.setLong(1, categoryId);
    ps.setInt(2, allowedAlphabets);
    ps.setBoolean(3, shortOnly);
    return ps.executeQuery();
  }

  public SequenceVersionView getVersions(long listingId) {
    return new SequenceVersionView(listingId, false, EnumSet.allOf(Alphabet.class));
  }

  public SequenceVersionView getVersions(long listingId, boolean shortOnly,
      EnumSet<Alphabet> allowedAlphabets) {
    return new SequenceVersionView(listingId, shortOnly, allowedAlphabets);
  }

  public SequenceDB getSequenceFile(long listingId, long edition,
      EnumSet<Alphabet> alphabets)
      throws SQLException {
    Connection conn = null;
    PreparedStatement pstmt = null;
    ResultSet rset = null;
    SequenceDB sequenceDB = null;
    try {
      conn = ds.getConnection();
      // get the name and description
      pstmt = conn.prepareStatement(SQL.SELECT_LISTING);
      pstmt.setLong(1, listingId);
      rset = pstmt.executeQuery();
      if (!rset.next()) throw new SQLException("No listing by that ID");
      String listingName = rset.getString(1);
      String listingDescription = rset.getString(2);
      rset.close();
      pstmt.close();
      pstmt = conn.prepareStatement(SELECT_SEQUENCE_FILE_OF_LISTING);
      pstmt.setLong(1, listingId);
      pstmt.setLong(2, edition);
      pstmt.setInt(3, SQL.enumsToInt(alphabets));
      rset = pstmt.executeQuery();
      if (rset.next()) {
        long sequenceId = rset.getLong(1);
        Alphabet alphabet = Alphabet.fromInt(rset.getInt(2));
        String version = rset.getString(3);
        String description = rset.getString(4);
        String fileSeq = rset.getString(5);
        String fileBg = rset.getString(6);
        long sequenceCount = rset.getLong(7);
        long totalLen = rset.getLong(8);
        long minLen = rset.getLong(9);
        long maxLen = rset.getLong(10);
        double avgLen = rset.getDouble(11);
        double stdDLen = rset.getDouble(12);
        sequenceDB = new SequenceDB(listingName, listingDescription,
            sequenceId, alphabet, edition, version,
            description, fileSeq, fileBg, sequenceCount, totalLen,
            minLen, maxLen, avgLen, stdDLen);
      }
      rset.close();
      rset = null;
      pstmt.close();
      pstmt = null;
      conn.close();
      conn = null;
    } finally {
      if (rset != null) {
        try {
          rset.close();
        } catch (SQLException e) { /* ignore */ }
      }
      if (pstmt != null) {
        try {
          pstmt.close();
        } catch (SQLException e) { /* ignore */ }
      }
      if (conn != null) {
        try {
          conn.close();
        } catch (SQLException e) { /* ignore */ }
      }
    }
    return sequenceDB;
  }

  public class SequenceVersionView implements View<SequenceVersion> {
    private long listingId;
    private String listingName;
    private String listingDescription;
    private boolean shortOnly;
    private EnumSet<Alphabet> allowedAlphabets;
    protected Connection conn;
    protected ResultSet rset;
    protected SequenceVersion storage;
    protected SQLException firstError;

    public SequenceVersionView(long listingId, boolean shortOnly,
        EnumSet<Alphabet> allowedAlphabets) {
      this.listingId = listingId;
      this.shortOnly = shortOnly;
      this.allowedAlphabets = allowedAlphabets;
      conn = null;
      rset = null;
      storage = null;
      firstError = null;
      boolean error = true;
      try {
        PreparedStatement ps = null;
        conn = ds.getConnection();
        try {
          ps = conn.prepareStatement(SELECT_LISTING);
          ps.setLong(1, listingId);
          rset = ps.executeQuery();
          if (!rset.next()) return;
          listingName = rset.getString(1);
          listingDescription = rset.getString(2);
        } finally {
          if (rset != null) {
            try {
              rset.close();
            } catch (SQLException e) { /* ignore */ }
            rset = null;
          }
          if (ps != null) {
            try {
              ps.close();
            } catch (SQLException e) { /* ignore */ }
          }
        }
        ps = conn.prepareStatement(SELECT_SEQUENCE_FILES_OF_LISTING);
        ps.setLong(1, listingId);
        ps.setInt(2, SQL.enumsToInt(allowedAlphabets));
        ps.setBoolean(3, shortOnly);
        rset =  ps.executeQuery();
        if (!rset.next()) return;
        storage = loadNext();
        error = false;
      } catch (SQLException e) {
        logger.log(Level.WARNING, "Problem accessing db", e);
        if (firstError == null) firstError = e;
      } finally {
        if (error) close();
      }
    }

    private ResultSet query() throws SQLException {
      PreparedStatement ps;
      ps = conn.prepareStatement(SELECT_SEQUENCE_FILES_OF_LISTING);
      ps.setLong(1, listingId);
      ps.setInt(2, SQL.enumsToInt(allowedAlphabets));
      ps.setBoolean(3, shortOnly);

      return ps.executeQuery();
    }

    private SequenceDB loadSequenceFile() throws SQLException {
      long id = rset.getLong(1);
      Alphabet alphabet = Alphabet.fromInt(rset.getInt(2));
      long edition = rset.getLong(3);
      String version = rset.getString(4);
      String description = rset.getString(5);
      String fileSeq = rset.getString(6);
      String fileBg = rset.getString(7);
      long sequenceCount = rset.getLong(8);
      long totalLen = rset.getLong(9);
      long minLen = rset.getLong(10);
      long maxLen = rset.getLong(11);
      double avgLen = rset.getDouble(12);
      double stdDLen = rset.getDouble(13);
      return new SequenceDB(listingName, listingDescription, id, alphabet,
          edition, version, description, fileSeq, fileBg, sequenceCount,
          totalLen, minLen, maxLen, avgLen, stdDLen);
    }

    private SequenceVersion loadNext() throws SQLException {
      boolean hasRow;
      long edition;
      String version;
      SequenceDB sequenceDB = loadSequenceFile();
      edition = sequenceDB.getEdition();
      version = sequenceDB.getVersion();
      List<SequenceDB> files = new ArrayList<SequenceDB>(3);
      files.add(sequenceDB);
      while ((hasRow = rset.next())) {
        sequenceDB = loadSequenceFile();
        if (sequenceDB.getEdition() != edition) break;
        files.add(sequenceDB);
      }
      if (!hasRow) {
        close();
      }
      return new SequenceVersion(edition, version, files);
    }

    @Override
    public void close() {
      if (rset != null) {
        try {
          rset.close();
        } catch (SQLException e) {
          logger.log(Level.WARNING, "Problem closing result set", e);
          if (firstError == null) firstError = e;
        }
        rset = null;
      }
      if (conn != null) {
        try {
          conn.close();
        } catch (SQLException e) {
          logger.log(Level.WARNING, "Problem closing connection to db", e);
          if (firstError == null) firstError = e;
        }
        conn = null;
      }
    }

    @Override
    public boolean hasError() {
      return (firstError != null);
    }

    @Override
    public SQLException getError() {
      return firstError;
    }

    @Override
    public boolean hasNext() {
      return (storage != null);
    }

    @Override
    public SequenceVersion next() {
      if (storage == null) return null;
      SequenceVersion value = storage;
      storage = null;
      if (rset != null) {
        try {
          storage = loadNext();
        } catch (SQLException e) {
          logger.log(Level.WARNING, "Problem retrieving next item from db", e);
          if (firstError == null) firstError = e;
          close();
        }
      }
      return value;  // generated code
    }

    @Override
    public void remove() {
      throw new UnsupportedOperationException("Remove is not supported");
    }
  }
}
