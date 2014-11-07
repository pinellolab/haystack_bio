var vis_ids = [];

function $(id) {
  return document.getElementById(id);
}

function on_page_show() {
  pasted_sequences_enable($('use_pasted').checked);
  compar_pasted_sequences_enable($('use_compar_pasted').checked);
  upload_secondaries_enable($('motif_db').value == 'upload');
  update_compar();
  $("max_region").disabled = !$("use_max_region").checked;
  toggle_class($('adv_opts'), 'expanded', adv_changed());
}

function on_form_submit() {
  return check();
}

function on_form_reset() {
  setTimeout(on_page_show, 50);
  return true;
}

function on_ch_db() {
  upload_secondaries_enable($('motif_db').value == 'upload');
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

function upload_secondaries_enable(enable) {
  $('db_upload_div').style.display = (enable ? 'block' : 'none');
}

function pasted_sequences_enable(enable) {
  $('sequences').disabled = enable;
  $('pasted_sequences').style.display = (enable ? 'block' : 'none');
}

function update_compar() {
  $('compar_sequences_area').style.display = ($('compar_on').checked ? 'block' : 'none');
}

function compar_pasted_sequences_enable(enable) {
  $('compar_sequences').disabled = enable;
  $('compar_pasted_sequences').style.display = (enable ? 'block' : 'none');
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
  if ($("compar_on").checked) {
    if (!$('use_compar_pasted').checked) {
      if ($('compar_sequences').value == '') {
        alert("Please input a file of FASTA formatted sequences for the comparative set.\n");
        return false;
      }
    } else {
      if ($('compar_pasted_sequences').value == '') {
        alert("Please input FASTA formatted sequences for the comparative set.\n");
        return false;
      }
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
  return adv_check();
}

function adv_check() {
  var min_score, use_max_region, max_region, ethresh;
  min_score = +$("min_score").value;
  if (typeof min_score !== "number" || isNaN(min_score)) {
    alert('Please input a number for the minimum score threshold. ' +
        'A typical value would be "5" (without the quotes).\n');
    return false;
  }
  use_max_region = $("use_max_region").checked;
  if (use_max_region) {
    max_region = +$("max_region").value;
    if (typeof max_region !== "number" || isNaN(max_region) || max_region <= 0) {
      alert('Please input a whole number \u2265 1 for the maximum bin size. ' +
          'A typical value would be "200" (without the quotes).\n');
      return false;
    }
  }
  ethresh = +$("ethresh").value;
  if (typeof ethresh !== "number" || isNaN(ethresh) || ethresh <= 0) {
    alert('Please input a number > 0 for the E-value threshold. ' +
        'A typical value would be "10" (without the quotes).\n');
    return false;
  }
  return true;
}

function adv_changed() {
  if ($("bfile").value != "") return true;
  if (!$("strands_both").checked) return true;
  if (!/^\s*5\s*$/.test($("min_score").value)) return true;
  if ($("opt_score").checked) return true;
  if ($("use_max_region").checked) return true;
  if (!/^\s*10\s*$/.test($("ethresh").value)) return true;
  if (!$("store_ids").checked) return true;
  return false;
}

function adv_reset(evt) {
  stop_evt(evt);
  if (!adv_changed()) return;
  if (!confirm("Reset the advanced options to the defaults?")) return;
  //reset options
  $("bfile").value = "";
  $("strands_both").checked = true;
  $("min_score").value = 5;
  $("opt_score").checked = false;
  $("use_max_region").checked = false;
  $("max_region").disabled = true;
  $("max_region").value = 200;
  $("ethresh").value = 10;
  $("store_ids").checked = true;
  toggle_class($('adv_opts'), 'modified', false);
  update_compar();
}
