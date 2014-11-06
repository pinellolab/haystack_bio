/*
 * $
 *
 * Shorthand function for getElementById
 */
function $(el) {
  return document.getElementById(el);
}

/*
 * coords
 *
 * Calculates the x and y offset of an element.
 * From http://www.quirksmode.org/js/findpos.html
 * with alterations to take into account scrolling regions
 */
function coords(elem) {
  var myX = myY = 0;
  if (elem.getBoundingClientRect) {
    var rect;
    rect = elem.getBoundingClientRect();
    myX = rect.left + ((typeof window.pageXOffset !== "undefined") ?
        window.pageXOffset : document.body.scrollLeft);
    myY = rect.top + ((typeof window.pageYOffset !== "undefined") ?
        window.pageYOffset : document.body.scrollTop);
  } else {
    // this fall back doesn't properly handle absolutely positioned elements
    // inside a scrollable box
    var node;
    if (elem.offsetParent) {
      // subtract all scrolling
      node = elem;
      do {
        myX -= node.scrollLeft ? node.scrollLeft : 0;
        myY -= node.scrollTop ? node.scrollTop : 0;
      } while (node = node.parentNode);
      // this will include the page scrolling (which is unwanted) so add it back on
      myX += (typeof window.pageXOffset !== "undefined") ? window.pageXOffset : document.body.scrollLeft;
      myY += (typeof window.pageYOffset !== "undefined") ? window.pageYOffset : document.body.scrollTop;
      // sum up offsets
      node = elem;
      do {
        myX += node.offsetLeft;
        myY += node.offsetTop;
      } while (node = node.offsetParent);
    }
  }
  return [myX, myY];
}

/*
 * position_popup
 *
 * Positions a popup relative to an anchor element.
 *
 * The avaliable positions are:
 * 0 - Centered below the anchor.
 */
function position_popup(anchor, popup, position) {
  "use strict";
  var a_x, a_y, a_w, a_h, p_x, p_y, p_w, p_h;
  var a_xy, spacer, margin, scrollbar, page_w;
  // define constants
  spacer = 5;
  margin = 15;
  scrollbar = 15;
  // define the positions and widths
  a_xy = coords(anchor);
  a_x = a_xy[0];
  a_y = a_xy[1];
  a_w = anchor.offsetWidth;
  a_h = anchor.offsetHeight;
  p_w = popup.offsetWidth;
  p_h = popup.offsetHeight;
  page_w = null;
  if (window.innerWidth) {
    page_w = window.innerWidth;
  } else if (document.body) {
    page_w = document.body.clientWidth;
  }
  // check the position type is defined
  if (typeof position !== "undefined") {
    position = 0;
  }
  // calculate the popup position
  switch (position) {
    case 0:
    default:
      p_x = a_x + (a_w / 2) - (p_w / 2);
      p_y = a_y + a_h + spacer;
      break;
  }
  // constrain the popup position
  if (p_x < margin) {
    p_x = margin;
  } else if (page_w != null && (p_x + p_w) > (page_w - margin - scrollbar)) {
    p_x = page_w - margin - scrollbar - p_w;
  }
  if (p_y < margin) {
    p_y = margin;
  }
  // position the popup
  popup.style.left = p_x + "px";
  popup.style.top = p_y + "px";
}

/*
 * help_popup
 *
 * Moves around help pop-ups so they appear
 * below an activator.
 */
function help_popup(activator, popup_id) {
  // set default values
  if (typeof help_popup.popup === "undefined") {
    help_popup.popup = null;
  }
  if (typeof help_popup.activator === "undefined") {
    help_popup.activator = null;
  }
  if (typeof(activator) == "undefined") { // no activator so hide
    if (help_popup.popup) {
      help_popup.popup.style.display = 'none';
      help_popup.popup = null;
    }
    return;
  }
  var pop = $(popup_id);
  if (pop == help_popup.popup) {
    if (activator == help_popup.activator) {
      //hide popup (as we've already shown it for the current help button)
      help_popup.popup.style.display = 'none';
      help_popup.popup = null;
      return; // toggling complete!
    }
  } else if (help_popup.popup != null) {
    //activating different popup so hide current one
    help_popup.popup.style.display = 'none';
  }
  help_popup.popup = pop;
  help_popup.activator = activator;

  //must make the popup visible to measure it or it has zero width
  pop.style.display = 'block';
  position_popup(activator, pop);
}

/*
 * update_scroll_pad
 *
 * Creates padding at the bottom of the page to allow
 * scrolling of anything into view.
 */
