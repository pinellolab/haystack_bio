package au.edu.uq.imb.memesuite.io.json;
/*
 * An enumeration of the different types of tokens.
 */
public enum JsonType {
  ILLEGAL,
  STARTOBJ, // {
  ENDOBJ, // }
  STARTLST, // [
  ENDLST, // ]
  COLON, // :
  COMMA, // ,
  NULL, // null
  TRUE, // true
  FALSE, // false
  NUMBER,
  STRING;
}

