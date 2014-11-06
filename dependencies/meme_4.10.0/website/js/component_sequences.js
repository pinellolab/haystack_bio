


var FIH = function (container, indicator, popup, options, change_handler) {
  "use strict";
  this.container = container;
  this.indicator = indicator;
  this.popup = popup;
  this.change_handler = change_handler;
  this.update_timer = null;
  this.display_error = false;
  this.display_warn = false;
  FastaHandler.call(this, options);
};
FIH.prototype = Object.create(FastaHandler.prototype);
FIH.prototype.constructor = FIH;


FIH.prototype.clear_indicators = function() {
  "use strict";
  if (this.update_timer) {
    clearInterval(this.update_timer);
    this.update_timer = null;
  }
  this.popup.innerHTML = "";
  this.indicator.style.width = "0%";
  substitute_classes(this.container, ["good", "error", "warning"], []);
  this.display_type = 0;
};

FIH.prototype._update_display = function () {
  "use strict";
  var table, summary, i, j, msg;
  var row, cell, i, ul, li;
  
  this.indicator.style.width = (this.fraction * 100) + "%";
  if (!this.updated) return;

  summary = this.summary();
  if (summary.error) {
    if (this.display_type != 3) {
      substitute_classes(this.container, ["good", "warning"], ["error"]); 
      this.display_type = 3;
    }
  } else if (summary.warning) {
    if (this.display_type != 2) {
      substitute_classes(this.container, ["good", "error"], ["warning"]);
      this.display_type = 2;
    }
  } else {
    if (this.display_type != 1) {
      substitute_classes(this.container, ["warning", "error"], ["good"]);
      this.display_type = 1;
    }
  }
  table = document.createElement("table");
  for (i = 0; i < summary.messages.length; i++) {
    msg = summary.messages[i];
    row = table.insertRow(table.rows.length);
    row.className = (msg.is_error ? "error" : "warning");
    cell = row.insertCell(row.cells.length);
    cell.appendChild(document.createTextNode(msg.is_error ? "\u2718" : "\u26A0"));
    cell = row.insertCell(row.cells.length);
    cell.appendChild(document.createTextNode(msg.message));
    if (typeof msg.reasons !== "undefined") {
      ul = document.createElement("ul");
      for (j = 0; j < msg.reasons.length; j++) {
        li = document.createElement("li");
        li.appendChild(document.createTextNode(msg.reasons[j]));
        ul.appendChild(li);
      }
      cell.appendChild(ul);
    }
  }
  
  this.popup.innerHTML = "";
  this.popup.appendChild(table);

};

FIH.prototype.reset = function () {
  "use strict";
  FastaHandler.prototype.reset.apply(this, arguments);
  this.clear_indicators();
};

FIH.prototype.begin = function () {
  "use strict";
  var me;
  FastaHandler.prototype.begin.apply(this, arguments);
  if (this.update_timer) clearInterval(this.update_timer);
  me = this; // reference 'this' inside closure
  this.update_timer = setInterval(function () { me._update_display();}, 100);
};

FIH.prototype.end = function () {
  "use strict";
  FastaHandler.prototype.end.apply(this, arguments);
  if (this.update_timer) {
    clearInterval(this.update_timer);
    this.update_timer = null;
  }
  this._update_display();
  if (this.display_type == 1) this.clear_indicators();
  this.change_handler();
};


