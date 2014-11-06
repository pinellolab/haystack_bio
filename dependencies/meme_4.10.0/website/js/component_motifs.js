
var MotifInput = function(container, options) {
  "use strict";
  var me, i, alphabet, alphabet_enabled;
  me = this;
  // parameters
  this.container = container;
  this.options = options;
  // other settings
  this.is_dna = true;
  this.alphabet = new Alphabet("ACGT");
  this.text_motif_bounds = [];
  this.text_motifs = [];
  this.last_update = Date.now() - 500;
  this.update_timer = null;
  // keeps track of the currently drawn motif logo in the popup
  this.displayed_motif = null;
  // keeps track of where the mouse was last
  this.last_x = 0;
  this.last_y = 0;
  // file parsing related stuff
  this.file_parser = null;
  this.file_motifs = [];
  this.file_meta = null;
  this.file_error = false;
  // embed parsing related stuff
  this.embed_motifs = [];
  this.embed_meta = null;
  this.embed_error = false;
  // db related 
  this.db_alphabet = AlphType.UNKNOWN;
  this.db_motif_count = 0;
  // get the source selector
  this.source = this.container.querySelector("select.motif_source");
  // get the text related parts
  // this selects the alphabet of the text motif editor
  this.text_alphabet = this.container.querySelector("select.motif_text_alphabet");
  // this surrounds the text editor components
  this.text_surround = this.container.querySelector("div.motif_text");
  // hidden input field where the motifs are written
  this.text_hin = this.text_surround.querySelector("input");
  // the div that constrains the editor
  this.text_div = this.text_surround.querySelector("div");
  // the text input
  this.text_area = this.text_surround.querySelector("textarea");
  // the span where the text is replicated with additional formatting
  this.text_backdrop = this.text_surround.querySelector("span");
  this.text_backdrop.innerHTML = "";
  // get the file related parts
  this.file_surround = this.container.querySelector("span.motif_file");
  this.file_indicator = this.file_surround.querySelector("span.indicator");
  this.file_input = this.file_surround.querySelector("input");
  this.file_popup = this.file_surround.querySelector("div.popup");
  // get the embed related parts
  this.embed_surround = this.container.querySelector("span.motif_embed");
  this.embed_hin = (this.embed_surround != null ? this.embed_surround.querySelector("input.data") : null);
  // disable alphabets that are not allowed
  for (i = 0; i < this.text_alphabet.options.length; i++) {
    alphabet_enabled = this.options["alphabets"][this.text_alphabet.options[i].value];
    if (typeof alphabet_enabled === "boolean") {
      this.text_alphabet.options[i].disabled = !alphabet_enabled;
    } else {
      this.text_alphabet.options[i].disabled = true;
    }
  }
  // check that the seleted alphabet is not disabled
  if (this.text_alphabet.options[this.text_alphabet.selectedIndex].disabled) {
    for (i = 0; i < this.text_alphabet.options.length; i++) {
      if (!this.text_alphabet.options[i].disabled) {
        this.text_alphabet.options[i].selected = true;
        break;
      }
    }
    if (i == this.text_alphabet.options.length) {
      throw new Error("No alphabets were allowed!");
    }
  }
  // parse the embeded motifs
  if (this.embed_hin) {
    (new MotifParser({
      "error": function(is_error, message, reasons) {
        me._embed_error(is_error, message, reasons); },
      "begin": function(size) { me._embed_begin(size); },
      "end": function () { me._embed_end(); },
      "meta": function (info) { me._embed_motif_meta(info); },
      "motif": function (motif) { me._embed_motif(motif); }
    })).process_blob(new Blob([this.embed_hin.value], {"type": "text\/plain"}));
  }

  // create the popup
  this.popup = document.createElement("canvas");
  this.popup.className = "pop_logo";
  document.getElementsByTagName("body")[0].appendChild(this.popup);
  // activate motif editor
  this._source_update();
  this._text_alphabet_update();
  // add event listeners
  // detect changes in the motif source selection
  this.source.addEventListener("change", function() { me._source_update(); }, false);
  // detect changing of the uploaded file
  this.file_input.addEventListener("change", function(e) { me._file_update(); }, false);
  // detect changes in the alphabet selection for the text motif input
  this.text_alphabet.addEventListener("change", function() { me._text_alphabet_update(); }, false);
  // detect typing or pasting into motif text area
  this.text_area.addEventListener('input', function() { me._text_update(); }, false);
  // detect which text motif the mouse is over
  this.text_div.addEventListener("mousemove", function(e) { me._text_mousemove(e); }, false);
  // show the logo popup when over the text box
  this.text_div.addEventListener("mouseover", function(e) { me._text_mouseover(e); }, false);
  // hide the logo popup when mouse is not over the text box
  this.text_div.addEventListener("mouseout", function (e) { me._text_mouseout(e); }, false);
  // detect form resets and reset properly
  if (this.source.form != null) {
    this.source.form.addEventListener("reset", function() {
      window.setTimeout(function () {
        me.reset();
      }, 50);
    }, false);
  }
};

