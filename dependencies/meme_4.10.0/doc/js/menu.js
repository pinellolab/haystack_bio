
var MemeMenu = function() {

};

// set defaults
// configure defaults
MemeMenu.config = {
  "VERSION": "",
  "URL": "",
  "PREV_VER": "",
  "PREV_URL": "",
  "NOTICES": "",
  "NEWS": "",
  "CONTACT": "",
  "DEV_CONTACT": "meme@sdsc.edu"
};
// menu data defaults
MemeMenu.data = null;
MemeMenu.config_loaded = false;
MemeMenu.page_loaded = false;
MemeMenu.created = false;
MemeMenu.is_server = false;
MemeMenu.path_prefix = "";
MemeMenu.online_prefix = "";

// calculate the path to this script
MemeMenu.base = (
  function (filename) {
    var scripts, i, src, src_l, script_l;
    // get all the scripts
    scripts = document.getElementsByTagName('script');
    // try the last script first, as unless this is
    // deferred then it will be the correct one!
    for (i = scripts.length - 1; i >= 0; --i) {
      src = scripts[i].src;
      src_l = src.length;
      fn_l = filename.length;
      if (src.substr(src_l - fn_l) === filename) {
        return src.substr(0, src_l - fn_l);
      }
    }
  }
)("menu.js");

MemeMenu.locale = (
  function () {
    if (navigator.language) return navigator.language; // standard way
    else if (navigator.userLanguage) return navigator.userLanguage; // IE way
    return "en"; // provide a default
  })();


// Called when a component is loaded to check if we're ready to create the menu.
MemeMenu.script_loaded = function() {
  if (typeof menu_path_prefix === "string" && // check path loaded
      MemeMenu.config_loaded && // check configuration loaded
      MemeMenu.data != null && // check data loaded
      MemeMenu.page_loaded && // check page loaded
      typeof Markdown !== "undefined"
  ) MemeMenu.make_menu();
};

MemeMenu.load = function (e) {
  // try to load menu_path.js to determine how to make relative links appear
  var head = document.getElementsByTagName('head')[0];

  // these should always exist
  var style = document.createElement('link');
  style.rel = 'stylesheet';
  style.type = 'text/css';
  style.href = MemeMenu.base + '../css/menu.css';
  head.appendChild(style);

  if (typeof menu_path_loaded === "undefined") {
    // unlike the other files this is relative to the html file
    var path_script= document.createElement('script');
    path_script.type= 'text/javascript';
    path_script.src= 'js/menu-path.js';
    head.appendChild(path_script);
  }

  var markdown_script = document.createElement('script');
  markdown_script.type = 'text/javascript';
  markdown_script.src = MemeMenu.base + 'Markdown.Converter.js';
  head.appendChild(markdown_script);

  var data_script = document.createElement('script');
  data_script.type = 'text/javascript';
  data_script.src = MemeMenu.base + 'menu-data.js';
  head.appendChild(data_script);

  // when configure has not been run, this may not yet exist
  var subin_script = document.createElement('script');
  subin_script.type = 'text/javascript';
  subin_script.src = MemeMenu.base + 'menu-configure.js';
  head.appendChild(subin_script);

  // give up waiting and continue with the defaults after 2 seconds
  setTimeout(function() {MemeMenu.config_loaded = true; MemeMenu.script_loaded();}, 2000);
};

MemeMenu.sub_replace = function (val, re) {
  var match, i, result, offset;
  offset = 0;
  re.lastIndex = 0;
  match = re.exec(val);
  if (match === null) return val;
  result = "";
  while (match) {
    result += val.substring(offset, match.index);
    if (typeof MemeMenu.config[match[1]] === "string") {
      result += MemeMenu.config[match[1]];
    } else {
      result += match[0];
    }
    offset = re.lastIndex;
    match = re.exec(val);
  }
  return result;
};

