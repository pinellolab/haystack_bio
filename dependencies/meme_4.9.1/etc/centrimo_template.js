var palate = ["white", "cyan", "blue", "#FF00FF", "#00FF00", "red", "orange", "#008000", "#8A2BE2", "black"];
var text_palate = ["black", "black", "white", "white", "black", "white", "black", "white", "white", "white"];
var INT_MAX = 9007199254740992;

var s_motifs = [];
var s_swatches = [];
var s_logos = []
var s_count = 0;
var swap = null;
var dna_alphabet = new Alphabet("ACGT", "A 0.25 C 0.25 G 0.25 T 0.25");
var logo_timer = null; // timer to draw motif when the mouse pauses on a row
var logo_motif = null; // motif to draw when timer expires
var logo_row = null; // the row (or tbody) that the mouse is over
var legend_x = 1000;
var legend_y = 1000;
var union_seq_ids = [];
var intersect_seq_ids = [];
var graph_metrics = new CentrimoGraphMetrics(null, 0, 0, 0, 0, 0, 0);
var graph_click = {"start": 0, "stop": 0};
var zooms = [];

var sort_table = {
  "motif": [
    {"name": "<i>E</i>-value", "fn": sort_motif_evalue, "priority": 1, "pair": "evalue"},
    {"name": "Fisher <i>E</i>-value", "fn": sort_motif_fisherpv, "priority": 2, 
      "show": data['options']['neg_sequences'], "pair": "fisherpv"},
    {"name": "Region Center", "fn": sort_motif_center, "pair": "center"},
    {"name": "Region Width", "fn": sort_motif_spread, "pair": "spread"},
    {"name": "Region Matches", "fn": sort_motif_sites, "pair": "sites"},
    {"name": "<i>p</i>-value (Neg Data)", "fn": sort_motif_neg_pvalue,
      "show": data['options']['neg_sequences'], "pair": "neg_pvalue"},
    {"name": "Region Matches in (Neg Data)", "fn": sort_motif_neg_sites, 
      "show": data['options']['neg_sequences'], "pair": "neg_sites"},
    {"name": "Matthew's Correlation Coefficient", "fn": sort_motif_mcc, 
      "show": data['options']['mcc'], "pair": "mcc"},
    {"name": "Total Matches", "fn": sort_motif_total_sites, "pair": "evalue"},
    {"name": "Max Probability", "fn": sort_motif_probability, "pair": "evalue"},
    {"name": "ID", "fn": sort_motif_id, "pair": "evalue"},
    {"name": "Name", "fn": sort_motif_alt, "pair": "evalue"},
  ],
  "region": [
    {"id": "evalue", "name": "<i>E</i>-value", "fn": sort_peak_evalue,
      "priority": 1},
    {"id": "fisherpv", "name": "Fisher <i>E</i>-value", "fn": sort_peak_fisherpv,
      "priority": 2, "show": data['options']['neg_sequences']},
    {"id": "center", "name": "Region Center", "fn": sort_peak_center},
    {"id": "spread", "name": "Region Width", "fn": sort_peak_spread},
    {"id": "sites", "name": "Region Matches", "fn": sort_peak_sites},
    {"id": "neg_pvalue", "name": "<i>p</i>-value (Neg Data)",
      "fn": sort_peak_neg_pvalue, "show": data['options']['neg_sequences']},
    {"id": "neg_sites", "name": "Region Matches in (Neg Data)",
      "fn": sort_peak_neg_sites, "show": data['options']['neg_sequences']},
    {"id": "mcc", "name": "Matthew's Correlation Coefficient",
      "fn": sort_peak_mcc, "show": data['options']['mcc']}
  ],
};

pre_load_setup();


/*
 * name_from_source
 *
 * Makes a file name more human friendly to read.
 */
function name_from_source(source) {
  "use strict";
  var file, noext;
  if (source == "-") {
    return "-"
  }
  //assume source is a file name
  file = source.replace(/^.*\/([^\/]+)$/,"$1");
  noext = file.replace(/\.[^\.]+$/, "");
  return noext.replace(/_/g, " ");
}


//
// See http://stackoverflow.com/questions/814613/how-to-read-get-data-from-a-url-using-javascript
// Slightly modified with information from
// https://developer.mozilla.org/en/DOM/window.location
//
function parse_params() {
  "use strict";
  var search, queryStart, queryEnd, query, params, nvPairs, i, nv, n, v;
  search = window.location.search;
  queryStart = search.indexOf("?") + 1;
  queryEnd   = search.indexOf("#") + 1 || search.length + 1;
  query      = search.slice(queryStart, queryEnd - 1);

  if (query === search || query === "") return {};

  params  = {};
  nvPairs = query.replace(/\+/g, " ").split("&");

  for (i = 0; i < nvPairs.length; i++) {
    nv = nvPairs[i].split("=");
    n  = decodeURIComponent(nv[0]);
    v  = decodeURIComponent(nv[1]);
    // allow a name to be used multiple times
    // storing each value in the array
    if (!(n in params)) {
      params[n] = [];
    }
    params[n].push(nv.length === 2 ? v : null);
  }
  return params;
}

/*
 * pre_load_setup
 *
 *  Sets up initial variables which may be
 *  required for the HTML document creation.
 */
function pre_load_setup() {
  "use strict";
  var i, motifs, params, showlist, show, parts, db_i, id, j, motif;
  var seq_db, db, dbs, sites, max_site, max_site_pos;
  for (i = 0; i < palate.length -1; i++) {
    s_motifs[i] = null;
    s_swatches[i] = null;
    s_logos[i] = null;
  }
  s_count = 0;
  motifs = data["motifs"];
  if (data["options"]["neg_sequences"]) {
    motifs.sort(sort_motif_fisherpv);
  } else {
    motifs.sort(sort_motif_evalue);
  }
  // put a copy of the best evalue in the motif
  for (i = 0; i < motifs.length; i++) {
    motif = motifs[i];
    motif["best_log_adj_pvalue"] = motif["peaks"][0]["log_adj_pvalue"];
    // calculate the best probability
    max_site = 0;
    max_site_pos = 0;
    sites = motif["sites"];
    for (j = 0; j < sites.length; j++) {
      if (sites[j] > max_site) {
        max_site = sites[j];
        max_site_pos = j;
      }
    }
    motif["max_prob"] = max_site / motif["total_sites"];
    motif["max_prob_loc"] = max_site_pos + (motif["len"] / 2) - (data["seqlen"] / 2);
  }
  params = parse_params();
  if (params["show"]) {
    showlist = params["show"];
    for (i = 0; i < showlist.length; i++) {
      show = showlist[i];
      parts = show.match(/^db(\d+) (.+)$/);
      db_i = parseInt(parts[1]);
      id = parts[2];
      // loop over the motifs looking for a match
      for (j = 0; j < motifs.length; j++) {
        motif = motifs[j];
        if (motif["db"] == db_i && motif["id"] == id) {
          motif["colouri"] = s_count + 1;
          s_motifs[s_count] = motif;
          s_count++;
          break;
        }
      }
      // give up if we've used all the available colours
      if ((s_count + 1) >= palate.length) break; 
    }
  } else {
    s_count = Math.min(3, motifs.length);
    for (i = 0; i < s_count; i++) {
      motif = motifs[i];
      motif["colouri"] = i+1;
      s_motifs[i] = motif;
    }
  }
  calculate_seq_ids();
  // get the name of the sequence db
  seq_db = data['sequence_db'];
  if (!seq_db['name']) seq_db['name'] = name_from_source(seq_db['source']);
  // get the names of the motif databases
  dbs = data['motif_dbs'];
  for (i = 0; i < dbs.length; i++) {
    db = dbs[i];
    if (!db['name']) db['name'] = name_from_source(db['source']);
  }
}

function num_keys(e) {
  if (!e) var e = window.event;
  var code = (e.keyCode ? e.keyCode : e.which);
  var keychar = String.fromCharCode(code);
  var numre = /\d/;
  // only allow 0-9 and various control characters (Enter, backspace, delete)
  if (code != 8 && code != 9 && code != 13 && code != 46 && !numre.test(keychar)) {
    e.preventDefault();
  }
}

/*
 * toggle_column
 *
 * Adds or removes a class from the table displaying the 
 * centrally enriched motifs. This is primary used to set the visibility
 * of columns by using css rules. If the parameter 'show' is not passed
 * then the existence of the class will be toggled, otherwise it will be
 * included if show is false.
 */
function toggle_column(cls) {
  toggle_class($("motifs"), cls);
}

/*
 * toggle_filter
 *
 * Called when the user clicks a checkbox
 * to enable/disable a filter option.
 */
