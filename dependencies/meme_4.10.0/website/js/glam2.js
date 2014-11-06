
var sequences = null;

function register_component(id, element, controler) {
  "use strict";
  if (id == "sequences") {
    sequences = controler;
  }
}

function check() {
  "use strict";
  var alphabet, initial_cols, min_cols, max_cols;
  if (sequences != null) {
    if (!sequences.check()) return false;
    alphabet = sequences.get_alphabet();
  } else {
    alphabet = AlphType.DNA | AlphType.RNA | AlphType.PROTEIN;
  }
  if (!check_job_details()) return false;
  if (!check_int_value("minimum aligned sequences", "min_seqs", 2, null, 2)) return false;
  if (!check_int_value("initial aligned columns", "initial_cols", 2, 300, 20)) return false;
  if (!check_int_range("minimum aligned columns", "min_cols", 2,
        "maximum aligned columns", "max_cols", 50, 2, 300)) return false;
  initial_cols = +($('initial_cols').value);
  min_cols = +($('min_cols').value);
  max_cols = +($('max_cols').value);
  if (initial_cols < min_cols || initial_cols > max_cols) {
    alert('Please input a whole number for the initial aligned columns which ' +
        ' is between or equal to the minimum (' + min_cols + ') and maximum (' +
          max_cols + ') aligned columns.\n');
    return false;
  }
  if (!check_num_value("deletion pseudocounts", "pseudo_del", 0, null, 0.1)) return false;
  if (!check_num_value("no-deletion pseudocounts", "pseudo_nodel", 0, null, 2)) return false;
  if (!check_num_value("insertion pseudocounts", "pseudo_ins", 0, null, 0.02)) return false;
  if (!check_num_value("no-insertion pseudocounts", "pseudo_noins", 0, null, 1)) return false;
  if (!check_int_value("number of replicates", "replicates", 1, 100, 10)) return false;
  if (!check_int_value("number of iterations without improvement", "max_iter", 1, 1000000, 2000)) return false;
  return true;
}

function options_changed() {
  "use strict";
  var min_seqs = +($('min_seqs').value);
  var initial_cols = +($('initial_cols').value);
  var min_cols = +($('min_cols').value);
  var max_cols = +($('max_cols').value);
  var pseudo_del = +($('pseudo_del').value);
  var pseudo_nodel = +($('pseudo_nodel').value);
  var pseudo_ins = +($('pseudo_ins').value);
  var pseudo_noins = +($('pseudo_noins').value);
  var replicates = +($('replicates').value);
  var max_iter = +($('max_iter').value);
  if (typeof min_seqs !== "number" || isNaN(min_seqs) || min_seqs != 2) return true;
  if (typeof initial_cols !== "number" || isNaN(initial_cols) || initial_cols != 20) return true;
  if (typeof min_cols !== "number" || isNaN(min_cols) || min_cols != 2) return true;
  if (typeof max_cols !== "number" || isNaN(max_cols) || max_cols != 50) return true;
  if (typeof pseudo_del !== "number" || isNaN(pseudo_del) || pseudo_del != 0.1) return true;
  if (typeof pseudo_nodel !== "number" || isNaN(pseudo_nodel) || pseudo_nodel != 2) return true;
  if (typeof pseudo_ins !== "number" || isNaN(pseudo_ins) || pseudo_ins != 0.02) return true;
  if (typeof pseudo_noins !== "number" || isNaN(pseudo_noins) || pseudo_noins != 1) return true;
  if (typeof replicates !== "number" || isNaN(replicates) || replicates != 10) return true;
  if (typeof max_iter !== "number" || isNaN(max_iter) || max_iter != 2000) return true;
  if ($("norc").checked) return true;
  if ($("shuffle").checked) return true;
  if (!$("embed").checked) return true;
  return false;
}

function options_reset(evt) {
  "use strict";
  $("min_seqs").value = 2;
  $("initial_cols").value = 20;
  $("min_cols").value = 2;
  $("max_cols").value = 50;
  $("pseudo_del").value = 0.1;
  $("pseudo_nodel").value = 2.0;
  $("pseudo_ins").value = 0.02;
  $("pseudo_noins").value = 1.0;
  $("replicates").value = 10;
  $("max_iter").value = 2000;
  $("norc").checked = false;
  $("shuffle").checked = false;
  $("embed").checked = true;
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
  $("glam2_form").addEventListener("submit", on_form_submit, false);
  $("glam2_form").addEventListener("reset", on_form_reset, false);
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
