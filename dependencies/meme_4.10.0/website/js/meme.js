
var sequences = null;
var control_sequences = null;
var background = null;

function register_component(id, element, controler) {
  "use strict";
  if (id == "sequences") {
    sequences = controler;
  } else if (id == "control_sequences") {
    control_sequences = controler;
  } else if (id == "background") {
    background = controler;
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
  if ($("discr_on").checked) {
    if (control_sequences != null && !control_sequences.check(alphabet)) return false;
  }
  if (!check_int_value("number of motifs", "nmotifs", 1, null, 3)) return false;
  if (!check_job_details()) return false;
  if (background != null && !background.check(alphabet)) return false;
  if (!check_int_range("minimum motif width", "minw", 6,
        "maximum motif width", "maxw", 30, 2, 300)) return false;
  if ($("dist").value !== "oops") {
    if (!check_int_range("minimum sites", "minsites", 2,
          "maximum sites", "maxsites", 600, 2, 600)) return false;
  }
  return true;
}


function options_changed() {
  if (background != null && background.changed()) return true;
  if ($("norc").checked) return true;
  if (!/^\s*6\s*$/.test($("minw").value)) return true;
  if (!/^\s*50\s*$/.test($("maxw").value)) return true;
  if ($("minsites_enable").checked) return true;
  if ($("maxsites_enable").checked) return true;
  if ($("pal").checked) return true;
  if ($("shuffle").checked) return true;
  return false;
}

function options_reset(evt) {
  if (background != null) background.reset();
  $("norc").checked = false;
  $("minw").value = 6;
  $("maxw").value = 50;
  $("sites").style.display = 'block';
  $("minsites_enable").checked = false;
  $("minsites").value = 2;
  $("minsites").disabled = true;
  $("maxsites_enable").checked = false;
  $("maxsites").value = 600;
  $("maxsites").disabled = true;
  $("pal").checked = false;
  $("shuffle").checked = false;
}

function fix_reset() {
  $("sites").style.display = ($('dist').value == 'oops' ? 'none' : 'block');
  $('discr_sequences_area').style.display = ($('discr_on').checked ? 'block' : 'none');
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

function on_ch_dist() {
  $("sites").style.display = ($('dist').value == 'oops' ? 'none' : 'block');
}

function on_ch_discr() {
  $('discr_sequences_area').style.display = ($('discr_on').checked ? 'block' : 'none');
}


function on_load() {
  // add listeners for the motif discovery mode
  $("discr_off").addEventListener("click", on_ch_discr, false);
  $("discr_on").addEventListener("click", on_ch_discr, false);
  // add listener for changing the motif distribution
  $("dist").addEventListener("change", on_ch_dist, false);
  // add listener to the form to check the fields before submit
  $("meme_form").addEventListener("submit", on_form_submit, false);
  $("meme_form").addEventListener("reset", on_form_reset, false);
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