function toggle_filter(chkbox, filter_id) {
  var filter = $(filter_id);
  filter.disabled = !(chkbox.checked);
  if (!filter.disabled) {
    filter.focus();
    if (filter.select) filter.select();
  }
}

/*
 * enable_filter
 *
 * Called when the user clicks a filter label.
 * Enables the filter.
 */
function enable_filter(chkbox_id, filter_id) {
  var chkbox = $(chkbox_id);
  if (!chkbox.checked) {
    var filter = $(filter_id);
    $(chkbox_id).checked = true;
    filter.disabled = false;
    filter.focus();
    if (filter.select) filter.select();
  }
}

/*
 * update_filter
 *
 * If the key event is an enter key press then
 * update the filter on the CEM table
 */
function update_filter(e) {
  if (!e) var e = window.event;
  var code = (e.keyCode ? e.keyCode : e.which);
  if (code == 13) {
    e.preventDefault();
    make_CEM_table();
  }
}

/*
 * toggle_lock
 *
 */

/*
 * clear_selection
 *
 * Called when the user clicks the X in the
 * title of the centrally enriched motifs and
 * causes all the motifs to be deselected.
 */
function clear_selection() {
  for (var i = 1; i < palate.length; i++) {
    var motif = s_motifs[i-1];
    var swatch = s_swatches[i-1];
    s_motifs[i-1] = null;
    s_swatches[i-1] = null;
    s_logos[i-1] = null;
    if (motif) delete motif['colouri'];
    if (swatch) swatch.style.backgroundColor = palate[0];
  }
  s_count = 0;
  if (!data['options']['noseq']) {
    //reset_sequences_IDs_list();
    union_seq_ids = [];
    intersect_seq_ids = [];
    display_seq_ids();
  }
  make_PM_table();
  make_MP_graph();
}

/*
 * toggle_graph_motif
 *
 * Activated when the user clicks on a swatch in the centrally enriched motifs 
 * table. If the motif is already selected it deselects it otherwise it picks 
 * an unused colour and selects it for graphing.
 */
function toggle_graph_motif(e) {
  "use strict";
  var swatch;
  if (!e) var e = window.event;
  if (e.target) swatch = e.target;
  else if (e.srcElement) swatch = e.srcElement;
  // in case we land on a text node
  if (swatch.nodeType == 3) swatch = swatch.parentNode;
  // find the containing tbody
  var group = find_parent(swatch.parentNode, "peak_group");
  // get the attached motif
  var motif = group['data_motif'];
  if (motif['colouri'] && motif['colouri'] != 0) {
    // deselect the motif
    var i = motif['colouri'];
    s_motifs[i-1] = null;
    s_swatches[i-1] = null;
    s_logos[i-1] = null;
    delete motif['colouri'];
    swatch.style.backgroundColor = palate[0];
    s_count--;
    if (!data['options']['noseq']) calculate_seq_ids();
  } else {
    // select an unused colour
    var i;
    for (i = 1; i < palate.length; i++) {
      if (!s_motifs[i-1]) break;
    }
    if (i == palate.length) {
      alert("All graph colours used. Please deselect motifs before adding more.");
      return;
    }
    s_motifs[i-1] = motif;
    s_swatches[i-1] = swatch;
    motif['colouri'] = i;
    swatch.style.backgroundColor = palate[i];
    s_count++;
    if (!data['options']['noseq']) {
      if (s_count == 1) {
        union_seq_ids = motif["seqs"].slice(0);
        intersect_seq_ids = union_seq_ids.slice(0);
      } else {
        union_seq_ids = union_nums(union_seq_ids, motif["seqs"]);
        intersect_nums(intersect_seq_ids, motif["seqs"]);
      }
    }
  }
  if (!data['options']['noseq']) display_seq_ids();
  make_PM_table();
  make_MP_graph();
}

/*
 * move_legend
 *
 * Called when the user clicks on the graph
 * to move the legend location.
 */
function move_legend(e) {
  var target;
  if (!e) var e = window.event;
  if (e.target) target = e.target;
  else if (e.srcElement) target = e.srcElement;
  var elemXY = coords(target);
  var posx = 0;
  var posy = 0;
  if (e.pageX || e.pageY)   {
    posx = e.pageX;
    posy = e.pageY;
  }
  else if (e.clientX || e.clientY)   {
    posx = e.clientX + document.body.scrollLeft
      + document.documentElement.scrollLeft;
    posy = e.clientY + document.body.scrollTop
      + document.documentElement.scrollTop;
  }
  var x = posx - elemXY[0];
  var y = posy - elemXY[1];
  legend_x = x - graph_metrics.legend_width/2;
  legend_y = y - graph_metrics.legend_height/2;
  if (parseInt($("legend").value) != 0) make_MP_graph();
}

function highlight_peak(e) {
  var target, pop;
  pop = $("pop_peak");
  if (pop.style.visibility === "visible") return;
  if (!e) var e = window.event;
  if (e.target) target = e.target;
  else if (e.srcElement) target = e.srcElement;
  while (!target['data_peak']) {
    if (target.nodeName == 'BODY') return;
    target = target.parentNode;
  }
  var peak = target['data_peak'];

  var l = graph_metrics.left_mark;
  var w = graph_metrics.right_mark - graph_metrics.left_mark;
  var d = graph_metrics.right_val - graph_metrics.left_val;
  var unit = w / d;

  var peak_width = peak["spread"];
  var peak_left = peak["center"] - (peak["spread"] / 2);
  var peak_offscreen = 0;
  if (peak_left < graph_metrics.left_val) {
    peak_offscreen = graph_metrics.left_val - peak_left;
    peak_width = Math.max(0, peak_width - peak_offscreen);
    peak_left = graph_metrics.left_val;
  } else if ((peak_left + peak_width) > graph_metrics.right_val) {
    peak_offscreen = (peak_left + peak_width) - graph_metrics.right_val;
    peak_width = Math.max(0, peak_width - peak_offscreen);
    peak_left = Math.min(peak_left, graph_metrics.right_val);
  }
  
  if (peak_width > 0) {
    pop.style.width = (peak_width * unit) + "px";
    pop.style.left = (l + ((peak_left - graph_metrics.left_val) * unit)) + "px";
    pop.style.visibility = "visible";
  }
}

function dehighlight_peak(e) {
  $("pop_peak").style.visibility = "hidden";
}

/*
 * hover_logo
 *
 * Activated when the user hovers their cursor over a row in the centrally 
 * enriched motifs table. After a fifth of a second delay, displays a box with 
 * the logo and reverse complement logo.
 */
function hover_logo(e) {
  var popup;
  if (this === logo_row) return;
  logo_row = this;
  if (logo_timer) clearTimeout(logo_timer);
  logo_row.addEventListener('mousemove', move_logo, false);
  popup = $("logo_popup");
  if (popup["data_motif"] != logo_row["data_motif"]) {
    logo_timer = setTimeout(popup_logo, 200);
  } else {
    popup.style.display = "block";
  }
}

/*
 * dehover_logo
 *
 * Activated when the user moves their cursor off a row in the centrally 
 * enriched motifs table. Hides the logo box (or stops it from being displayed).
 */
function dehover_logo(e) {
  var popup = $("logo_popup");
  popup.style.display = "none";
  if (logo_timer) clearTimeout(logo_timer);
  if (logo_row) logo_row.removeEventListener('mousemove', move_logo, false);
  logo_row = null;
  logo_timer = null;
}

/*
 * move_logo
 * 
 * keeps the motif logo at a set distance from the cursor.
 */
function move_logo(e) {
  var popup = $("logo_popup");
  popup.style.left = (e.pageX + 20) + "px";
  popup.style.top = (e.pageY + 20) + "px";
}

/*
 * popup_logo
 *
 * Activated when the user has had the cursor over a row in the centrally 
 * enriched motifs table for longer than 1/5th of a second. It draws the
 * motif logos in a popup and displays the popup.
 */
function popup_logo() {
  if (logo_row == null) return;
  var motif = logo_row["data_motif"];
  var pspm = new Pspm(motif['pwm'], motif['id'], 0, 0, 
      motif['motif_nsites'], motif['motif_evalue']);
  var canvas = $("logo_popup_canvas");
  var logo = logo_1(dna_alphabet, "", pspm);
  draw_logo_on_canvas(logo, canvas, false, 0.5);
  var pspm_rc = pspm.copy().reverse_complement(dna_alphabet);
  var canvas_rc = $("logo_popup_canvas_rc");
  var logo_rc = logo_1(dna_alphabet, "", pspm_rc);
  draw_logo_on_canvas(logo_rc, canvas_rc, false, 0.5);

  var popup = $("logo_popup");
  popup.style.display = "block";
  popup['data_motif'] = motif;
  logo_timer = null;
}

