
function adv_check() {
  return true;
}

function meme_check() {
  var dist = $("meme_dist");
  var nmotifs = +($('meme_nmotifs').value);
  var minwidth = +($('meme_minw').value);
  var maxwidth = +($('meme_maxw').value);
  var sites_enabled = (dist.options[dist.selectedIndex].value !== "oops");
  var minsites_enabled = $('meme_minsites_enable').checked;
  var minsites = +($('meme_minsites').value);
  var maxsites_enabled = $('meme_maxsites_enable').checked;
  var maxsites = +($('meme_maxsites').value);
  if (typeof nmotifs !== "number" || isNaN(nmotifs) || nmotifs < 1) {
    alert('Please input a whole number \u2265 1 for the MEME motif count. ' +
        'A typical value would be 3.\n');
    return false;
  }
  if (typeof minwidth !== "number" || isNaN(minwidth) || minwidth < 2) {
    alert('Please input a whole number \u2265 2 for the MEME motif ' +
        'minimum width. A typical value would be 6.\n');
    return false;
  }
  if (typeof maxwidth !== "number" || isNaN(maxwidth) || maxwidth < 2 || maxwidth > 300) {
    alert('Please input a whole number \u2265 2 and \u2264 300 for '+
        'the MEME motif maximum width. A typical value would be 30.\n');
    return false;
  }
  if (minwidth > maxwidth) {
    alert('Please select MEME minimum and maximum motif widths where the ' +
        'minimum width is smaller or equal to the maximum width.\n');
    return false;
  }
  if (sites_enabled) {
    if (minsites_enabled) {
      if (typeof minsites !== "number" || isNaN(minsites) || minsites < 2) {
        alert('Please input a whole number \u2265 2 for the MEME motif ' +
            'minimum sites.');
        return false;
      }
    } else {// set to minimum for range test
      minsites = 2;
    }
    if (maxsites_enabled) {
      if (typeof maxsites !== "number" || isNaN(maxsites) || maxsites < 2 || maxsites > 600) {
        alert('Please input a whole number \u2265 2 and \u2264 600 for the ' +
            'MEME motif maximum sites.\n');
        return false;
      }
    } else {
      maxsites = 600;
    }
    if (minsites > maxsites) {
      alert('Please select MEME minimum and maximum motif sites where the ' +
          'minimum sites is smaller or equal to the maximum sites.\n');
      return false;
    }
  }

  return true;
}

function dreme_check() {
  var evalue = +($('dreme_ethresh').value);
  var nmotifs_enabled = $('dreme_nmotifs_enable').checked;
  var nmotifs = +($('dreme_nmotifs').value)
  if (typeof evalue !== "number" || isNaN(evalue) || evalue <= 0) {
    alert('Please input a number > 0 for the DREME E-value threshold. ' +
        'A typical value would be "0.05" (without the quotes).\n');
    return false;
  }
  if (nmotifs_enabled && (typeof nmotifs !== "number" || isNaN(nmotifs) || nmotifs < 1)) {
    alert('Please input a whole number \u2265 1 for the DREME motif count. ' +
        'A typical value would be 10.\n');
    return false;
  }
  return true;
}

function centrimo_check() {
  var score = +($('centrimo_score').value);
  var maxreg_enabled = $('centrimo_maxreg_enable').checked;
  var maxreg = +($('centrimo_maxreg').value);
  var evalue = +($('centrimo_ethresh').value);
  if (typeof score !== "number" || isNaN(score)) {
    alert('Please input a number for the CentriMo minimum site score. ' +
        'A typical value would be 5.\n');
    return false;
  }
  if (maxreg_enabled && (typeof maxreg !== "number" || isNaN(maxreg) || maxreg < 1)) {
    alert('Please input a whole number \u2265 1 for the CentriMo maximum central window size.\n');
    return false;
  }
  if (typeof evalue !== "number" || isNaN(evalue) || evalue <= 0) {
    alert('Please input a number > 0 for the CentriMo central enrichment E-value threshold.\n');
    return false;
  }
  return true;
}

function check() {
  if (!$('use_pasted').checked) {
    if ($('sequences').value == '') {
      alert("Please input a file of FASTA formatted sequences.\n");
      return false;
    }
  } else {
    if ($('pasted_sequences').value == '') {
      alert("Please input FASTA formatted sequences.\n");
      return false;
    }
  }
  if ($('motif_db').value == 'upload') {
    if ($('db_upload').value == '') {
      alert("Please input a secondary motif file.\n");
      return false;
    }
  }
  var email = $('email').value;
  var email_confirm = $('email_confirm').value;
  if (email.indexOf("@") === -1) {
    alert("Please input an email to send the job details to.\n");
    return false;
  }
  if (email != email_confirm) {
    alert("The confirmation email does not match. Please check the email entered.\n");
    return false;
  }
  return adv_check() && meme_check() && dreme_check() && centrimo_check();
}

