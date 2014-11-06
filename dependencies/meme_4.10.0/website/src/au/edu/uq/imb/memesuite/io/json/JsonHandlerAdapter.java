package au.edu.uq.imb.memesuite.io.json;

/*
 * Default JSON handler which does nothing.
 */
public class JsonHandlerAdapter implements JsonHandler {
    public void jsonStartData(String name){}
    public void jsonEndData(){}
    public void jsonStartObject(){}
    public void jsonEndObject(){}
    public void jsonStartList(){}
    public void jsonEndList(){}
    public void jsonStartProperty(String name){}
    public void jsonEndProperty(){}
    public void jsonValue(Object value){}
    public void jsonError(){}
}