function update_scroll_pad() {
  var page, pad;
  page = (document.compatMode === "CSS1Compat") ? document.documentElement : document.body;
  pad = $("scrollpad");
  if (pad === null) {
    pad = document.createElement("div");
    pad.id = 'scrollpad';
    document.getElementsByTagName('body')[0].appendChild(pad);
  }
  pad.style.height = Math.abs(page.clientHeight - 100) + "px";
}


/*
 * toggle_class
 *
 * Adds or removes a class from the node. If the parameter 'enabled' is not 
 * passed then the existence of the class will be toggled, otherwise it will be
 * included if enabled is true.
 */
function toggle_class(node, cls, enabled) {
  var classes = node.className;
  var list = classes.replace(/^\s+/, '').replace(/\s+$/, '').split(/\s+/);
  var found = false;
  for (var i = 0; i < list.length; i++) {
    if (list[i] == cls) {
      list.splice(i, 1);
      i--;
      found = true;
    }
  }
  if (enabled === undefined) {
    if (!found) list.push(cls);
  } else {
    if (enabled) list.push(cls);
  }
  node.className = list.join(" ");
}

/*
 * find_child
 *
 * Searches child nodes in depth first order and returns the first it finds
 * with the className specified.
 */
function find_child(node, className) {
  var pattern;
  if (typeof node !== "object") {
    return null;
  }
  if (typeof className === "string") {
    pattern = new RegExp("\\b" + className + "\\b");
  } else {
    pattern = className;
  }
  if (node.nodeType == Node.ELEMENT_NODE && 
      pattern.test(node.className)) {
    return node;
  } else {
    var result = null;
    for (var i = 0; i < node.childNodes.length; i++) {
      result = find_child(node.childNodes[i], pattern);
      if (result != null) break;
    }
    return result;
  }
}

/*
 * find_parent
 *
 * Searches parent nodes outwards from the node and returns the first it finds
 * with the className specified.
 */
function find_parent(node, className) {
  var pattern;
  pattern = new RegExp("\\b" + className + "\\b");
  do {
    if (node.nodeType == Node.ELEMENT_NODE && 
        pattern.test(node.className)) {
      return node;
    }
  } while (node = node.parentNode);
  return null;
}

/*
 * __toggle_help
 *
 * Uses the 'topic' property of the this object to
 * toggle display of a help topic.
 *
 * This function is not intended to be called directly.
 */
function __toggle_help(e) {
  if (!e) e = window.event;
  if (e.type === "keydown") {
    if (e.keyCode !== 13 && e.keyCode !== 32) {
      return;
    }
    // stop a submit or something like that
    e.preventDefault();
  }

  help_popup(this, this.topic);
}

/*
 * help_button
 *
 * Makes a help button for the passed topic.
 */
function help_button(topic) {
  var btn = document.createElement("div");
  btn.className = "help";
  btn.topic = topic;
  btn.tabIndex = "0";
  btn.addEventListener("click", __toggle_help, false);
  btn.addEventListener("keydown", __toggle_help, false);
  return btn;
}

/*
 * add_cell
 *
 * Add a cell to the table row.
 */
function add_cell(row, node, cls, click_action) {
  var cell = row.insertCell(row.cells.length);
  if (node) cell.appendChild(node);
  if (cls && cls !== "") cell.className = cls;
  if (click_action) cell.onclick = click_action;
}

/*
 * add_cell
 *
 * Add a header cell to the table row.
 */
function add_header_cell(row, node, help_topic, cls) {
  var th = document.createElement("th");
  row.appendChild(th);
  if (node) th.appendChild(node);
  if (help_topic && help_topic !== "") th.appendChild(help_button(help_topic));
  if (cls && cls !== "") th.className = cls;
}

/*
 * add_text_cell
 *
 * Add a text cell to the table row.
 */
function add_text_cell(row, text, cls, click_action) {
  var node = null;
  if (typeof(text) != 'undefined') node = document.createTextNode(text);
  add_cell(row, node, cls, click_action);
}

/*
 * add_text_header_cell
 *
 * Add a text header cell to the table row.
 */
function add_text_header_cell(row, text, help_topic, cls) {
  var node = null;
  if (typeof(text) != 'undefined') {
    var nbsp = (help_topic ? "\u00A0" : "");
    var str = "" + text;
    var parts = str.split(/\n/);
    if (parts.length === 1) {
      node = document.createTextNode(str + nbsp);
    } else {
      node = document.createElement("span");
      for (var i = 0; i < parts.length; i++) {
        if (i !== 0) {
          node.appendChild(document.createElement("br"));
        }
        node.appendChild(document.createTextNode(parts[i]));
      }
    }
  }
  add_header_cell(row, node, help_topic, cls);
}