/*
 * swap_colour
 *
 * Activated when the user clicks a swatch in the plotted motifs table.
 * If a swatch has already been selected then swap colours with this one,
 * otherwise record which swatch has been selected.
 */
function swap_colour(e) {
  var swatch;
  if (!e) var e = window.event;
  if (e.target) swatch = e.target;
  else if (e.srcElement) swatch = e.srcElement;
  var row = swatch;
  while (!row['motif']) row = row.parentNode;
  var motif = row['motif'];
  if (swap == null) {
    swatch.appendChild(document.createTextNode("\u21c5"));
    swap = motif;
  } else if (swap == motif) {
    while (swatch.firstChild) swatch.removeChild(swatch.firstChild);
    swap = null;
  } else {
    var swapi = swap['colouri'];
    var motifi = motif['colouri'];
    motif['colouri'] = swapi;
    swap['colouri'] = motifi;
    // swap swatches
    var temp = s_swatches[swapi-1];
    s_swatches[swapi-1] = s_swatches[motifi-1];
    s_swatches[motifi-1] = temp;
    // swap motifs
    temp = s_motifs[swapi-1];
    s_motifs[swapi-1] = s_motifs[motifi-1];
    s_motifs[motifi-1] = temp;
    // swap logos
    temp = s_logos[swapi-1];
    s_logos[swapi-1] = s_logos[motifi-1];
    s_logos[motifi-1] = temp;
    // update swatch colours
    s_swatches[swapi-1].style.backgroundColor = palate[swapi];
    s_swatches[motifi-1].style.backgroundColor = palate[motifi];
    swap = null;
    make_PM_table();
    make_MP_graph();
  }
}

/*
 * set_colour
 *
 * Activated when the user clicks a swatch in the unused colours section.
 * If a swatch has already been selected then set its colour to this one,
 * otherwise warn the user that they must select a motif swatch first.
 */
function set_colour(e) {
  var swatch;
  if (!e) var e = window.event;
  if (e.target) swatch = e.target;
  else if (e.srcElement) swatch = e.srcElement;
  var colouri = swatch['colouri'];
  if (swap) {
    var swapi = swap['colouri'];
    swap['colouri'] = colouri;
    s_swatches[colouri-1] = s_swatches[swapi-1];
    s_motifs[colouri-1] = s_motifs[swapi-1];
    s_logos[colouri-1] = s_logos[swapi-1];
    s_swatches[swapi-1] = null;
    s_motifs[swapi-1] = null;
    s_logos[swapi-1] = null;
    s_swatches[colouri-1].style.backgroundColor = palate[colouri];
    swap = null;
    make_PM_table();
    make_MP_graph();
  } else {
    alert("You must select a motif to set to this colour first.");
  }
}

/*
 * page_loaded
 *
 * Called when the page has loaded for the first time.
 */
function page_loaded() {
  first_load_setup();
  post_load_setup();
}

/*
 * page_loaded
 *
 * Called when a cached page is reshown.
 */
function page_shown(e) {
  if (e.persisted) post_load_setup();
}

/*
 * union_nums
 *
 * Returns a union of two sorted lists of numbers.
 */
function union_nums(list1, list2) {
  "use strict";
  var i, j, list_out;
  i = j = 0;
  list_out = [];
  while (i < list1.length && j < list2.length) {
    if (list1[i] < list2[j]) {
      list_out.push(list1[i]);
      i++;
    } else if (list1[i] === list2[j]) {
      list_out.push(list1[i]);
      i++;
      j++;
    } else { // list1[i] > list2[j]
      list_out.push(list2[j]);
      j++;
    }
  }
  if (i < list1.length) {
    for (; i < list1.length; i++) {
      list_out.push(list1[i]);
    }
  } else {
    for (; j < list2.length; j++) {
      list_out.push(list2[j]);
    }
  }
  return list_out;
}

/*
 * intersect_nums
 *
 * Removes numbers from the target_list which do not also appear in the
 * incoming_list. Both lists must be sorted.
 */
function intersect_nums(target_list, incoming_list) {
  "use strict";
  var i, j, shift;
  shift = i = j = 0;
  while (i < target_list.length && j < incoming_list.length) {
    if (target_list[i] > incoming_list[j]) {
      j++;
    } else if (target_list[i] < incoming_list[j]) {
      i++;
      shift++;
    } else {
      if (shift > 0) target_list[i - shift] = target_list[i];
      i++;
      j++;
    }
  }
  target_list.length = i - shift;
}

function calculate_seq_ids() {
  var i, motif;
  // calculate sequence id sets
  if (!data['options']['noseq']) {
    if (s_count == 0) {
      union_seq_ids = [];
      intersect_seq_ids = [];
    } else {
      for (i = 0; i < s_motifs.length; i++) {
        motif = s_motifs[i];
        if (motif) {
          union_seq_ids = motif["seqs"].slice(0);
          intersect_seq_ids = union_seq_ids.slice(0);
          break;
        }
      }
      for (; i < s_motifs.length; i++) {
        motif = s_motifs[i];
        if (motif === null) continue;
        union_seq_ids = union_nums(union_seq_ids, motif["seqs"]);
        intersect_nums(intersect_seq_ids, motif["seqs"]);
      }
    }
  }
}

function display_seq_ids() {
  var u_n_cont, u_p_cont, i_n_cont, i_p_cont, i_s_cont;
  var union_count, intersect_count, total_count;
  var union_percent, intersect_percent, intersect_seqs, i;
  var sequences;
  sequences = data["sequences"];
  union_count = union_seq_ids.length;
  intersect_count = intersect_seq_ids.length;
  total_count = sequences.length;
  union_percent = Math.round((union_count / total_count) * 100);
  intersect_percent = Math.round((intersect_count / total_count) * 100);
  u_n_cont = $('matching_union_number');
  u_p_cont = $('matching_union_percentage');
  i_n_cont = $('matching_intersection_number');
  i_p_cont = $('matching_intersection_percentage');
  i_s_cont = $('matching_intersection_sequences');
  u_n_cont.innerHTML = "";
  u_n_cont.appendChild(document.createTextNode(union_count));
  u_p_cont.innerHTML = "";
  u_p_cont.appendChild(document.createTextNode(union_percent));
  i_n_cont.innerHTML = "";
  i_n_cont.appendChild(document.createTextNode(intersect_count));
  i_p_cont.innerHTML = "";
  i_p_cont.appendChild(document.createTextNode(intersect_percent));
  intersect_seqs = [];
  for (i = 0; i < intersect_count; i++) {
    intersect_seqs.push(sequences[intersect_seq_ids[i]]);
  }
  i_s_cont.value = intersect_seqs.join("\n");
}

/*
 * find_highest_peaks
 *
 * Locate highest peaks for every motifs
 * Compute every x position and keep the highest one
 */
/*
function find_highest_peaks(smoothing_type) {
  var sequence_length=data['seqlen'];
  var windo=parseInt($("windo").value);
  var half_window = windo / 2;
  var weights = [];
  var tbl = $("motifs");
  var tbody = tbl.tBodies[0];
  if (smoothing_type==1) { // Only computed for weighted moving average
    var total_weight = 0;
    for (var i = 0; i < windo; i++) {
      var pos = i + 0.5;
      var weight = (pos < half_window ? pos / half_window : (windo - pos) / half_window);
      weights.push(weight);
      total_weight += weight;
    }
  }
  for (var i = 0; i < data['motifs'].length; i++) { // For each motif
    var motif = data['motifs'][i];
    var sites = motif['sites'];
    var max_prob=-1;
    var index_max_prob=-1;
    var xpos = (motif.len / 2) - (sequence_length / 2) + half_window;
    var end = sites.length - windo;
    for (var j = 0; j < end; j++, xpos += 1) {
      var sum = 0;
      for (var k = 0; k < windo; k++) {
        sum += (smoothing_type==1) ? sites[j + k] * weights[k] : sites[j + k];
      }
      var avg = (smoothing_type==1) ? sum / total_weight : sum / windo;
      var prob = avg / motif['total_sites'];
      if (prob>max_prob) { // If Y axis value is higher than saved one
        max_prob=prob; // Save it
        index_max_prob=Math.floor(xpos); // Save the location
      }
    }
    motif["highest_peak"]=index_max_prob; // Save highest peak location for this motif
  }
}*/

/*
 * first_load_setup
 *
 * Setup state that is dependent on everything having been loaded already.
 * On browsers which cache state this is only run once.
 */
