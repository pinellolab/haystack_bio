// timer for updating job status
var job_status_timer;

//Returns true if it is a DOM node
function isNode(o){
  return (
    typeof Node === "object" ? o instanceof Node : 
    o && typeof o === "object" && typeof o.nodeType === "number" && typeof o.nodeName==="string"
  );
}

function replace_tokens(htmlWithTokens) {
  "use strict";
  var container, tokens, values, locations, i, pos, which, chunk;
  if (arguments.length % 2 != 1) {
    throw new Error("Expected token and value pairs after the html-with-tokens variable.");
  }
  tokens = [];
  values = [];
  locations = [];
  for (i = 1; i < arguments.length; i += 2) {
    tokens.push(arguments[i]);
    values.push(arguments[i+1]);
    locations.push(htmlWithTokens.indexOf(arguments[i]));
  }
  pos = 0;
  container = document.createElement("span");
  while (pos < htmlWithTokens.length) {
    // find the first token
    which = null;
    for (i = 0; i < locations.length; i++) {
      if (locations[i] != -1) {
        if (which == null) which = i;
        else if (locations[i] < locations[which]) which = i;
      }
    }
    if (which == null) break;
    // output everything up to the first token
    if (pos < locations[which]) {
      chunk = document.createElement("span");
      chunk.innerHTML = htmlWithTokens.substring(pos, locations[which]);
      container.appendChild(chunk);
    }
    // output the replacement value, if it is text then wrap in a text node, otherwise clone
    if (typeof values[which] === "string" || typeof values[which] === "number") {
      container.appendChild(document.createTextNode(values[which]));
    } else if (isNode(values[which])) {
      container.appendChild(values[which].cloneNode(true));
    }
    // skip over the token
    pos = locations[which] + tokens[which].length;
    // update the token locations where required
    for (i = 0; i < locations.length; i++) {
      if (locations[i] != -1 && locations[i] < pos) {
        locations[i] = htmlWithTokens.indexOf(tokens[i], pos);
      }
    }
  }
  // output the final chunk
  if (pos < htmlWithTokens.length) {
    chunk = document.createElement("span");
    chunk.innerHTML = htmlWithTokens.substring(pos);
    container.appendChild(chunk);
  }
  return container;
}

function create_row(even, name, value) {
  "use strict";
  var row, cell;
  row = document.createElement("tr");
  row.className = (even ? "even" : "odd");
  cell = document.createElement("th");
  if (typeof name === "string") {
    cell.innerHTML = name;
  } else if (isNode(name)) {
    cell.appendChild(name);
  }
  row.appendChild(cell);
  cell = row.insertCell(row.cells.length);
  if (typeof value === "string") {
    cell.innerHTML = value;
  } else if (isNode(value)) {
    cell.appendChild(value);
  }
  return row;
}

function get_job(job_id) {
  var job_str, job_obj;
  // check for local storage support
  try {
    if (!('localStorage' in window && window['localStorage'] !== null)) return;
  } catch (e) {
    return null;
  }
  // check for JSON support
  if (!window.JSON) return null;
  // get the job data in string format
  job_str = localStorage.getItem(job_id);
  if (job_str == null) return null;
  // parse the job data into object format
  job_obj = JSON.parse(job_str);
  // return job data
  return job_obj;
}

function create_job_detail_sequence(item, inputs, even) {
  var input, value, text, file;
  if (typeof inputs[item["key"]] == "undefined" ||  inputs[item["key"]] == null) {
    return null;
  }
  input = inputs[item["key"]];
  value = document.createElement("span");
  if (input["count"] > 1) {
    text = "A set of " + input["count"];
  } else {
    text = "One";
  }
  if (input["alphabet"] == "DNA") {
    text += " DNA";
  } else if (input["alphabet"] == "RNA") {
    text += " RNA";
  } else if (input["alphabet"] == "PROTEIN") {
    text += " protein";
  }
  if (input["count"] > 1) {
    text += " sequences, ";
    if (input["min"] === input["max"]) {
      text += "all " + input["min"] + " in length,";
    } else {
      text += "between " + input["min"] + " and " + input["max"] + 
        " in length (average length " + input["avg"].toFixed(1) + "),";
    }
  } else {
    text += " sequence, with a length of " + input["min"];
  }
  value.appendChild(document.createTextNode(text));
  if (input["source"] == "file") {
    value.appendChild(document.createTextNode(" from the file "));
    file = document.createElement("span");
    file.className = "file";
    file.appendChild(document.createTextNode(input["orig-file"]));
    value.appendChild(file);
    if (input["orig-file"] !== input["safe-file"]) {
      value.appendChild(document.createTextNode(" which was renamed to "));
      file = document.createElement("span");
      file.className = "file";
      file.appendChild(document.createTextNode(input["safe-file"]));
      value.appendChild(file);
    }
  } else if (input["source"] == "db") {
    value.appendChild(document.createTextNode(" from the database "));
    file = document.createElement("span");
    file.className = "file";
    file.appendChild(document.createTextNode(input["db_name"]));
    value.appendChild(file);
  }
  value.appendChild(document.createTextNode("."));
  return create_row(even, item["name"], value);
}

