/*
 * $
 *
 * Shorthand for document.getElementById.
 */
function $(id) {
  return document.getElementById(id);
}

/*
 * stop_evt
 *
 * Stop propagation of a event.
 */
function stop_evt(evt) {
  evt = evt || window.event;
  if (typeof evt.stopPropagation != "undefined") {
    evt.stopPropagation();
  } else {
    evt.cancelBubble = true;
  }
}

/*
 * coords
 *
 * Calculates the x and y offset of an element.
 * From http://www.quirksmode.org/js/findpos.html
 */
function coords(elem) {
  var myX = myY = 0;
  if (elem.offsetParent) {
    do {
      myX += elem.offsetLeft;
      myY += elem.offsetTop;
    } while (elem = elem.offsetParent);
  }
  return [myX, myY];
}

/*
 * help_popup
 *
 * Moves around help pop-ups so they appear
 * below an activator.
 */
function help_popup(activator, popup_id) {
  if (help_popup.popup === undefined) {
    help_popup.popup = null;
  }
  if (help_popup.activator === undefined) {
    help_popup.activator = null;
  }

  if (typeof(activator) == 'undefined') { // no activator so hide
    help_popup.popup.style.display = 'none';
    help_popup.popup = null;
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
  var xy = coords(activator);
  var padding = 10;
  var edge_padding = 15;
  var scroll_padding = 15;

  var pop_left = (xy[0] + (activator.offsetWidth / 2)  - (pop.offsetWidth / 2));
  var pop_top = (xy[1] + activator.offsetHeight + padding);

  // ensure the box is not past the top or left of the page
  if (pop_left < 0) pop_left = edge_padding;
  if (pop_top < 0) pop_top = edge_padding;
  // ensure the box does not cause horizontal scroll bars
  var page_width = null;
  if (window.innerWidth) {
    page_width = window.innerWidth;
  } else if (document.body) {
    page_width = document.body.clientWidth;
  }
  if (page_width) {
    if (pop_left + pop.offsetWidth > page_width) {
      pop_left = page_width - pop.offsetWidth - edge_padding - scroll_padding; //account for scrollbars
    }
  }

  pop.style.left = pop_left + "px";
  pop.style.top = pop_top + "px";
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
  if (typeof enabled === 'undefined') {
    if (!found) list.push(cls);
  } else {
    if (enabled) list.push(cls);
  }
  node.className = list.join(" ");
}

/*
 * has_class
 *
 * Checks for the existance of a class on a node.
 */
function has_class(node, cls) {
  var classes = node.className;
  var list = classes.replace(/^\s+/, '').replace(/\s+$/, '').split(/\s+/);
  for (var i = 0; i < list.length; i++) {
    if (list[i] == cls) {
      return true;
    }
  }
  return false;
}

/*
 * more_opts
 *
 * Shows more options by toggling an expansion section.
 * When hiding the expanded options it checks if they've
 * changed by calling a caller defined function and gives
 * the user a chance to abort. If the user continues the
 * options are hidden and reset to their defaults by calling
 * another caller defined function.
 */
function more_opts(node, fn_changed) {
  if (has_class(node, 'expanded')) {
    toggle_class(node, 'modified', fn_changed());
  }
  toggle_class(node, 'expanded');
}

/*
 * more_opts_key
 *
 * Calls more_opts when spacebar has been pressed.
 */
function more_opts_key(evt, node, fn_changed) {
  if (evt.which == 32 || evt.keyCode == 32) {
    more_opts(node, fn_changed);
  }
}

/*
 * num_key
 *
 * Limits input to numbers and various control characters such as 
 * enter, backspace and delete.
 */
function num_key(evt) {
  evt = evt || window.event;
  var code = (evt.keyCode ? evt.keyCode : evt.which);
  var keychar = String.fromCharCode(code);
  var numre = /\d/;
  // only allow 0-9 and various control characters (Enter, backspace, delete)
  if (code != 8 && code != 13 && code != 46 && !numre.test(keychar)) {
    evt.preventDefault();
  }
}

/*
 * validate_background
 *
 * Takes a markov model background in text form, parses and validates it.
 * Returns an object with the boolean property ok. On success it is true
 * on error it is false. On error it has the additional properties: line and
 * reason. On success it has the property alphabet.
 */
function validate_background(bg_str) {
  var lines, items, i, fields, prob, j, alphabet;
  var indexes, letters, prob_sum, overflowed;
  if (typeof delta !== "number") delta = 0.01;
  lines = bg_str.split(/\r\n|\r|\n/g);
  items = [];
  for (i = 0; i < lines.length; i++) {
    //skip blank lines and comments
    if (/^\s*$/.test(lines[i]) || /^#/.test(lines[i])) {
      continue;
    }
    // split into two space separated items, allowing for leading and trailing space
    if(!(fields = /^\s*(\S+)\s+(\S+)\s*/.exec(lines[i]))) {
      return {ok: false, line: i+1, reason: "expected 2 space separated fields"};
    }
    prob = +(fields[2]);
    if (typeof prob !== "number" || isNaN(prob) || prob < 0 || prob > 1) {
      return {ok: false, line: i+1, 
        reason: "expected number between 0 and 1 inclusive for the probability field"};
    }
    items.push({key: fields[1], probability: prob, line: i+1});
  }
  // now sort entries
  items.sort( 
    function(a, b) {
      if (a.key.length != b.key.length) { 
        return a.key.length - b.key.length;
      } 
      return a.key.localeCompare(b.key); 
    } 
  );
  // determine the alphabet (all 1 character keys)
  alphabet = [];
  prob_sum = 0;
  for (i = 0; i < items.length; i++) {
    if (items[i].key.length > 1) {
      break;
    }
    alphabet.push(items[i].key);
    prob_sum += items[i].probability;
    if (i > 0 && alphabet[i] == alphabet[i-1]) {
      return {ok: false, line: Math.max(items[i-1].line, items[i].line), 
        reason: "alphabet letter seen before"}; // alphabet letter repeats!
    }
  }
  // test that the probabilities approximately sum to 1
  if (Math.abs(prob_sum - 1.0) > delta) {
    return {ok: false, line: items[alphabet.length -1].line, 
      reason: "order-0 probabilities do not sum to 1"};
  }
  //now iterate over all items checking the key is as expected
  indexes = [0, 0];
  order_complete = true;
  prob_sum = 0;
  for (j = alphabet.length; j < items.length; j++) {
    letters = items[j].key.split("");
    prob_sum += items[j].probability;
    // see if the length of the letters is expected
    if (letters.length != indexes.length) {
      return {ok: false, line: items[j].line, 
        reason: "missing or erroneous transition name"};
    }
    for (i = 0; i < letters.length; i++) {
      if (letters[i] != alphabet[indexes[i]]) {
        return {ok: false, line: items[j].line, 
          reason: "missing or erroneous transition name"};
      }
    }
    //increment the indexes starting from the right
    for (i = indexes.length - 1; i >= 0; i--) {
      indexes[i]++;
      if (indexes[i] >= alphabet.length) {
        indexes[i] = 0;
        continue;
      }
      break;
    }
    if (i == -1) {
      // all indexes overflowed so add another one. Technically we should 
      // add it at the start but as all existing entries are zero we
      // actually just add another 0 to the end.
      indexes.push(0);
      // test that the probabilities for the markov states approximately sum to 1
      if (Math.abs(prob_sum - 1.0) > delta) {
        return {ok: false, line: items[j].line, 
          reason: "order-" + (indexes.length - 2) + 
            " probabilities do not sum to 1"};
      }
      order_complete = true;
      prob_sum = 0;
    } else {
      order_complete = false;
    }
  }
  if (!order_complete) {
    // not enough states for a markov model of this order
    return {ok: false, line: lines.length, 
      reason: "not enough transitions for a complete order-" + 
      (indexes.length - 1) + " markov model background"};
  }
  return {ok: true, alphabet: alphabet.join("")};
}