MemeMenu.sub_data = function (val, re) {
  var val2, property, i;
  if (typeof re === "undefined") {
    re = /@([A-Z_]+)@/g;
  }
  if (typeof val === "object") {
    if (val instanceof Array) {
      for (i = 0; i < val.length; i++) {
        val2 = val[i];
       if (typeof val2 === "string") {
          val[i] = MemeMenu.sub_replace(val2, re);          
        } else if (typeof val2 === "object") {
          MemeMenu.sub_data(val2, re);
        }
      }
    } else {
      for (property in val) {
        if (val.hasOwnProperty(property)) {
          val2 = val[property];
          if (typeof val2 === "string") {
            val[property] = MemeMenu.sub_replace(val2, re);
          } else if (typeof val2 === "object") {
            MemeMenu.sub_data(val2, re);
          }
        }
      }
    }
  }
};

MemeMenu.make_topic = function (is_open, title, info, id) {
  "use strict";
  var topic, closed_arrow, open_arrow;
  // make a menu item
  topic = document.createElement("div");
  topic.className = "menu_topic" + (is_open ? " open" : "");
  topic.setAttribute("data-info", info);
  topic.setAttribute("data-id", id);
  // add some arrows
  closed_arrow = document.createElement("div");
  closed_arrow.className = "menu_arrow closed";
  closed_arrow.appendChild(document.createTextNode("\u25B6"));
  topic.appendChild(closed_arrow);
  open_arrow = document.createElement("div");
  open_arrow.className = "menu_arrow open";
  open_arrow.appendChild(document.createTextNode("\u25BC"));
  topic.appendChild(open_arrow);
  // add the title
  topic.appendChild(document.createTextNode(title));
  // add the click handler
  topic.addEventListener('click', MemeMenu.toggle, false);
  topic.addEventListener('mouseover', MemeMenu.show_info, false);
  topic.addEventListener('mouseout', MemeMenu.hide_info, false);
  return topic;
};

MemeMenu.make_divider = function (title) {
  var divider;
  divider = document.createElement("div");
  divider.className = "menu_divider";
  divider.appendChild(document.createTextNode(title));
  return divider;
};

MemeMenu.make_subtopic = function (title, info, url, is_absolute_link, is_online_link) {
  "use strict";
  var subtopic, job, link, program, when, time, close;
  // create the subtopic
  if (url) {
    subtopic = document.createElement("a");
    if (is_absolute_link) {
      subtopic.href = url;
    } else if (is_online_link) {
      subtopic.href = MemeMenu.online_prefix + url;
    } else {
      subtopic.href = MemeMenu.path_prefix + url;
    }
  } else {
    subtopic = document.createElement("div");
  }
  subtopic.className = "menu_subtopic";
  subtopic.appendChild(document.createTextNode(title));
  if (info) subtopic.setAttribute("data-info", info);
  subtopic.addEventListener('mouseover', MemeMenu.show_info, false);
  subtopic.addEventListener('mouseout', MemeMenu.hide_info, false);
  return subtopic;
};

MemeMenu.make_recent_job = function(service, id, program, when, expiry, description) {
  "use strict";
  var timestamp, recent_job, link, prog, time, close;
  timestamp = new Date(when);
  recent_job = document.createElement("div");
  recent_job.className = "menu_recent_job";
  close = document.createElement("span");
  close.className = "close";
  close.appendChild(document.createTextNode("\u2715"));
  close.setAttribute("data-id", id);
  close.addEventListener("click", MemeMenu.clear_job, false);
  recent_job.appendChild(close);
  link = document.createElement("a");
  link.href = MemeMenu.online_prefix + "info/status?service=" + service + "&id=" + id;
  prog = document.createElement("span");
  prog.appendChild(document.createTextNode(program));
  prog.className = "program";
  link.appendChild(prog);
  time = document.createElement("span");
  time.appendChild(document.createTextNode(
        timestamp.toLocaleTimeString(MemeMenu.locale, {hour: "numeric", minute: "numeric"})));
  time.className = "time";
  link.appendChild(time);
  recent_job.appendChild(link);
  recent_job.setAttribute("data-program", program);
  recent_job.setAttribute("data-when", when);
  recent_job.setAttribute("data-expiry", expiry);
  if (description) recent_job.setAttribute("data-description", description);
  recent_job.addEventListener('mouseover', MemeMenu.show_info, false);
  recent_job.addEventListener('mouseout', MemeMenu.hide_info, false);
  return recent_job;
};

