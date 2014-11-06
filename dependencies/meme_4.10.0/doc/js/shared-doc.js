/*
 * This file defines documentation that is repeated in many files and so needs
 * to be centralized. This is the equalivent of the old options.xsl file except
 * it actually works with HTML5 and allows variable substitution, conditional
 * sections and lists.
 */

var shared_doc = {

  /* Fairly common options  */

  /* verbosity */
  "all-verbosity-opt": ["verbosity"],
  "all-verbosity-itm": [{"lst": ["1", "2", "3", "4", "5"]}],
  "all-verbosity-desc": ["A number that regulates the verbosity level of the ",
    "output information messages. If set to <span class=\"popt\">1</span> ",
    "(quiet) then it will only output error messages whereas the other ",
    "extreme <span class=\"popt\">5</span> (dump) outputs lots of mostly ",
    "useless information."],
  "all-verbosity-def": ["The verbosity level is set to ",
    "<span class=\"popt\">2</span> (normal)."],


  /* output without clobber */
  "all-o-opt": ["o"],
  "all-o-val": ["dir"],
  "all-o-desc": [
    "Create a folder called <span class=\"pdat\">dir</span> and write output ",
    "files in it. This option is not compatible with ",
    "<span class=\"popt\">", {"if": "ddash", "yes": ["-"]} ,"-", {"if": "glam2", "yes": "O", "no": "oc"}, "</span> as ",
    "only one output folder is allowed."],
  "all-o-def": [
    {"if": "dir",
      "no": ["The program writes to standard output."], 
      "yes": ["The program behaves as if <code>",
        {"if": "ddash", "yes": ["-"]} ,"-", {"if": "glam2", "yes": "O", "no": "oc"}, " ", {"key": "dir", "def": "dir_out"},
        "</code> had been specified."]
    }],

  /* output with clobber */
  "all-oc-opt": [{"if": "glam2", "yes": "O", "no": "oc"}],
  "all-oc-val": ["dir"],
  "all-oc-desc": [
    "Create a folder called <span class=\"pdat\">dir</span> but if it already ",
    "exists allow overwriting the contents. This option is not compatible ",
    "with <span class=\"popt\">", {"if": "ddash", "yes": ["-"]} ,"-o</span> ",
    "as only one output folder is allowed."],
  "all-oc-def": [
    {"if": "dir",
      "no": ["The program writes to standard output."],
      "yes": ["The program behaves as if <code>",
      {"if": "ddash", "yes": ["-"]}, "-", {"if": "glam2", "yes": "O", "no": "oc"}, " ", {"key": "dir", "def": "dir_out"},
      "</code> had been specified."]
    }],


  /* For format2meme conversion scripts, hence the 2meme prefix */

  /* output description */
  "2meme-output": [
    "<p>Writes <a href=\"meme-format.html\">MEME motif format</a> to standard ",
    "output.</p>",
    "<p>A probability matrix and optionally a log-odds matrix are output for ",
    "each motif in the file. ",
    {"if": "meme", "no": [ "The probability matrix is computed using ",
    "pseudo-counts consisting of the background frequency (see ",
    "<span class=\"popt\">-bg</span>, below) multiplied by the total ",
    "pseudocounts (see <span class=\"popt\">-pseudo</span>, below). "]},
    "The log-odds matrix uses the background frequencies in the denominator ",
    "and is log base 2.</p>"
    ],

  /* skip */
  "2meme-skip-opt": ["skip"],
  "2meme-skip-val": ["ID"],
  "2meme-skip-desc": [
    "Skip the motif identified by <span class=\"pdat\">ID</span>. ",
    "This option can be repeated to skip more than one motif."
    ],
  "2meme-skip-def": ["Motifs are not skipped."],
  /* numbers */
  "2meme-numbers-opt": ["numbers"],
  "2meme-numbers-desc": ["Use a number based on the position in the input ",
    "instead of the ", {"key": "id_source", "def": "motif name"}, " as the ",
    "motif identifier."],
  "2meme-numbers-def": ["The ", {"key": "id_source", "def": "motif name"},
    " is used as the motif identifier."],
  /* bg */
  "2meme-bg-opt": ["bg"],
  "2meme-bg-val": ["background file"],
  "2meme-bg-desc": [
    "The <span class=\"pdat\">background file</span> should be a ",
    "<a href=\"bfile-format.html\">Markov background model</a>. ",
    {"if" : "meme", 
      "no": ["It contains the background frequencies of letters use for assigning pseudocounts. "]
    }, "The background frequencies will be included in the resulting MEME file."
    ],
  "2meme-bg-def": [
    {
      "if": "meme",
      "yes": ["Uses background in first specified motif file."],
      "no": ["Uses uniform background frequencies."]
    }],
  /* pseudo */
  "2meme-pseudo-opt": ["pseudo"],
  "2meme-pseudo-val": ["total pseudocounts"],
  "2meme-pseudo-desc": [
    "Add <span class=\"pdat\">total pseudocounts</span> times letter ",
    "background to each frequency."],
  "2meme-pseudo-def": ["No pseudocount is added."],
  /* log odds */
  "2meme-logodds-opt": ["logodds"],
  "2meme-logodds-desc": [
    "Include a log-odds matrix in the output. This is not required for ",
    "versions of the MEME Suite &ge; 4.7.0."],
  "2meme-logodds-def": ["The log-odds matrix is not included in the output."],
  /* url */
  "2meme-url-opt": ["url"],
  "2meme-url-val": ["website"],
  "2meme-url-desc": [
    "The provided website URL will be stored with the motif ",
    {"if": "may_have_url", "yes": ["(unless the motif already has a URL) "]},
    "and this can be ",
    "used by MEME Suite programs to provide a direct link to that information ",
    "in their output. If <span class=\"pdat\">website</span> contains the ",
    "keyword MOTIF_NAME the ", {"key": "type", "def": "motif ID"}, " is ",
    "substituted in place of MOTIF_NAME in the output.<br> ",
    "For example if the url is <div class=\"indent cmd\">",
    "http://big-box-of-motifs.com/motifs/MOTIF_NAME.html</div> and the ",
    {"key": "type", "def": "motif ID"}, " is <code>", 
    {"key": "example", "def": "motif_id"}, "</code>, the motif will ",
    "contain a link to <div class=\"indent cmd\">",
    "http://big-box-of-motifs.com/motifs/",
    {"key": "example", "def": "motif_id"},".html</div>"],
  "2meme-url-def": [
    "The output does not include a URL with the motifs", 
    {"if": "may_have_url", "yes": [" unless the original motif already had a URL"]},"."]

};

