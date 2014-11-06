var show_opts_link;
var task_queue = new Array();
var task_delay = 100;
var my_alphabet;
var query_pspm;
function LoadQueryTask(target_id, query_name) {
  "use strict";
  this.target_id = target_id;
  this.query_name = query_name;
  this.run = LoadQueryTask_run;
}
function LoadQueryTask_run() {
  "use strict";
  var alpha, query_pspm;
  alpha = new Alphabet("ACGT");
  query_pspm = new Pspm(document.getElementById('q_' + this.query_name).value, this.query_name);
  replace_logo(logo_1(alpha, "Tomtom", query_pspm), this.target_id, 0.5, "Preview of " + this.query_name, "block");
}
function FixLogoTask(image_id, query_name, target_db_id, target_name, target_rc, target_offset) {
  "use strict";
  this.image_id = image_id;
  this.query_name = query_name;
  this.target_db_id = target_db_id;
  this.target_name = target_name;
  this.target_rc = target_rc;
  this.target_offset = target_offset;
  this.run = FixLogoTask_run;
}
function FixLogoTask_run() {
  "use strict";
  var image, canvas, alpha, query_pspm, target_pspm, logo;
  // create a logo
  image = document.getElementById(this.image_id);
  canvas = create_canvas(image.width, image.height, image.id, image.title, image.style.display);
  if (canvas == null) return;
  alpha = new Alphabet("ACGT");
  query_pspm = new Pspm(document.getElementById('q_' + this.query_name).value, this.query_name);
  target_pspm = new Pspm(document.getElementById('t_' + this.target_db_id + '_' + this.target_name).value, this.target_name);
  if (this.target_rc) target_pspm.reverse_complement(alpha);
  logo = logo_2(alpha, "Tomtom", target_pspm, query_pspm, this.target_offset);
  draw_logo_on_canvas(logo, canvas, true, 1);
  image.parentNode.replaceChild(canvas, image);
}
function push_task(task) {
  "use strict";
  task_queue.push(task);
  if (task_queue.length == 1) {
    window.setTimeout("process_tasks()", task_delay);
  }
}
function process_tasks() {
  "use strict";
  var task;
  if (task_queue.length == 0) return; //no more tasks
  //get next task
  task = task_queue.shift();
  task.run();
  //allow UI updates between tasks
  window.setTimeout("process_tasks()", task_delay);
}
function showHidden(prefix) {
  "use strict";
  document.getElementById(prefix + '_activator').style.display = 'none';
  document.getElementById(prefix + '_deactivator').style.display = 'block';
  document.getElementById(prefix + '_data').style.display = 'block';
}
function hideShown(prefix) {
  "use strict";
  document.getElementById(prefix + '_activator').style.display = 'block';
  document.getElementById(prefix + '_deactivator').style.display = 'none';
  document.getElementById(prefix + '_data').style.display = 'none';
}
function show_more(query_index, match_index, query_id, target_id, offset, rc) {
  "use strict";
  var link, match_table, match_row, query_pspm, query_length, target_pspm;
  var target_length, total_length, download_row, cell, box, form, tbl, row;
  // create the download logo buttons
  link = document.getElementById("show_" + query_index + "_" + match_index);
  match_table = document.getElementById("table_" + query_index);
  match_row = document.getElementById("tr_" + query_index + "_" + match_index);
  query_pspm = document.getElementById(query_id).value;
  query_length = (new Pspm(query_pspm)).get_motif_length();
  target_pspm = document.getElementById(target_id).value;
  target_length = (new Pspm(target_pspm)).get_motif_length();
  total_length = (offset < 0 ? 
      Math.max(query_length, target_length - offset) : 
      Math.max(query_length + offset, target_length));
  download_row = document.getElementById("tr_download");
  if (download_row) {
    //remove existing
    match_table.deleteRow(download_row.rowIndex);
    show_opts_link.style.visibility = "visible";
  }
  show_opts_link = link;
  show_opts_link.style.visibility = "hidden";
  download_row = match_table.insertRow(match_row.rowIndex + 1);
  download_row.id = "tr_download";
  cell = download_row.insertCell(0);
  cell.colSpan = 2;
  cell.style.verticalAlign = "top";
  //create the download options
  box = document.createElement("div");
  box.appendChild(create_title("Create custom LOGO", "pop_match_download"));
  form = create_form(target_url, "post");
  add_hfield(form, "mode", "logo");
  add_hfield(form, "version", version);
  add_hfield(form, "background", background);
  add_hfield(form, "target_id", target_id);
  add_hfield(form, "target_length", target_length);
  add_hfield(form, "target_pspm", target_pspm);
  add_hfield(form, "target_rc", rc);
  add_hfield(form, "query_id", query_id);
  add_hfield(form, "query_length", query_length);
  add_hfield(form, "query_pspm", query_pspm);
  add_hfield(form, "query_offset", offset);
  tbl = document.createElement("table");
  row = tbl.insertRow(tbl.rows.length);
  add_label(row.insertCell(-1),"Image Type", "pop_dl_type");
  add_label(row.insertCell(-1), "Error bars", "pop_dl_err");
  add_label(row.insertCell(-1), "SSC", "pop_dl_ssc");
  add_label(row.insertCell(-1), "Flip", "pop_dl_flip");
  add_label(row.insertCell(-1), "Width", "pop_dl_width");
  add_label(row.insertCell(-1), "Height", "pop_dl_height");
  row.insertCell(-1).appendChild(document.createTextNode("\u00A0"));
  row = tbl.insertRow(tbl.rows.length);
  add_combo(row.insertCell(-1), "image_type", {"png" : "PNG (for web)", "eps" : "EPS (for publication)"}, "png");
  add_combo(row.insertCell(-1), "error_bars", {"1" : "yes", "0" : "no"}, "1");
  add_combo(row.insertCell(-1), "small_sample_correction", {"0" : "no", "1" : "yes"}, "0");
  add_combo(row.insertCell(-1), "flip", {"0" : "no", "1" : "yes"}, "0");
  add_tfield(row.insertCell(-1), "image_width", total_length, 2);
  add_tfield(row.insertCell(-1), "image_height", 10, 2);
  add_submit(row.insertCell(-1), "Download");
  form.appendChild(tbl);
  box.appendChild(form);
  cell.appendChild(box);
}
function create_title(title, help_topic) {
  "use strict";
  var title_ele, words, link, helpdiv;
  title_ele = document.createElement("h4");
  words = document.createTextNode(title + "\u00A0"); // non-breaking space
  title_ele.appendChild(words);
  if (help_topic) {
    title_ele.appendChild(help_button(help_topic));
  }
  return title_ele;
}
function add_hfield(form, name, value) {
  "use strict";
  var hidden;
  hidden = document.createElement("input");
  hidden.type = "hidden";
  hidden.name = name;
  hidden.value = value;
  form.appendChild(hidden);
}
function add_label(ele, label, help_topic) {
  "use strict";
  var label_b;
  ele.className = "downloadTd";
  label_b = document.createElement("b");
  label_b.appendChild(document.createTextNode(label));
  ele.appendChild(label_b);
  if (help_topic) {
    ele.appendChild(document.createTextNode("\u00A0"));
    ele.appendChild(help_button(help_topic));
  }
}
function add_tfield(ele, name, value, size) {
  "use strict";
  var field;
  ele.className = "downloadTd";
  field = document.createElement("input");
  field.type = "text";
  field.style.marginLeft = "5px";
  field.name = name;
  field.value = value;
  field.size = size;
  ele.appendChild(field);
}
function add_combo(ele, name, options, selected) {
  "use strict";
  var combo, value, display, option;
  ele.className = "downloadTd";
  combo = document.createElement("select");
  combo.name = name;
  combo.style.marginLeft = "5px";
  for (value in options) {
    //exclude inherited properties and undefined properties
    if (!options.hasOwnProperty(value) || options[value] === undefined) continue;
    display = options[value];
    option = document.createElement("option");
    option.value = value;
    if (value == selected) option.selected = true;
    option.appendChild(document.createTextNode(display));
    combo.appendChild(option);
  }
  ele.appendChild(combo);
}
function add_submit(ele, value) {
  "use strict";
  var submit;
  ele.className = "downloadTd";
  submit = document.createElement("input");
  submit.type = "submit";
  submit.value = value;
  ele.appendChild(submit);
}
function create_form(path,method) {
  "use strict";
  var form;
  method = method || "post";
  form = document.createElement("form");
  form.method = method;
  form.action = path;
  return form;
}