MemeMenu.make_clear_jobs = function() {
  "use strict";
  var clear_jobs_btn;
  clear_jobs_btn = document.createElement("div");
  clear_jobs_btn.className = "menu_clear_jobs";
  clear_jobs_btn.appendChild(document.createTextNode("Clear All"));
  clear_jobs_btn.addEventListener("click", MemeMenu.clear_jobs, false);
  return clear_jobs_btn;
};

MemeMenu.make_menu = function () {
  "use strict";
  var i, j, allow_server, open_topic, is_open;
  var _body, container, header, prev_link, topic, subtopics, measure_box;
  var subtopic, link, popup;
  var closed_arrow, open_arrow;
  var data_header, data_topics, data_topic, data_sts, data_st, blurb;
  var all_spans;
  var jobs, job_topics, job, job_date;
  if (MemeMenu.created) return;
  MemeMenu.created = true;
  // get path information which is hopefully provided by menu-path.js
  if (typeof menu_path_prefix === "string") MemeMenu.path_prefix = menu_path_prefix;
  if (typeof menu_is_server === "boolean") MemeMenu.is_server = menu_is_server;
  // derive some secondary information
  MemeMenu.online_prefix = (!MemeMenu.is_server ? MemeMenu.config["URL"] + "/" : MemeMenu.path_prefix);
  allow_server = (MemeMenu.is_server || MemeMenu.config["URL"] !== "");
  // check for data
  if (MemeMenu.data === null) return; 
  // recursively iterate over menu data and make substutitions
  MemeMenu.sub_data(MemeMenu.data);
  // substitute in the version string
  if (MemeMenu.config["VERSION"] !== "") {
    all_spans = document.querySelectorAll("span.version_string");
    for (i = 0; i < all_spans.length; i++) {
      all_spans[i].innerHTML = "";
      all_spans[i].appendChild(document.createTextNode(MemeMenu.config["VERSION"]));
    }
  }
  // substitute in the blurbs
  all_spans = document.querySelectorAll(".blurb");
  for (i = 0; i < all_spans.length; i++) {
    if (all_spans[i].hasAttribute("data-id")) {
      blurb = MemeMenu.blurbs[all_spans[i].getAttribute("data-id")];
      all_spans[i].innerHTML = (blurb != null ? blurb : "<em>Error: missing blurb</em>");
    }
  }

  // check cookies for open topics
  open_topic = MemeMenu.get_open_topics();
  // get the container
  _body = document.getElementsByTagName("body")[0];
  container = document.createElement("div");
  container.className = "menu_container";
  _body.insertBefore(container, _body.firstChild);
  // make a popup for displaying the information
  popup = document.createElement("div");
  popup.className = "menu_info blurb";
  popup.id = "menu_info";
  container.appendChild(popup);
  // create title
  data_header = MemeMenu.data["header"];
  header = document.createElement("a");
  header.className = "menu_header";
  header.appendChild(document.createTextNode(data_header["title"]));
  header.href = MemeMenu.path_prefix + (MemeMenu.is_server ? data_header["web_url"] : data_header["doc_url"]);
  container.appendChild(header);
  // loop over topics
  data_topics = MemeMenu.data["topics"];
  for (i = 0; i < data_topics.length; i++) {
    is_open = open_topic[""+i];
    data_topic = data_topics[i];
    if (data_topic["server"] && !allow_server) continue;
    if (data_topic["local"] && MemeMenu.is_server) continue;
    // add the topic
    container.appendChild(MemeMenu.make_topic(is_open, data_topic["title"], data_topic["info"], i));
    // process sub-topics
    subtopics = document.createElement("div");
    subtopics.className = "menu_subtopics";
    if (!is_open) subtopics.style.maxHeight = 0;
    measure_box = document.createElement("div");
    // get subtopics
    data_sts = data_topic["topics"];
    for (j = 0; j < data_sts.length; j++) {
      data_st = data_sts[j];
      if (data_st["server"] && !allow_server) continue;
      if (data_st["local"] && MemeMenu.is_server) continue;
      if (typeof data_st["show_test"] === "function") {
        if (!data_st["show_test"](MemeMenu.is_server, MemeMenu.config)) continue;
      }
      // add the subtopic
      if (data_st["divider"]) {
        measure_box.appendChild(MemeMenu.make_divider(data_st["title"]));
      } else {
        measure_box.appendChild(MemeMenu.make_subtopic(
              data_st["title"], data_st["info"],
              data_st["url"], data_st["absolute"], data_st["server"]));
      }
    }
    subtopics.appendChild(measure_box);
    container.appendChild(subtopics);
    if (is_open) {
      subtopics.style.maxHeight = (subtopics.firstChild.clientHeight + 1) + "px";
    }
  }
  // add recent jobs
  if (MemeMenu.is_server) {
    var last_date, job_date, date_str;
    is_open = open_topic["" + data_topics.length];
    // add the Recent Jobs topic
    container.appendChild(
        MemeMenu.make_topic(is_open, "Recent Jobs", 
          "The recent jobs you have run from this web browser.",
          data_topics.length));
    // process sub-topics
    subtopics = document.createElement("div");
    subtopics.className = "menu_subtopics";
    if (!is_open) subtopics.style.maxHeight = 0;
    measure_box = document.createElement("div");
    jobs = MemeMenu.read_recent_jobs(MemeMenu.config["VERSION"]);
    (last_date = new Date()).setHours(0,0,0,0);
    for (j = 0; j < jobs.length; j++) {
      job = jobs[j];
      (job_date = new Date(job["when"])).setHours(0,0,0,0);
      if (job_date.getTime() != last_date.getTime()) {
        // add date divider
        date_str = job_date.toLocaleDateString(MemeMenu.locale,
            {weekday: "long", month: "long", day: "numeric"});
        measure_box.appendChild(MemeMenu.make_divider(date_str));
        last_date = job_date;
      }
      // add job entry
      measure_box.appendChild(MemeMenu.make_recent_job(job["service"], job["id"],
            job["program"], job["when"], job["expiry"], job["description"]));
    }
    measure_box.appendChild(MemeMenu.make_clear_jobs());
    subtopics.appendChild(measure_box);
    container.appendChild(subtopics);
    if (is_open) {
      subtopics.style.maxHeight = (subtopics.firstChild.clientHeight + 1) + "px";
    }
  }
  
  // add a link to the previous version
  if (MemeMenu.config["PREV_URL"] !== "" && MemeMenu.config["PREV_VER"] !== "") {
    prev_link = document.createElement("a");
    prev_link.className = "menu_previous";
    prev_link.appendChild(document.createTextNode("\u21AA Previous version " + MemeMenu.config["PREV_VER"]));
    prev_link.href = MemeMenu.config["PREV_URL"];
    container.appendChild(prev_link);
  }

  if (MemeMenu.is_server) {
    MemeMenu.check_notices([{
          "url": MemeMenu.path_prefix + MemeMenu.config["NOTICES"],
          "cookie": "meme_notice_dismiss_date",
          "important": true
        }, {
          "url": MemeMenu.path_prefix + MemeMenu.config["NEWS"],
          "cookie": "meme_news_dismiss_date",
          "important": false
        }]);
  }
};