MotifInput.prototype.check = function(restrict_alphabets) {
  "use strict";
  var source, i, j, tmotif, exists, warnings, errors, alphabet;
  source = this.source.value;
  warnings = false;
  errors = false;
  alphabet = AlphType.UNKNOWN;
  if (source == "text") {
    exists = /\S/.test(this.text_area.value);
    // check for problems
    for (i = 0; i < this.text_motifs.length; i++) {
      tmotif = this.text_motifs[i];
      for (j = 0; j < tmotif.tokens.length; j++) {
        if ((tmotif.tokens[j].type & SimpleMotifTokEn.ERROR) != 0) {
          errors = true;
        } else if ((tmotif.tokens[j].type & SimpleMotifTokEn.WARN) != 0) {
          warnings = true;
        }
      }
    }
    alphabet = AlphUtil.from_name(this.text_alphabet.value);
  } else if (source == "file") {
    exists = this.file_input.value.length > 0;
    errors = this.file_error;
    if (this.file_meta != null) alphabet = this.file_meta["alphabet"];
  } else if (source == "embed") {
    exists = true;
    errors = this.embed_error;
    if (this.embed_meta != null) alphabet = this.embed_meta["alphabet"];
  } else {// db
    exists = true;
    alphabet = this.db_alphabet;
  }
  if (!exists) {
    alert("Please input " + this.options.field + ".");
    return false;
  }
  if (errors) {
    alert("Please correct errors in the " + this.options.field + ".");
    return false;
  }
  if (warnings) {
    if (!confirm("There are warnings for the " + this.options.field + ". Continue anyway?")) {
      return false;
    }
  }
  if (typeof restrict_alphabets == "number" && alphabet != AlphType.UNKNOWN) {
    if ((alphabet & restrict_alphabets) == 0) {
      alert("The " + this.options.field + " uses the " + 
          AlphUtil.display(alphabet) + " alphabet but to be compatibile " +
          "with other inputs it should be " + 
          AlphUtil.display(restrict_alphabets) + ".");
      return false;
    }
  }
  return true;
};

MotifInput.prototype.changed = function() {
  "use strict";
  var source;
  if (!this.source.options[this.source.selectedIndex].defaultSelected) return true;
  source = this.source.value;
  if (source == "text") {
    if (this.text_area.value.length != 0) return true;
  } else if (source == "file") {
    if (this.file_input.value.length != 0) return true;
  }
  return false;
};

MotifInput.prototype.reset = function() {
  "use strict";
  var i, opt;
  // stop actions in progress
  if (this.file_parser) {
    this.file_parser.cancel();
    this.file_parser = null;
  }
  if (this.update_timer != null) {
    clearTimeout(this.update_timer);
    this.update_timer = null;
  }
  // reset the text input
  this.text_area.value = "";
  for (i = 0; i < this.text_alphabet.options.length; i++) {
    opt = this.text_alphabet.options[i];
    opt.selected = opt.defaultSelected;
  }
  if (this.text_alphabet.options[this.text_alphabet.selectedIndex].disabled) {
    for (i = 0; i < this.text_alphabet.options.length; i++) {
      if (!this.text_alphabet.options[i].disabled) {
        this.text_alphabet.options[i].selected = true;
        break;
      }
    }
  }
  this.last_update = Date.now() - 500;
  this._text_alphabet_update();
  // reset the file input
  this.file_input.value = "";
  this._file_update();
  // reset the selected source option
  for (i = 0; i < this.source.options.length; i++) {
    opt = this.source.options[i];
    opt.selected = opt.defaultSelected;
  }
  this._source_update();
};