function first_load_setup() {
  if (data['options']['noseq']) {
    // no sequence ids to display
    $('seq_area').style.display='none';
  }
  if (!data['options']['local']) {
    // bins are centered, there is only one peak
    $('show_blocation').checked = false;
    $('CB_show_blocation').style.display='none';
    $('box_peak_sort').style.display = 'none';
    toggle_class($('motifs'), 'hide_OEbins', true);
  }
  if (!data['options']['neg_sequences']) {
    $('show_fisherpv').checked = false;
    $('show_negtsites').checked = false;
    $('show_negbsites').checked = false;
    $('show_negative_binomial').checked = false;
    // no negative sites, so hide negative bin sites and total
    $('CB_show_plot_negative').style.display= 'none';
    $('plot_negative').value = 0;
    $('CB_show_negbsites').style.display= 'none';
    $('CB_show_negtsites').style.display= 'none';
    $('CB_show_negative_binomial').style.display='none';
    $('CB_filter_on_negbintest').style.display='none';
    $('cb_show_fisherpv').style.display = 'none';
    $('cb_filter_on_fisherpv').style.display = 'none';
  }
  if (data['options']['disc']) {
    // negative data set was not used with a discriminative search
    $('cb_show_fisherpv').style.display = 'none';
  }
  if (!data['options']['mcc']) {
    $('CB_show_MCC').style.display='none';
  }
  if (!data['options']['optimize_score']) {
    // per motif score optimisation turned off, so hide column
    $('show_score').checked = false;
  }
  // rebrand centrimo
  $('title_img').alt = data['program'];
  document.title = data['program'];
}

/*
 * post_load_setup
 *
 * Setup state that is dependent on everything having been loaded already.
 */
function post_load_setup() {
  "use strict";
  var tbl, i;
  $("filter_id").disabled = !($("filter_on_id").checked);
  $("filter_alt").disabled = !($("filter_on_alt").checked);
  $("filter_evalue").disabled = !($("filter_on_evalue").checked);
  $("filter_binwidth").disabled = !($("filter_on_binwidth").checked);
  $("filter_negbintest").disabled = !($("filter_on_negbintest").checked);
  tbl = $("motifs");
  toggle_class(tbl, "hide_db", !$("show_db").checked);
  toggle_class(tbl, "hide_id", !$("show_id").checked);
  toggle_class(tbl, "hide_name", !$("show_name").checked);
  toggle_class(tbl, "hide_fisherpv", !$("show_fisherpv").checked);
  toggle_class(tbl, "hide_evalue", !$("show_evalue").checked);
  toggle_class(tbl, "hide_pvalue", !$("show_pvalue").checked);
  toggle_class(tbl, "hide_MCC", !$("show_MCC").checked);
  toggle_class(tbl, "hide_blocation", !$("show_blocation").checked);
  toggle_class(tbl, "hide_bwidth", !$("show_bwidth").checked);
  toggle_class(tbl, "hide_bsites", !$("show_bsites").checked);
  toggle_class(tbl, "hide_tsites", !$("show_tsites").checked);
  toggle_class(tbl, "hide_negbsites", !$("show_negbsites").checked);
  toggle_class(tbl, "hide_negtsites", !$("show_negtsites").checked);
  toggle_class(tbl, "hide_negative_binomial", !$("show_negative_binomial").checked);
  toggle_class(tbl, "hide_maxprob", !$("show_maxprob").checked);
  toggle_class(tbl, "hide_hpeak", !$("show_hpeak").checked); //max prob location
  toggle_class(tbl, "hide_nbinstested", !$("show_nbinstested").checked);
  toggle_class(tbl, "hide_score", !$("show_score").checked);
  make_PM_table();
  make_CEM_table();
  make_MP_graph();
  add_MP_graph_listeners();
  if (!data['options']['noseq']) display_seq_ids();
}

/*
 *  sort_motif_id
 *
 *  Takes 2 motif objects and compares them based on id and database.
 */
function sort_motif_id(m1, m2) {
  var diff;
  diff = m1['id'].localeCompare(m2['id']);
  if (diff == 0) {
    diff = m1['db'] - m2['db'];
  }
  return diff;
}

/*
 * sort_motif_alt
 *
 * Takes 2 motif objects and compares them based on alternate id.
 */
function sort_motif_alt(m1, m2) {
  var diff;
  if (m1['alt'] && m2['alt']) {
    diff = m1['alt'].localeCompare(m2['alt']);
    if (diff != 0) return diff;
    return sort_motif_evalue(m1, m2);
  } else {
    if (m1['alt']) {
      return -1;
    } else {
      return 1;
    }
  }
}

/*
 * sort_motif_fisherpv
 *
 * Takes 2 motif objects and compares them based on the Fisher pvalue
 */
function sort_motif_fisherpv(m1, m2) {
  var diff, p1, p2;
  p1 = m1['peaks'][0];
  p2 = m2['peaks'][0];
  diff = p1['fisher_log_adj_pvalue'] - p2['fisher_log_adj_pvalue'];
  if (diff != 0) return diff;
  diff = p2['pos_sites'] - p1['pos_sites'];
  if (diff != 0) return diff;
  return sort_motif_evalue(m1, m2);
}

function sort_motif_neg_pvalue(m1, m2) {
  var diff, p1, p2;
  //return sort_motif_evalue(m1, m2);
  p1 = m1['peaks'][0];
  p2 = m2['peaks'][0];
  diff = p1['neg_log_adj_pvalue'] - p2['neg_log_adj_pvalue'];
  if (diff != 0) return diff;
  diff = p2['neg_sites'] - p1['neg_sites'];
  if (diff != 0) return diff;
  return sort_motif_evalue(m1, m2);
}

function sort_motif_neg_sites(m1, m2) {
  var diff, p1, p2;
  p1 = m1['peaks'][0];
  p2 = m2['peaks'][0];
  diff = p1['neg_sites'] - p2['neg_sites'];
  if (diff != 0) return diff;
  diff = p1['neg_log_adj_pvalue'] - p2['neg_log_adj_pvalue'];
  if (diff != 0) return diff;
  return sort_motif_evalue(m1, m2);
}

/*
 * sort_motif_evalue
 *
 * Takes 2 motif objects and compares them based on the log_adj_pvalue.
 */
function sort_motif_evalue(m1, m2) {
  var diff, p1, p2;
  p1 = m1['peaks'][0];
  p2 = m2['peaks'][0];
  diff = p1['log_adj_pvalue'] - p2['log_adj_pvalue'];
  if (diff != 0) return diff;
  diff = p1['spread'] - p2['spread'];
  if (diff != 0) return diff;
  return sort_motif_id(m1, m2);
}

/*
 * sort_motif_spread
 *
 * Takes 2 motif objects and compares them based on the bin_width.
 */
function sort_motif_spread(m1, m2) {
  var diff, p2, p2;
  p1 = m1['peaks'][0];
  p2 = m2['peaks'][0];
  diff = p1['spread'] - p2['spread'];
  if (diff != 0) return diff;
  diff = p1['log_adj_pvalue'] - p2['log_adj_pvalue'];
  if (diff != 0) return diff;
  return sort_motif_id(m1, m2);
}

/*
 * sort_motif_probability
 *
 * Takes 2 motif objects and compares them based on the maximum probability.
 */
function sort_motif_probability(m1, m2) {
  var diff;
  diff = m2['max_prob'] - m1['max_prob'];
  if (diff != 0) return diff;
  return sort_motif_evalue(m1, m2);
}

/*
 * sort_motif_sites
 *
 * Takes 2 motif objects and compares them based on the bin sites.
 */
function sort_motif_sites(m1, m2) {
  var diff, p1, p2;
  p1 = m1['peaks'][0];
  p2 = m2['peaks'][0];
  diff = p2['sites'] - p1['sites'];
  if (diff != 0) return diff;
  return sort_motif_evalue(m1, m2);
}

/*
 * sort_motif_center
 *
 * Takes 2 motif objects and compares them based on the abs(bin location).
 */
function sort_motif_center(m1, m2) {
  var diff, p1, p2;
  p1 = m1['peaks'][0];
  p2 = m2['peaks'][0];
  diff = Math.abs(p1['center']) - Math.abs(p2['center']);
  if (diff != 0) return diff;
  diff = p1['center'] - p2['center'];
  if (diff != 0) return diff;
  return sort_motif_evalue(m1, m2);
}

/*
 * sort_motif_mcc
 *
 * Takes 2 motif objects and compares them based on the Matthews Correlation Coefficient.
 */
function sort_motif_mcc(m1, m2) {
  var diff, p1, p2;
  p1 = m1['peaks'][0];
  p2 = m2['peaks'][0];
  diff = p2['mcc'] - p1['mcc'];
  if (diff != 0) return diff;
  return sort_motif_evalue(m1, m2);
}

