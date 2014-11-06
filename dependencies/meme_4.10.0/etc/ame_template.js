var dna_alphabet = new Alphabet("ACGT", "A 0.25 C 0.25 G 0.25 T 0.25");

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
  var seq_db = data['sequence_db'];
  if (!seq_db['name']) seq_db['name'] = name_from_source(seq_db['source']);
  // get the names of the motif databases
  var dbs = data['motif_dbs'];
  var motif_count = 0;
  for (var i = 0; i < dbs.length; i++) {
    var db = dbs[i];
    if (!db['name']) db['name'] = name_from_source(db['source']);
    motif_count += db['count'];
  }
  dbs['count'] = motif_count;
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
 * post_load_setup
 *
 * Setup state that is dependant on everything having been loaded already.
 */
function post_load_setup() {
  update_scroll_pad();
}

/*
 * add_cell
 *
 * Add a cell to the table row.
 */
function add_cell(row, node, cls) {
  var cell = row.insertCell(row.cells.length);
  if (node) cell.appendChild(node);
  if (cls) cell.className = cls;
}

/*
 * add_text_cell
 *
 * Add a text cell to the table row.
 */
function add_text_cell(row, text, cls) {
  var node = null;
  if (text) node = document.createTextNode(text);
  add_cell(row, node, cls);
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
  if (text) textNode = document.createTextNode(text);
  if (url) {
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

function toExp(num, prec) {
  if (typeof(num) === "number") return num.toExponential(prec);
  return "";
}

/*
 * make_results_table
 *
 * Add the results to the results table.
 */
function make_results_table() {
  // clear the table and add the items
  var tbl = $("results");
  if (data['options']['fix_partition']) {
    tbl.className += " fixed";
  }
  if (data['options']['pvalue_method'] == "Spearman's correlation coefficient") { 
    tbl.className += " spearman";
  } else if (data['options']['pvalue_method'] == "linear regression") { 
    tbl.className += " linreg";
  } else {
    tbl.className += " pvalue";
  }
  var tbody = tbl.tBodies[0];
  while (tbody.rows.length > 0) {
    tbody.deleteRow(0);
  }
  var motif_dbs = data['motif_dbs'];
  var motifs = data['motifs'];
  // add the new rows to the table
  for (var i = 0; i < motifs.length; i++) {
    var motif = motifs[i];
    var row = tbody.insertRow(tbody.rows.length);
    row.id = "db_" + motif['db'] + "_motif_" + motif['id'];
    row['motif'] = motif;
    var canvas = document.createElement('canvas');
    var pspm = new Pspm(motif['pwm'], motif['id'], 0, 0, 
        motif['motif_nsites'], motif['motif_evalue']);
    var logo = logo_1(dna_alphabet, '', pspm);
    draw_logo_on_canvas(logo, canvas, false, 0.5);
    add_cell(row, canvas, 'col_logo');
    add_text_cell(row, motif_dbs[motif['db']]['name'], 'col_db');
    add_cell(row, make_link(motif['id'], motif['url']), 'col_id');
    add_text_cell(row, motif['alt'], 'col_name');
    add_text_cell(row, motif['split'], 'col_split');
    add_text_cell(row, toExp(motif['pvalue'], 2), 'col_pvalue');
    add_text_cell(row, toExp(motif['corrected_pvalue'], 2), 'col_corrected_pvalue');
    add_text_cell(row, toExp(motif['spearmans_rho'], 2), 'col_spearmans_rho');
    add_text_cell(row, toExp(motif['mean_square_error'], 2), 'col_mean_square_error');
    add_text_cell(row, toExp(motif['m'], 2), 'col_m');
    add_text_cell(row, toExp(motif['b'], 2), 'col_b');
  }
}