/*
 * Returns the alphabet of the current motifs
 */
MotifInput.prototype.get_alphabet = function () {
  "use strict";
  var source;
  // determine the source
  source = this.source.value;
  if (source == "text") {
    return AlphUtil.from_name(this.text_alphabet.value);
  } else if (source == "file") {
    if (this.file_meta) return this.file_meta["alphabet"];
  } else if (source == "embed") {
    if (this.embed_meta) return this.embed_meta["alphabet"];
  } else { // db
    return this.db_alphabet;
  }
  return AlphType.UNKNOWN;
};

/*
 * returns the number of motifs 
 */
MotifInput.prototype.get_motif_count = function () {
  "use strict";
  var source;
  source = this.source.value;
  if (source == "text") {
    return this.text_motifs.length;
  } else if (source == "file") {
    return this.file_motifs.length;
  } else if (source == "embed") {
    return this.embed_motifs.length;
  } else { // db
    return this.db_motif_count;
  }
  return 0;
};

MotifInput.prototype._text_mousemove = function(e) {
  "use strict";
  var now_time, me;
  // position the popup offset 5px right to the mouse pointer
  this.popup.style.left = (e.pageX + 5) + "px";
  this.popup.style.top = (e.pageY) + "px";
  // display the motif which the mouse is over
  this._update_popup(e.clientX, e.clientY);
};

MotifInput.prototype._text_mouseover = function(e) {
  toggle_class(this.popup, "mouseover", true);
};

MotifInput.prototype._text_mouseout = function (e) {
  toggle_class(this.popup, "mouseover", false);
};

MotifInput.prototype._file_update = function () {
  var file, me;
  // cancel previous parsing operation
  if (this.file_parser) {
    this.file_parser.cancel();
    this.file_parser = null;
  }
  // check for a file to load
  if (!(file = this.file_input.files[0])) {
    // no file to load! Clear the status. 
    substitute_classes(this.file_surround, ["good", "warning", "error"], []);
    this.file_indicator.style.width = "0%";
    this.file_popup.innerHTML = "";
    return;
  }
  me = this;
  this.file_parser = new MotifParser({
    "error": function(is_error, message, reasons) {
      me._file_error(is_error, message, reasons); },
    "begin": function(size) { me._file_begin(size); },
    "end": function (error_list) { me._file_end(error_list); },
    "progress": function (fraction) { me._file_progress(fraction); },
    "meta": function (info) { me._file_motif_meta(info); },
    "motif": function (motif) { me._file_motif(motif); }
  });
  this.file_parser.process_blob(file);
};

MotifInput.prototype._embed_error = function (is_error, message, reasons) {
  this.embed_error |= is_error;
};

MotifInput.prototype._embed_begin = function (size) {
};

MotifInput.prototype._embed_end = function () {
  var me;
  me = this;
  if (this.source.value == "embed") {
    try {
      this.container.dispatchEvent(new CustomEvent("motifs_loaded", {detail: {controler: me}}));
    } catch (e) {
      if (e.message && e.name && window.console) {
        console.log("Suppressed exception " + e.name + ": " + e.message);
      }
    }
  }
};

MotifInput.prototype._embed_motif_meta = function (info) {
  "use strict";
  this.embed_meta = info;
};

MotifInput.prototype._embed_motif = function (motif) {
  "use strict";
  this.embed_motifs.push(motif);
};

MotifInput.prototype._file_error = function (is_error, message, reasons) {
  "use strict";
  var table, row, cell, i, ul, li;
  table = this.file_popup.querySelector("table");
  if (table == null) {
    table = document.createElement("table");
    this.file_popup.innerHTML = "";
    this.file_popup.appendChild(table);
  }
  row = table.insertRow(table.rows.length);
  row.className = (is_error ? "error" : "warning");
  cell = row.insertCell(row.cells.length);
  cell.appendChild(document.createTextNode(is_error ? "\u2718" : "\u26A0"));
  cell = row.insertCell(row.cells.length);
  cell.appendChild(document.createTextNode(message));
  if (typeof reasons !== "undefined") {
    ul = document.createElement("ul");
    for (i = 0; i < reasons.length; i++) {
      li = document.createElement("li");
      li.appendChild(document.createTextNode(reasons[i]));
      ul.appendChild(li);
    }
    cell.appendChild(ul);
  }
  this.file_error |= is_error;
  if (this.file_error) {
    substitute_classes(this.file_surround, ["good", "warning"], ["error"]);
  } else {
    substitute_classes(this.file_surround, ["good"], ["warning"]);
  }
};