function version_string(default_value) {
  if (MemeMenu.config["VERSION"] === "") {
    if (typeof default_value === "undefined") default_value = "";
    document.write('<span class="version_string">' + default_value + '</span>');
  } else {
    document.write(MemeMenu.config["VERSION"]);
  }
}

MemeMenu.toggle = function(e) {
  var subtopics, id, open_topics;
  open_topics = MemeMenu.get_open_topics();
  id = this.getAttribute("data-id");
  subtopics = this.nextSibling;
  if (subtopics && !/\bmenu_subtopics\b/.test(subtopics.className)) {
    subtopics = null;
  }
  if (subtopics && !/\banimate\b/.test(subtopics.className)) {
    subtopics.className += " animate";
  }
  if (/\bopen\b/.test(this.className)) {
    open_topics[id] = false;
    this.className = this.className.replace(/\s*\bopen\b/, "");
    if (subtopics) subtopics.style.maxHeight = "0";
  } else {
    open_topics[id] = true;
    this.className += " open";
    if (subtopics) subtopics.style.maxHeight = (subtopics.firstChild.clientHeight + 1) + "px";
  }
  MemeMenu.set_open_topics(open_topics);
};

MemeMenu.show_info = function (e) {
  var popup, rect, myX, myY;
  var program, date_when, date_expiry, description;
  var table, row, cell;
  popup = document.getElementById("menu_info");
  if (!popup) return;
  popup.innerHTML = "";
  if (this.hasAttribute("data-info")) {
    popup.innerHTML = this.getAttribute("data-info");
  } else if (this.hasAttribute("data-program") && this.hasAttribute("data-when") && this.hasAttribute("data-expiry")) {
    program = this.getAttribute("data-program"); 
    date_when = new Date(parseInt(this.getAttribute("data-when"), 10));
    date_expiry = new Date(parseInt(this.getAttribute("data-expiry"), 10));
    description = null;
    if (this.hasAttribute("data-description")) {
      description = this.getAttribute("data-description");
    }
    table = document.createElement("table");
    // add program row
    row = table.insertRow(-1);
    cell = document.createElement("th");
    cell.appendChild(document.createTextNode("Program:"));
    row.appendChild(cell);
    cell = document.createElement("td");
    cell.appendChild(document.createTextNode(program));
    row.appendChild(cell);
    // add program row
    row = table.insertRow(-1);
    cell = document.createElement("th");
    cell.appendChild(document.createTextNode("Started:"));
    row.appendChild(cell);
    cell = document.createElement("td");
    cell.appendChild(document.createTextNode(date_when.toLocaleString()));
    row.appendChild(cell);
    // add program row
    row = table.insertRow(-1);
    cell = document.createElement("th");
    cell.appendChild(document.createTextNode("Expires:"));
    row.appendChild(cell);
    cell = document.createElement("td");
    cell.appendChild(document.createTextNode(date_expiry.toLocaleString()));
    row.appendChild(cell);
    if (description != null) {
      // add program row
      row = table.insertRow(-1);
      cell = document.createElement("th");
      cell.appendChild(document.createTextNode("Description:"));
      row.appendChild(cell);
      cell = document.createElement("td");
      cell.appendChild(document.createTextNode(description));
      row.appendChild(cell);
    }
    popup.appendChild(table);
  } else {
    return;
  }
  rect = this.getBoundingClientRect();
  // see below for the explaination of the -5.
  myX = rect.left + ((typeof window.pageXOffset !== "undefined") ?
      window.pageXOffset : document.body.scrollLeft) - 5;
  // I have no idea why, but by setting the element to be absolutely positioned
  // 80px down the page it adds an extra 80px so it's now showing 160px from the
  // original position. Needless to say the -80 is a hack which I intend
  // to remove when I figure out why it's happening.
  myY = rect.top + ((typeof window.pageYOffset !== "undefined") ?
      window.pageYOffset : document.body.scrollTop) - 80;

  popup.style.top = myY + "px";
  popup.style.left = (myX + this.clientWidth + 5) + "px";
  popup.style.visibility = "visible";
};

