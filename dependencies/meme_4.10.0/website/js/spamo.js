
var sequences = null;
var primary_motif = null;
var secondary_motifs = null;

function register_component(id, element, controler) {
  "use strict";
  if (id == "sequences") {
    sequences = controler;
  } else if (id == "primary") {
    primary_motif = controler;
  } else if (id == "secondaries") {
    secondary_motifs = controler;
  }
}

function check() {
  "use strict";
  var alphabet;
  if (sequences != null) {
    if (!sequences.check()) return false;
    alphabet = sequences.get_alphabet();
  } else {
    alphabet = AlphType.DNA | AlphType.RNA | AlphType.PROTEIN;
  }
  if (primary_motif != null && !primary_motif.check(alphabet)) return false;
  if (secondary_motifs != null && !secondary_motifs.check(alphabet)) return false;
  if (!check_job_details()) return false;
  return true;
}

function options_changed() {
  if ($('dumpseqs').checked) return true;
  return false;
}

function options_reset() {
  $('dumpseqs').checked = false;
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
  $("spamo_form").addEventListener("submit", on_form_submit, false);
  $("spamo_form").addEventListener("reset", on_form_reset, false);
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
