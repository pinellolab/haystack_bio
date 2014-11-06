
var sequences = null;
var motifs = null;
var background = null;

function register_component(id, element, controler) {
  "use strict";
  if (id == "sequences") {
    sequences = controler;
  } else if (id == "motifs") {
    motifs = controler;
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
  if (motifs != null) {
    if (!motifs.check(alphabet)) return false;
  }
  if (!check_job_details()) return false;
  return (general_check(alphabet) && meme_check(alphabet) && 
      dreme_check(alphabet) && centrimo_check(alphabet));
}

function general_check(alphabet) {
  "use strict";
  if (background != null && !background.check(alphabet)) return false;
  return true;
}

function general_changed() {
  "use strict";
  if (background != null && background.changed()) return true;
  if ($("norc").checked) return true;
  if ($("norand").checked) return true;
  return false;
}

function general_reset(evt) {
  "use strict";
  if (background != null) background.reset();
  $("norc").checked = false;
  $("norand").checked = false;
}

function meme_check(alphabet) {
  "use strict";
  if (!check_int_value("MEME number of motifs", "meme_nmotifs", 0, null, 3)) return false;
  if (!check_int_range("MEME minimum motif width", "meme_minw", 6,
        "MEME maximum motif width", "meme_maxw", 30, 2, 300)) return false;
  if ($("meme_dist").value !== "oops") {
    if (!check_int_range("MEME minimum sites", "meme_minsites", 2,
          "MEME maximum sites", "meme_maxsites", 600, 2, 600)) return false;
  }
  return true;
}

function meme_changed() {
  "use strict";
  var dist = $("meme_dist");
  if (dist.options[dist.selectedIndex].value != "zoops") return true;
  if (!/^\s*3\s*$/.test($("meme_nmotifs").value)) return true;
  if (!/^\s*6\s*$/.test($("meme_minw").value)) return true;
  if (!/^\s*30\s*$/.test($("meme_maxw").value)) return true;
  if ($("meme_minsites_enable").checked) return true;
  if ($("meme_maxsites_enable").checked) return true;
  if ($("meme_pal").checked) return true;
  return false;
}

function meme_reset(evt) {
  "use strict";
  $("meme_dist").value = "zoops";
  $("meme_nmotifs").value = 3;
  $("meme_minw").value = 6;
  $("meme_maxw").value = 30;
  $("meme_sites").style.display = 'block';
  $("meme_minsites_enable").checked = false;
  $("meme_minsites").value = 2;
  $("meme_minsites").disabled = true;
  $("meme_maxsites_enable").checked = false;
  $("meme_maxsites").value = 600;
  $("meme_maxsites").disabled = true;
  $("meme_pal").checked = false;
}

function dreme_check(alphabet) {
  "use strict";
  if (!check_num_value("DREME E-value threshold", "dreme_ethresh", 0, 1, 0.05)) return false;
  if (!check_int_value("DREME motif count", "dreme_nmotifs", 0, null, 10)) return false;
  return true;
}

function dreme_changed() {
  "use strict";
  if (!/^\s*0\.05\s*$/.test($("dreme_ethresh").value)) return true;
  if ($("dreme_nmotifs_enable").checked) return true;
  return false;
}

function dreme_reset(evt) {
  "use strict";
  $("dreme_ethresh").value = 0.05;
  $("dreme_nmotifs_enable").checked = false;
  $("dreme_nmotifs").value = 10;
  $("dreme_nmotifs").disabled = true;
}

function centrimo_check(alphabet) {
  "use strict";
  if (!check_num_value("CentriMo minimum site score", "centrimo_score", null, null, 5)) return false;
  if (!check_int_value("CentriMo maximum region width", "centrimo_maxreg", 1, null, 200)) return false;
  if (!check_num_value("CentriMo E-value threshold", "centrimo_ethresh", 0, null, 10)) return false;
  return true;
}

function centrimo_changed() {
  "use strict";
  if (!/^\s*5\s*$/.test($("centrimo_score").value)) return true;
  if ($("centrimo_maxreg_enable").checked) return true;
  if (!/^\s*10\s*$/.test($("centrimo_ethresh").value)) return true;
  if ($("centrimo_local").checked) return true;
  if (!$("centrimo_store_ids").checked) return true;
  return false;
}

function centrimo_reset(evt) {
  "use strict";
  $("centrimo_score").value = 5;
  $("centrimo_maxreg_enable").checked = false;
  $("centrimo_maxreg").value = 200;
  $("centrimo_maxreg").disabled = true;
  $("centrimo_ethresh").value = 10;
  $("centrimo_local").checked = false;
  $("centrimo_store_ids").checked = true;
}

function fix_reset() {
  "use strict";
  on_ch_dist();
}

function on_form_submit(evt) {
  "use strict";
  if (!check()) {
    evt.preventDefault();
  }
}

function on_form_reset(evt) {
  "use strict";
  window.setTimeout(function(evt) {
    "use strict";
    fix_reset();
  }, 50);
}

function on_ch_dist() {
  "use strict";
  $("meme_sites").style.display = ($('meme_dist').value == 'oops' ? 'none' : 'block');
}

function on_ch_norc() {
  "use strict";
  var norc = $("norc").checked;
  var centrimo_local = $("centrimo_local").checked;
  if (norc && !centrimo_local) {
    if (confirm("CentriMo's localized search works well with this option. Enable it now?")) {
      $("centrimo_local").checked = true;
      toggle_class($('centrimo_opts'), 'expanded', true);
    }
  }
}

function on_load() {
  "use strict";
  // setup norc action
  $("norc").addEventListener("click", on_ch_norc, false);
  // setup meme dist
  on_ch_dist();
  $("meme_dist").addEventListener("change", on_ch_dist, false);
  // add listener to the form to check the fields before submit
  $("memechip_form").addEventListener("submit", on_form_submit, false);
  $("memechip_form").addEventListener("reset", on_form_reset, false);
}

// anon function to avoid polluting global scope
(function() {
  "use strict";
  window.addEventListener("load", function load(evt) {
    "use strict";
    window.removeEventListener("load", load, false);
    on_load();
  }, false);
})();