function adv_changed() {
  if ($("norc").checked) return true;
  if ($("bfile").value != "") return true;
  return false;
}

function adv_reset(evt) {
  stop_evt(evt);
  if (!adv_changed()) return;
  if (!confirm("Reset the Universal options to the defaults?")) return;
  $("norc").checked = false;
  $("bfile").value = "";
  toggle_class($('adv_opts'), 'modified', false);
}

function meme_changed() {
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
  stop_evt(evt);
  if (!meme_changed()) return;
  if (!confirm("Reset the MEME options to the defaults?")) return;
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
  toggle_class($('meme_opts'), 'modified', false);
}

function dreme_changed() {
  if (!/^\s*0\.05\s*$/.test($("dreme_ethresh").value)) return true;
  if ($("dreme_nmotifs_enable").checked) return true;
  return false;
}

function dreme_reset(evt) {
  stop_evt(evt);
  if (!dreme_changed()) return;
  if (!confirm("Reset the DREME options to the defaults?")) return;
  $("dreme_ethresh").value = 0.05;
  $("dreme_nmotifs_enable").checked = false;
  $("dreme_nmotifs").value = 10;
  $("dreme_nmotifs").disabled = true;
  toggle_class($('dreme_opts'), 'modified', false);
}

function centrimo_changed() {
  if (!/^\s*5\s*$/.test($("centrimo_score").value)) return true;
  if ($("centrimo_maxreg_enable").checked) return true;
  if (!/^\s*10\s*$/.test($("centrimo_ethresh").value)) return true;
  if ($("centrimo_local").checked) return true;
  if (!$("centrimo_store_ids").checked) return true;
  return false;
}

function centrimo_reset(evt) {
  stop_evt(evt);
  if (!centrimo_changed()) return;
  if (!confirm("Reset the CentriMo options to the defaults?")) return;
  $("centrimo_score").value = 5;
  $("centrimo_maxreg_enable").checked = false;
  $("centrimo_maxreg").value = 200;
  $("centrimo_maxreg").disabled = true;
  $("centrimo_ethresh").value = 10;
  $("centrimo_local").checked = false;
  $("centrimo_store_ids").checked = true;

  toggle_class($('centrimo_opts'), 'modified', false);
}

function upload_secondaries_enable(enable) {
  $('db_upload_div').style.display = (enable ? 'block' : 'none');
}

function pasted_sequences_enable(enable) {
  $('sequences').disabled = enable;
  $('pasted_sequences').style.display = (enable ? 'block' : 'none');
}

function on_page_show() {
  pasted_sequences_enable($('use_pasted').checked);
  upload_secondaries_enable($('motif_db').value == 'upload');
  toggle_class($('adv_opts'), 'expanded', adv_changed());
  toggle_class($('meme_opts'), 'expanded', meme_changed());
  toggle_class($('dreme_opts'), 'expanded', dreme_changed());
  toggle_class($('centrimo_opts'), 'expanded', centrimo_changed());

  $("meme_minsites").disabled = !$("meme_minsites_enable").checked;
  $("meme_maxsites").disabled = !$("meme_maxsites_enable").checked;
  $("meme_sites").style.display = ($('meme_dist').value == 'oops' ? 'none' : 'block');

  $("dreme_nmotifs").disabled = !$("dreme_nmotifs_enable").checked;

  $("centrimo_maxreg").disabled = !$("centrimo_maxreg_enable").checked;
}

function on_form_submit() {
  return check();
}

function on_form_reset() {
  return true;
}

function on_ch_db() {
  upload_secondaries_enable($('motif_db').value == 'upload');
}

function on_ch_dist() {
  $("meme_sites").style.display = ($('meme_dist').value == 'oops' ? 'none' : 'block');
}

function on_ch_norc() {
  var norc = $("norc").checked;
  var centrimo_local = $("centrimo_local").checked;
  if (norc && !centrimo_local) {
    if (confirm("CentriMo's localized search works well with this option. Enable it now?")) {
      $("centrimo_local").checked = true;
      toggle_class($('centrimo_opts'), 'expanded', true);
    }
  }
}

function on_ch_bfile() {
  if (window.File && window.FileReader && window.FileList) {
    var input = $("bfile");
    if (input.files.length == 1) {
      reader = new FileReader();
      reader.onload = function(evt) {
        evt = evt || window.event;
        var info = validate_background(evt.target.result);
        if (!info.ok) {
          alert("Background file is not valid. " + 
              "Reason: " + info.reason + " on line " + info.line);
          input.value = ""; 
        } else if (info.alphabet.toUpperCase() != "ACGT") {
          alert("Background file is not valid. " + 
              "Reason: alphabet is not DNA");
          input.value = "";
        }
      };
      reader.readAsText(input.files[0]);
    }
  }
}
