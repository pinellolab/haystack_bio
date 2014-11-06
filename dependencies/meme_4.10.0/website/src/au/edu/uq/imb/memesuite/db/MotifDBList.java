package au.edu.uq.imb.memesuite.db;

import java.io.*;
import java.util.ArrayList;
import java.util.logging.Level;
import java.util.logging.Logger;
import java.util.regex.*;
import java.sql.*;

import au.edu.uq.imb.memesuite.data.Alphabet;
import au.edu.uq.imb.memesuite.data.MotifStats;
import au.edu.uq.imb.memesuite.servlet.util.MotifValidator;
import au.edu.uq.imb.memesuite.util.GlobFilter;
import org.sqlite.*;

import static au.edu.uq.imb.memesuite.db.SQL.SELECT_MOTIF_LISTINGS_OF_CATEGORY;

public class MotifDBList extends DBList {
  private File csv;

  protected static Logger logger = Logger.getLogger("au.edu.uq.imb.memesuite.motifdb");

  public MotifDBList(File csv, File motifsDir) throws ClassNotFoundException, IOException, SQLException {
    super(loadMotifCSV(csv, motifsDir), true);
    this.csv = csv;
  }

  /**
   * Get the source file that the database was created from.
   * @return the csv file that was loaded to create the database.
   */
  public File getCSV() {
    return csv;
  }

  /**
   * Does a search for motif listings.
   * @param conn the connection.
   * @param categoryId the category.
   * @param allowedAlphabets the allowed alphabets.
   * @param shortOnly this option is ignored for motifs.
   * @return a result set of motif listings.
   * @throws SQLException
   */
  @Override
  protected ResultSet queryListings(Connection conn, long categoryId,
      int allowedAlphabets, boolean shortOnly) throws SQLException {
    PreparedStatement ps = conn.prepareStatement(SELECT_MOTIF_LISTINGS_OF_CATEGORY);
    ps.setLong(1, categoryId);
    ps.setInt(2, allowedAlphabets);
    return ps.executeQuery();
  }

  public MotifDB getMotifListing(long listingId) throws SQLException {
    Connection connection = null;
    MotifDB motifList;
    try {
      PreparedStatement stmt;
      ResultSet rset;
      // create a database connection
      connection = ds.getConnection();
      // get the name and description
      stmt = connection.prepareStatement(SQL.SELECT_LISTING);
      stmt.setLong(1, listingId);
      rset = stmt.executeQuery();
      if (!rset.next()) throw new SQLException("No listing by that ID");
      String name = rset.getString(1);
      String description = rset.getString(2);
      rset.close();
      stmt.close();
      // get the motif files
      stmt = connection.prepareStatement(SQL.SELECT_MOTIFS_OF_LISTING);
      stmt.setLong(1, listingId);
      rset = stmt.executeQuery();
      ArrayList<MotifDBFile> files = new ArrayList<MotifDBFile>();
      while (rset.next()) {
        long id = rset.getLong(1);
        String fileName = rset.getString(2);
        Alphabet alphabet = Alphabet.fromInt(rset.getInt(3));
        int motifCount = rset.getInt(4);
        int totalCols = rset.getInt(5);
        int minCols = rset.getInt(6);
        int maxCols = rset.getInt(7);
        double avgCols = rset.getDouble(8);
        double stdDCols = rset.getDouble(9);
        files.add(new MotifDBFile(id, fileName,
              alphabet, motifCount, totalCols,
              minCols, maxCols, avgCols, stdDCols));
      }
      rset.close();
      stmt.close();
      connection.close();
      connection = null;
      motifList = new MotifDB(name, description, files);
    } finally {
      try {
        if(connection != null) connection.close();
      } catch(SQLException e) {
        // connection close failed.
        logger.log(Level.SEVERE, "Cleanup (after error) failed to close the DB connection", e);
      }
    }
    return motifList;
  }

  private static File loadMotifCSV(File csv, File motifsDir) throws ClassNotFoundException, IOException, SQLException {
    // load the JDBC needed
    Class.forName("org.sqlite.JDBC");
    // create the file to contain the database
    File db = File.createTempFile("motif_db_", ".sqlite");
    db.deleteOnExit();
    // configure the database
    SQLiteConfig config = new SQLiteConfig();
    config.enforceForeignKeys(true);
    SQLiteDataSource ds = new SQLiteDataSource(config);
    ds.setUrl("jdbc:sqlite:" + db);
    // open a connection
    Connection connection = null;
    try {
      connection = ds.getConnection();
      Statement statement = connection.createStatement();
      statement.executeUpdate(SQL.CREATE_TBL_CATEGORY);
      statement.executeUpdate(SQL.CREATE_TBL_LISTING);
      statement.executeUpdate(SQL.CREATE_TBL_MOTIF_FILE);
      statement.executeUpdate(SQL.CREATE_TBL_LISTING_MOTIF);
      connection.setAutoCommit(false);
      importMotifCSV(csv, motifsDir, connection);
      connection.commit();
    } finally {
      if (connection != null) {
        try {
          connection.close();
        } catch (SQLException e) { /* ignore */ }
      }
    }
    if (!db.setLastModified(csv.lastModified())) {
      logger.log(Level.WARNING, "Failed to set last modified date on " + db);
    }
    logger.log(Level.INFO, "Loaded Motif CSV \"" + csv + "\" into \"" + db + "\"");
    return db;
  }

