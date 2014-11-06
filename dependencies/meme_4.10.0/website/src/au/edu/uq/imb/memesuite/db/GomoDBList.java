package au.edu.uq.imb.memesuite.db;

import java.io.*;
import java.util.ArrayList;
import java.util.List;
import java.util.logging.Level;
import java.util.logging.Logger;
import java.util.regex.*;
import java.sql.*;

import org.sqlite.*;

public class GomoDBList extends DBList {
  private File csv;

  private static Logger logger = Logger.getLogger("au.edu.uq.imb.memesuite.web");

  public GomoDBList(File csv) throws ClassNotFoundException, IOException, SQLException {
    super(loadGomoCSV(csv), true);
    this.csv = csv;
  }

  /**
   * Get the source file that the database was created from.
   * @return the csv file that was loaded to create the database.
   */
  public File getCSV() {
    return csv;
  }

  public GomoDB getGomoListing(long listingId) throws SQLException {
    GomoDB gomoDB;
    Connection connection = null;
    try {
      PreparedStatement stmt;
      ResultSet rset;
      // create a database connection
      connection = ds.getConnection();
      stmt = connection.prepareStatement(SQL.SELECT_GOMO_PRIMARY_OF_LISTING);
      stmt.setLong(1, listingId);
      rset = stmt.executeQuery();
      if (!rset.next()) throw new SQLException("No listing by that ID");
      String name = rset.getString(1);
      String description = rset.getString(2);
      int promoterStart = rset.getInt(3);
      int promoterStop = rset.getInt(4);
      String goMapFileName = rset.getString(5);
      String sequenceFileName = rset.getString(6);
      String backgroundFileName = rset.getString(7);
      rset.close();
      stmt.close();
      List<GomoDBSecondary> secondaries = new ArrayList<GomoDBSecondary>();
      stmt = connection.prepareStatement(SQL.SELECT_GOMO_SECONDARIES_OF_LISTING);
      stmt.setLong(1, listingId);
      rset = stmt.executeQuery();
      while (rset.next()) {
        String secondaryDescription = rset.getString(1);
        String secondarySequenceFileName = rset.getString(2);
        String secondaryBackgroundFileName = rset.getString(3);
        secondaries.add(new GomoDBSecondary(secondaryDescription,
            secondarySequenceFileName, secondaryBackgroundFileName));
      }
      rset.close();
      stmt.close();
      connection.close();
      connection = null;
      gomoDB = new GomoDB(listingId, name, description, promoterStart, promoterStop,
          goMapFileName, sequenceFileName, backgroundFileName, secondaries);
    } finally {
      try {
        if(connection != null) connection.close();
      } catch(SQLException e) {
        // connection close failed.
        logger.log(Level.SEVERE, "Cleanup (after error) failed to close the DB connection", e);
      }
    }
    return gomoDB;
  }

  private static File loadGomoCSV(File csv) throws ClassNotFoundException, IOException, SQLException {
    // load the JDBC needed
    Class.forName("org.sqlite.JDBC");
    // create the file to contain the database
    File db = File.createTempFile("gomo_db_", ".sqlite");
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
      statement.executeUpdate(SQL.CREATE_TBL_GOMO_PRIMARY);
      statement.executeUpdate(SQL.CREATE_TBL_GOMO_SECONDARY);
      connection.setAutoCommit(false);
      importGomoCSV(csv, connection);
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
    logger.log(Level.INFO, "Loaded GOMO CSV \"" + csv + "\" into \"" + db + "\"");
    return db;
  }

