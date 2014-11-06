
var Glam2Input = function(container, options) {
  "use strict";
  var me;
  me = this;
  // parameters
  this.container = container;
  this.options = options;
  // get the source selector
  this.source = this.container.querySelector("select.motif_source");
  // get the file input
  this.file_input = this.container.querySelector("span.motif_file input");
  // get the embedded input
  this.embed_data = this.container.querySelector("span.motif_embed input.data");
  this.embed_name = this.container.querySelector("span.motif_embed input.name");
  // activate motif editor
  this._source_update();
  // add event listeners
  // detect changes in the motif source selection
  this.source.addEventListener("change", function() { me._source_update(); }, false);
  // detect reset events
  if (this.source.form != null) {
    this.source.form.addEventListener("reset", function() { me.reset(); }, false);
  }
};

Glam2Input.prototype.check = function (restrict_alphabet) {
  //TODO actually determine if the file is ok
  if (this.source.value == "file") {
    if (this.file_input.value.length == 0) {
      alert("Please input " + this.options.field + ".");
      return false;
    }
  }
  return true;
};

Glam2Input.prototype.changed = function() {
  "use strict";
  var source;
  if (!this.source.options[this.source.selectedIndex].defaultSelected) return true;
  source = this.source.value;
  if (source == "file") {
    if (this.file_input.value.length != 0) return true;
  }
  return false;
};

Glam2Input.prototype.reset = function() {
  // clear the file
  this.file_input.value = "";
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
Glam2Input.prototype.get_alphabet = function () {
  "use strict";
  //TODO when we read the file get the alphabet
  return AlphType.UNKNOWN;
};

Glam2Input.prototype._source_update = function() {
  "use strict";
  var source, classes;
  source = this.source.value;
  classes = this.container.className;
  this.container.className = classes.replace(/(file|embed)/, source);
  if (source == "file") {
    this.file_input.disabled = false;
    if (this.embed_data != null) this.embed_data.disabled = true;
    if (this.embed_name != null) this.embed_name.disabled = true;
  } else if (source == "embed") {
    this.embed_data.disabled = false;
    this.embed_name.disabled = false;
    this.file_input.disabled = true;
  }
  
};