var SequenceInput = function(container, options) {
  "use strict";
  var me;
  // make 'this' accessable in inner scopes
  me = this;
  // default fasta options
  // store the parameters
  this.container = container;
  this.options = options;
  // lookup relevent components
  this.source = this.container.querySelector("select.sequence_source");
  // get the text related parts
  this.text_surround = this.container.querySelector("div.sequence_text");
  this.text_indicator = this.text_surround.querySelector("span.indicator");
  this.text_control = this.text_surround.querySelector(".editor");
  this.text_area = this.text_control.querySelector("textarea");
  this.text_backdrop = this.text_control.querySelector("span");
  this.text_popup = this.text_surround.querySelector("div.popup");
  this.text_dbh = new FIH(this.text_surround, this.text_indicator, this.text_popup, options, function() {
    me._fire_sequences_checked_event();
  });
  // get the file related parts
  this.file_surround = this.container.querySelector("span.sequence_file");
  this.file_indicator = this.file_surround.querySelector("span.indicator");
  this.file_input = this.file_surround.querySelector("input");
  this.file_popup = this.file_surround.querySelector("div.popup");
  this.file_dbh = new FIH(this.file_surround, this.file_indicator, this.file_popup, options, function() {
    me._fire_sequences_checked_event();
  });
  // get the database related parts
  this.db_listing = container.querySelector("select.sequence_db.listing");
  this.db_version = container.querySelector("select.sequence_db.version");
  // get the embed related parts
  this.embed_surround = this.container.querySelector("span.sequence_embed");
  if (this.embed_surround != null) {
    this.embed_name = this.embed_surround.querySelector("input.name");
    this.embed_data = this.embed_surround.querySelector("input.data");
    this.embed_handler = new FastaHandler();
    // override end function to hook in our event
    this.embed_handler.end = function() {
      FastaHandler.prototype.end.apply(this, arguments);
      me._fire_sequences_checked_event();
    };
  }
  // create a list of submittable fields so we can disable the ones we are not using
  this.submittables = [this.text_area, this.file_input];
  if (this.db_listing != null) this.submittables.push(this.db_listing);
  if (this.db_version != null) this.submittables.push(this.db_version);
  if (this.embed_surround != null) {
    this.submittables.push(this.embed_name);
    this.submittables.push(this.embed_data);
  }
  // other things
  this.parser = null;
  this.timer = null;
  this.db_alphabets = 0;
  // parse the embeded motifs
  if (this.embed_surround != null) {
    (new FastaChecker(this.embed_handler)).process_blob(
        new Blob([this.embed_data.value], {"type": "text\/plain"}));
  }
  // initialise
  this._source_update();
  this._text_update();
  this._file_update();
  // add listeners
  this.source.addEventListener('change', function() {
    me._source_update();
  }, false);
  // detect typing or pasting into motif text area
  this.text_area.addEventListener('input', function() {
    me._text_update(); 
  }, false);
  // detect file changes
  this.file_input.addEventListener('change', function() {
    me._file_update();
  }, false);
  // detect db changes
  if (this.db_listing != null) {
    this.db_listing.addEventListener('change', function() {
      me._load_versions(+me.db_listing.value);
    }, false);
  }
  if (this.db_version != null) {
    this.db_version.addEventListener('change', function() {
      me._update_db_alphabets();
    },false);
  }
  // detect form resets and reset properly
  if (this.source.form != null) {
    this.source.form.addEventListener("reset", function() {
      window.setTimeout(function () {
        me.reset();
      }, 50);
    }, false);
  }
};

SequenceInput.prototype.check = function(restrict_alphabets) {
  "use strict";
  var source, id, exists, handler, summary, alphabet;
  // find out what source we're using
  source = this.source.value;
  if (/^\d+$/.test(source)) {
    id = parseInt(source, 10);
    source = "db";
    if (typeof restrict_alphabets == "number") {
      if ((this.db_alphabets & restrict_alphabets) == 0) {
        alert("Please select a " + AlphUtil.display(restrict_alphabets) + 
            " database for the " + this.options.field + ".");
        return false;
      }
    }
    return true;
  } else {
    if (source == "text") {
      exists = /\S/.test(this.text_area.value);
      handler = this.text_dbh;
    } else if (source == "file") {
      exists = this.file_input.value.length > 0;
      handler = this.file_dbh;
    } else if (source == "embed") {
      exists = true;
      handler = this.embed_handler;
    } else {
      throw new Error("Unknown source");
    }
    if (!exists) {
      alert("Please input " + this.options.field + ".");
      return false;
    }
    summary = handler.summary();
    if (summary.error) {
      alert("Please correct errors in the " + this.options.field + ".");
      return false;
    }
    if (summary.warning) {
      if (!confirm("There are warnings for the " + this.options.field + ". Continue anyway?")) {
        return false;
      }
    }
    if (typeof restrict_alphabets == "number") {
      if ((summary.alphabet & restrict_alphabets) == 0) {
        if (!confirm("The " + this.options.field + 
              " appear to use a different alphabet to other inputs. " +
              "Continue anyway?")) {
          return false;
        }
      }
    }
    return true;
  }
};