function create_job_detail_motif(item, inputs, even) {
  var input, value, text, file;
  if (typeof inputs[item["key"]] == "undefined" ||  inputs[item["key"]] == null) {
    return null;
  }
  input = inputs[item["key"]];
  value = document.createElement("span");
  if (input["count"] > 1) {
    text = "A set of " + input["count"];
  } else {
    text = "One";
  }
  if (input["alphabet"] == "DNA") {
    text += " DNA";
  } else if (input["alphabet"] == "RNA") {
    text += " RNA";
  } else if (input["alphabet"] == "PROTEIN") {
    text += " protein";
  }
  if (input["count"] > 1) {
    text += " motifs, ";
    if (input["min"] === input["max"]) {
      text += "all " + input["min"] + " in width";
    } else {
      text += "between " + input["min"] + " and " + input["max"] + 
        " in width (average width " + input["avg"].toFixed(1) + ")";
    }
  } else {
    text += " motif, with a width of " + input["min"];
  }
  value.appendChild(document.createTextNode(text));
  if (input["source"] == "file") {
    value.appendChild(document.createTextNode(", from the file "));
    file = document.createElement("span");
    file.className = "file";
    file.appendChild(document.createTextNode(input["orig-file"]));
    value.appendChild(file);
    if (input["orig-file"] !== input["safe-file"]) {
      value.appendChild(document.createTextNode(" which was renamed to "));
      file = document.createElement("span");
      file.className = "file";
      file.appendChild(document.createTextNode(input["safe-file"]));
      value.appendChild(file);
    }
  } else if (input["source"] == "db") {
    value.appendChild(document.createTextNode(", from the database "));
    file = document.createElement("span");
    file.className = "file";
    file.appendChild(document.createTextNode(input["db_name"]));
    value.appendChild(file);
  }
  value.appendChild(document.createTextNode("."));
  return create_row(even, item["name"], value);
}

function create_job_detail_gomo(item, inputs, even) {
  "use strict";
  var input, value, file, description;
  if (typeof inputs[item["key"]] === "undefined" || inputs[item["key"]] == null) {
    return null;
  }
  input = inputs[item["key"]];
  value = document.createElement("span");
  value.appendChild(document.createTextNode("The database "));
  file = document.createElement("span");
  file.className = "file";
  file.innerHTML = input["db_name"];
  value.appendChild(file);
  value.appendChild(document.createTextNode(" described as "));
  description = document.createElement("span");
  description.innerHTML = input["db_description"];
  value.appendChild(description);
  return create_row(even, item["name"], value);
}

function create_job_detail_background(item, inputs, even) {
  "use strict";
  var input, source, order, file, text, fileOrig, fileSafe, value;
  if (typeof inputs[item["key"]] === "undefined" ||  inputs[item["key"]] == null) {
    return null;
  }
  input = inputs[item["key"]];
  source = input["source"];
  if (source == "UNIFORM") {
    value = document.createTextNode("A uniform background.");
  } else if (source == "MEME") {
    value = document.createTextNode("The background specified in the motif input.");
  } else if (source == "FILE") {
    file = input["file"];
    text = (file["safe-file"] === file["orig-file"] ? 
        "The background from !!SAFE-NAME!!." : 
        "The background from !!ORIG-NAME!! which was renamed to !!SAFE-NAME!!.");
    fileOrig = document.createElement("span");
    fileOrig.className = "file";
    fileOrig.appendChild(document.createTextNode(file["orig-file"]));
    fileSafe = document.createElement("span");
    fileSafe.className = "file";
    fileSafe.appendChild(document.createTextNode(file["safe-file"]));
    value = replace_tokens(text, "!!ORIG-NAME!!", fileOrig, "!!SAFE-NAME!!", fileSafe);
  } else { // source == "ORDER_X" where X is 0, 1, 2, 3 or 4
    order = input["order"];
    value = document.createTextNode("A order-" + order + 
        " background generated from the supplied sequences.");
  }
  return create_row(even, item["name"], value);
}

