var vis_ids = [];

function on_page_show() {
  count_enable(document.getElementById('limit_count_enabled').checked);
  paste_pos_enable(document.getElementById('pos_paste').checked);
  paste_neg_enable(document.getElementById('neg_paste').checked);
  enable_negs(document.getElementById('provided_negatives').checked);
}

function enable_negs(enable) {
  document.getElementById('negatives').style.display = (enable ? 'block' : 'none');
}

function reset_form() {
  count_enable(false);
  negatives_enable(false);
}

function click_lce() {
  document.getElementById('limit_count_enabled').click();
}

function count_enable(enable) {
  document.getElementById('limit_count').disabled = !enable;
}

function paste_pos_enable(enable) {
  document.getElementById('positive_sequences').disabled = enable;
  document.getElementById('positive_pasted').style.display = (enable ? 'block' : 'none');
}
function paste_neg_enable(enable) {
  document.getElementById('negative_sequences').disabled = enable;
  document.getElementById('negative_pasted').style.display = (enable ? 'block' : 'none');
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