// Has this field changed from the default "reset" state.
SequenceInput.prototype.changed = function() {
  "use strict";
  var source;
  if (!this.source.options[this.source.selectedIndex].defaultSelected) return true;
  source = this.source.value;
  if (/^\d+$/.test(source)) source = "db";
  if (source == "text") {
    if (this.text_area.value.length != 0) return true;
  } else if (source == "file") {
    if (this.file_input.value.length != 0) return true;
  } else if (source == "db") {
    if (!this.db_listing.options[this.db_listing.selectedIndex].defaultSelected) return true;
    if (!this.db_version.options[this.db_version.selectedIndex].defaultSelected) return true;
  }
  return false;
};

// reset the fields to a consistant state
SequenceInput.prototype.reset = function() {
  "use strict";
  var i, opt;
  if (this.parser) this.parser.cancel();
  this.parser = null;
  if (this.timer) window.clearTimeout(this.timer);
  this.timer = null;
  this.file_input.value = "";
  this.file_dbh.reset();
  this.text_area.value = "";
  this.text_dbh.reset();
  if (this.db_listing != null) this._clear_select(this.db_listing);
  if (this.db_version != null) this._clear_select(this.db_version);
  for (i = 0; i < this.source.options.length; i++) {
    opt = this.source.options[i];
    opt.selected = opt.defaultSelected;
  }
  this._source_update();
};

SequenceInput.prototype.get_alphabet = function() {
  "use strict";
  var source, id;
  source = this.source.value;
  if (/^\d+$/.test(source)) {
    id = parseInt(source, 10);
    source = "db";
  }
  if (source == "text") {
    return this.text_dbh.guess_alphabet();
  } else if (source == "file") {
    return this.file_dbh.guess_alphabet();
  } else if (source == "db") {
    return this.db_alphabets;
  } else if (source == "embed") {
    return this.embed_handler.guess_alphabet();
  }
};

SequenceInput.prototype._source_update = function() {
  "use strict";
  var source, id, i;
  source = this.source.value;
  if (/^\d+$/.test(source)) {
    id = parseInt(source, 10);
    source = "db";
  }
  substitute_classes(this.container, ["text", "file", "embed", "db"], [source]);
  for (i = 0; i < this.submittables.length; i++) {
    this.submittables[i].disabled = true;
  }
  if (this.parser) this.parser.cancel();
  if (source == "text") {
    this.text_area.disabled = false;
    this._text_validate();
  } else if (source == "file") {
    this.file_input.disabled = false;
    this._file_validate();
  } else if (source == "db") {
    // the db hidden inputs will be enabled by the _load_listings command
    this._load_listings(id);
  } else if (source == "embed") {
    this.embed_name.disabled = false;
    this.embed_data.disabled = false;
  }
  this._fire_sequences_checked_event();
};

SequenceInput.prototype._fire_sequences_checked_event = function() {
  "use strict";
  var me, alphabet;
  me = this;
  alphabet = this.get_alphabet();
  toggle_class(this.container, "dna", (alphabet & AlphType.DNA) != 0);
  toggle_class(this.container, "rna", (alphabet & AlphType.RNA) != 0);
  toggle_class(this.container, "protein", (alphabet & AlphType.PROTEIN) != 0);
  try {
    // IE sometimes has problems with this line.
    // I think they are related to the page not being fully loaded.
    this.container.dispatchEvent(new CustomEvent("sequences_checked", {detail: {controler: me}}));
  } catch (e) {
    if (e.message && e.name && window.console) {
      console.log("Suppressed exception " + e.name + ": " + e.message);
    }
  }
};

SequenceInput.prototype._embed_process = function() {
  file = new Blob([this.text_area.value], {"type": "text\/plain"});
  this.parser = new FastaChecker({});
  this.parser.process_blob(file);
};

SequenceInput.prototype._text_validate = function() {
  var file;
  if (this.parser) this.parser.cancel();
  if (this.text_area.value.length == 0) {
    this.text_dbh.clear_indicators();
    substitute_classes(this.container, ["rna", "dna", "protein"], []);
    return;
  }
  file = new Blob([this.text_area.value], {"type": "text\/plain"});
  this.text_dbh.configure(this.options);
  this.parser = new FastaChecker(this.text_dbh);
  this.parser.process_blob(file);
};