function create_job_detail_file(item, inputs, even) {
  var input, text, fileOrig, fileSafe;
  if (typeof inputs[item["key"]] == "undefined" || inputs[item["key"]] == null) {
    return null;
  }
  input = inputs[item["key"]];
  text = (input["safe-file"] === input["orig-file"] ? item["normal"] : item["rename"]);
  fileOrig = document.createElement("span");
  fileOrig.className = "file";
  fileOrig.appendChild(document.createTextNode(input["orig-file"]));
  fileSafe = document.createElement("span");
  fileSafe.className = "file";
  fileSafe.appendChild(document.createTextNode(input["safe-file"]));
  return create_row(even, item["name"], replace_tokens(text, "!!ORIG-NAME!!", fileOrig, "!!SAFE-NAME!!", fileSafe));
}

function create_job_detail_choice(item, inputs, even) {
  var input;
  if (typeof inputs[item["key"]] == "undefined" ||  inputs[item["key"]] == null) {
    return null;
  }
  input = inputs[item["key"]];
  if (typeof input !== "string") throw new Error("Expected string for property value");
  if (typeof item["options"][input] == "undefined") return null;
  return create_row(even, item["name"], item["options"][input]);
}

function create_job_detail_number(item, inputs, even) {
  var input, text;
  if (typeof inputs[item["key"]] == "undefined" ||  inputs[item["key"]] == null) {
    return null;
  }
  input = inputs[item["key"]];
  if (typeof input !== "number") throw new Error("Expected number for property value");
  if (input == 0 && typeof item["zero"] === "string") {
    text = item["zero"];
  } else if (input == 1 && typeof item["one"] === "string") {
    text = item["one"];
  } else if (typeof item["any"] === "string") {
    text = item["any"];
  } else {
    text = "!!VALUE!!";
  }
  return create_row(even, item["name"], replace_tokens(text, "!!VALUE!!", input));
}

function create_job_detail_count_range(item, inputs, even) {
  var low, high, text;
  low = null;
  high = null;
  if (typeof inputs[item["keyLow"]] == "number") {
    low = inputs[item["keyLow"]];
  }
  if (typeof inputs[item["keyHigh"]] == "number") {
    high = inputs[item["keyHigh"]];
  }
  if (low == null && high == null) return null;
  if (low != null && high != null) {
    if (low == high) {
      text = item["same"];
    } else {
      text = item["both"];
    }
  } else if (low != null) {
    text = item["low"];
  } else {
    text = item["high"];
  }
  return create_row(even, item["name"], replace_tokens(text, "!!LOW!!", low, "!!HIGH!!", high));
}

function create_job_detail_flag(item, inputs, even) {
  var input;
  if (typeof inputs[item["key"]] != "boolean") return null;
  input = inputs[item["key"]];
  if (typeof item[(input ? "on" : "off")] != "string") return null;
  return create_row(even, item["name"], item[(input ? "on" : "off")]);
}

function create_job_row(item, inputs, even) {
  var type;
  type = item["type"];
  if (type == "sequences") {
    return create_job_detail_sequence(item, inputs, even);
  } else if (type == "motifs") {
    return create_job_detail_motif(item, inputs, even);
  } else if (type == "gomo") {
    return create_job_detail_gomo(item, inputs, even);
  } else if (type == "background") {
    return create_job_detail_background(item, inputs, even);
  } else if (type == "file") {
    return create_job_detail_file(item, inputs, even);
  } else if (type == "choice") {
    return create_job_detail_choice(item, inputs, even);
  } else if (type == "count" || type == "number") {
    return create_job_detail_number(item, inputs, even);
  } else if (type == "range") {
    return create_job_detail_count_range(item, inputs, even);
  } else if (type == "flag") {
    return create_job_detail_flag(item, inputs, even);
  }
  return null;
}

