package au.edu.uq.imb.memesuite.servlet.util;

import au.edu.uq.imb.memesuite.template.HTMLSub;
import au.edu.uq.imb.memesuite.template.HTMLTemplate;

import javax.servlet.ServletException;
import java.util.EnumSet;

/**
 * Some utility methods for a page component.
 */
public abstract class PageComponent {

  /**
   * Get the text of a subtemplate or a default if it does not exist.
   * @param info the template containing the subtemplate.
   * @param name the name of the subtemplate.
   * @param defVal the default value to return if the subtemplate does not exist.
   * @return the text of a subtemplate or a specified default value.
   */
  protected static String getText(HTMLTemplate info, String name, String defVal) {
    if (info.containsSubtemplate(name)) {
      return info.getSubtemplate(name).toString();
    } else {
      return defVal;
    }
  }

  protected static Integer getInt(HTMLTemplate info, String name, Integer defVal) throws ServletException {
    if (info.containsSubtemplate(name)) {
      String text = info.getSubtemplate(name).toString();
      if (text.equals("any")) {
        return null;
      } else {
        try {
          return Integer.parseInt(text, 10);
        } catch (NumberFormatException e) {
          throw new ServletException("Can not parse the page parameter \"" + name + "\" as an Integer.", e);
        }
      }
    } else {
      return defVal;
    }
  }

  /**
   * Get a subtemplate or a default if it does not exist.
   * @param info the template containing the subtemplate.
   * @param name the name of the subtemplate.
   * @param defVal the default template to return if the subtemplate does not exist.
   * @return the subtemplate or a specified default template.
   */
  protected static HTMLTemplate getTemplate(HTMLTemplate info, String name, HTMLTemplate defVal) {
    if (info.containsSubtemplate(name)) {
      return info.getSubtemplate(name);
    } else {
      return defVal;
    }
  }

  protected static <E extends Enum<E>> E getEnum(HTMLTemplate info, String name, Class<E> enumClass, E defVal) throws ServletException {
    if (info.containsSubtemplate(name)) {
      try {
        return Enum.valueOf(enumClass, info.getSubtemplate(name).getSource());
      } catch (IllegalArgumentException e) {
        throw new ServletException("Unrecognised enum in " + name, e);
      }
    } else {
      return defVal;
    }
  }

  protected static <E extends Enum<E>> EnumSet<E> getEnums(HTMLTemplate info, String name, Class<E> enumClass, EnumSet<E> defVal) throws ServletException {
    if (info.containsSubtemplate(name)) {
      EnumSet<E> set = EnumSet.noneOf(enumClass);
      String[] enumStrs = info.getSubtemplate(name).getSource().split("\\s");
      try {
        for (String enumStr : enumStrs) {
          set.add(Enum.valueOf(enumClass, enumStr));
        }
      } catch (IllegalArgumentException e) {
        throw new ServletException("Unrecognised enum in " + name, e);
      }
      return set;
    } else {
      return defVal;
    }
  }

  public abstract HTMLSub getComponent();
  public abstract HTMLSub getHelp();
}
