
var motifs = null;
var sequences = null;

function register_component(id, element, controler) {
  "use strict";
  if (id == "motifs") {
    motifs = controler;
  } else if (id == "sequences") {
    sequences = controler;
  }
}

function check() {
  "use strict";
  var alphabet;
  if (motifs != null) {
    if (!motifs.check()) return false;
    alphabet = motifs.get_alphabet();
  } else {
    alphabet = AlphType.DNA | AlphType.RNA | AlphType.PROTEIN;
  }
  if (sequences != null && !sequences.check(alphabet)) return false;
  if (!check_job_details()) return false;
  return true;
}

function options_changed() {
  var output_pv = +($("output_pv").value);
  if (typeof output_pv !== "number" || isNaN(output_pv) || output_pv != 1e-4) return true;
  if ($("norc").checked) return true;
  return false;
}

function options_reset(evt) {
  $("output_pv").value = "1e-4";
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
  $("fimo_form").addEventListener("submit", on_form_submit, false);
  $("fimo_form").addEventListener("reset", on_form_reset, false);
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

