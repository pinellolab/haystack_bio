package au.edu.uq.imb.memesuite.db;

import java.util.EnumSet;
import java.util.Set;

public final class SQL {
  // DB version queries
  /** max length allowed in short sequence category */
  private static final int SML = 1000;
  /** the version of the database schema */
  public static final int SCHEMA_VERSION = 2;
  /** Set the database version */
  public static final String DECLARE_VERSION =
    "PRAGMA user_version = " + SCHEMA_VERSION;
  /** Get the database schema version */
  public static final String QUERY_VERSION =
    "PRAGMA user_version";
  // CREATE tables
  /** Create a table to store categories. */
  public static final String CREATE_TBL_CATEGORY = 
    "CREATE TABLE tblCategory (\n" +
    "  id INTEGER PRIMARY KEY,\n" +
    "  name TEXT UNIQUE NOT NULL\n" +
    ")";
  /** Create a table to store listings. */
  public static final String CREATE_TBL_LISTING = 
    "CREATE TABLE tblListing (\n" +
    "  id INTEGER PRIMARY KEY,\n" +
    "  categoryId INTEGER NOT NULL REFERENCES tblCategory (id),\n" +
    "  name TEXT NOT NULL,\n" +
    "  description TEXT NOT NULL,\n" +
    "  UNIQUE (categoryId, name)\n" +
    ")";
  /** Create a table to store information about a motif file */
  public static final String CREATE_TBL_MOTIF_FILE =
    "CREATE TABLE tblMotifFile (\n" +
    "  id INTEGER PRIMARY KEY,\n" +
    "  fileName TEXT UNIQUE NOT NULL,\n" +
    "  alphabet INTEGER NOT NULL CHECK (alphabet IN (1, 2, 4)),\n" +
    "  motifCount INTEGER NOT NULL,\n" +
    "  totalCols INTEGER NOT NULL,\n" +
    "  minCols INTEGER NOT NULL,\n" +
    "  maxCols INTEGER NOT NULL,\n" +
    "  avgCols REAL NOT NULL,\n" +
    "  stdDCols REAL NOT NULL\n" +
    ")";
  /** Create a table to store the links between a listing and a motif file */
  public static final String CREATE_TBL_LISTING_MOTIF =
    "CREATE TABLE tblListingMotif (\n" +
    "  listingId INTEGER NOT NULL REFERENCES tblListing (id),\n" +
    "  motifFileId INTEGER NOT NULL REFERENCES tblMotifFile (id),\n" +
    "  PRIMARY KEY (listingId, motifFileId)\n" +
    ")";
  /** Create a table to store information about a sequence file */
  public static final String CREATE_TBL_SEQUENCE_FILE =
    "CREATE TABLE tblSequenceFile (\n" +
    "  id INTEGER PRIMARY KEY,\n" +
    "  retriever INTEGER NOT NULL,\n" +
    "  listingId INTEGER NOT NULL REFERENCES tblListing (id),\n" +
    "  alphabet INTEGER NOT NULL CHECK (alphabet IN (1, 2, 4)),\n" +
    "  edition INTEGER NOT NULL,\n" +
    "  version TEXT NOT NULL,\n" +
    "  description TEXT NOT NULL,\n" +
    "  fileSeq TEXT UNIQUE NOT NULL,\n" +
    "  fileBg TEXT UNIQUE NOT NULL,\n" +
    "  sequenceCount INTEGER NOT NULL,\n" +
    "  totalLen INTEGER NOT NULL,\n" +
    "  minLen INTEGER NOT NULL,\n" +
    "  maxLen INTEGER NOT NULL,\n" +
    "  avgLen REAL NOT NULL,\n" +
    "  stdDLen REAL NOT NULL,\n" +
    "  obsolete INTEGER DEFAULT 0,\n" +
    "  UNIQUE (listingId, alphabet, edition)\n" +
    ")";

  /** Create a table to store information about a GOMO database */
  public static final String CREATE_TBL_GOMO_PRIMARY =
    "CREATE TABLE tblGomoPrimary (\n" +
    "  listingId INTEGER REFERENCES tblListing (id),\n" +
    "  fileGoMap TEXT UNIQUE NOT NULL,\n" +
    "  fileSeq TEXT UNIQUE NOT NULL,\n" +
    "  fileBg TEXT UNIQUE NOT NULL,\n" +
    "  promoterStart INTEGER NOT NULL,\n" +
    "  promoterStop INTEGER NOT NULL,\n" +
    "  PRIMARY KEY (listingId)\n" +
    ")";