/*
 * sort_motif_total_sites
 *
 * Takes 2 motif objects and compares them based on the total sites.
 */
function sort_motif_total_sites(m1, m2) {
  var diff;
  diff = m2['total_sites'] - m1['total_sites'];
  if (diff != 0) return diff;
  return sort_motif_evalue(m1, m2);
}


/*
 * get_sort_cmp
 *
 * Gets the sorting comparator by index.
 *
 */
function motif_sort_cmp(index) {
  "use strict";
  if (index > 0 && index < sort_table["motif"].length) {
    return sort_table["motif"][index]["fn"];
  }
  return sort_motif_evalue;
}

/*
 * sort_peak_fisherpv
 *
 * Takes 2 peak objects and compares them based on the Fisher pvalue
 */
function sort_peak_fisherpv(p1, p2) {
  var diff;
  if (p1['filtered'] != p2['filtered']) return (p1['filtered'] ? 1 : -1);
  diff = p1['fisher_log_adj_pvalue'] - p2['fisher_log_adj_pvalue'];
  if (diff != 0) return diff;
  diff = p2['pos_sites'] - p1['pos_sites'];
  if (diff != 0) return diff;
  return sort_peak_evalue(p1, p2);
}

function sort_peak_evalue(p1, p2) {
  var diff;
  if (p1['filtered'] != p2['filtered']) return (p1['filtered'] ? 1 : -1);
  diff = p1['log_adj_pvalue'] - p2['log_adj_pvalue'];
  if (diff != 0) return diff;
  diff = p1['spread'] - p2['spread'];
  if (diff != 0) return diff;
  return p1['center'] - p2['center']; 
}

function sort_peak_mcc(p1, p2) {
  var diff;
  if (p1['filtered'] != p2['filtered']) return (p1['filtered'] ? 1 : -1);
  diff = p2['mcc'] - p1['mcc'];
  if (diff != 0) return diff;
  return sort_peak_evalue(p1, p2);
}

function sort_peak_center(p1, p2) {
  if (p1['filtered'] != p2['filtered']) return (p1['filtered'] ? 1 : -1);
  diff = Math.abs(p1['center']) - Math.abs(p2['center']); 
  if (diff != 0) return diff;
  return p1['center'] - p2['center']; 
}

function sort_peak_spread(p1, p2) {
  var diff;
  if (p1['filtered'] != p2['filtered']) return (p1['filtered'] ? 1 : -1);
  diff = p1['spread'] - p2['spread'];
  if (diff != 0) return diff;
  diff = p1['log_adj_pvalue'] - p2['log_adj_pvalue'];
  if (diff != 0) return diff;
  return p1['center'] - p2['center']; 
}

function sort_peak_sites(p1, p2) {
  var diff;
  if (p1['filtered'] != p2['filtered']) return (p1['filtered'] ? 1 : -1);
  diff = p2['sites'] - p1['sites'];
  if (diff != 0) return diff;
  return sort_peak_evalue(p1, p2);
}

function sort_peak_neg_pvalue(p1, p2) {
  var diff;
  if (p1['filtered'] != p2['filtered']) return (p1['filtered'] ? 1 : -1);
  diff = p1['neg_log_adj_pvalue'] - p2['neg_log_adj_pvalue'];
  if (diff != 0) return diff;
  diff = p2['neg_sites'] - p1['neg_sites'];
  if (diff != 0) return diff;
  return sort_peak_evalue(p1, p2);
}

function sort_peak_neg_sites(p1, p2) {
  var diff;
  if (p1['filtered'] != p2['filtered']) return (p1['filtered'] ? 1 : -1);
  diff = p1['neg_sites'] - p2['neg_sites'];
  if (diff != 0) return diff;
  diff = p1['neg_log_adj_pvalue'] - p2['neg_log_adj_pvalue'];
  if (diff != 0) return diff;
  return sort_peak_evalue(p1, p2);
}

function peak_sort_cmp(index) {
  "use strict";
  if (index > 0 && index < sort_table["region"].length) {
    return sort_table["region"][index]["fn"];
  }
  return sort_peak_evalue;
}

/*
 * log2str
 *
 * Converts a log value into scientific notation.
 */
function log2str(log_val, precision, max) {
  var log10_val, e, m;
  if (log_val < 0) {
    log10_val = log_val / Math.log(10);
    e = Math.floor(log10_val);
    m = Math.pow(10, log10_val - e);
    if (m + (0.5 * Math.pow(10, -precision)) >= 10) {
      m = 1;
      e += 1;
    }
    return "" + m.toFixed(precision) + "e" + e;
  } else if (typeof max !== "number" || Math.exp(log_val) < max) {
    return Math.exp(log_val).toFixed(precision);
  } else {
    return ">"+max;
  }
}

/*
 * str2log
 *
 * Converts a scientific notation number string to a log value.
 */
function str2log(str, default_value) {
  var sci_num_re, parts, m, e, log_num;
  if (typeof default_value === "undefined") {
    default_value = null;
  }
  sci_num_re = /^([+]?\d+(?:\.\d+)?)(?:[eE]([+-]\d+))?$/;
  parts = sci_num_re.exec(str);
  if (parts && parseFloat(parts[1]) != 0) {
    m = parseFloat(parts[1]);
    if (m === 0) return default_value;
    e = (parts.length == 3 && parts[2] != null ? parseInt(parts[2]) : 0);
    log_num = (((Math.log(m)/Math.log(10)) + e) * Math.log(10));
    return log_num;
  } else {
    return default_value;
  }
}

/*
 * pvstr
 *
 * Gets the p-value of the motif in string form.
 */
function pvstr(peak) {
  return log2str(peak['log_adj_pvalue'], 1);
}

/*
 * evstr
 *
 * Gets the E-value of the motif in string form.
 */
function evstr(peak, type) {
  return log2str(peak[type] + Math.log(data['tested']), 1);
}

/*
 *  make_link
 *
 *  Creates a text node and if a URL is specified it surrounds it with a link. 
 *  If the URL doesn't begin with "http://" it automatically adds it, as 
 *  relative links don't make much sense in this context.
 */
function make_link(text, url) {
  var textNode = null;
  var link = null;
  if (typeof text !== "undefined" && text !== null) textNode = document.createTextNode(text);
  if (typeof url === "string") {
    if (url.indexOf("//") == -1) {
      url = "http://" + url;
    }
    link = document.createElement('a');
    link.href = url;
    if (textNode) link.appendChild(textNode);
    return link;
  } 
  return textNode;
}

/*
 * make_swatch
 *
 * Make a swatch block.
 */
function make_swatch(colouri) {
  var swatch = document.createElement('div');
  swatch.className = 'swatch';
  swatch.style.backgroundColor = palate[colouri];
  swatch.style.color = text_palate[colouri];
  return swatch;
}

function make_CEM_main_row(tbody, motif) {
  "use strict";
  var colouri, row, swatch, peak, expander;
  peak = motif['peaks'][0];
  colouri = motif['colouri'];
  if (!colouri) colouri = 0;
  row = tbody.insertRow(tbody.rows.length);
  row['data_peak'] = peak;
  swatch = make_swatch(colouri);
  swatch.onclick = toggle_graph_motif;
  if (colouri != 0) {
    s_swatches[colouri -1] = swatch;
  }
  add_cell(row, swatch);
  add_text_cell(row, motif_dbs[motif['db']]['name'], 'col_db');
  add_cell(row, make_link(motif['id'], motif['url']), 'col_id');
  add_text_cell(row, motif['alt'], "col_name");
  add_text_cell(row, evstr(peak, 'log_adj_pvalue'), 'col_evalue');
  if (data['options']['neg_sequences']) {
    add_text_cell(row, evstr(peak, 'fisher_log_adj_pvalue'), "col_fisherpv");
  }
  add_text_cell(row, pvstr(peak), 'col_pvalue');
  if (data['options']['neg_sequences']) {
    add_text_cell(row, log2str(peak['neg_log_adj_pvalue'],1), "col_negative_binomial");
  } else {
    add_text_cell(row, '', "col_negative_binomial");
  }
  if (data['options']['mcc']) {
    add_text_cell(row, peak['mcc'].toFixed(3), "col_MCC");
  } else {
    add_text_cell(row, '', "col_MCC");
  }
  add_text_cell(row, peak['center'], "col_blocation");
  add_text_cell(row, peak['spread'], 'col_bwidth');
  add_text_cell(row, Math.round(peak['sites']), "col_bsites");
  add_text_cell(row, motif['total_sites'], "col_tsites");
  if (data['options']['neg_sequences']) {
    add_text_cell(row, Math.round(peak['neg_sites']), "col_negbsites");
    add_text_cell(row, motif['neg_total_sites'], "col_negtsites");
  } else {
    add_text_cell(row, '', "col_negbsites")
    add_text_cell(row, '', "col_negtsites");
  }
  add_text_cell(row, motif["max_prob"].toFixed(4), "col_maxprob");
  add_text_cell(row, motif["max_prob_loc"], "col_problocation");
  add_text_cell(row, motif['n_tested'], "col_nbinstested");
  add_text_cell(row, motif['score_threshold'].toFixed(2), "col_score");
  if (motif['peaks'].length > 1) {
    expander = document.createElement('span');
    expander.appendChild(document.createTextNode("\u21A7"));
    expander.className = "expander";
    expander.addEventListener("click", toggle_peaks, false);
    add_cell(row, expander, "col_OEbins");
  } else {
    add_text_cell(row, '-', "col_OEbins");
  }
  row.addEventListener('mouseover', highlight_peak, false);
  row.addEventListener('mouseout', dehighlight_peak, false);
}