MotifInput.prototype._file_begin = function (size) {
  "use strict";
  substitute_classes(this.file_surround, ["warning", "error"], ["good"]);
  this.file_indicator.style.width = "0%";
  this.file_popup.innerHTML = "";
  this.file_error = false;
  if (typeof this.options["max_size"] === "number" && size > this.options["max_size"]) {
    this._file_error(true, "The file is larger than the maximum accepted size.",
        ["" + size + " > " +  this.options["max_size"]]);
  }
};

MotifInput.prototype._file_end = function () {
  "use strict";
  var i, ul, li, me;
  me = this;
  this.file_indicator.style.width = "100%";
  if (!(this.file_error)) substitute_classes(this.file_surround, ["good"], []);
  if (this.source.value == "file") {
    try {
      this.container.dispatchEvent(new CustomEvent("motifs_loaded", {detail: {controler: me}}));
    } catch (e) {
      if (e.message && e.name && window.console) {
        console.log("Suppressed exception " + e.name + ": " + e.message);
      }
    }
  }
};

MotifInput.prototype._file_progress = function (fraction) {
  this.file_indicator.style.width = (fraction * 100) + "%";
};

MotifInput.prototype._file_motif_meta = function (info) {
  this.file_meta = info;
  if (info["alphabet"] === AlphType.DNA) {
    if (!this.options["alphabets"]["DNA"]) {
      this._file_error(true, "DNA motifs are not accepted by this tool.");
    }
  } else if (info["alphabet"] === AlphType.PROTEIN) {
    if (!this.options["alphabets"]["PROTEIN"]) {
      this._file_error(true, "Protein motifs are not accepted by this tool.");
    }
  } else {
    throw new Error("Unknown alphabet");
  }
};

MotifInput.prototype._file_motif = function (motif) {
  this.file_motifs.push(motif);
};

MotifInput.prototype._text_alphabet_update = function() {
  "use strict";
  this.is_dna = (this.text_alphabet.value == "DNA" || this.text_alphabet.value == "RNA");
  if (this.is_dna) {
    this.alphabet = new Alphabet("ACGT");
  } else {
    this.alphabet = new Alphabet("ACDEFGHIKLMNPQRSTVWY");
  }
  this._text_update();
};

MotifInput.prototype._make_backdrop = function(simple_motif) {
  "use strict";
  var all, box, i, east, south, indicator;
  all = document.createElement("span");
  for (i = 0; i < simple_motif.tokens.length; i++) {
    if ((simple_motif.tokens[i].type & SimpleMotifTokEn.SPACE) !== 0) {
      all.appendChild(document.createTextNode(simple_motif.tokens[i].text));
    } else {
      box = document.createElement("span");
      box.className = "token";
      box.appendChild(document.createTextNode(simple_motif.tokens[i].text));
      if ((simple_motif.tokens[i].type & SimpleMotifTokEn.ERROR) !== 0) {
        box.className += "  error";
      } else if ((simple_motif.tokens[i].type & SimpleMotifTokEn.WARN) !== 0) {
        box.className += " warn";
      }
      east = ((simple_motif.tokens[i].type & SimpleMotifTokEn.EAST) !== 0);
      south = ((simple_motif.tokens[i].type & SimpleMotifTokEn.SOUTH) !== 0);
      if (east || south) {
        indicator = document.createElement("div");
        indicator.className = "indicator";
        indicator.className += (east ? " east" : " south");
        indicator.appendChild(document.createTextNode(simple_motif.tokens[i].indicator));
        box.appendChild(indicator);
      }
      all.appendChild(box);
    }
  }
  return all;
};