function display_job_information(container, layout, job) {
  var table, thead, tbody, row, cell, inputs, i, even, item;
  // output: when, description, inputs
  table = document.createElement("table");
  table.className = "job_details";
  tbody = document.createElement("tbody");
  tbody.appendChild(create_row(false, document.createTextNode("Submitted"), 
        document.createTextNode((new Date(job["when"])).toLocaleString())));
  tbody.appendChild(create_row(true, document.createTextNode("Expires"), 
        document.createTextNode((new Date(job["expiry"])).toLocaleString())));
  // description row
  even = false;
  if (typeof job["description"] === "string") {
    tbody.appendChild(create_row(false, document.createTextNode("Description"), 
          document.createTextNode(job["description"])));
    even = true;
  }
  if (typeof job["inputs"] !== "undefined" && layout != null) {
    inputs = job["inputs"];
    for (i = 0; i < layout.length; i++) {
      item = layout[i];
      if ((row = create_job_row(item, inputs, even)) != null) {
        tbody.appendChild(row);
        even = !even;
      }
    }
  }
  table.appendChild(tbody);
  container.innerHTML = "";
  container.appendChild(table);
}


function viewport_height() {
  if (typeof window.innerWidth != 'undefined') {
    // the more standards compliant browsers (mozilla/netscape/opera/IE7)
    // use window.innerWidth and window.innerHeight
    return window.innerHeight
  } else if (typeof document.documentElement != 'undefined' &&
      typeof document.documentElement.clientWidth != 'undefined' 
      && document.documentElement.clientWidth != 0) {
    // IE6 in standards compliant mode (i.e. with a valid doctype as the first line in the document)
    return document.documentElement.clientHeight
  } else {
    // older versions of IE
    return document.getElementsByTagName('body')[0].clientHeight
  }
}

function resize_preview() {
  var page, iframe, rect, scroll;      
  page = (document.compatMode === "CSS1Compat") ? document.documentElement : document.body;
  iframe = $("preview");
  if (iframe == null) return;

  rect = iframe.getBoundingClientRect();
  scroll = ((typeof window.pageYOffset !== "undefined") ?
      window.pageYOffset : document.body.scrollTop);
  h = viewport_height() - (rect.top + scroll) - 20;
  iframe.style.height = h + "px";
}

function query_status() {
  if (typeof service === "undefined" || typeof id === "undefined") {
    clearInterval(job_status_timer);
    return;
  }
  var url, request;
  url = "../info/status?service=" + service + "&id=" + id + "&xml=1";
  request = new XMLHttpRequest();
  request.addEventListener("load", function(e) {
    "use strict";
    var doc, job_status, has_index, preview, preview_is_new;
    doc = request.responseXML;
    // query the job status and if a index page is present
    job_status = doc.getElementsByTagName("status")[0].textContent;
    has_index = (doc.getElementsByTagName("url").length != 0);
    // stop refreshing if we are done
    if (job_status == "expired" || job_status == "failed" || job_status == "done" || job_status == "unknown") {
      clearInterval(job_status_timer);
    }
    // show the correct message for the job status
    substitute_classes($("message"), 
      ["expired", "pending", "active", "suspended", "failed", "done", "unknown"], [job_status]);
    if (has_index) {
      // get the preview iframe
      preview = $("preview");
      // refresh the preview
      preview.src = preview.src;
      if (!/\bexpanded\b/.test(preview.className)) {
        // wait a second before displaying the page to allow it to load
        window.setTimeout( function() {
          "use strict";
          // collapse the details
          toggle_class($("details"), "expanded", false);
          // resize the preview
          resize_preview();
          // display the preview
          toggle_class($("preview"), "expanded", true);
        }, 1000);
      }
    }
  }, false);
  request.open("GET", url, true);
  request.send();
}

function page_load() {
  resize_preview();
  job_status_timer = window.setInterval(query_status, 20000);
}

window.addEventListener('load', page_load, false);
window.addEventListener('resize',resize_preview, false); 
