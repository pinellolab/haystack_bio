package au.edu.uq.imb.memesuite.template;

import java.io.IOException;
import java.io.StringWriter;
import java.io.Writer;
import java.util.*;
import java.util.Map.Entry;

/**
 * This provides a mechanism for storing substitutions that need to be made
 * into a HTMLTemplate.
 *
 * Usage Example:
 * <code>
 *  String[] names = {"James", "Tim", "Charles"};
 *  String templateSource = "&lt;html&gt;&lt;head&gt;&lt;title&gt;Hello World&lt;/title&gt;&lt;/head&gt;&lt;body&gt;&lt;!--{person}--&gt;&lt;h1&gt;Hello ${name}&lt;/h1&gt;&lt;!--{/person}--&gt;&lt;/body&gt;&lt;/html&gt;";
 *  HTMLTemplate template = new HTMLTemplate(templateSource);
 *  HTMLSub tmpl = template.toSub();
 *  List&lt;HTMLSub&gt; people = new ArrayList&lt;HTMLSub&gt;();
 *  HTMLSub person = tmpl.getSub("person");
 *  for (int i = 0; i &lt; people.length; i++) {
 *    people.add(person.set("name", names[i]).copy());
 *  }
 *  tmpl.set("person", people);
 *  tmpl.output(new BufferedWriter(new OutputStreamWriter(System.out)));
 * </code>
 */
public class HTMLSub implements Cloneable {
  /** the template that substitutions are made into */
  protected HTMLTemplate template;
  /** the replacements for subtemplates */
  protected SortedMap<String,Object> replacements;

  /**
   * Construct a HTMLSub based on a HTMLTemplate so the output
   * from the HTMLSub would be identical to that of the HTMLTemplate
   * @param template the template to base on
   */
  public HTMLSub(HTMLTemplate template) {
    this.template = template;
    this.replacements = new TreeMap<String,Object>();
  }

  /**
   * Copy constructor, copies nested HTMLSub objects too.
   */
  public HTMLSub(HTMLSub sibling) {
    this.template = sibling.template;
    this.replacements = new TreeMap<String,Object>(sibling.replacements);
    for (Entry<String,Object> item : sibling.replacements.entrySet()) {
      Object oSub = item.getValue();
      if (oSub instanceof HTMLSub) {
        this.replacements.put(item.getKey(), ((HTMLSub)oSub).copy());
      }
      // any other types should be imutable
    }
  }

  /**
   * Copys all nested HTMLSub objects.
   * @return a copy of the current object
   */
  public HTMLSub copy() {
    return new HTMLSub(this);
  }

  /**
   * Checks if this template contains a subtemplate with the given name.
   * @param name the name of the subtemplate to check exists
   * @return true if a subtemplate with the given name exists
   */
  public boolean contains(String name) {
    return this.template.containsSubtemplate(name);
  }

  /**
   * Get a HTMLSub object for the given template or return it if it already exists.
   *
   * If the replacement for the named subtemplate is already set to a HTMLSub object
   * then return it, if it is set to a HTMLTemplate object then wrap it with a
   * HTMLSub object and set it as the new replacement before returning it, if it
   * is set to a String or Iterable then complain, or if it is still unset then
   * get the subtemplate with the given name and wrap it in a HTMLSub object
   * before storing it as the replacement and returning it.
   *
   * @param name the subtemplate name
   * @return a HTMLSub object for the given subtemplate name
   * @throws IllegalArgumentException if no subtemplate with that name exists
   * @throws IllegalStateException if the subtemplate with that name has been
   * replaced with something that is not a template
   */
  public HTMLSub getSub(String name) {
    Object oSub = this.replacements.get(name);
    if (oSub != null) {
      if (oSub instanceof HTMLSub) {
        return (HTMLSub)oSub;
      } if (oSub instanceof HTMLTemplate) {
        // automatically convert any HTMLTemplate objects
        HTMLSub sub = new HTMLSub((HTMLTemplate)oSub);
        this.replacements.put(name, sub);
        return sub;
      } else {
        throw new IllegalStateException("Subtemplate already replaced with " +
            "non-template value so reset must be used first");
      }
    } else {
      HTMLSub sub = new HTMLSub(this.template.getSubtemplate(name));
      this.replacements.put(name, sub);
      return sub;
    }
  }

  /**
   * Set the subtemplate with the given name to the string value.
   * @param name the name of a subtemplate.
   * @param value the value to replace the subtemplate with.
   * @return the current HTMLSub object so these can be chained.
   * @throws IllegalArgumentException if no such subtemplate exists
   */
  public HTMLSub set(String name, String value) {
    if (name == null) throw new IllegalArgumentException("name is null");
    if (!this.template.containsSubtemplate(name)) 
      throw new IllegalArgumentException("No subtemplate with name {" + name + "}");
    if (value != null) {
      this.replacements.put(name, value);
    } else {
      this.replacements.remove(name);
    }
    return this;
  }

  /**
   * Set the subtemplate with the given name to the int value.
   * @param name the name of a subtemplate.
   * @param value the value to replace the subtemplate with.
   * @return the current HTMLSub object so these can be chained.
   * @throws IllegalArgumentException if no such subtemplate exists
   */
  public HTMLSub set(String name, int value) {
    return this.set(name, Integer.toString(value));
  }

  /**
   * Set the subtemplate with the given name to the long value.
   * @param name the name of a subtemplate.
   * @param value the value to replace the subtemplate with.
   * @return the current HTMLSub object so these can be chained.
   * @throws IllegalArgumentException if no such subtemplate exists
   */
  public HTMLSub set(String name, long value) {
    return this.set(name, Long.toString(value));
  }