MemeMenu.hide_info = function(e) {
  popup = document.getElementById("menu_info");
  if (!popup) return;
  popup.style.visibility = "hidden";
}

MemeMenu.get_open_topics = function() {
  var cookie_re, match, open_topics, topics, i;
  cookie_re = /^(?:.*;)?\s*meme_open_menu_topics\s*=([^;]*)(?:;.*)?$/;
  open_topics = {};
  if (match = cookie_re.exec(document.cookie)) {
    topics = unescape(match[1]).split(/\s+/);
    for (i = 0; i < topics.length; i++) {
      open_topics[topics[i]] = true;
    }
  }
  return open_topics;
}

MemeMenu.set_open_topics = function(open_topics) {
  var topics, topic;
  topics = [];
  for (topic in open_topics) {
    if (open_topics.hasOwnProperty(topic) && open_topics[topic]) {
      topics.push(topic);
    }
  }
  document.cookie = "meme_open_menu_topics=" + escape(topics.join(" ")) + "; path=/";
}

/*
 * Looks up the local storage key "jobs" which should contain the
 * following json structure:
 * {
 *  "version_str_1": [
 *   {
 *    "service": "Service name",
 *    "id": "job identifier",
 *    "program": "Human readable program name"
 *    "when": "UTC date string",
 *    "expiry": "UTC date string",
 *   }, {
 *    ... as above ..
 *   }
 *  ],
 *  "version_str_2": ...as above...
 * }
 *
 */
