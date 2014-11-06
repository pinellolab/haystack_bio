package au.edu.uq.imb.memesuite.io.json;

public interface JsonHandler {
  /**
   * This indicates that the start of a JSON section
   * has been detected with the given name.
   * @param name the name of the JSON section.
   */
  void jsonStartData(String name);
  /**
   * This indicates that the end of a JSON section
   * has been detected.
   */
  void jsonEndData();
  /**
   * This indicates the start of a JSON object.
   */
  void jsonStartObject();
  /**
   * This indicates the end of a JSON object.
   */
  void jsonEndObject();
  /**
   * This indicates the start of a JSON list.
   */
  void jsonStartList();
  /**
   * This indicates the end of a JSON list.
   */
  void jsonEndList();
  /**
   * This indicates the start of a property.
   * @param name the name of the property.
   */
  void jsonStartProperty(String name);
  /**
   * This indicates the end of a property.
   */
  void jsonEndProperty();
  /**
   * A JSON value which can be null or one of the types String, Double or Boolean.
   * @param value the value of the parameter or list entry.
   */
  void jsonValue(Object value);
  /**
   * This indicates that an error has occured and no more of the current data
   * section will be parsed.
   */
  void jsonError();
}