function make_CEM_sub_row(tbody, peak) {
  var row = tbody.insertRow(tbody.rows.length);
  row['data_peak'] = peak;
  row.className = "sub_peak";
  add_text_cell(row,'\u2022', 'no_swatch');
  add_text_cell(row, '', 'col_db');
  add_text_cell(row, '', 'col_id');
  add_text_cell(row, '', 'col_name');
  add_text_cell(row, evstr(peak, 'log_adj_pvalue'), 'col_evalue');
  if (data['options']['neg_sequences']) {
    add_text_cell(row, evstr(peak, 'fisher_log_adj_pvalue'), "col_fisherpv");
  }
  add_text_cell(row, pvstr(peak), 'col_pvalue');
  if (data['options']['neg_sequences']) {
    add_text_cell(row, log2str(peak['neg_log_adj_pvalue'],1), "col_negative_binomial");
  } else {
    add_text_cell(row, '', "col_negative_binomial");
  }
  if (data['options']['mcc']) {
    add_text_cell(row, peak['mcc'].toFixed(3), "col_MCC");
  } else {
    add_text_cell(row, '', "col_MCC");
  }
  add_text_cell(row, peak['center'], "col_blocation");
  add_text_cell(row, peak['spread'], 'col_bwidth');
  add_text_cell(row, Math.round(peak['sites']), "col_bsites");
  add_text_cell(row, '', "col_tsites");
  if (data['options']['neg_sequences']) {
    add_text_cell(row, Math.round(peak['neg_sites']), "col_negbsites");
  } else {
    add_text_cell(row, '', "col_negbsites")
  }
  add_text_cell(row, '', "col_negtsites");
  add_text_cell(row, '', "col_maxprob");
  add_text_cell(row, '', "col_problocation");
  add_text_cell(row, '', "col_nbinstested");
  add_text_cell(row, '', "col_score");
  add_text_cell(row, '', "col_OEbins");
  row.addEventListener('mouseover', highlight_peak, false);
  row.addEventListener('mouseout', dehighlight_peak, false);
}

function make_CEM_skip_row(tbody, n_skipped) {
  var row, cell, desc;
  if (n_skipped === 0) return;
  row = tbody.insertRow(tbody.rows.length);
  row.className = "sub_peak";

  if (n_skipped === 1) {
    desc = "1 peak hidden due to filters";
  } else {
    desc = n_skipped + " peaks hidden due to filters";
  }

  cell = row.insertCell(row.cells.length);
  cell.colSpan = 19;
  cell.style.textAlign = "center";
  cell.style.fontWeight = "bold";
  cell.appendChild(document.createTextNode(desc));
}

/*
 * make_CEM_table
 *
 * Generate the table which lists centrally enriched motifs.
 */
function make_CEM_table() {
  var motif_sort, peak_sort;
  var filter, motifs, filtered, motif, peaks, motif_dbs;
  var tbl, tbody, row, cell, i, j, skipped;
  // get the filter and sort comparator
  motif_sort = motif_sort_cmp(parseInt($('motif_sort').value));
  peak_sort = peak_sort_cmp(parseInt($('peak_sort').value));
  filter = get_filter();

  motifs = data['motifs'];
  // sort the motif peaks
  for (i = 0; i < motifs.length; i++) {
    motif = motifs[i];
    peaks = motif['peaks'];
    for (j = 0; j < peaks.length; j++) {
      peaks[j]["filtered"] = filter_peak(filter, peaks[j]);
    }
    peaks.sort(peak_sort);
  }
  // filter the motifs
  filtered = [];
  for (i = 0; i < motifs.length; i++) {
    if (filter_motif(filter, motifs[i])) continue;
    if (motifs[i]["peaks"][0]["filtered"]) continue;
    filtered.push(motifs[i]);
  }
  // sort
  filtered.sort(motif_sort);
  // limit
  if (filter["on_count"]) {
    if (filtered.length > filter["count"]) filtered.length = filter["count"];
  }
  // re-add any omitted s_motifs motifs
  outer_loop:
  for (i = 0; i < s_motifs.length; i++) {
    if (s_motifs[i] == null) continue;
    for (j =0; j < filtered.length; j++) {
      if (filtered[j] === s_motifs[i]) {
        continue outer_loop;
      }
    }
    filtered.push(s_motifs[i]);
  }
  // sort again
  filtered.sort(motif_sort);

  // clear the table
  tbl = $("motifs");
  for (i = tbl.tBodies.length - 1; i >= 0; i--) {
    tbody = tbl.tBodies[i];
    tbody.parentNode.removeChild(tbody);
  }

  motif_dbs = data['motif_dbs'];
  // add the new rows to the table
  for (i = 0; i < filtered.length; i++) {
    motif = filtered[i];
    peaks = motif['peaks'];
    tbody = document.createElement('tbody');
    tbody.className = "peak_group";
    tbody["data_motif"] = motif;
    tbody.addEventListener('mouseover', hover_logo, false);
    tbody.addEventListener('mouseout', dehover_logo, false);
    make_CEM_main_row(tbody, motif);
    skipped = 0;
    for (j = 1; j < peaks.length; j++) {
      if (filter_peak(filter, peaks[j])) {
        skipped++;
        continue;
      }
      make_CEM_sub_row(tbody, peaks[j]);
    }
    make_CEM_skip_row(tbody, skipped);
    tbl.appendChild(tbody);
  }
  // note the count of filtered motifs
  if (filtered.length != motifs.length) {
    skipped =  motifs.length - filtered.length;
    tbody = document.createElement('tbody');
    row = tbody.insertRow(tbody.rows.length);

    if (skipped === 1) {
      desc = "1 motif hidden due to filters";
    } else {
      desc = skipped + " motifs hidden due to filters";
    }
    cell = row.insertCell(row.cells.length);
    cell.colSpan = 19;
    cell.style.textAlign = "center";
    cell.style.fontWeight = "bold";
    cell.appendChild(document.createTextNode(desc));
    tbl.appendChild(tbody);
  }
}

function get_filter() {
  var filter, id_pat, alt_pat, alt_re, log_evalue_max, spread, count;
  var neg_log_pvalue_min;
  filter = {};
  // get the db filter
  filter["on_db"] = $("filter_on_db").checked;
  filter["db"] = $("filter_db").value;
  // get the id filter
  filter["on_id"] = $("filter_on_id").checked;
  id_pat = $("filter_id").value;
  try {
    filter["id"] = new RegExp(id_pat, "i");
    $("filter_id").className = "";
  } catch (err) {
    $("filter_id").className = "error";
    filter["on_id"] = false;
  }
  // get the name filter
  filter["on_alt"] = $("filter_on_alt").checked;
  alt_pat = $("filter_alt").value;
  try {
    filter["alt"] = new RegExp(alt_pat, "i");
    $("filter_alt").className = "";
  } catch (err) {
    filter["on_alt"] = false;
    $("filter_alt").className = "error";
  }
  // get the evalue filter
  filter["on_pvalue"] = $("filter_on_evalue").checked;
  if ((log_evalue_max = str2log($("filter_evalue").value)) != null) {
    filter["log_pvalue"] = log_evalue_max - Math.log(data['tested']);
    $("filter_evalue").className = "";
  } else {
    filter["on_pvalue"] = false;
    $("filter_evalue").className = "error";
  }
  // get the Fisher pv filter
  filter["on_fisherpv"] = $("filter_on_fisherpv").checked;
  if ((log_fisherpv_max = str2log($("filter_fisherpv").value)) != null) {
    filter["log_fisherpv"] = log_fisherpv_max - Math.log(data['tested']);
    $("filter_fisherpv").className = "";
  } else {
    filter["on_fisherpv"] = false;
    $("filter_fisherpv").className = "error";
  }
  // get the spread filter
  filter["on_spread"] = $("filter_on_binwidth").checked;
  spread = parseInt($("filter_binwidth").value);
  if (isNaN(spread) || spread < 1) {
    filter["on_spread"] = false;
    $("filter_binwidth").className = "error";
  } else {
    filter["spread"] = spread;
    $("filter_binwidth").className = "";
  }
  // get the negative binomial test filter
  filter["on_neg"] = data['options']['neg_sequences'] && $("filter_on_negbintest").checked;
  if ((neg_log_pvalue_min = str2log($("filter_negbintest").value)) != null) {
    $("filter_negbintest").className = "";
    filter["neg_log_pvalue"] = neg_log_pvalue_min - Math.log(data['tested']);
  } else {
    filter["on_neg"] = false;
    $("filter_negbintest").className = "error";
  }
  // get the motif count limit
  filter["on_count"] = $("filter_on_top").checked;
  count = parseInt($("filter_top").value);
  if (isNaN(count) || count < 1) {
    filter["on_count"] = false;
    $("filter_top").className = "error";
  } else {
    filter["count"] = count;
    $("filter_top").className = "";
  }
  return filter; 
}