MemeMenu.read_recent_jobs = function(target_version) {
  var jobs_json, jobs_all, jobs_version, version, i, job, now, updated;
  // check for local storage support
  try {
    if (!('localStorage' in window && window['localStorage'] !== null)) return [];
  } catch (e) {
    return [];
  }
  // check for JSON support
  if (!window.JSON) return [];
  // get the jobs for all versions as a JSON string
  jobs_json = localStorage.getItem("jobs");
  if (jobs_json === null) return [];
  // convert to javascript object
  jobs_all = JSON.parse(jobs_json);
  // eliminate all expired jobs
  updated = false;
  now = new Date();
  for (version in jobs_all) {
    if (jobs_all.hasOwnProperty(version)) {
      jobs_version = jobs_all[version];
      for (i = 0; i < jobs_version.length; i++) {
        job = jobs_version[i];
        expiryDate = new Date(job["expiry"]);
        if (expiryDate < now) {
          updated = true;
          localStorage.removeItem(job["id"]);
          jobs_version.splice(i, 1);
          i--; // reprocess this index
        }
      }
      // MDN says I can do this for the current property of the iteration
      if (jobs_version.length === 0) delete jobs_all[version];
    }
  }
  if (updated) {
    localStorage.setItem("jobs", JSON.stringify(jobs_all));
  }
  // get the jobs for this version
  jobs_version = jobs_all[target_version];
  return (jobs_version ? jobs_version : []);
}

MemeMenu.clear_jobs = function() {
  "use strict";
  var clear_btn, prev, jobs_json, jobs_all, jobs_ver, i;
  clear_btn = this;
  // remove the entries
  while ((prev = clear_btn.previousSibling) != null) {
    prev.parentNode.removeChild(prev);
  }
  // hide the popup
  MemeMenu.hide_info();
  // check for local storage support
  try {
    if (!('localStorage' in window && window['localStorage'] !== null)) return;
  } catch (e) {
    return;
  }
  // get the jobs for all versions as a JSON string
  jobs_json = localStorage.getItem("jobs");
  if (jobs_json === null) return [];
  // convert to javascript object
  jobs_all = JSON.parse(jobs_json);
  // remove jobs for this version
  jobs_ver = jobs_all[MemeMenu.config["VERSION"]];
  if (typeof jobs_ver !== "undefined") {
    for (i = 0; i < jobs_ver.length; i++) {
      localStorage.removeItem(jobs_ver[i]["id"]);
    }
    delete jobs_all[MemeMenu.config["VERSION"]];
    localStorage.setItem("jobs", JSON.stringify(jobs_all));
  }
};

MemeMenu.clear_job = function() {
  "use strict";
  function is_divider(elem) {
    return elem != null && /\bmenu_divider\b/.test(elem.className);
  }
  function is_btn(elem) {
    return elem != null && /\bmenu_clear_jobs\b/.test(elem.className);
  }
  var close_btn, item, prev, next, id, jobs_json, jobs_all, jobs_ver, updated, i, job;
  // get the button and menu item
  close_btn = this;
  id = close_btn.getAttribute("data-id");
  item = close_btn.parentNode;
  // check if there is a heading that should be removed
  prev = item.previousSibling;
  next = item.nextSibling;
  if (is_divider(prev) && (is_divider(next) || is_btn(next))) {
    // remove the heading
    prev.parentNode.removeChild(prev);
  }
  // remove the menu item
  item.parentNode.removeChild(item);
  // hide the popup
  MemeMenu.hide_info();
  // check for local storage support
  try {
    if (!('localStorage' in window && window['localStorage'] !== null)) return;
  } catch (e) {
    return;
  }
  // get the jobs for all versions as a JSON string
  jobs_json = localStorage.getItem("jobs");
  if (jobs_json === null) return [];
  // convert to javascript object
  jobs_all = JSON.parse(jobs_json);
  // remove a job from this version
  updated = false;
  jobs_ver = jobs_all[MemeMenu.config["VERSION"]];
  if (typeof jobs_ver !== "undefined") {
    for (i = 0; i < jobs_ver.length; i++) {
      job = jobs_ver[i];
      if (job["id"] === id) {
        localStorage.removeItem(id);
        jobs_ver.splice(i, 1);
        updated = true;
        break;
      }
    }
    // MDN says I can do this for the current property of the iteration
    if (jobs_ver.length === 0) delete jobs_all[MemeMenu.config["VERSION"]];
  }
  if (updated) {
    localStorage.setItem("jobs", JSON.stringify(jobs_all));
  }
};

