package au.edu.uq.imb.memesuite.io.html;


import au.edu.uq.imb.memesuite.io.json.JsonHandlerAdapter;

/**
 * An implementation of HDataHandler which does nothing.
 */
public class HDataHandlerAdapter extends JsonHandlerAdapter implements HDataHandler {
  public void htmlHiddenData(String name, String value) {}
}