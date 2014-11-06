
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
  if (!check_num_value("pseudocount", "pseudocount", 0, null, 4)) return false;
  if (!check_num_value("minimum hit p-value", "motif_pv", 0, 1, 0.0005)) return false;
  if (!check_int_value("spacing", "max_gap", 0, null, 50)) return false;
  return true;
}

function options_changed() {
  var pseudocount = +($("pseudocount").value);
  var motif_pv = +($("motif_pv").value);
  var spacing = +($("max_gap").value);
  var output_ev = +($("output_ev").value);
  if (typeof pseudocount !== "number" || isNaN(pseudocount) || pseudocount != 4) return true;
  if (typeof motif_pv !== "number" || isNaN(motif_pv) || motif_pv != 0.0005) return true;
  if (typeof spacing !== "number" || isNaN(spacing) || spacing != 50) return true;
  if (typeof output_ev !== "number" || isNaN(output_ev) || output_ev != 10.0) return true;
  return false;
}

function options_reset(evt) {
  $("pseudocount").value = 4;
  $("motif_pv").value = 0.0005;
  $("max_gap").value = 50;
  $("output_ev").value = 10.0;
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
  $("mcast_form").addEventListener("submit", on_form_submit, false);
  $("mcast_form").addEventListener("reset", on_form_reset, false);
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