SequenceInput.prototype._text_update = function() {
  "use strict";
  var nodes, me;
  nodes = document.createTextNode(this.text_area.value);
  if (this.text_backdrop.firstChild) {
    this.text_backdrop.replaceChild(nodes, this.text_backdrop.firstChild);
  } else {
    this.text_backdrop.appendChild(nodes);
  }
  if (this.source.value == "text") {
    if (this.timer) window.clearTimeout(this.timer);
    me = this;
    this.timer = window.setTimeout(function() {
      me.timer = null;
      me._text_validate();
    }, 300);
  }
};

SequenceInput.prototype._file_validate = function() {
  var file;
  if (this.parser) this.parser.cancel();
  if (!(file = this.file_input.files[0])) {
    this.file_dbh.clear_indicators();
    substitute_classes(this.container, ["rna", "dna", "protein"], []);
    return;
  }
  this.file_dbh.configure(this.options);
  this.parser = new FastaChecker(this.file_dbh);
  this.parser.process_blob(file);
};

SequenceInput.prototype._file_update = function() {
  "use strict";
  if (this.source.value == "file") {
    this._file_validate();
  }
};

SequenceInput.prototype._clear_select = function(list) {
  "use strict";
  var optgroup;
  // disable until update completes
  list.disabled = true;
  optgroup = list.getElementsByTagName("optgroup")[0];
  // clear previous values
  while (optgroup.childNodes.length > 0) {
    optgroup.removeChild(optgroup.lastChild);
  }
};

SequenceInput.prototype._load_listings = function(category_id) {
  "use strict";
  var request, url, i, self, list, optgroup;
  // so we can access in the inner function
  self = this;
  list = this.db_listing;
  optgroup = list.getElementsByTagName("optgroup")[0];
  this._clear_select(list);
  // now send the request
  url = "../db/sequences?category=" + category_id;
  request = new XMLHttpRequest();
  request.addEventListener("load", function(evt) {
    var xml_doc, listings, all_l, listing, i, id, name, alphabets;
    xml_doc = request.responseXML;
    listings = xml_doc.firstChild;
    // add the other options
    all_l = listings.getElementsByTagName("l");
    for (i = 0; i < all_l.length; i++) {
      listing = all_l[i];
      id = listing.getAttribute("i");
      name = listing.getAttribute("n");
      //alphabets = listing.getAttribute("a");
      optgroup.appendChild(new Option(name, id));
    }
    // re-enable the list
    list.disabled = false;
    if (list.value) {
      self._load_versions(parseInt(list.value, 10));
    }
  }, false);
  request.open("GET", url, true);
  request.send();
};

SequenceInput.prototype._load_versions = function(listing_id) {
  "use strict";
  var me, request, url, i, list, optgroup;
  // so we can access in the inner function
  me = this;
  list = this.db_version;
  optgroup = list.getElementsByTagName("optgroup")[0];
  this._clear_select(list);
  // now send the request
  url = "../db/sequences?listing=" + listing_id;
  request = new XMLHttpRequest();
  request.addEventListener("load", function(evt) {
    var xml_doc, versions, all_v, version, i, opt, id, name, alphabets;
    xml_doc = request.responseXML;
    versions = xml_doc.firstChild;
    // add the other options
    all_v = versions.getElementsByTagName("v");
    for (i = 0; i < all_v.length; i++) {
      version = all_v[i];
      id = version.getAttribute("i");
      name = version.getAttribute("n");
      alphabets = version.getAttribute("a"); // Bitset where DNA = 1, RNA = 2, PROTEIN = 4
      opt = new Option(name, id);
      opt.setAttribute("data-alphabets", alphabets);
      optgroup.appendChild(opt);
    }
    // re-enable the list
    list.disabled = false;
    me._update_db_alphabets();
  }, false);
  request.open("GET", url, true);
  request.send();
};

SequenceInput.prototype._update_db_alphabets = function() {
  "use strict";
  var opt;
  opt = this.db_version.options[this.db_version.selectedIndex];
  if (opt != null) {
    this.db_alphabets = +(opt.getAttribute("data-alphabets"));
  } else {
    this.db_alphabets = 0;
  }
  this._fire_sequences_checked_event();
};