function wrdoc2(doc, subs) {
  var i, part;
  if (typeof doc === "string") {
    document.write(doc);
    return;
  }
  for (i = 0; i < doc.length; i++) {
    part = doc[i];
    if (typeof part === "string") {
      document.write(part);
    } else if (typeof part === "object") {
      if (part.hasOwnProperty("key")) {
        if (typeof subs === "object" && typeof subs[part["key"]] === "string") {
          document.write(subs[part["key"]]);
        } else if (part.hasOwnProperty("def")) {
          document.write(part["def"]);
        }
      } else if (part.hasOwnProperty("if")) {
        if (typeof subs === "object" && subs[part["if"]]) {
          if (part.hasOwnProperty("yes")) wrdoc2(part["yes"], subs);
        } else {
          if (part.hasOwnProperty("no")) wrdoc2(part["no"], subs);
        }
      } else if (part.hasOwnProperty("lst")) {
        var list, lside, rside, sep, j;
        // set defaults
        list = [];
        lside = "<span class=\"popt\">";
        rside = "</span>";
        sep = "|&#8203;";
        if (typeof part["lst"] === "string") {
          if (typeof subs === "object" && typeof subs[part["lst"]] === "object") {
            list = subs[part["lst"]];
          }
        } else if (typeof part["lst"] === "object") {
          list = part["lst"];
        }
        if (typeof part["lside"] === "string") {
          if (typeof subs === "object" && typeof subs[part["lside"]] === "object") {
            lside = subs[part["lside"]];
          }
        } else if (typeof part["lside"] === "object") {
          lside = part["lside"];
        }
        if (typeof part["rside"] === "string") {
          if (typeof subs === "object" && typeof subs[part["rside"]] === "object") {
            rside = subs[part["rside"]];
          }
        } else if (typeof part["rside"] === "object") {
          rside = part["rside"];
        }
        if (typeof part["sep"] === "string") {
          if (typeof subs === "object" && typeof subs[part["sep"]] === "object") {
            sep = subs[part["sep"]];
          }
        } else if (typeof part["sep"] === "object") {
          sep = part["sep"];
        }
        for (j = 0; j < list.length; j++) {
          if (j !== 0) wrdoc2(sep, subs);
          wrdoc2(lside, subs);
          wrdoc2(list[j], subs);
          wrdoc2(rside, subs);
        }

      }
    }
  }
}

function wrdoc(doc_key, subs) {
  if (shared_doc.hasOwnProperty(doc_key)) {
    wrdoc2(shared_doc[doc_key], subs);
  }
}

function wropt(opt_key, subs) {
  document.write("<tr><td class=\"popt\">");
  if (typeof subs === "object" && subs["ddash"]) {
    document.write("--");
  } else {
    document.write("-");
  }
  wrdoc(opt_key + "-opt", subs);
  document.write("</td><td>");
  if (shared_doc.hasOwnProperty(opt_key + "-val")) {
    document.write("<span class=\"pdat\">");
    wrdoc(opt_key + "-val", subs);
    document.write("</span>");
  } else if (shared_doc.hasOwnProperty(opt_key + "-itm")) {
    wrdoc(opt_key + "-itm", subs);
  }
  document.write("</td><td>");
  wrdoc(opt_key + "-desc", subs);
  document.write("</td><td>");
  wrdoc(opt_key + "-def", subs);
  document.write("</td></tr>");
}
