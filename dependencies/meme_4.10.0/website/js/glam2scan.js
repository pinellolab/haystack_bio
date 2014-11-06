
var glam2motifs = null;
var sequences = null;
var background = null;

function register_component(id, element, controler) {
  "use strict";
  if (id == "glam2motifs") {
    glam2motifs = controler;
  } else if (id == "sequences") {
    sequences = controler;
  }
}

function check() {
  "use strict";
  var alphabet = AlphType.UNKNOWN;
  if (glam2motifs != null) {
    if (!glam2motifs.check()) return false;
    alphabet = glam2motifs.get_alphabet();
  }
  if (alphabet == AlphType.UNKNOWN) alphabet = AlphType.DNA | AlphType.RNA | AlphType.PROTEIN;
  if (sequences != null && !sequences.check(alphabet)) return false;
  if (!check_job_details()) return false;
  if (!check_int_value("number of alignments", "alignments", 1, 200, 25)) return false;
  return true;
}


function options_changed() {
  if (!/^\s*25\s*$/.test($("alignments").value)) return true;
  if ($("norc").checked) return true;
  return false;
}

function options_reset(evt) {
  $("alignments").value = 25;
  $("norc").checked = false;
}

function fix_reset() {
}

function on_form_submit(evt) {
  if (!check()) {
    evt.preventDefault();
  }
}

function on_form_reset(evt) {
  window.setTimeout(function(evt) {
    fix_reset();
  }, 50);
}

function on_load() {
  // add listener to the form to check the fields before submit
  $("glam2scan_form").addEventListener("submit", on_form_submit, false);
  $("glam2scan_form").addEventListener("reset", on_form_reset, false);
}

// add a load
(function() {
  "use strict";
  window.addEventListener("load", function load(evt) {
    "use strict";
    window.removeEventListener("load", load, false);
    on_load();
  }, false);
})();

