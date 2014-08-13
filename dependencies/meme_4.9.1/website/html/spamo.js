var vis_ids = [];

function on_page_show() {
  pasted_sequences_enable(document.getElementById('use_pasted').checked);
  upload_secondaries_enable(document.getElementById('secondary_db').value == 'upload');
  submitted_motif_enable(document.getElementById('use_inline').checked);
  if (document.getElementById('inline_count').value > 0) {
    document.getElementById('inline_span').style.display = 'block';
  }
}

function on_form_submit() {
  return check();
}

function on_form_reset() {
  return true;
}

function on_ch_db() {
  upload_secondaries_enable(document.getElementById('secondary_db').value == 'upload');
}

function upload_secondaries_enable(enable) {
  document.getElementById('db_upload_div').style.display = (enable ? 'block' : 'none');
}

function pasted_sequences_enable(enable) {
  document.getElementById('sequences').disabled = enable;
  document.getElementById('pasted_sequences').style.display = (enable ? 'block' : 'none');
}

function submitted_motif_enable(enable) {
  document.getElementById('primary_motif').disabled = enable;
}

function check() {
  if (!document.getElementById('use_pasted').checked) {
    if (document.getElementById('sequences').value == '') {
      alert("Please input a file of FASTA formatted sequences.\n");
      return false;
    }
  } else {
    if (document.getElementById('pasted_sequences').value == '') {
      alert("Please input FASTA formatted sequences.\n");
      return false;
    }
  }
  if (!document.getElementById('use_inline').checked) {
    if (document.getElementById('primary_motif').value == '') {
      alert("Please input a primary motif file.\n");
      return false;
    }
  }
  if (document.getElementById('secondary_db').value == 'upload') {
    if (document.getElementById('db_upload').value == '') {
      alert("Please input a secondary motif file.\n");
      return false;
    }
  }
  var email = document.getElementById('email').value;
  var email_confirm = document.getElementById('email_confirm').value;
  if (email.indexOf("@") === -1) {
    alert("Please input an email to send the job details to.\n");
    return false;
  }
  if (email != email_confirm) {
    alert("The confirmation email does not match. Please check the email entered.\n");
    return false;
  }
  return true;
}

//
// toggle
//
// Generic toggle visibility of a previously hidden element.
//
function show_toggle(id) {
  var vis = vis_ids[id];
  if (vis) {
    document.getElementById(id).style.display = 'none';
    delete vis_ids[id];
  } else {
    document.getElementById(id).style.display = 'block';
    vis_ids[id] = true;
  }
}

//
// show_hide
//
// Generic show/hide function for making toggleable sections.
//
function show_hide(show_id, hide_id) {
  if (hide_id != '') document.getElementById(hide_id).style.display = 'none';
  if (show_id != '') document.getElementById(show_id).style.display = 'block';
}