MotifInput.prototype._text_update = function() {
  "use strict";
  var nodes, text, line_starts, part, i, last, groups, box, bracket;
  var now_time, me;
  // get the text
  text = this.text_area.value;
  // 1) find positions of line starts
  line_starts = [0];
  for (i = text.indexOf('\n'); i != -1; i = text.indexOf('\n', i + 1)) {
    line_starts.push(i + 1);
  }
  line_starts.push(text.length);
  // 2) find starts and ends of contiguous non-empty lines as each of these
  // will be processed as a motif.
  for (i = 0, last = -1, groups = []; i < (line_starts.length - 1); i++) {
    part = text.substring(line_starts[i], line_starts[i+1]);
    if (/^\s*$/.test(part)) {
      // empty line
      if (last != -1) {
        groups.push({"start": line_starts[last], "end": line_starts[i] - 1});
        last = -1;
      }
    } else {
      //non-empty line
      if (last == -1) {
        last = i;
      }
    }
  }
  if (last != -1) {
    groups.push({"start": line_starts[last], "end": text.length});
  }
  // 3) determine the type of each motif (IUPAC or matrix)
  this.text_motifs = [];
  nodes = document.createElement("span");
  for (i = 0; i < groups.length; i++) {
    if (i == 0) {
      if (groups[0].start > 0) {
        nodes.appendChild(document.createTextNode(text.substring(0, groups[0].start)));
      }
    } else  {
      nodes.appendChild(document.createTextNode(text.substring(groups[i-1].end, groups[i].start)));
    }
    var tmotif = new SimpleMotif(text.substring(groups[i].start, groups[i].end), this.is_dna, i+1);
    box = document.createElement("div");
    box.className = "motif_box";
    box.appendChild(this._make_backdrop(tmotif));
    bracket = document.createElement("div");
    bracket.className = "motif_bracket_left";
    box.appendChild(bracket);
    bracket = document.createElement("div");
    bracket.className = "motif_bracket_right";
    box.appendChild(bracket);
    nodes.appendChild(box);
    this.text_motifs[i] = tmotif;
  }
  if (groups.length == 0) {
    nodes.appendChild(document.createTextNode(text));
  } else if (groups[groups.length - 1].end < text.length) {
    nodes.appendChild(document.createTextNode(text.substring(groups[groups.length - 1].end)));
  }
  if (this.text_backdrop.firstChild) {
    this.text_backdrop.replaceChild(nodes, this.text_backdrop.firstChild);
  } else {
    this.text_backdrop.appendChild(nodes);
  }
  this.text_hin.value = this._text_as_meme();
  this._index_active_regions();
  now_time = Date.now();
  me = this;
  if (this.update_timer != null) {
    clearTimeout(this.update_timer);
    this.update_timer = null;
  }
  if (now_time - this.last_update < 500) {
    this.update_timer = setTimeout(function() { me._update_popup(); }, 500 - (now_time - this.last_update));
  } else {
    this._update_popup(); 
    this.last_update = now_time;
  }
  try {
    this.container.dispatchEvent(new CustomEvent("motifs_loaded", {detail: {controler: me}}));
  } catch (e) {
    if (e.message && e.name && window.console) {
      console.log("Suppressed exception " + e.name + ": " + e.message);
    }
  }
};

MotifInput.prototype._db_update2 = function (doc) {
  "use strict";
  var query, alphabet, name, description, file, src, count, cols, min, max, me;
  me = this;
  this.db_alphabet = AlphUtil.from_name(doc.evaluate("/motif_db/@alphabet", doc,
        null, XPathResult.STRING_TYPE, null).stringValue);
  name = doc.evaluate("/motif_db/name", doc, null, XPathResult.STRING_TYPE, null).stringValue;
  description = doc.evaluate("/motif_db/description", doc, null, XPathResult.STRING_TYPE, null).stringValue;
  query = doc.evaluate("/motif_db/file", doc, null, XPathResult.ORDERED_NODE_ITERATOR_TYPE, null);
  this.db_motif_count = 0;
  while ((file = query.iterateNext()) != null) {
    src = doc.evaluate("@src", file, null, XPathResult.STRING_TYPE, null).stringValue;
    count = doc.evaluate("@count", file, null, XPathResult.NUMBER_TYPE, null).numberValue;
    this.db_motif_count += count;
    cols = doc.evaluate("@cols", file, null, XPathResult.NUMBER_TYPE, null).numberValue;
    min = doc.evaluate("@min", file, null, XPathResult.NUMBER_TYPE, null).numberValue;
    max = doc.evaluate("@max", file, null, XPathResult.NUMBER_TYPE, null).numberValue;
  }
  try {
    this.container.dispatchEvent(new CustomEvent("motifs_loaded", {detail: {controler: me}}));
  } catch (e) {
    if (e.message && e.name && window.console) {
      console.log("Suppressed exception " + e.name + ": " + e.message);
    }
  }
};

