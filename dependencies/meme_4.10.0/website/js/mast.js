
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
  var motif_alphabet, seq_alphabet, translate_dna;
  translate_dna = $("translate_dna").checked;
  if (motifs != null) {
    if (!motifs.check()) return false;
    motif_alphabet = motifs.get_alphabet();
    if (translate_dna && motif_alphabet != AlphType.UNKNOWN && (motif_alphabet & AlphType.PROTEIN) == 0) {
      alert("Please provide protein motifs when using the translate DNA option.");
      return false;
    }
  } else {
    motif_alphabet = AlphType.DNA | AlphType.RNA | AlphType.PROTEIN;
  }
  if (sequences != null) {
    if (!sequences.check(translate_dna ? null : motif_alphabet)) return false;
    seq_alphabet = sequences.get_alphabet();
    if (translate_dna && seq_alphabet != AlphType.UNKNOWN && (seq_alphabet & (AlphType.DNA | AlphType.RNA)) == 0) {
      alert("Please provide DNA or RNA sequences when using the translate DNA option.");
      return false;
    }
  }
  if (!check_job_details()) return false;
  if (!check_num_value("sequence E-value threshold", "seq_evalue", 0, null, 10)) return false;
  if (!check_num_value("motif E-value threshold", "motif_evalue", 0, null, 0.05)) return false;
  return true;
}


function options_changed() {
  if ($("strands").value !== "combine") return true;
  if ($("translate_dna").checked) return true;
  if (!/^\s*10\s*$/.test($("seq_evalue").value)) return true;
  if ($("use_seq_comp").checked) return true;
  if ($("scale_m").checked) return true;
  if ($("motif_evalue_enable").checked) return true;
  return false;
}

function options_reset(evt) {
  $("strands").value = "combine";
  $("translate_dna").checked = false;
  $("seq_evalue").value = "10";
  $("use_seq_comp").checked = false;
  $("scale_m").checked = false;
  $("motif_evalue_enable").checked = false;
  $("motif_evalue").disabled = true;
  $("motif_evalue").value = "0.05";
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
  $("mast_form").addEventListener("submit", on_form_submit, false);
  $("mast_form").addEventListener("reset", on_form_reset, false);
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
