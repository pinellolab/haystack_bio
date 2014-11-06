package au.edu.uq.imb.memesuite.io.json;

/**
 * A token representing part of a JSON document.
 */
public class JsonToken {
  // the type of the token
  private JsonType type;
  // the value of the token if it is a string, number, boolean or null.
  private Object value;

  /**
   * Construct a token given the type of the token.
   * Boolean, number or string token can not be constructed this way.
   * @param type the type of the token.
   * @throws IllegalArgumentException if the type requires a value.
   */
  public JsonToken(JsonType type) {
    switch (type) {
      case ILLEGAL:
      case STARTOBJ:
      case ENDOBJ:
      case STARTLST:
      case ENDLST:
      case COLON:
      case COMMA:
      case NULL:
        this.value = null;
        break;
      case TRUE:
        this.value = true;
        break;
      case FALSE:
        this.value = false;
        break;
      case NUMBER:
      case STRING:
        throw new IllegalArgumentException("Can't construct boolean, " +
            "number or string token without the value.");
    }
    this.type = type;
  }

  /**
   * Construct a boolean token.
   * @param value a boolean value.
   */
  public JsonToken(boolean value) {
    this.type = (value ? JsonType.TRUE : JsonType.FALSE);
    this.value = value;
  }

  /**
   * Construct a number token.
   * @param value a numeric value.
   */
  public JsonToken(double value) {
    this.type = JsonType.NUMBER;
    this.value = value;
  }

  /**
   * Construct a number token.
   * @param value a numeric value.
   */
  public JsonToken(long value) {
    this.type = JsonType.NUMBER;
    this.value = value;
  }

  /**
   * Construct a string token.
   * @param value a string value.
   */
  public JsonToken(String value) {
    if (value == null) throw new NullPointerException("String value must not be null.");
    this.type = JsonType.STRING;
    this.value = value;
  }

  /**
   * Get the token type.
   * @return the token type.
   */
  public JsonType getType() {
    return this.type;
  }

  /**
   * Get the value of a string token.
   * @return the value of the string token.
   * @throws IllegalStateException if the token is not a string token.
   */
  public String getString() {
    if (this.type != JsonType.STRING) throw new IllegalStateException("The type of the token is not 'STRING'.");
    if (this.value instanceof String) {
      return (String)this.value;
    } else {
      throw new Error("Impossible!");
    }
  }

  /**
   * Get the value of a number token.
   * @return the value of the number token.
   * @throws IllegalStateException if the token is not a number token.
   */
  public double getNumber() {
    if (this.type != JsonType.NUMBER) throw new IllegalStateException("The type of the token is not 'NUMBER'.");
    if (this.value instanceof Number) {
      return ((Number)this.value).doubleValue();
    } else {
      throw new Error("Impossible!");
    }
  }

  /**
   * Get the value of a boolean token.
   * @return the value of the boolean token.
   * @throws IllegalStateException if the token is not a boolean token.
   */
  public boolean getBoolean() {
    if (this.type == JsonType.TRUE) {
      return true;
    } else if (this.type == JsonType.FALSE) {
      return false;
    } else {
      throw new IllegalStateException("The type of the token is not 'TRUE' or 'FALSE'.");
    }
  }

  /**
   * Get the value of a token.
   * @return the value of the token or null if the token has no value.
   */
  public Object getValue() {
    return this.value;
  }
}
