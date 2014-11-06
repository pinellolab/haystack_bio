
var BIH = function (container, indicator, popup, options) {
  "use strict";
  this.container = container;
  this.indicator = indicator;
  this.popup = popup;
  this.update_timer = null;
  this.display_type = 0;
  BgHandler.call(this, options);
};
BIH.prototype = Object.create(BgHandler.prototype);
BIH.prototype.constructor = BIH;


BIH.prototype.clear_indicators = function() {
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

BIH.prototype._update_display = function () {
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

BIH.prototype.reset = function () {
  "use strict";
  BgHandler.prototype.reset.apply(this, arguments);
  this.clear_indicators();
};

BIH.prototype.begin = function () {
  "use strict";
  var me;
  BgHandler.prototype.begin.apply(this, arguments);
  if (this.update_timer) clearInterval(this.update_timer);
  me = this; // reference 'this' inside closure
  this.update_timer = setInterval(function () { me._update_display();}, 100);
};

BIH.prototype.end = function () {
  "use strict";
  BgHandler.prototype.end.apply(this, arguments);
  if (this.update_timer) {
    clearInterval(this.update_timer);
    this.update_timer = null;
  }
  this._update_display();
  if (this.display_type == 1) this.clear_indicators();
};

var BfileInput = function (container, options) {
  "use strict";
  var me;
  // make 'this' accessable in inner scopes
  me = this;
  // default fasta options
  // store the parameters
  this.container = container;
  this.options = options;
  // lookup relevent components
  this.source = this.container.querySelector("select.bfile_source");
  // get the file related parts
  this.file_surround = this.container.querySelector("span.bfile_file");
  this.file_indicator = this.file_surround.querySelector("span.indicator");
  this.file_input = this.file_surround.querySelector("input");
  this.file_popup = this.file_surround.querySelector("div.popup");
  this.file_handler = new BIH(this.file_surround, this.file_indicator, this.file_popup, this.options);
  this.file_parser = null;
  // initialise
  this._source_update();
  this._file_update();
  // add listeners
  this.source.addEventListener('change', function() {
    me._source_update();
  }, false);
  // detect file changes
  this.file_input.addEventListener('change', function() {
    me._file_update();
  }, false);
  // detect form resets and reset properly
  if (this.source.form != null) {
    this.source.form.addEventListener("reset", function() {
      window.setTimeout(function () {
        me.reset();
      }, 50);
    }, false);
  }
};

BfileInput.prototype.check = function(restrict_alphabets) {
  "use strict";
  var source, summary;
  // find out what source we're using.
  source = this.source.value;
  if (source == "file") {
    if (this.file_input.value.length == 0) {
      alert("Please input " + this.options.field + ".");
      return false;
    }
    if (typeof restrict_alphabets == "number") {
      summary = this.file_handler.summary();
      if ((summary.alphabet & restrict_alphabets) == 0) {
        if (!confirm("The " + this.options.field + 
              " appear to use a different alphabet to other inputs. " +
              "Continue anyway?")) {
          return false;
        }
      }
    }
  }
  return true;
};

// check if the default settings are selected
BfileInput.prototype.changed = function () {
  "use strict";
  if (!this.source.options[this.source.selectedIndex].defaultSelected) return true;
  if (this.source.value == "file" && this.file_input.value.length != 0) return true;
  return false;
};

// reset to the default settings
BfileInput.prototype.reset = function() {
  "use strict";
  var i, opt;
  for (i = 0; i < this.source.options.length; i++) {
    opt = this.source.options[i];
    opt.selected = opt.defaultSelected;
  }
  this.file_input.value = "";
  this._source_update();
  this._file_update();
};


// Start parsing a file
BfileInput.prototype._file_update = function () {
  "use strict";
  var file;
  if (this.file_parser != null) {
    this.file_parser.cancel();
    this.file_parser = null;
  }
  this.file_handler.reset();
  if ((file = this.file_input.files[0]) != null) {
    this.file_parser = new BgParser(this.file_handler);
    this.file_parser.process_blob(file);
  }
};

//
// Choose to upload a background file or generate from the sequences
//
BfileInput.prototype._source_update = function () {
  "use strict";
  toggle_class(this.container, "file", (this.source.value === "file"));
  this.file_input.disabled = (this.source.value !== "file");
};