  private static void importGomoCSV(File csv, Connection connection) throws IOException, SQLException {
    String line;
    Long categoryId = null;
    Pattern emptyLine = Pattern.compile("^\\s*$");
    Pattern commentLine = Pattern.compile("^#");
    Pattern dashedName = Pattern.compile("^-*([^-](?:.*[^-])?)-*$");
    BufferedReader in = null;
    try {
      // open the csv file for reading
      in = new BufferedReader(new InputStreamReader(new FileInputStream(csv), "UTF-8"));
      // create the prepared statements
      PreparedStatement pstmtCategory = connection.prepareStatement(
          SQL.INSERT_CATEGORY, Statement.RETURN_GENERATED_KEYS);
      PreparedStatement pstmtListing = connection.prepareStatement(
          SQL.INSERT_LISTING, Statement.RETURN_GENERATED_KEYS);
      PreparedStatement pstmtGomoPrimary = connection.prepareStatement(
          SQL.INSERT_GOMO_PRIMARY, Statement.NO_GENERATED_KEYS);
      PreparedStatement pstmtGomoSecondary = connection.prepareStatement(
          SQL.INSERT_GOMO_SECONDARY, Statement.NO_GENERATED_KEYS);
      // now read the csv file
      while ((line = in.readLine()) != null) {
        // skip any empty lines or comments
        if (emptyLine.matcher(line).find()) continue;
        if (commentLine.matcher(line).find()) continue;
        line = line.trim();
        // check we have enough items on the line to do something
        String[] values = line.split("\\s*,\\s*");
        if (values.length < 5) {
          logger.log(Level.WARNING, "CSV line has " + values.length +
              " values but expected at least 5.");
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
          ResultSet rs = pstmtCategory.getGeneratedKeys();
          if (!rs.next()) throw new SQLException("Failed to get Category Id.\n");
          categoryId = rs.getLong(1);
          rs.close();
          logger.log(Level.FINE, "Loaded GOMo Category: " + name);
        } else {
          // listing
          if (values.length < 8) {
            logger.log(Level.WARNING, "CSV line has " + values.length +
                " values but expected at least 8 for a GOMO listing.");
            continue;
          }
          String filesPattern = values[0];
          String name = values[4];
          String description = values[5];
          int promStart, promStop;
          try {
            promStart = Integer.parseInt(values[6], 10);
            promStop = Integer.parseInt(values[7], 10);
          } catch (NumberFormatException e) {
            logger.log(Level.WARNING,
                "GOMO listing doesn't have numbers for the promoter start/stop;");
            continue;
          }
          // check we have a category to store the listing under
          if (categoryId == null) {
            // create a dummy category with an empty name
            pstmtCategory.setString(1, "");
            pstmtCategory.executeUpdate();
            // now get the category Id
            ResultSet rs = pstmtCategory.getGeneratedKeys();
            if (!rs.next()) throw new SQLException("Failed to get Category Id.\n");
            categoryId = rs.getLong(1);
            rs.close();
          }
          // now create the listing
          pstmtListing.setLong(1, categoryId);
          pstmtListing.setString(2, name);
          pstmtListing.setString(3, description);
          pstmtListing.executeUpdate();
          ResultSet rs = pstmtListing.getGeneratedKeys();
          if (!rs.next()) throw new SQLException("Failed to get Listing Id.\n");
          long listingId = rs.getLong(1);
          rs.close();
          // create the GOMO primary listing
          pstmtGomoPrimary.setLong(1, listingId);
          pstmtGomoPrimary.setString(2, filesPattern + ".na.csv"); // GO Map
          pstmtGomoPrimary.setString(3, filesPattern + ".na"); // sequences
          pstmtGomoPrimary.setString(4, filesPattern + ".na.bfile"); //background
          pstmtGomoPrimary.setInt(5, promStart);
          pstmtGomoPrimary.setInt(6, promStop);
          pstmtGomoPrimary.executeUpdate();
          // create the GOMO secondary listings
          int rank = 1;
          pstmtGomoSecondary.setLong(1, listingId);
          for (int offset = 8; offset + 2 <= values.length; offset += 2) {
            String secondaryPattern = values[offset];
            String secondaryDescription = values[offset + 1];
            pstmtGomoSecondary.setInt(2, rank++);
            pstmtGomoSecondary.setString(3, secondaryPattern + ".na");
            pstmtGomoSecondary.setString(4, secondaryPattern + ".na.bfile");
            pstmtGomoSecondary.setString(5, secondaryDescription);
            pstmtGomoSecondary.executeUpdate();
          }
          logger.log(Level.FINE, "Loaded GOMo Listing: " + name);
        }
      }
      // close the prepared statements
      pstmtCategory.close();
      pstmtListing.close();
      pstmtGomoPrimary.close();
      pstmtGomoSecondary.close();
    } finally {
      if (in != null) {
        try {
          in.close();
        } catch (IOException e) {
          // ignore
        }
      }
    }
  }

  
}

