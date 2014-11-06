package au.edu.uq.imb.memesuite.io.html;

import au.edu.uq.imb.memesuite.io.json.JsonHandler;

public interface HDataHandler extends JsonHandler {
  /**
   * The content of a hidden data input from the HTML.
   * @param name the name field of the data.
   * @param value the value field of the data.
   */
  void htmlHiddenData(String name, String value);
}