  public static final String CREATE_TBL_GOMO_SECONDARY =
    "CREATE TABLE tblGomoSecondary (\n" +
    "  listingId INTEGER REFERENCES tblListing (id),\n" +
    "  rank INTEGER NOT NULL,\n" +
    "  fileSeq TEXT UNIQUE NOT NULL,\n" +
    "  fileBg TEXT UNIQUE NOT NULL,\n" +
    "  description TEXT NOT NULL,\n" +
    "  PRIMARY KEY (listingId, rank)\n" +
    ")";


  // INSERT rows
  /** Insert a row describing a category */
  public static final String INSERT_CATEGORY =
    "INSERT INTO tblCategory (name) VALUES (?)";
  /** Insert a row describing a listing */
  public static final String INSERT_LISTING = 
    "INSERT INTO tblListing\n" +
    "  (categoryId, name, description)\n" +
    "  VALUES (?, ?, ?)";
  /** Insert a row describing a motif file */
  public static final String INSERT_MOTIF_FILE =
    "INSERT INTO tblMotifFile\n" +
    "  (fileName, alphabet, motifCount, totalCols,\n" +
    "      minCols, maxCols, avgCols, stdDCols)\n" +
    "VALUES (?, ?, ?, ?, ?, ?, ?, ?)";
  /** Insert a row linking a motif file to a motif listing */
  public static final String INSERT_LISTING_MOTIF =
    "INSERT INTO tblListingMotif (listingId, motifFileId)\n" +
    "VALUES (?, ?)";
  public static final String INSERT_SEQUENCE_FILE =
    "INSERT INTO tblSequenceFile\n" +
    "  (retriever, listingId, alphabet, edition, version, description,\n" +
    "      fileSeq, fileBg, sequenceCount, totalLen, minLen, maxLen,\n" +
    "      avgLen, stdDLen)\n" +
    "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)";
  /** Insert a row describing gomo input files */
  public static final String INSERT_GOMO_PRIMARY =
    "INSERT INTO tblGomoPrimary\n" +
    "  (listingId, fileGoMap, fileSeq, fileBg,\n" +
    "      promoterStart, promoterStop)\n" +
    "VALUES (?, ?, ?, ?, ?, ?)";

  public static final String INSERT_GOMO_SECONDARY =
    "INSERT INTO tblGomoSecondary\n" +
    "  (listingId, rank, fileSeq, fileBg, description)\n" +
    "VALUES (?, ?, ?, ?, ?)";

  // UPDATE things

  public static final String MARK_OLD_LISTING_FILES_OBSOLETE =
    "UPDATE tblSequenceFile SET obsolete = 1\n" +
    "WHERE listingId = ? AND alphabet = ? AND id NOT IN (\n" +
    "  SELECT id FROM tblSequenceFile\n" +
    "  WHERE listingId = ? AND alphabet = ? ORDER BY edition DESC LIMIT ?\n" +
    ")";

  public static final String MARK_RETRIEVER_FILES_FLAGGED =
    "UPDATE tblSequenceFile SET obsolete = 2 WHERE retriever = ? AND obsolete = 0";

  public static final String MARK_RETRIEVER_LISTING_OK =
    "UPDATE tblSequenceFile SET obsolete = 0 WHERE retriever = ? AND listingId = ? AND obsolete = 2";

  public static final String MARK_FLAGGED_RETRIEVER_FILES_OBSOLETE =
    "UPDATE tblSequenceFile SET obsolete = 1 WHERE retriever = ? AND obsolete = 2";

  public static final String MARK_SEQUENCE_FILE =
    "UPDATE tblSeqenceFile SET obsolete = ? WHERE id = ?";

  // COUNT things
  public static final String COUNT_CATEGORIES =
    "SELECT COUNT(*) FROM tblCategory";

  public static final String COUNT_ALL_LISTINGS =
    "SELECT COUNT(*) FROM tblListing";

  public static final String COUNT_CATEGORY_LISTINGS =
    "SELECT COUNT(*) FROM tblListing WHERE categoryId = ?";

