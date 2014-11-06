pre_load_setup();


/*
 * name_from_source
 *
 * Makes a file name more human friendly to read.
 */
function name_from_source(source) {
  if (source == "-") {
    return "-"
  }
  //assume source is a file name
  var file = source.replace(/^.*\/([^\/]+)$/,"$1");
  var noext = file.replace(/\.[^\.]+$/, "");
  return noext.replace(/_/g, " ");
}

/*
 * pre_load_setup
 *
 *  Sets up initial variables which may be
 *  required for the HTML document creation.
 */
function pre_load_setup() {
  "use strict";
  var seq_db, dbs, i, db, progs, prog;
  seq_db = data['sequence_db'];
  if (!seq_db['name']) {
    seq_db['name'] = name_from_source(seq_db['source']);
  }
  // get the names of the motif databases
  dbs = data['motif_dbs'];
  for (i = 0; i < dbs.length; i++) {
    db = dbs[i];
    if (!db['name']) {
      db['name'] = name_from_source(db['source']);
    }
  }
}

/*
 * page_loaded
 *
 * Called when the page has loaded for the first time.
 */
function page_loaded() {
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
 * page_loaded
 *
 * Called when a page has been resized
 */
function page_resize() {
  update_scroll_pad();
}

function post_load_setup() {
  update_scroll_pad();
  delayed_process_draw_tasks();
}

function make_description(container, description) {
  "use strict";
  var header, box, paragraphs, p, lines, i, j;
  container.innerHTML = "";
  if (description) {
    header = document.createElement("h2");
    header.className = "mainh pad2";
    header.id = "description";
    header.appendChild(document.createTextNode("Description"));
    container.appendChild(header);
    box = document.createElement("div");
    box.className = "box";
    container.appendChild(box);
    paragraphs = data['description'].split("\n\n");
    for (i = 0; i < paragraphs.length; i++) {
      p = document.createElement("p");
      lines = paragraphs[i].split("\n");
      for (j = 0; j < lines.length; j++) {
        if (j != 0) p.appendChild(document.createElement('br'));
        p.appendChild(document.createTextNode(lines[j]));
      }
      box.appendChild(p);
    }
  }
}

function make_clustered_motifs(container) {
  var dna_alphabet = new Alphabet("ACGT", "A 0.25 C 0.25 G 0.25 T 0.25");
  var groups = data['groups'];
  var motifs = data['motifs'];
  var seq_len = data["sequence_db"]["centrimo_seqlen"];
  var sync_tables = [];
  var i, j;

  container.innerHTML = "";
  toggle_class(container, "more_logos", 1);
  toggle_class(container, "show_known_motifs", data["motif_dbs"].length > 0);
  // check centrimo finished correctly
  var centrimo_ok = false;
  var programs = data["programs"];
  for (i = 0; i < programs.length; i++) {
    if (programs[i]["name"] === "centrimo") {
      centrimo_ok = (programs[i]["status"] == 0);
      break;
    }
  }
  // if centrimo failed then hide it's column
  toggle_class(container, "show_distribution", centrimo_ok);

  // add groups
  for (i = 0; i < groups.length; i++) {
    var alignment = groups[i];
    // calculate the alignment length
    var align_len = 0;
    for (j = 0; j < alignment.length; j++) {
      var align = alignment[j];
      var len = align['offset'] + motifs[align["motif"]]["len"];
      if (align_len < len) align_len = len;
    }
    // create the box to hold group
    var motifbox = document.createElement('div');
    container.appendChild(motifbox);
    motifbox.className = "motifbox";
    // create a table to display this cluster of motifs
    var tbl = document.createElement('table');
    tbl.className = "motifs";
    sync_tables.push(tbl);
    // create the table header
    var thead = document.createElement('thead');
    tbl.appendChild(thead);
    var rhead = thead.insertRow(thead.rows.length);
    add_text_header_cell(rhead, "Motif Found");
    add_text_header_cell(rhead, "Discovery/\u200BEnrichment Program", "pop_source");
    add_text_header_cell(rhead, "E-value", "pop_ev");
    add_text_header_cell(rhead, "Known or Similar Motifs", "pop_tomtom", "col_known_motifs");
    add_text_header_cell(rhead, "Distribution", "pop_centrimo", "col_distribution");
    add_text_header_cell(rhead, "SpaMo & FIMO", "pop_other_prog", "col_other_prog");

    // insert motifs in the table
    var centrimo_list = [];
    var tbody = document.createElement('tbody');
    tbl.appendChild(tbody);
    for (j = 0; j < alignment.length; j++) {
      make_alignment_row(dna_alphabet, seq_len, motifs, tbody, align_len, alignment[j], j != 0);
      var motif = motifs[alignment[j]["motif"]];
      if (motif["centrimo_sites"]) centrimo_list.push(motif);
    }
    var expandbox = document.createElement('div');
    expandbox.addEventListener("transitionend", logos_end_transition, true);
    expandbox.addEventListener("oTransitionEnd", logos_end_transition, true);
    expandbox.addEventListener("webkitTransitionEnd", logos_end_transition, true);
    expandbox.addEventListener("MSTransitionEnd", logos_end_transition, true);
    expandbox.appendChild(tbl);
    motifbox.appendChild(expandbox);
    motifbox.appendChild(make_flip(motifbox));
    if (alignment.length > 1) {
      motifbox.appendChild(make_expand(motifbox, alignment.length, expandbox, tbl));
      var btn = make_compact_btn(motifbox, alignment.length, expandbox, tbl);
      motifbox.appendChild(btn);
      motifbox.compact_btn = btn;
      motifbox.addEventListener("mousemove", position_compact_btn, true);
    }
    if (centrimo_list.length > 1) {
      var group = document.createElement("span");
      group.className = "action";
      var link = document.createElement('a');
      link.appendChild(document.createTextNode("CentriMo Group \u21B7"));
      link.href = centrimo_url(centrimo_list);
      group.appendChild(link);
      group.appendChild(help_button("pop_centrimo_group"));
      motifbox.appendChild(group);
    }
  }
  sync_table_columns(sync_tables);
  toggle_class(container, "more_logos", 0);
}

function make_alignment_row(alphabet, seq_len, motifs, tbody, align_len, align, more) {
  "use strict";
  var i, row, motif, pspm, cell, unaligned, aligned, link, links, matches, match, name;
  var other_prog_list, other_prog;
  // create the row
  row = tbody.insertRow(tbody.rows.length);
  if (more) row.className = "more_logo";
  // get the motif information
  motif = motifs[align["motif"]];
  pspm = new Pspm(motif["pwm"], motif["id"], 0, 0, 
      motif["sites"], motif["evalue"]);
  // Motif Logos
  cell = document.createElement("td");
  if (!more) {// make unaligned logos
    unaligned = document.createElement("span");
    unaligned.className = "unaligned";
    unaligned.appendChild(make_logo(alphabet, pspm, false, 
          0, "normal_logo"));
    unaligned.appendChild(make_logo(alphabet, pspm, true, 
        0, "flipped_logo"));
    cell.appendChild(unaligned);
  }
  aligned = document.createElement("span");
  aligned.className = "aligned";
  aligned.appendChild(make_logo(alphabet, pspm, align["rc"], 
        align["offset"], "normal_logo"));
  aligned.appendChild(make_logo(alphabet, pspm, !align["rc"], 
      align_len - (align["offset"] + motif["len"]), "flipped_logo"));
  cell.appendChild(aligned);
  row.appendChild(cell);
  // Motif Source
  link = document.createElement("a");
  links = link;
  if (motif["prog"] == "meme") {
    link.appendChild(document.createTextNode("MEME"));
    link.title = "Click to view the MEME output";
    link.href = "meme_out/meme.html#motif_" + encodeURIComponent(motif["id"]);
  } else if (motif["prog"] == "dreme") {
    link.appendChild(document.createTextNode("DREME"));
    link.title = "Click to view the DREME output";
    link.href = "dreme_out/dreme.html?more=" + encodeURIComponent(motif["id"]);
  } else { // CentriMo
    links = document.createElement("span");
    link.appendChild(document.createTextNode("CentriMo"));
    link.title = "Click to view the CentriMo output";
    link.href = centrimo_url([motif]);
    links.appendChild(link);
    if (motif["db"] == -1) { // from MEME
      links.appendChild(document.createTextNode(" from "));
      link = document.createElement("a");
      link.appendChild(document.createTextNode("MEME"));
      link.title = "Click to view the MEME output";
      link.href = "meme_out/meme.html#motif_" + encodeURIComponent(motif["id"]);
      links.appendChild(link);
    } else if (motif["db"] == -2) { // from DREME
      links.appendChild(document.createTextNode(" from "));
      link = document.createElement("a");
      link.appendChild(document.createTextNode("DREME"));
      link.title = "Click to view the DREME output";
      link.href = "dreme_out/dreme.html?more=" + encodeURIComponent(motif["id"]);
      links.appendChild(link);
    }
  }
  add_cell(row, links);
  // Significance
  add_text_cell(row, motif["sig"]);
  // TOMTOM Summary
  cell = document.createElement("td");
  cell.className = "col_known_motifs";
  if (motif["db"] < 0) { // meme or dreme motif
    matches = motif["tomtom_matches"];
    for (i = 0; i < matches.length && i < 3; i++) {
      match = matches[i];
      name = (match["alt"]? match["alt"] + " (" + match["id"] + ")" : match["id"]);
      if (i != 0) cell.appendChild(document.createElement("br"));
      link = document.createElement("a");
      link.title = "Click to view the Tomtom alignment for this motif.";
      link.href = motif["prog"] + "_tomtom_out/tomtom.html#match_q_" + 
        encodeURIComponent(motif["id"]) + "_t_" + (match["db"] + 1) + "_" + 
        encodeURIComponent(match["id"]);
      link.appendChild(document.createTextNode(name));
      cell.appendChild(link);
    }
  } else if (typeof motif["url"] === "string") { // database motif
    link = document.createElement("a");
    link.href = motif["url"];
    link.title = "Click to view motif website.";
    name = (motif["alt"]? motif["alt"] + " (" + motif["id"] + ")" : motif["id"]);
    link.appendChild(document.createTextNode(name));
    cell.appendChild(link);
  }
  row.appendChild(cell);
  // CentriMo Summary
  add_cell(row, make_distribution(seq_len, motif), "col_distribution");
  // Other program links
  other_prog_list = document.createElement("ul");
  if (motif["spamo_html"]) {
    link = document.createElement("a");
    link.appendChild(document.createTextNode("Motif Spacing Analysis"));
    link.href = motif["spamo_html"];
    other_prog = document.createElement("li");
    other_prog.appendChild(link);
    other_prog_list.appendChild(other_prog);
  }
  if (motif["fimo_gff"]) {
    link = document.createElement("a");
    link.appendChild(document.createTextNode("Motif Sites in GFF"));
    link.href = motif["fimo_gff"];
    other_prog = document.createElement("li");
    other_prog.appendChild(link);
    other_prog_list.appendChild(other_prog);
  }

  add_cell(row, other_prog_list, "col_other_prog");
}

function make_program_listing(container) {
  container.innerHTML = "";
  var tbl = document.createElement("table");
  tbl.className = "programs";
  var thead = document.createElement("thead");
  tbl.appendChild(thead);
  row = thead.insertRow(thead.rows.length);
  add_text_header_cell(row, "Command", "", "col_cmd");
  add_text_header_cell(row, "Running Time", "", "col_runtime");
  add_text_header_cell(row, "Status", "", "col_status");
  add_text_header_cell(row, "Outputs", "", "col_outputs");
  var tbody = document.createElement("tbody");
  tbl.appendChild(tbody);
  var progs = data["programs"];
  for (var i = 0; i < progs.length; i++) {
    row = tbody.insertRow(tbody.rows.length);
    var prog = progs[i];
    var cmd_area = document.createElement("div");
    cmd_area.className = "col_cmd";
    var prog_end = prog["cmd"].indexOf(" ");
    if (prog_end == -1) prog_end = prog["cmd"].length;
    var bold_name = document.createElement("b");
    bold_name.appendChild(document.createTextNode(prog["cmd"].substr(0, prog_end)));
    cmd_area.appendChild(bold_name);
    cmd_area.appendChild(document.createTextNode(prog["cmd"].substr(prog_end)));
    add_cell(row, cmd_area, "col_cmd");
    var hours = Math.floor(prog["time"] / 3600);
    var minutes = Math.floor((prog["time"] - 3600 * hours) / 60);
    var seconds = prog["time"] - 3600 * hours - 60 * minutes;
    var time_str;
    if (hours > 0) {
      time_str = "" + hours + "h " + minutes + "m " + seconds.toFixed(2) + "s";
    } else if (minutes > 0) {
      time_str = "" + minutes + "m " + seconds.toFixed(2) + "s";
    } else {
      time_str = "" + seconds.toFixed(2) + "s";
    }
    add_text_cell(row, time_str, "col_runtime");
    var status_text;
    if (prog["oot"]) {
      status_text = "Out of Time";
    } else if (prog["status"] === 0) {
      if (prog["messages_file"]) {
        status_text = "Warnings";
      } else {
        status_text = "Success";
      }
    } else if (prog["status"] === -1) {
      status_text = "Can't Run";
    } else if (prog["status"] & 127 !== 0) {
      status_text = sig_name(prog["status"] & 127);
    } else {
      status_text = "Error " + (prog["status"] >>> 8);
    }
    if (prog["messages_file"]) {
      var link = document.createElement("a");
      link.href = prog["messages_file"];
      link.appendChild(document.createTextNode(status_text));
      add_cell(row, link, "col_status");
    } else {
      add_text_cell(row, status_text, "col_status");
    }
    if (prog["outputs"].length == 0) {
      add_text_cell(row, "", "col_outputs");
    } else {
      var outputs = prog["outputs"];
      var list = document.createElement("ul");
      for (var j = 0; j < outputs.length; j++) {
        var link = document.createElement("a");
        link.href = outputs[j]["file"];
        var name = outputs[j]["file"];
        if (outputs[j]["name"]) {
          name = outputs[j]["name"];
        }
        link.appendChild(document.createTextNode(name));
        var li = document.createElement("li");
        li.appendChild(link);
        list.appendChild(li);
      }
      add_cell(row, list, "col_outputs");
    }
  }
  container.appendChild(tbl);
}

function sig_name(sig_num) {
  "use strict";
  var sig_names = [
    "SIGHUP", "SIGINT", "SIGQUIT", "SIGILL", "SIGTRAP", "SIGABRT", "SIGBUS", 
    "SIGFPE", "SIGKILL", "SIGUSR1", "SIGSEGV", "SIGUSR2", "SIGPIPE", "SIGALRM", 
    "SIGTERM", "SIG16", "SIGCHILD", "SIGCONT", "SIGSTOP", "SIGTSTP", "SIGTTIN", 
    "SIGTTOU", "SIGURG", "SIGXCPU", "SIGXFSZ", "SIGVTALRM", "SIGPROF", "SIG28", 
    "SIGPOLL", "SIG30", "SIGSYS"
  ];
  if ((sig_num - 1) < sig_names.length) {
    return sig_names[sig_num - 1];
  } else {
    return "SIG" + sig_num;
  }
}

function make_sequence_db_listing(container) {
  "use strict";
  var tbl, thead, tbody, row, db, link;
  container.innerHTML = "";
  tbl = document.createElement("table");
  tbl.className = "inputs";
  thead = document.createElement("thead");
  tbl.appendChild(thead);
  row = thead.insertRow(thead.rows.length);
  add_text_header_cell(row, "Database");
  add_text_header_cell(row, "Source");
  add_text_header_cell(row, "Sequence Count");
  tbody = document.createElement("tbody");
  tbl.appendChild(tbody);
  db = data['sequence_db'];
  row = tbody.insertRow(tbody.rows.length);
  link = document.createElement("a");
  link.href = db["file"];
  link.appendChild(document.createTextNode(db["name"]));
  add_cell(row, link);
  add_text_cell(row, db['source']);
  add_text_cell(row, db['count']);
  container.appendChild(tbl);
}

function make_motif_db_listing(container) {
  "use strict";
  var tbl, thead, tbody, row, i, db, motif_dbs;
  container.innerHTML = "";
  motif_dbs = data['motif_dbs'];
  if (motif_dbs.length == 0) {
    $("motif_dbs_header").style.display = "none";
    return;
  }
  tbl = document.createElement("table");
  tbl.className = "inputs";
  thead = document.createElement("thead");
  tbl.appendChild(thead);
  row = thead.insertRow(thead.rows.length);
  add_text_header_cell(row, "Database");
  add_text_header_cell(row, "Source");
  add_text_header_cell(row, "Motif Count");
  tbody = document.createElement("tbody");
  tbl.appendChild(tbody);
  for (i = 0; i < motif_dbs.length; i++) {
    db = motif_dbs[i];
    row = tbody.insertRow(tbody.rows.length);
    add_text_cell(row, db['name']);
    add_text_cell(row, db['source']);
    add_text_cell(row, db['count']);
  }
  container.appendChild(tbl);
}

var DelayLogoTask = function(logo, canvas) {
  this.logo = logo;
  this.canvas = canvas;
};

DelayLogoTask.prototype.run = function () {
  draw_logo_on_canvas(this.logo, this.canvas, false);
};


function make_logo(alphabet, pspm, rc, offset, className) {
  if (rc) pspm = pspm.copy().reverse_complement(alphabet);
  var logo = new Logo(alphabet, "");
  logo.add_pspm(pspm, offset);
  var canvas = document.createElement('canvas');
  canvas.height = 90;
  canvas.width = 0;
  canvas.className = className;
  size_logo_on_canvas(logo, canvas, false);
  add_draw_task(canvas, new DelayLogoTask(logo, canvas));
  return canvas;
}

var DelayCentrimoPlotTask = function(graph, canvas) {
  this.graph = graph;
  this.canvas = canvas;
};

DelayCentrimoPlotTask.prototype.run = function () {
  this.graph.draw_lines(this.canvas.getContext("2d"), 
      this.canvas.width, this.canvas.height);
};

function make_distribution(seq_len, motif) {
  if (motif["centrimo_sites"]) {
    var canvas = document.createElement("canvas");
    canvas.title = "Click to view the CentriMo output";
    canvas.width = 200;
    canvas.height = 90;
    var rset = new CentrimoRSet(seq_len);
    rset.add("", "", "#000", motif["len"], motif["centrimo_total_sites"], motif["centrimo_sites"]);
    var graph = new CentrimoGraph(rset, triangular_weights(20));
    add_draw_task(canvas, new DelayCentrimoPlotTask(graph, canvas));
    var link = document.createElement("a");
    link.appendChild(canvas);
    link.href = centrimo_url([motif]);
    return link;
  } else {
    return document.createTextNode("Not Centrally Enriched");
  }
}

function centrimo_url(motifs) {
  var url = "centrimo_out/centrimo.html";
  for (var i = 0; i < motifs.length; i++) {
    var motif = motifs[i];
    var db = motif["db"];
    // If MEME or DREME were run we have to modify the db as it will be 
    // different to the db that CentriMo uses.
    if (db == -1) {
      db = 0; // if meme finds motifs it is always specified first
    } else if (db == -2) {
      // dreme is normally specified second unless meme finds no motifs
      db = (data["motif_count"]["meme"] > 0 ? 1 : 0);
    } else { // program is centrimo
      // other databases always come after meme and dreme databases
      if (data["motif_count"]["meme"] > 0) db++;
      if (data["motif_count"]["dreme"] > 0) db++;
    }
    url += (i == 0 ? "?" : "&");
    url += "show=db" + db + "+" + encodeURIComponent(motif["id"]);
  }
  return url;
}

function make_flip(target) {
  var flip_activator = document.createElement('span');
  flip_activator.className = "action";
  flip_activator.appendChild(document.createTextNode("Reverse Complement \u21C6"));
  flip_activator.flip_target = target;
  flip_activator.onclick = flip_logos;
  return flip_activator;
}

function flip_logos() {
  toggle_class(this.flip_target, "flipped_logos");
}

function show_all(show) {
  "use strict";
  var buttonList, i, button, target, cbtn;
  // find all expansion buttons
  buttonList = document.querySelectorAll("span.more_action");
  for (i = 0; i < buttonList.length; i++) {
    button = buttonList[i];
    target = button.expand_target;
    toggle_class(target, "more_logos", show);
    cbtn = target.compact_btn;
    if (cbtn) cbtn.style.top = "20px";
  }
}

function make_expand(target, nmotifs, grow_box, grow_table) {
  var control = document.createElement('span');
  control.className = "action";
  var activator = document.createElement('span');
  activator.appendChild(document.createTextNode("Show " + (nmotifs - 1) + 
        " More \u21A7"));
  activator.className = "more_action";
  activator.expand_target = target;
  activator.grow_box = grow_box;
  activator.grow_table = grow_table;
  activator.onclick = more_logos;
  control.appendChild(activator);
  var deactivator = document.createElement('span');
  deactivator.appendChild(document.createTextNode("Show  Less \u21A5"));
  deactivator.className = "less_action";
  deactivator.expand_target = target;
  deactivator.grow_box = grow_box;
  deactivator.grow_table = grow_table;
  deactivator.onclick = less_logos;
  control.appendChild(deactivator);
  control.appendChild(help_button("pop_show_clustered"));
  return control;
}

function make_compact_btn(target, nmotifs, grow_box, grow_table) {
  var canvas = document.createElement('canvas');
  canvas.className = "moving_less_action";
  canvas.width = 30;
  canvas.height = 160;
  if (canvas.getContext) {
    var ctx = canvas.getContext('2d');
    draw_compact_btn(ctx, canvas.width, canvas.height);
  }
  canvas.expand_target = target;
  canvas.grow_box = grow_box;
  canvas.grow_table = grow_table;
  canvas.onclick = less_logos;
  return canvas;
}

function draw_compact_btn(ctx, w, h) {
  ctx.save();
  ctx.fillStyle = "black";
  ctx.font = "30px Helvetica";
  ctx.save();
  ctx.textAlign = "center";
  ctx.textBaseline = "top";
  ctx.fillText("\u21A7", w/2 , 0);
  ctx.restore();
  ctx.save();
  ctx.font = "20px Helvetica";
  ctx.translate(w/2, h/2);
  ctx.rotate(-Math.PI / 2);
  ctx.textAlign = "center";
  ctx.textBaseline = "middle";
  ctx.fillText("Show Less", 0, 0);
  ctx.restore();
  ctx.save();
  ctx.textAlign = "center";
  ctx.textBaseline = "alphabetic";
  ctx.fillText("\u21A5", w/2 , h);
  ctx.restore();
  ctx.restore();
}

function position_compact_btn(e) {
  if (!e) var e = window.event;
  var btn = this.compact_btn;
  if (btn) {
    var btn_rect = btn.getBoundingClientRect();
    if (e.clientY > btn_rect.top &&
        e.clientY < (btn_rect.top + btn_rect.height)) {
      return;
    }
    var rect = this.getBoundingClientRect();
    var t = e.clientY - rect.top - (btn.height / 2);
    var tpad = 20;
    var bpad = 50;
    if ((t + btn.height + bpad) > rect.height) t = rect.height - btn.height - bpad;
    if (t < tpad) t = tpad;

    if (typeof btn.update_timer === "undefined") {
      btn.style.top = t + "px";
    }
    if (btn.update_timer) {
      btn.update_top = t;
    } else {
      btn.update_timer = setTimeout(
          function (a_btn) { 
            a_btn.style.top = a_btn.update_top + "px"; 
            btn.update_timer = null;
          }, 500, btn);
    }
  }
}

// from 
// http://stackoverflow.com/questions/7264899/detect-css-transitions-using-javascript-and-without-modernizr
function supportsTransitions() {
    var b = document.body || document.documentElement;
    var s = b.style;
    var p = 'transition';
    if(typeof s[p] == 'string') {return true; }

    // Tests for vendor specific prop
    v = ['Moz', 'Webkit', 'Khtml', 'O', 'ms'],
    p = p.charAt(0).toUpperCase() + p.substr(1);
    for(var i=0; i<v.length; i++) {
      if(typeof s[v[i] + p] == 'string') { return true; }
    }
    return false;
}

function more_logos(e) {
  var will_transition = supportsTransitions();
  if (!more_logos.prev_caller) {
    // stage 1
    toggle_class(this.grow_box, "more_logos", 0);
    if (will_transition) {
      // set size to clip to currently displayed row
      this.grow_box.style.maxHeight = this.grow_table.offsetHeight + "px";
    }
    // need to let the UI update so this works
    more_logos.prev_caller = this;
    setTimeout(more_logos, 0);
    return;
  } else {
    // stage 2
    var me = more_logos.prev_caller;
    if (will_transition) {
      toggle_class(me.expand_target, "inprogress", 1);
      toggle_class(me.grow_box, "motifexpander", 1);
    }
    // show other rows
    toggle_class(me.expand_target, "more_logos", 1);
    if (me.expand_target.compact_btn) {
      me.expand_target.compact_btn.style.top = "20px";
    }
    if (will_transition) {
      // expand size
      me.grow_box.style.maxHeight = me.grow_table.offsetHeight + "px";
    }
    more_logos.prev_caller = null;
  }
}

function less_logos() {
  var will_transition = supportsTransitions();
  if (!less_logos.prev_caller) {
    //stage 1
    toggle_class(this.grow_box, "more_logos", 0);
    if (will_transition) {
      // set size to clip to currently displayed rows
      this.grow_box.style.maxHeight = this.grow_table.offsetHeight + "px";
    }
    // need to let the UI update so this works
    less_logos.prev_caller = this;
    setTimeout(less_logos, 0);
    return;
  } else {
    // stage 2
    var me = less_logos.prev_caller;
    if (will_transition) {
      toggle_class(me.expand_target, "inprogress", 1);
      toggle_class(me.grow_box, "motifexpander", 1);
    }
    toggle_class(me.expand_target, "more_logos", 0);
    if (will_transition) {
      // collapse size
      me.grow_box.style.maxHeight = me.grow_table.offsetHeight + "px";
      toggle_class(me.grow_box, "more_logos", 1);
    } else {
      if (!element_in_viewport(me.expand_target)) {
        me.expand_target.scrollIntoView(true);
      }
    }
    less_logos.prev_caller = null;
  }
}

function logos_end_transition(e) {
  var targ;
  if (!e) var e = window.event;
  if (e.target) targ = e.target;
  else if (e.srcElement) targ = e.srcElement;
  if (targ.nodeType == 3) // defeat Safari bug
    targ = targ.parentNode;

  toggle_class(targ, "motifexpander", 0);
  toggle_class(targ, "more_logos", 0);
  targ.style.maxHeight = "";
  if (!element_in_viewport(targ)) {
    targ.scrollIntoView(true);
  }
  var motifbox_re = /\bmotifbox\b/;
  var p = targ.parentNode;
  while (p) {
    if (motifbox_re.test(p.className)) {
      toggle_class(p, "inprogress", 0);
      break;
    }
    p = p.parentNode;
  }
}


/*
 * sync_table_columns
 *
 * Sets the column widths to be the maximum of the required column widths
 *
 */
function sync_table_columns(tables) {
  var colw = [];
  for (var t = 0; t < tables.length; t++) {
    var table = tables[t];
    for (var r = 0; r < table.rows.length; r++) {
      var row = table.rows[r];
      for (var c = colw.length; c < row.cells.length; c++) {
        colw[c] = 0;
      }
      for (var c = 0; c < row.cells.length; c++) {
        var cell = row.cells[c];
        if (cell.offsetWidth > colw[c]) colw[c] = cell.offsetWidth;
      }
    }
  }
  for (var t = 0; t < tables.length; t++) {
    var table = tables[t];
    for (var r = 0; r < table.rows.length; r++) {
      var row = table.rows[r];
      for (var c = 0; c < row.cells.length; c++) {
        var cell = row.cells[c];
        cell.style.width = colw[c] + "px";
      }
    }
  }
}