  /**
   * Set the subtemplate with the given name to the double value.
   * @param name the name of a subtemplate.
   * @param value the value to replace the subtemplate with.
   * @return the current HTMLSub object so these can be chained.
   * @throws IllegalArgumentException if no such subtemplate exists
   */
  public HTMLSub set(String name, double value) {
    return this.set(name, Double.toString(value));
  }

  /**
   * Set the subtemplate with the given name to the HTMLSub value.
   * @param name the name of a subtemplate.
   * @param value the value to replace the subtemplate with.
   * @return the current HTMLSub object so these can be chained.
   * @throws IllegalArgumentException if no such subtemplate exists
   */
  public HTMLSub set(String name, HTMLSub value) {
    if (name == null) throw new IllegalArgumentException("name is null");
    if (!this.template.containsSubtemplate(name)) 
      throw new IllegalArgumentException("No subtemplate with name {" + name + "}");
    if (value != null) {
      this.replacements.put(name, value);
    } else {
      this.replacements.remove(name);
    }
    return this;
  }

  /**
   * Set the subtemplate with the given name to the HTMLTemplate value.
   * @param name the name of a subtemplate.
   * @param value the value to replace the subtemplate with.
   * @return the current HTMLSub object so these can be chained.
   * @throws IllegalArgumentException if no such subtemplate exists
   */
  public HTMLSub set(String name, HTMLTemplate value) {
    if (name == null) throw new IllegalArgumentException("name is null");
    if (!this.template.containsSubtemplate(name)) 
      throw new IllegalArgumentException("No subtemplate with name {" + name + "}");
    if (value != null) {
      this.replacements.put(name, value);
    } else {
      this.replacements.remove(name);
    }
    return this;
  }

  /**
   * Set the subtemplate with the given name to the array of HTMLSub values.
   * @param name the name of a subtemplate.
   * @param value the value to replace the subtemplate with.
   * @return the current HTMLSub object so these can be chained.
   * @throws IllegalArgumentException if no such subtemplate exists
   */
  public HTMLSub set(String name, HTMLSub[] value) {
    set(name, Arrays.asList(value));
    return this;
  }

  /**
   * Set the subtemplate with the given name to the Iterable value.
   * @param name the name of a subtemplate.
   * @param value the value to replace the subtemplate with.
   * @return the current HTMLSub object so these can be chained.
   * @throws IllegalArgumentException if no such subtemplate exists
   */
  public HTMLSub set(String name, Iterable<? extends HTMLSub> value) {
    if (name == null) throw new IllegalArgumentException("name is null");
    if (!this.template.containsSubtemplate(name)) 
      throw new IllegalArgumentException("No subtemplate with name {" + name + "}");
    if (value != null) {
      this.replacements.put(name, value);
    } else {
      this.replacements.remove(name);
    }
    return this;
  }

  /**
   * Set a subtemplate to the empty string effectivly hiding it.
   * @param name the subtemplate to hide
   * @throws IllegalArgumentException if no such subtemplate exists
   */
  public HTMLSub empty(String name) {
    return this.set(name, "");
  }

  /**
   * Set multiple subtemplates to the empty string effectively hiding them.
   * @param names the subtemplates to hide
   * @throws IllegalArgumentException if one of the subtemplates doesn't exist
   */
  public HTMLSub empty(String ... names) {
    for (int i = 0; i < names.length; i++) this.empty(names[i]);
    return this;
  }

  /**
   * Reset a subtemplate to it's original value effectively showing it.
   * @param name the subtemplate to show
   * @throws IllegalArgumentException if no such subtemplate exists
   */
  public HTMLSub reset(String name) {
    return this.set(name, (String)null);
  }

  /**
   * Reset multiple subtemplates to their original values effectively showing them.
   * @param names the subtemplates to show
   * @throws IllegalArgumentException if one of the subtemplates doesn't exist
   */
  public HTMLSub reset(String ... names) {
    for (int i = 0; i < names.length; i++) this.reset(names[i]);
    return this;
  }
  
  /**
   * Write the HTML out.
   * @param out where to write the html
   * @throws IOException if something breaks while writing
   */
  public void output(Writer out) throws IOException {
    List<Object> parts = this.template.getParts();
    for (int i = 0; i < parts.size(); i++) {
      Object part = parts.get(i);
      if (part instanceof HTMLTemplate) {
        HTMLTemplate tmpl = (HTMLTemplate)part;
        Object sub = this.replacements.get(tmpl.getName());
        if (sub == null) {
          tmpl.output(out);
        } else {
          if (sub instanceof HTMLSub) {
            ((HTMLSub)sub).output(out);
          } else if (sub instanceof String) {
            out.write((String)sub);
          } else if (sub instanceof HTMLTemplate) {
            ((HTMLTemplate)sub).output(out);
          } else if (sub instanceof Iterable) {
            Iterable iterable = (Iterable)sub;
            Iterator iter = iterable.iterator();
            while (iter.hasNext()) {
              Object item = iter.next();
              if (item != null && item instanceof HTMLSub) {
                ((HTMLSub)item).output(out);
              }
            }
          } else {
            throw new Error("Impossible state");
          }
        }
      } else { // part is String
        out.write((String)part);
      }
    }
  }

  /**
   * Return the HTML as a string
   * @return the HTML
   */
  public String toString() {
    StringWriter out = new StringWriter();
    try {
      this.output(out);
    } catch (IOException e) {
      // this should not be possible
      throw new Error(e);
    }
    return out.toString();
  }

  public HTMLTemplate base() {
    return template;
  }

}