MotifInput.prototype._db_update = function (listing_id) {
  "use strict";
  var request, url, i, me;
  // so we can access in the inner function
  me = this;
  // now send the request
  url = "../db/motifs?listing=" + listing_id;
  request = new XMLHttpRequest();
  request.addEventListener("load", function(evt) { me._db_update2(request.responseXML); }, false);
  request.open("GET", url, true);
  request.send();
};

MotifInput.prototype._source_update = function() {
  "use strict";
  var source, match, classes;
  classes = this.container.className;
  source = this.source.value;
  // disable things to avoid sending data that is not needed
  this.text_hin.disabled = true;
  this.file_input.disabled = true;
  if (this.embed_hin != null) this.embed_hin.disabled = true;
  if (source == "text") {
    this.text_hin.disabled = false;
  } else if (source == "file") {
    this.file_input.disabled = false;
  } else if (source == "embed") {
    if (this.embed_hin != null) this.embed_hin.disabled = false;
  }
  // hide/show things
  if (/^(text|file|embed)$/.test(source)) {
    this.container.className = classes.replace(/(text|file|embed|db)/, this.source.value);
  } else { // db
    this.container.className = classes.replace(/(text|file|embed|db)/, "db");
    // do a remote request for more details on this database
    this._db_update(source);
  }
};

MotifInput.prototype._index_active_regions = function() {
  var error_boxes, motif_boxes, i, node;
  var y, prev_end_y, limits;
  motif_boxes = this.text_backdrop.querySelectorAll(".motif_box");

  this.text_motif_bounds = [];
  for (i = 0; i < motif_boxes.length; i++) {
    node = motif_boxes[i];
    y = 0;
    while (node != null && node != this.text_div) {
      y += node.offsetTop;
      node = node.offsetParent;
    }
    if (i > 0) {
      this.text_motif_bounds[i-1] = ((y - prev_end_y) / 2) + prev_end_y;
    }
    prev_end_y = y + motif_boxes[i].offsetHeight;
  }
}

MotifInput.prototype._update_popup = function(x, y) {
  "use strict";
  var rel_box, rel_y, motif;
  var imin, imax, imid;
  // allow using previous arguments
  if (arguments.length >= 2) {
    this.last_x = x;
    this.last_y = y;
  } else {
    x = this.last_x;
    y = this.last_y;
  }
  // calculate relative to the editor box
  rel_box = this.text_div.getBoundingClientRect();
  rel_y = y - rel_box.top;
  // check to see if we have any choice
  if (this.text_motifs.length == 0) {
    motif = null;
  } else {
    // binary search for closest motif
    imin = 0;
    imax = this.text_motifs.length - 1;
    while (imin < imax) {
      imid = imin + Math.floor((imax - imin) / 2);
      // note imid < imax
      if (this.text_motif_bounds[imid] < rel_y) {
        imin = imid + 1;
      } else {
        imax = imid;
      }
    }
    motif = this.text_motifs[imin];
  }
  // draw closest motif
  if (motif != null) {
    if (motif != this.displayed_motif) {
      if (Date.now() - this.last_update > 400) {
        draw_logo_on_canvas(logo_1(this.alphabet, "", 
              MotifUtils.as_pspm(motif)), this.popup, false, 0.5);
      }
      this.displayed_motif = motif;
    }
  }
  toggle_class(this.popup, "logo_exists", motif != null);
};

MotifInput.prototype._text_as_meme = function() {
  var i, out, motif, pspm;
  out = "";
  for (i = 0; i < this.text_motifs.length; i++) {
    motif = this.text_motifs[i];
    pspm = MotifUtils.as_pspm(motif);
    out += pspm.as_meme({"with_header": (i === 0)});
  }
  return out;
};