function filter_motif(filter, motif) {
  if (filter["on_db"] && motif["db"] != filter["db"]) return true;
  if (filter["on_id"] && !filter["id"].test(motif["id"])) return true;
  if (filter["on_alt"] && !filter["alt"].test(motif["alt"])) return true;
  return false;
}

function filter_peak(filter, peak) {
  if (filter["on_fisherpv"] && peak["fisher_adj_pvalue"] > filter["log_fisherpv"]) return true;
  if (filter["on_pvalue"] && peak["log_adj_pvalue"] > filter["log_pvalue"]) return true;
  if (filter["on_spread"] && peak["spread"] > filter["spread"]) return true;
  if (filter["on_neg"] && peak["neg_log_adj_pvalue"] < filter["neg_log_pvalue"]) return true;
  return false;
}

function filter_CEM_table() {
  "use strict";
  var tbl, tbody, tr, motif, peaks, i, j, filter;
  tbl = $("motifs");
  filter = get_filter();
  for (i = 0; i < tbl.tBodies.length; i++) {
    tbody = tbl.tBodies[i];
    motif = tbody["data_motif"];
    toggle_class(tbody, "filtered", filter_motif(filter, motif));
    for (j = 0; j < tbody.rows.length; j++) {

    }
  }
}

/*
 * show_all_peaks
 *
 * Show or hide sub-peaks
 */
function show_all_peaks(param) {
  var tbl = $("motifs");
  var tbody = tbl.tBodies[0];
  var NValue=param.childNodes[0].nodeValue; // Catch the motif (based on argument value sent by clicking on arrow)
  var bin_number=0;
  for (var i=0; i<tbody.rows.length;i++) { // For each motif
    if (tbody.rows[i].className==param.className) {    // For each sub-peaks
      bin_number++;
      if (must_be_display(param.className,bin_number)==false || NValue=="\u21A5") { // If it has to be hidden or arrow was up
        tbody.rows[i].style.display="none";    // Hide it
      }
      else {
      tbody.rows[i].style.display="table-row";    // Show it
      }
    }
  }
  param.childNodes[0].nodeValue= (NValue=="\u21A7") ? "\u21A5" : "\u21A7" ; // Change arrow orientation
}

function toggle_peaks(e) {
  var expander, group;
  if (!e) e = window.event;
  if (e.target) expander = e.target;
  else if (e.srcElement) expander = e.srcElement;
  group = find_parent(expander.parentNode, "peak_group");
  toggle_class(group, "more");
}

/*
 * must_be_display
 *
 * Check all sub-peaks and verify if they have to be displayed or not (based on filters)
 */
function must_be_display(ID, bin_number) {
  "use strict";
  var motifs, i, motif, peak, log_pvalue_max, log_evalue_max, log_pvalue_min;
  motifs = data['motifs'];
  for (i = 0; i < motifs.length; i++) {
    motif = motifs[i];
    if (motif['db'] + motif['id'] == ID) {    // Catch the motif
      peak = motif['peaks'][bin_number];    // Catch the sub-peaks
      if ($("filter_on_evalue").checked) {
        if ((log_evalue_max = str2log($("filter_evalue").value)) != null) {
          log_pvalue_max = log_evalue_max - Math.log(data['tested']);
        } else {
          log_pvalue_max = 0;
        }
        if (log_pvalue_max < peak['log_adj_pvalue']) return false; 
      }
      if ($("filter_on_fisherpv").checked) {
        if ((log_evalue_max = str2log($("filter_fisherpv").value)) != null) {
          log_pvalue_max = log_evalue_max - Math.log(data['tested']);
        } else {
          log_pvalue_max = 0;
        }
        if (log_pvalue_max < peak['fisher_log_adj_pv']) return false; 
      }
      if ($("filter_on_negbintest").checked) {
        if ((log_pvalue_max = str2log($("filter_negbintest").value)) == null) {
          log_pvalue_min = Math.log(data['tested']);
        }
        if (log_pvalue_min > peak['neg_log_adj_pvalue']) return false; 
      }
      if ($("filter_on_binwidth").checked && 
          parseInt($("filter_binwidth").value) > peak['spread']) {
            return false;
      }
    }
  }
  return true;
}

/*
 * make_PM_table
 *
 * Generate the table which lists plotted motifs
 */
function make_PM_table() {
  swap = null;
  var graphed = [];
  var unused = [];
  for (var i = 1; i < palate.length; i++) {
    if (s_motifs[i-1]) graphed.push(s_motifs[i-1]);
    else unused.push(i);
  }
  graphed.sort(sort_motif_evalue);

  var tbl = $("graph_list");
  var tbody = tbl.tBodies[0];
  while (tbody.rows.length > 0) {
    tbody.deleteRow(0);
  }
  // add the new rows to the table
  for (var i = 0; i < graphed.length; i++) {
    var motif = graphed[i];
    var colouri = motif['colouri'];
    if (!colouri) colouri = 0;
    var row = tbody.insertRow(tbody.rows.length);
    row['motif'] = motif;
    var swatch = make_swatch(colouri);
    swatch.onmousedown = swap_colour;
    add_cell(row, swatch);
    add_cell(row, make_link(motif['id'], motif['url']));
    if (!s_logos[colouri-1]) {
      s_logos[colouri-1] = document.createElement("canvas");
      var pspm = new Pspm(motif['pwm'], motif['id'], 0, 0, 
          motif['motif_nsites'], motif['motif_evalue']);
      var logo = logo_1(dna_alphabet, "", pspm);
      draw_logo_on_canvas(logo, s_logos[colouri-1], false, 0.3);
    }
    add_cell(row, s_logos[colouri-1]);
  }
  var div = $("unused_colours");
  while (div.firstChild) div.removeChild(div.firstChild);
  $("unused_colours_section").style.display = (unused.length > 0 ? "block" : "none");
  for (var i = 0; i < unused.length; i++) {
    var swatch = make_swatch(unused[i]);
    swatch['colouri'] = unused[i];
    swatch.onclick = set_colour;
    div.appendChild(swatch);
  }
}

/*
 * make_MP_graph
 *
 * Create a motif probability graph on the canvas.
 */
function make_MP_graph() {
  var canvas = $("graph");
  canvas.width = canvas.width;
  if (canvas.getContext) {
    var ctx = canvas.getContext('2d');
    make_MP_graph2(ctx, canvas.width, canvas.height);
  }
}

function handle_MP_graph_mousemove(e) {
  "use strict";
  var graph, start, stop, left, width, pop;
  graph = $("graph");
  graph_click["stop"] = e.pageX - graph.getBoundingClientRect().left;
  // constrain stop and start to the graph
  start = graph_click["start"];
  if (start < graph_metrics.left_mark) {
    start = graph_metrics.left_mark;
  } else if (start > graph_metrics.right_mark) {
    start = graph_metrics.right_mark;
  }
  stop = graph_click["stop"];
  if (stop < graph_metrics.left_mark) {
    stop = graph_metrics.left_mark;
  } else if (stop > graph_metrics.right_mark) {
    stop = graph_metrics.right_mark;
  }
  // pick the smaller to be the left
  if (start < stop) {
    left = start;
    width = stop - start;
  } else {
    left = stop;
    width = start - stop;
  }
  // visualize the selected range 
  pop = $("pop_peak");
  if (width > 10) {
    pop.style.width = width + "px";
    pop.style.left = left + "px";
    pop.style.visibility = "visible";
  } else {
    pop.style.visibility = "hidden";
  }
}