  protected static void importMotifCSV(File csv, File motifsDir, Connection connection) throws IOException, SQLException {
    logger.log(Level.INFO, "Importing motif csv from \"" + csv +
        "\" expecting motifs to be in \"" + motifsDir + "\"");
    String line;
    Long categoryId = null;
    Pattern emptyLine = Pattern.compile("^\\s*$");
    Pattern commentLine = Pattern.compile("^#");
    Pattern dashedName = Pattern.compile("^-*([^-](?:.*[^-])?)-*$");
    BufferedReader in = null;
    try {
      ResultSet rs;
      // open the csv file for reading
      in = new BufferedReader(new InputStreamReader(new FileInputStream(csv), "UTF-8"));
      // create the prepared statements
      PreparedStatement pstmtCategory = connection.prepareStatement(
          SQL.INSERT_CATEGORY, Statement.RETURN_GENERATED_KEYS);
      PreparedStatement pstmtListing = connection.prepareStatement(
          SQL.INSERT_LISTING, Statement.RETURN_GENERATED_KEYS);
      PreparedStatement pstmtMotifFile = connection.prepareStatement(
          SQL.INSERT_MOTIF_FILE, Statement.RETURN_GENERATED_KEYS);
      PreparedStatement pstmtListingMotif = connection.prepareStatement(
          SQL.INSERT_LISTING_MOTIF);
      PreparedStatement pstmtFindMotifFileId = connection.prepareStatement(
          SQL.SELECT_MOTIF_FILE_ID);
      // now read the csv file
      while ((line = in.readLine()) != null) {
        // skip any empty lines or comments
        if (emptyLine.matcher(line).find()) continue;
        if (commentLine.matcher(line).find()) continue;
        line = line.trim();
        // check we have enough items on the line to do something
        String[] values = line.split("\\s*,\\s*");
        if (values.length < 5) {
          logger.log(Level.WARNING,"CSV line has " + values.length + " values but expected at least 5.");
          continue;
        }
        // check that a name was supplied
        if (emptyLine.matcher(values[4]).find()) {
          logger.log(Level.WARNING, "CSV line has no entry for name column.");
          continue;
        }
        // test to see if we have a category or a selectable listing
        if (emptyLine.matcher(values[0]).find()) {
          // category
          String name = values[4].trim();
          // remove dashes from around name
          Matcher matcher = dashedName.matcher(name);
          if (matcher.matches()) {
            name = matcher.group(1);
          }
          pstmtCategory.setString(1, name);
          pstmtCategory.executeUpdate();
          // now get the category Id
          rs = pstmtCategory.getGeneratedKeys();
          if (!rs.next()) throw new SQLException("Failed to get Category Id.\n");
          categoryId = rs.getLong(1);
          rs.close();
          logger.log(Level.FINE, "Loaded Motif Category: " + name);
        } else {
          // listing
          // check we have a category to store the listing under
          if (categoryId == null) {
            // create a dummy category with an empty name
            pstmtCategory.setString(1, "");
            pstmtCategory.executeUpdate();
            // now get the category Id
            rs = pstmtCategory.getGeneratedKeys();
            if (!rs.next()) throw new SQLException("Failed to get Category Id.\n");
            categoryId = rs.getLong(1);
            rs.close();
          }
          // now create the listing
          String filesPattern = values[0];
          String name = values[4];
          String description = (values.length > 5 ? values[5] : "");
          pstmtListing.setLong(1, categoryId);
          pstmtListing.setString(2, name);
          pstmtListing.setString(3, description);
          pstmtListing.executeUpdate();
          // now get the listing Id
          rs = pstmtListing.getGeneratedKeys();
          if (!rs.next()) throw new SQLException("Failed to get Listing Id.\n");
          long listingId = rs.getLong(1);
          pstmtListingMotif.setLong(1, listingId);
          rs.close();
          logger.log(Level.FINE, "Loaded Motif Listing: " + name);
          // get the list of files that match
          File[] motifFiles = motifsDir.listFiles(new GlobFilter(filesPattern));
          // add each file to the database
          for (File motifFile : motifFiles) {
            // see if the file was already added
            pstmtFindMotifFileId.setString(1, motifFile.getName());
            rs = pstmtFindMotifFileId.executeQuery();
            Long motifFileId = null;
            if (rs.next()) {
              motifFileId = rs.getLong(1);
            }
            rs.close();
            if (motifFileId == null) {
              //get details about the motif
              MotifStats stats = null;
              try {
                stats = MotifValidator.validate(motifFile);
              } catch (IOException ignored) { }
              if (stats == null) {
                logger.log(Level.WARNING, "Failed to validate motifs in file: " + motifFile);
                continue;
              }
              // store details about the motif file
              pstmtMotifFile.setString(1, motifFile.getName());
              pstmtMotifFile.setInt(2, stats.getAlphabet().toInt());
              pstmtMotifFile.setInt(3, stats.getMotifCount());
              pstmtMotifFile.setInt(4, stats.getTotalCols());
              pstmtMotifFile.setInt(5, stats.getMinCols());
              pstmtMotifFile.setInt(6, stats.getMaxCols());
              pstmtMotifFile.setDouble(7, stats.getAverageCols());
              pstmtMotifFile.setDouble(8, stats.getStandardDeviationCols());
              pstmtMotifFile.executeUpdate();
              // now get the motif file id
              rs = pstmtListing.getGeneratedKeys();
              if (!rs.next()) throw new SQLException("Failed to get Motif File Id.\n");
              motifFileId = rs.getLong(1);
              rs.close();
              logger.log(Level.FINE, "Loaded Motif File: " + motifFile.getName());
            }
            // now link the listing to the motif file
            pstmtListingMotif.setLong(2, motifFileId);
            pstmtListingMotif.executeUpdate();
          }
        }
      }
      // close the prepared statements
      pstmtCategory.close();
      pstmtListing.close();
      pstmtMotifFile.close();
      pstmtListingMotif.close();
      pstmtFindMotifFileId.close();
    } finally {
      if (in != null) {
        try {
          in.close();
        } catch (IOException e) { /* ignore */ }
      }
    }
  }

}