  // SELECT things
  /** Select all categories */
  public static final String SELECT_UNSORTED_CATEGORIES =
    "SELECT id, name FROM tblCategory";
  /* the sequence databases sort the categories by name and filter by obsolete */
  public static final String SELECT_SEQUENCE_CATEGORIES =
    "SELECT id, name FROM tblCategory WHERE id IN (\n" +
     "  SELECT DISTINCT categoryId FROM tblListing WHERE id IN (\n" +
     "    SELECT DISTINCT listingId FROM tblSequenceFile WHERE obsolete = 0\n" +
     "  )\n" +
     ") ORDER BY name ASC";
  public static final String SELECT_CATEGORY_BY_NAME =
     "SELECT id FROM tblCategory WHERE name = ?";
  /** Select listings of a sequence category */
  public static final String SELECT_LISTINGS_OF_CATEGORY =
      "SELECT id, name, description, 7 AS alphabets FROM tblListing WHERE categoryId = ?";
  /** Select listings of a motif category */
  public static final String SELECT_MOTIF_LISTINGS_OF_CATEGORY =
    "SELECT l.id, l.name, l.description,\n" +
    "    MAX(m.alphabet & 1) + MAX(m.alphabet & 2) + MAX(m.alphabet & 4) AS alphabets\n" +
    "FROM tblListing AS l\n" +
    "  INNER JOIN tblListingMotif AS lm ON l.id = lm.listingId\n" +
    "  INNER JOIN tblMotifFile AS m ON m.id = lm.motifFileId\n" +
    "WHERE l.categoryId = ? AND (m.alphabet & ?) != 0\n" +
    "  GROUP BY l.id";
  /** Select listings of a sequence category */
  public static final String SELECT_SEQUENCE_LISTINGS_OF_CATEGORY =
    "SELECT l.id, l.name, l.description,\n" +
    "    MAX(s.alphabet & 1) + MAX(s.alphabet & 2) + MAX(s.alphabet & 4) AS alphabets\n" +
    "FROM tblListing AS l\n" +
    "  INNER JOIN tblSequenceFile as s ON l.id = s.listingId\n" +
    "WHERE l.categoryId = ? AND s.obsolete = 0 AND (s.alphabet & ?) != 0 AND (NOT ? OR s.maxLen < "+SML+")\n" +
    "  GROUP BY l.id";
  public static final String SELECT_LISTING =
    "SELECT name, description\n" +
    "FROM tblListing WHERE id = ?";
  public static final String SELECT_LISTING_BY_NAME =
    "SELECT id, name, description\n" +
    "FROM tblListing WHERE categoryId = ? AND name = ?";

  /** select motifs of a listing */
  public static final String SELECT_MOTIFS_OF_LISTING =
    "SELECT m.id, m.fileName, m.alphabet, m.motifCount, m.totalCols,\n" +
    "    m.minCols, m.maxCols, m.avgCols, m.stdDCols\n" +
    "FROM tblMotifFile AS m INNER JOIN tblListingMotif AS l\n" +
    "    ON m.id = l.motifFileId\n" +
    "WHERE l.listingId = ?";
  public static final String SELECT_MOTIF_FILE_ID = 
    "SELECT id FROM tblMotifFile WHERE fileName = ?";

  public static final String SELECT_SEQUENCE_FILES_OF_LISTING =
    "SELECT id, alphabet, edition, version, description, fileSeq, fileBg,\n" +
    "    sequenceCount, totalLen, minLen, maxLen, avgLen, stdDLen\n" +
    "FROM tblSequenceFile\n" +
    "WHERE listingId = ? AND obsolete = 0 AND (alphabet & ?) != 0 AND (NOT ? OR maxLen < "+SML+")\n" +
    "  ORDER BY edition DESC, alphabet ASC";
  /* used to return a sequence file result */
  public static final String SELECT_SEQUENCE_FILE_OF_LISTING =
      "SELECT id, alphabet, version, description, fileSeq, fileBg,\n" +
      "    sequenceCount, totalLen, minLen, maxLen, avgLen, stdDLen\n" +
      "FROM tblSequenceFile\n" +
      "WHERE listingId = ? AND edition = ? AND (alphabet & ?) != 0";