function handle_MP_graph_mouseup(e) {
  "use strict";
  var graph, start, stop, left, right, left_val, right_val, pop, l, w, d, unit;
  document.removeEventListener("mouseup", handle_MP_graph_mouseup, false);
  document.removeEventListener("mousemove", handle_MP_graph_mousemove, false);
  graph = $("graph");
  graph_click["stop"] = e.pageX - graph.getBoundingClientRect().left;
  // hide the range preview
  pop = $("pop_peak");
  pop.style.visibility = "hidden";
  // constrain stop and start to the graph
  start = graph_click["start"];
  if (start < graph_metrics.left_mark) {
    start = graph_metrics.left_mark;
  } else if (start > graph_metrics.right_mark) {
    start = graph_metrics.right_mark;
  }
  stop = graph_click["stop"];
  if (stop < graph_metrics.left_mark) {
    stop = graph_metrics.left_mark;
  } else if (stop > graph_metrics.right_mark) {
    stop = graph_metrics.right_mark;
  }
  // pick the smaller to be the left
  if (start < stop) {
    left = start;
    right = stop;
  } else {
    left = stop;
    right = start;
  }
  l = graph_metrics.left_mark;
  w = graph_metrics.right_mark - graph_metrics.left_mark;
  d = graph_metrics.right_val - graph_metrics.left_val;
  unit = w / d;
  // convert into graph values
  left_val = graph_metrics.left_val + Math.round((left - l) / unit);
  right_val =  graph_metrics.left_val + Math.round((right - l) / unit);
  // check if we're doing a legend move and not a zoom
  if ((right - left) < 10 || left_val == right_val) {
    move_legend(e);
    return;
  }
  // add a zoom to the stack
  zooms.push({"left_val": left_val, "right_val": right_val});
  make_MP_graph();
  $("zoom_out").disabled = false;
  $("zoom_center").disabled = false;
}

function handle_MP_graph_mousedown(e) {
  "use strict";
  var graph;
  document.addEventListener("mouseup", handle_MP_graph_mouseup, false);
  document.addEventListener("mousemove", handle_MP_graph_mousemove, false);
  graph = $("graph");
  graph_click["start"] = e.pageX - graph.getBoundingClientRect().left;
  
}

function handle_zoom_out_click() {
  if (zooms.length > 0) {
    zooms.pop();
    make_MP_graph();
    if (zooms.length == 0) {
      $("zoom_out").disabled = true;
      $("zoom_center").disabled = true;
    }
  }
}

function handle_zoom_center_click() {
  var zoom, reach;
  if (zooms.length > 0) {
    zoom = zooms.pop();
    reach = Math.max(1, Math.round((zoom["right_val"] - zoom["left_val"]) / 2));
    zooms.push({"left_val": -reach, "right_val": reach});
    make_MP_graph();
  }
}

function add_MP_graph_listeners() {
  "use strict";
  var graph, zoom_out, zoom_center;
  graph = $("graph");
  graph.addEventListener("mousedown", handle_MP_graph_mousedown, false);
  zoom_out = $("zoom_out");
  zoom_out.disabled = true;
  zoom_out.addEventListener("click", handle_zoom_out_click, false);
  zoom_center = $("zoom_center");
  zoom_center.addEventListener("click", handle_zoom_center_click, false);
  zoom_center.disabled = true;
}

function download_eps() {
  var canvas = $("graph");
  canvas.width = canvas.width;
  if (canvas.getContext) {
    var ctx = canvas.getContext('2d');
    var eps_ctx = new EpsContext(ctx, canvas.width, canvas.height);
    eps_ctx.register_font("14pt Helvetica", "Helvetica", 14 / 3 * 4);
    eps_ctx.register_font("16px Helvetica", "Helvetica", 16);
    eps_ctx.register_font("9px Helvetica", "Helvetica", 9);
    make_MP_graph2(ctx, canvas.width, canvas.height);
    $("eps_content").value = eps_ctx.eps();
  }
}

function make_MP_graph2(ctx, width, height) {
  "use strict";
  var windo, type, legend, plot_negative, motifs, rset, i, motif;
  var name, sig, colour, weights, graph, peak_highlight;
  // get the graph settings
  windo = parseInt($("windo").value);
  if (isNaN(windo) || windo < 1) {
    windo = 20;
    $("windo").value = 20;
  }
  type = parseInt($("plot_type").value);
  legend = parseInt($("legend").value);
  plot_negative = parseInt($("plot_negative").value);
  // get the selected motifs
  motifs = [];
  for (i = 0; i < s_motifs.length; i++) {
    if (s_motifs[i] != null) motifs.push(s_motifs[i]);
  }
  // sort the motifs
  motifs.sort(sort_motif_evalue);
  // create a result set with all the selected motifs
  rset = new CentrimoRSet(data["seqlen"]);
  for (i = 0; i < motifs.length; i++) {
    motif = motifs[i];
    name = motif["id"];
    if (motif["alt"]) name = motif["alt"] + " " + motif["id"];
    sig = "p=" + log2str(motif["best_log_adj_pvalue"], 1);
    colour = palate[motif["colouri"]];
    if (plot_negative) {
      rset.add(name, sig, colour, motif["len"], motif["total_sites"],
        motif["sites"], motif["neg_total_sites"], motif["neg_sites"]);
    } else {
      rset.add(name, sig, colour, motif["len"], motif["total_sites"],
        motif["sites"], 0, 0);
    }
  }
  // get the smoothing weights
  weights = (type == 1 ? triangular_weights(windo) : uniform_weights(windo));
  // get the zoom
  var range_left = null;
  var range_right = null;
  if (zooms.length > 0) {
    var zoom = zooms[zooms.length - 1];
    range_left = zoom["left_val"];
    range_right = zoom["right_val"];
  }
  // now make a new graph from the result set and smoothing weights
  graph = new CentrimoGraph(rset, weights, "CentriMo " + data["version"], range_left, range_right);
  graph_metrics = graph.draw_graph(ctx, width, height, legend, legend_x, legend_y);
  peak_highlight = $("pop_peak");
  peak_highlight.style.top = graph_metrics.top_edge + "px";
  peak_highlight.style.height = (graph_metrics.bottom_edge - graph_metrics.top_edge) + "px";
}

function sync_sort_selection () {
  "use strict";
  var i, mapper, motif_sort, region_sort;
  motif_sort = $("motif_sort");
  region_sort = $("peak_sort");
  if (typeof sync_sort_selection.mapper === "undefined") {
    var id2idx;
    // first map region id to index
    id2idx = {};
    for (i = 0; i < region_sort.options.length; i++) {
      id2idx[sort_table["region"][parseInt(region_sort.options[i].value, 10)]["id"]] = i;
    }
    // now generate a mapping of index in motif sort to index in region sort
    mapper = [];
    for (i = 0; i < motif_sort.options.length; i++) {
      mapper[i] = id2idx[sort_table["motif"][parseInt(motif_sort.options[i].value, 10)]["pair"]];
    }
    sync_sort_selection.mapper = mapper;
  } else {
    mapper = sync_sort_selection.mapper;
  }
  if (!$('allow_peak_sort').checked) {
    region_sort.selectedIndex = mapper[motif_sort.selectedIndex];
  }
}

function populate_sort_list (sellist, items) {
  var i, j, item, opt, priority, selected;
  priority = 0;
  selected = 0;
  for (i = 0, j = 0; i < items.length; i++) {
    item = items[i];
    if (typeof item["show"] === 'undefined' || item["show"]) {
      opt = document.createElement("option");
      opt.innerHTML = item["name"];
      opt.value = i;
      sellist.add(opt, null);
      if (typeof item["priority"] !== 'undefined' && item["priority"] > priority) {
        selected = j;
        priority = item["priority"];
      }
      j++;
    }
  }
  sellist.selectedIndex = selected;
}

function populate_sort_lists() {
  "use strict";
  var i, motif_sort, region_sort, motif_sort_list, region_sort_list;
  motif_sort = $("motif_sort");
  region_sort = $("peak_sort");
  try {motif_sort.removeEventListener("click", sync_sort_selection, false);} catch (e) {}
  try {motif_sort.removeEventListener("change", sync_sort_selection, false);} catch (e) {}
  for (i = motif_sort.options.length-1; i >= 0; i--) motif_sort.remove(i);
  for (i = region_sort.options.length-1; i >= 0; i--) region_sort.remove(i);
  motif_sort_list = sort_table["motif"];
  region_sort_list = sort_table["region"];
  populate_sort_list(motif_sort, motif_sort_list);
  populate_sort_list(region_sort, region_sort_list);
  sync_sort_selection();
  motif_sort.addEventListener("click", sync_sort_selection, false);
  motif_sort.addEventListener("change", sync_sort_selection, false);
}