MemeMenu.check_notices = function(sources) {
  "use strict";
  var xmlhttp, source, url, cookie_name, important;
  source = sources.shift();
  url = source["url"];
  cookie_name = source["cookie"];
  important = source["important"];
  console.log(url);
  console.log(cookie_name);
  console.log(important);
  try {
    xmlhttp = new XMLHttpRequest();
  } catch (e) {
    return;
  }
  xmlhttp.open("GET", url, true);
  xmlhttp.onreadystatechange = function() {
    "use strict";
    var cookie_re, match, last_modified, seen_and_dismissed;
    if (xmlhttp.readyState === 4) {
      if (xmlhttp.status === 200 && /\S/.test(xmlhttp.responseText)) {
        last_modified = new Date(xmlhttp.getResponseHeader("Last-Modified"));
        cookie_re = RegExp("^(?:.*;)?\\s*"+cookie_name+"\\s*=([^;]*)(?:;.*)?$");
        if ((match = cookie_re.exec(document.cookie)) !== null) {
          seen_and_dismissed = new Date(parseInt(match[1], 10));
          if (seen_and_dismissed >= last_modified) {
            // try next source
            if (sources.length > 0) MemeMenu.check_notices(sources);
            return;
          }
        }
        // found a source to show
        MemeMenu.make_notification(last_modified, xmlhttp.responseText, cookie_name, important);
      } else {
        // try next source
        if (sources.length > 0) MemeMenu.check_notices(sources);
      }
    }
  };
  xmlhttp.send(null);
}

MemeMenu.make_notification = function(last_updated_date, message, cookie_name, important) {
  var _body, notice_area, notice_box, dismiss_button, notice, timestamp, converter;
  _body = document.getElementsByTagName("body")[0];
  notice_area = document.createElement("div");
  notice_area.className = "notice_area" + (important ? " important" : "");
  notice_area.setAttribute("data-cookie-name", cookie_name);
  _body.appendChild(notice_area);
  notice_box = document.createElement("div");
  notice_box.className = "notice";
  notice_area.appendChild(notice_box);
  dismiss_button = document.createElement("div");
  dismiss_button.className = "notice_dismiss_button";
  dismiss_button.appendChild(document.createTextNode("Hide this message"));
  dismiss_button.addEventListener("click", MemeMenu.remove_notification, false);
  notice_box.appendChild(dismiss_button);
  timestamp = document.createElement("div");
  timestamp.className = "notice_timestamp";
  timestamp.appendChild(document.createTextNode("Posted: "));
  timestamp.appendChild(document.createTextNode(last_updated_date.toLocaleString()));
  notice_box.appendChild(timestamp);
  converter = new Markdown.Converter();
  notice = document.createElement("span");
  notice.innerHTML = converter.makeHtml(message);
  notice_box.appendChild(notice);
}

MemeMenu.remove_notification = function(e) {
  "use strict";
  var elem, important, cookie_name, now, expiry;
  elem = this;
  elem.removeEventListener("click", MemeMenu.remove_notification, false);
  while (elem && !/\bnotice_area\b/.test(elem.className)) elem = elem.parentNode;
  if (elem && elem.parentNode) elem.parentNode.removeChild(elem);
  important = /\bimportant\b/.test(elem.className);
  cookie_name = elem.getAttribute("data-cookie-name");
  now = new Date();
  // date when unimportant messages are shown again.
  expiry = new Date(now.getTime() + (30 * 24 * 60 * 60 * 1000)); // 1 month
  // note that cookies for important messages always expire with the session
  document.cookie = cookie_name + "=" + now.getTime() + 
    (important ? "" : "; expires=" + expiry.toGMTString()) +  "; path=/";
}

MemeMenu.load();
window.addEventListener('load', function() {MemeMenu.page_loaded = true; MemeMenu.script_loaded(); }, false);