  /* Only used for an existence check currently */
  public static final String SELECT_SEQUENCE_FILE_BY_INFO =
    "SELECT c.id, l.id, s.id, s.obsolete\n" +
    "FROM tblSequenceFile as s\n" +
    "  INNER JOIN tblListing AS l ON s.listingId = l.id\n" +
    "  INNER JOIN tblCategory AS c ON l.categoryId = c.id\n" +
    "WHERE c.name = ? AND l.name = ? AND s.alphabet & ? != 0 AND s.edition = ?";
  /* Used when we only want the latest  */
  public static final String SELECT_SEQUENCE_FILE_BY_INFO_NEWER =
    "SELECT c.id, l.id, s.id, s.obsolete\n" +
    "FROM tblSequenceFile as s\n" +
    "  INNER JOIN tblListing AS l ON s.listingId = l.id\n" +
    "  INNER JOIN tblCategory AS c ON l.categoryId = c.id\n" +
    "WHERE c.name = ? AND l.name = ? AND s.alphabet & ? != 0 AND s.edition > ?\n" +
    "ORDER BY s.edition DESC LIMIT 1";

  /** So we can check that the listed files actually exist */
 public static final String SELECT_ALL_SEQUENCE_FILES =
     "SELECT id, fileSeq, fileBg FROM tblSequenceFile";
  /** So we can check that the listed files actually exist */
 public static final String SELECT_ALL_OBSOLETE_SEQUENCE_FILES =
     "SELECT fileSeq, fileBg FROM tblSequenceFile WHERE obsolete != 0";

  public static final String SELECT_GOMO_PRIMARY_OF_LISTING =
    "SELECT l.name, l.description, g.promoterStart, g.promoterStop,\n" +
    "    g.fileGoMap, g.fileSeq, g.fileBg\n" +
    "FROM tblListing as l\n" +
    "  INNER JOIN tblGomoPrimary AS g ON l.id = g.listingId\n" +
    "WHERE l.id = ?";

  public static final String SELECT_GOMO_SECONDARIES_OF_LISTING =
    "SELECT description, fileSeq, fileBg\n" +
    "FROM tblGomoSecondary\n" +
    "WHERE listingId = ? ORDER BY rank ASC";

  /* Delete a sequence file entry */
  public static final String DELETE_SEQUENCE_FILE =
    "DELETE FROM tblSequenceFile WHERE id = ?";


  /* Delete a sequence file entry */
  public static final String DELETE_OBSOLETE_SEQUENCE_FILES =
    "DELETE FROM tblSequenceFile WHERE obsolete != 0";

  /* Delete listings without any Sequences */
  public static final String DELETE_LISTING_WITHOUT_SEQUENCE_FILE =
    "DELETE FROM tblListing WHERE id NOT IN (\n" +
    "  SELECT DISTINCT listingId\n" +
    "  FROM tblSequenceFile\n" +
    ")";

  /* Delete categories without any listings */
  public static final String DELETE_CATEGORY_WITHOUT_LISTING =
    "DELETE FROM tblCategory WHERE id NOT IN (\n" +
    "  SELECT DISTINCT categoryId\n" +
    "  FROM tblListing\n" +
    ")";

  /**
   * Converts a EnumSet into a integer bitmask suitable for storing in a database.
   * Uses the ordinal position of each enum as the bit position.
   * @param set the EnumSet to convert.
   * @return a bitmask with a 1 set at the ordinal of each enum in the set.
   * @see Enum#ordinal() ordinal
   */
  public static <E extends Enum<E>> int enumsToInt(Set<E> set) {
    int bitmask = 0;
    for (Enum e : set) {
      if (e.ordinal() > 31) throw new IndexOutOfBoundsException(
          "Can not set more than 31 bits in an integer bitmask");
      bitmask |= (1 << e.ordinal());
    }
    return bitmask;
  }

  /**
   * Creates a EnumSet populated with Enum constants that have an ordinal equal
   * to set bit positions in a bitmask. The sign bit of the mask can not be used
   * so this is limited to enumerations with 31 or less components.
   * @param enumClass the class of the enumeration.
   * @param bitmask the bitmask to convert.
   * @param <E> the type of the enumeration.
   * @return a enumeration conversion of the bitmask.
   */
  public static <E extends Enum<E>> EnumSet<E> intToEnums(Class<E> enumClass, int bitmask) {
    if (bitmask < 0) throw new IllegalArgumentException("Negative bitmasks are not allowed.");
    E[] values = enumClass.getEnumConstants();
    EnumSet<E> set = EnumSet.noneOf(enumClass);
    int max = Integer.highestOneBit(bitmask);
    for (int i = 0, bit = 1; i < values.length && bit <= max; i++, bit *= 2) {
      if ((bitmask & bit) != 0) set.add(values[i]);
    }
    return set;
  }

  private SQL() {}
}
