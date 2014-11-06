var LEGEND_LINE = 16;
var LEGEND_FONT = "Helvetica";
var LEGEND_PAD = 5;

var Dasher = function(ctx, start_x, start_y, pattern) {
  if (typeof pattern === "undefined") pattern = [];
  this.ctx = ctx;
  this.x = start_x;
  this.y = start_y;
  this.pattern = pattern;
  this.index = 0;     // where in the pattern are we up to?
  this.fragment = 0;  // how much of the current pattern have we drawn
  this.on = true;     // pen on paper?
  ctx.moveTo(start_x, start_y);
};

Dasher.prototype.dashTo = function(x, y) {
  var dx = x - this.x;
  var dy = y - this.y;
  var d = Math.pow(dx * dx + dy * dy, 0.5);
  if (this.pattern.length == 0) {
    // when no pattern is specified this just draws lines
    this.ctx.lineTo(x, y);
  } else {
    // calculate distance to complete dash
    var dc = this.pattern[this.index] - this.fragment;
    while (dc <= d) {
      // calculate the fraction of the line needed
      var f = dc / d;
      this.x += dx * f;
      this.y += dy * f;
      if (this.on) {
        this.ctx.lineTo(this.x, this.y);
      } else {
        this.ctx.moveTo(this.x, this.y);
      }
      // move to the next dash
      this.fragment = 0;
      // skip empty dashes, just toggling on state
      do {
        this.index = (this.index + 1) % this.pattern.length;
        this.on = !this.on;
      } while (this.pattern[this.index] <= 0);
      // update variables
      dc = this.pattern[this.index];
      dx = x - this.x;
      dy = y - this.y;
      d = Math.pow(dx * dx + dy * dy, 0.5);
    }
    this.fragment += d;
    this.x = x;
    this.y = y;
    if (this.on) {
      this.ctx.lineTo(this.x, this.y);
    } else {
      this.ctx.moveTo(this.x, this.y);
    }
  }
}

var CentrimoResult = function(name, sig, colour, motif_length, 
    total_sites, site_counts, neg_total_sites, neg_site_counts) {
  "use strict";
  if (typeof name === "undefined") {
    name = "";
  }
  if (typeof sig === "undefined") {
    sig = "";
  }
  if (typeof colour == "undefined") {
    colour = "#000000";
  }
  this.name = name;
  this.sig = sig;
  this.colour = colour;
  this.motif_length = motif_length;
  this.total_sites = total_sites;
  this.site_counts = site_counts;
  this.has_neg = (typeof neg_total_sites === "number" && 
      typeof neg_site_counts === "object" && neg_site_counts instanceof Array);
  this.neg_total_sites = neg_total_sites;
  this.neg_site_counts = neg_site_counts;
};

CentrimoResult.prototype.get_site_count = function (index, mirror, neg) {
  count = neg ? this.neg_site_counts[index] : this.site_counts[index];
  if (mirror) {
    i_mirror = this.site_counts.length - 1 - index;
    return (count + (neg ? this.neg_site_counts[i_mirror] : this.site_counts[i_mirror]));
  } else {
    return count;
  }
};

/*
 * CentrimoRSet
 * Construct a centrimo result set.
 */
var CentrimoRSet = function(sequence_length) {
  "use strict";
  this.sequence_length = sequence_length;
  this.results = [];
};

/*
 * seq_len
 * Get the sequence length of the result set.
 */
CentrimoRSet.prototype.seq_len = function() {
  "use strict";
  return this.sequence_length;
};

/*
 * add
 * Add a centrimo result to the set
 */
CentrimoRSet.prototype.add = function(name, sig, colour, motif_length, 
    total_sites, site_counts, neg_total_sites, neg_site_counts) {
  "use strict";
  this.results.push(new CentrimoResult(name, sig, colour, motif_length, 
    total_sites, site_counts, neg_total_sites, neg_site_counts));
};

/*
 * count
 * Return the count of centrimo results.
 */
CentrimoRSet.prototype.count = function() {
  "use strict";
  return this.results.length;
};

/*
 *  get
 *  Get an entry in the result set.
 */
CentrimoRSet.prototype.get = function(index) {
  "use strict";
  return this.results[index];
};

/*
 * CentrimoLine
 */
var CentrimoLine = function(name, sig, colour, dash, thickness, show_on_legend, xlist, ylist) {
  "use strict";
  if (xlist.length != ylist.length) {
    throw new Error("The list of x and y points must be the same length.");
  }
  this.xlist = xlist;
  this.ylist = ylist;
  this.name = name;
  this.sig = sig;
  this.colour = colour;
  this.dash = dash;
  this.thickness = thickness;
  this.show_on_legend = show_on_legend;
  this.max_prob = -1;
};

/*
 * trim
 *
 * Removes points from the ends so the lines are the same length.
 */
CentrimoLine.prototype.trim = function(left, right) {
  "use strict";
  var lefti, righti, gradient, gap, left_y, right_y;
  if (this.xlist.length == 0) return;
  if (typeof left !== "number"|| isNaN(left)) {
    throw new Error("left trim is not a number");
  }
  if (typeof right !== "number" || isNaN(right)) {
    throw new Error("right trim is not a number");
  }
  for (lefti = 0; lefti < this.xlist.length; lefti++) {
    if (this.xlist[lefti] >= left) {
      break;
    }
  }
  if (lefti === 0 && this.xlist[0] !== left) {
    throw new Error("Attempt to undertrim line on the left.");
  }
  for (righti = this.xlist.length - 1; righti >= 0; righti--) {
    if (this.xlist[righti] <= right) {
      break;
    }
  }
  if (righti === (this.xlist.length - 1) && this.xlist[righti] !== right) {
    throw new Error("Attempt to undertrim line on the right.");
  }
  if (lefti == this.xlist.length || righti == -1) {
    // all points outside of drawable region
    this.xlist = [];
    this.ylist = [];
    return;
  }
  if (this.xlist[lefti] != left) {
    // create a new point that is exactly at left by interpolating
    gradient = (this.ylist[lefti] - this.ylist[lefti-1]) / 
      (this.xlist[lefti] - this.xlist[lefti-1]);
    gap = left - this.xlist[lefti-1];
    left_y = this.ylist[lefti-1] + gradient * gap;
    this.xlist.splice(0, lefti, left);
    this.ylist.splice(0, lefti, left_y);
    righti = righti - lefti + 1;
  } else {
    this.xlist.splice(0, lefti);
    this.ylist.splice(0, lefti);
    righti -= lefti;
  }
  if (this.xlist[righti] != right) {
    // create a new point that is exactly at right by interpolating
    gradient = (this.ylist[righti + 1] - this.ylist[righti]) /
      (this.xlist[righti + 1] - this.xlist[righti]);
    gap = right - this.xlist[righti];
    right_y = this.ylist[righti] + gradient * gap;
    // truncate the lists
    this.xlist.length = righti + 1;
    this.ylist.length = righti + 1;
    // push the new item on the end of the lists
    this.xlist.push(right);
    this.ylist.push(right_y);
  } else {
    // truncate the lists
    this.xlist.length = righti + 1;
    this.ylist.length = righti + 1;
  }
  this.max_prob = -1;
};

CentrimoLine.prototype.max = function() {
  "use strict";
  var y_max, i;
  if (this.max_prob != -1) {
    return this.max_prob;
  }
  y_max = 0;
  for (i = 0; i < this.xlist.length; i++) {
    if (this.ylist[i] > y_max) {
      y_max = this.ylist[i];
    }
  }
  this.max_prop = y_max;
  return this.max_prop;
};

/*
 * CentrimoScale
 *
 * Calculates the increments on the y axis
 */
var CentrimoScale = function(max_prob) {
  "use strict";
  var decimals, inc, rounder, max, inc_plus;
  if (max_prob === 0) {
    this.max = 0.01;
    this.inc = 0.002;
    this.digits = 3;
    return;
  }
  // find the minimum number of decimals needed to display the largest
  // digit of the maximum probability
  decimals = Math.ceil(-(Math.log(max_prob) / Math.log(10)));
  // calculate an increment which is (at minimum) 10 times smaller the the max
  inc = Math.pow(10, -(decimals+1));
  // round-up the maximum probabilty to its largest digit
  rounder = Math.pow(10, decimals);
  max = Math.ceil(max_prob * rounder) / rounder;
  // adjust the increment so it's between 5 and 12 times the maximum
  // probability
  if (inc * 5 < max && inc * 12 > max) {
    inc *= 1;
  } else if (inc * 10 < max && inc * 24 > max) {
    inc *= 2;
  } else if (inc * 25 < max && inc * 60 > max) {
    inc *= 5;
  } else if (inc * 50 < max && inc * 120 > max) {
    inc *= 10;
  }
  max += inc;
  inc_plus = (1.125 * inc);
  while (max - inc_plus > max_prob) {
    max -= inc;
  }
  max = Math.min(max, 1.0);
  this.max = max;
  this.inc = inc;
  this.digits = decimals+1;
};

var CentrimoLegendMetrics = function(lw, lh, id_w, pv_w, sq_w) {
  "use strict";
  this.width = lw;
  this.height = lh;
  this.id_width = id_w;
  this.pv_width = pv_w;
  this.sq_width = sq_w;
};

var CentrimoGraphMetrics = function(legend_metrics, top_edge, bottom_edge,
    left_mark, left_val, right_mark, right_val) {
  if (typeof legend_metrics !== "object" || legend_metrics === null) {
    this.legend_width = 10;
    this.legend_height = 10;
  } else {
    this.legend_width = legend_metrics.width;
    this.legend_height = legend_metrics.height;
  }
  this.top_edge = top_edge;
  this.bottom_edge = bottom_edge;
  this.left_mark = left_mark;
  this.left_val = left_val;
  this.right_mark = right_mark;
  this.right_val = right_val;
};

/*
 * CentrimoGraph
 * Takes a results set and a window weight array and produces a graph. 
 * The weights in the window_weights should be non-negative and sum to 1.
 */
var CentrimoGraph = function(rset, window_weights, fine_text, start, end, mirror) {
  "use strict";
  var trim_left, trim_right, i, j, k, result, x, y, xpos, ypos, positions;
  var y_neg, ypos_neg, line_thickness;

  line_thickness = (window_weights.length < 10 ? 1 : 3);
  if (typeof fine_text !== "string") {
    fine_text = "";
  }
  if (typeof mirror !== "boolean") mirror = false;
  this.mirror = mirror;
  if (mirror) {
    if (typeof start === "number") {
      start = Math.max(0, start);
    } else {
      start = 0;
    }
  }
  this.fine_text = fine_text;
  this.lines = [];
  for (i = 0; i < rset.count(); i++) {
    result = rset.get(i);
    x = [];
    y = [];
    if (result.has_neg) {
      y_neg = [];
    }
    xpos = (result.motif_length - rset.seq_len() + (window_weights.length - 1)) / 2;
    if (typeof trim_left === "undefined") {
      trim_left = xpos;
    } else if (trim_left < xpos) {
      trim_left = xpos;
    }
    positions = result.site_counts.length - window_weights.length + 1;
    for (j = 0; j < positions; j++, xpos += 1) {
      ypos = 0;
      // avoid division by zero when no sites are found
      if (result.total_sites > 0) {
        for (k = 0; k < window_weights.length; k++) {
          ypos += window_weights[k] * result.get_site_count(j + k, mirror, 0);
        }
        ypos /= result.total_sites;
      }
      x.push(xpos);
      y.push(ypos);
      // handle the negative dataset
      if (result.has_neg) {
        ypos_neg = 0;
        if (result.neg_total_sites > 0) {
          for (k = 0; k < window_weights.length; k++) {
            ypos_neg += window_weights[k] * result.get_site_count(j + k, mirror, 1);
          }
          ypos_neg /= result.neg_total_sites;
        }
        y_neg.push(ypos_neg);
      }
    }
    // subtract 1 from xpos because for loop will increment it one more than needed.
    xpos -= 1;
    this.lines.push(new CentrimoLine(result.name, result.sig, result.colour, [], line_thickness, true, x, y));
    if (result.has_neg) {
      this.lines.push(new CentrimoLine(result.name, result.sig, result.colour, [6,4], 2, false, x.slice(0), y_neg));
    }
    if (positions > 0) {
      if (typeof trim_right === "undefined") {
        trim_right = xpos;
      } else if (trim_right > xpos) {
        trim_right = xpos;
      }
    }
  }
  if (typeof start !== "undefined" && start != null) {
    if (start > trim_left) {
      trim_left = start;
    }
    this.start = start;
  } else {
    this.start = -Math.ceil(rset.seq_len() / 2);
  }
  if (typeof end !== "undefined" && end != null) {
    if (end < trim_right) {
      trim_right = end;
    }
    this.end = end;
  } else {
    this.end = Math.floor(rset.seq_len() / 2);
  }
  this.max_prob = 0;
  for (i = 0; i < this.lines.length; i++) {
    this.lines[i].trim(trim_left, trim_right);
    this.max_prob = Math.max(this.max_prob, this.lines[i].max());
  }
  this.scale = new CentrimoScale(this.max_prob);
};

/*
 * draw_graph
 *
 * Draws a motif probability graph
 */
CentrimoGraph.prototype.draw_graph = function (ctx, w, h, draw_legend, legend_x, legend_y) {
  "use strict";
  var gap, l_margin, t_margin, b_margin, r_margin, legend_metrics, 
      legend_width, legend_height;
  gap = 10;
  l_margin = gap + 30 + 10 * (2 + this.scale.digits);
  t_margin = 20;
  b_margin = 60;
  r_margin = 30;
  legend_metrics = this.measure_legend(ctx);
  // setting global
  legend_width = legend_metrics.width;
  legend_height = legend_metrics.height;
  // constrain legend to within graph area
  legend_x = Math.round(legend_x);
  legend_y = Math.round(legend_y);
  if (legend_x < (l_margin + gap)) {
    legend_x = l_margin + gap;
  } else if ((legend_x + legend_metrics.width) > (w - r_margin - gap)) {
    legend_x = w - r_margin - legend_metrics.width - gap;
  }
  if (legend_y < (t_margin + gap)) {
    legend_y = t_margin + gap;
  } else if ((legend_y + legend_metrics.height) > (h - b_margin - gap)) {
    legend_y = h - b_margin - legend_metrics.height - gap;
  }

  // draw graph
  ctx.save();
  // draw border
  ctx.beginPath();
  ctx.moveTo(l_margin - 0.5, t_margin +0.5);
  ctx.lineTo(l_margin - 0.5, h - (b_margin - 0.5));
  ctx.lineTo(w - r_margin - 0.5, h - (b_margin - 0.5));
  ctx.lineTo(w - r_margin - 0.5, t_margin +0.5);
  ctx.closePath();
  ctx.stroke();
  // draw fineprint
  ctx.save();
  ctx.font = "9px Helvetica";
  ctx.textAlign = "right";
  ctx.textBaseline = "bottom";
  ctx.fillText(this.fine_text, w - 1, h - 2);
  ctx.restore();
  // draw y axis
  ctx.save();
  ctx.translate(l_margin, t_margin);
  this.draw_y_axis(ctx, 0, h - (t_margin + b_margin));
  ctx.restore();
  // draw y labels
  ctx.save();
  ctx.translate(gap, t_margin);
  this.draw_y_axis_label(ctx, l_margin, h - (t_margin + b_margin));
  ctx.restore();
  // draw x axis
  ctx.save();
  ctx.translate(l_margin, h - b_margin);
  this.draw_x_axis(ctx, w - (l_margin + r_margin), b_margin);
  ctx.restore();
  // draw top axis
  ctx.save();
  ctx.translate(l_margin, t_margin);
  this.draw_top_axis(ctx, w - (l_margin + r_margin), 0);
  ctx.restore();
  // draw x axis labels
  ctx.save();
  ctx.translate(l_margin, h - b_margin);
  this.draw_x_axis_label(ctx, w - (l_margin + r_margin), b_margin - gap);
  ctx.restore();
  // draw lines
  ctx.save();
  ctx.translate(l_margin, t_margin);
  this.draw_lines(ctx, w - (l_margin + r_margin), h - (t_margin + b_margin), 8);
  ctx.restore();
  // draw legend
  if (draw_legend) { 
    ctx.save();
    ctx.translate(legend_x, legend_y);
    this.draw_legend(ctx, legend_metrics);
    ctx.restore();
  }
  ctx.restore();
  return new CentrimoGraphMetrics(legend_metrics, t_margin, h - b_margin,
      l_margin, this.start, w - r_margin, this.end);
};

/*
 * draw_x_axis
 *
 */
CentrimoGraph.prototype.draw_x_axis = function(ctx, w, h) {
  "use strict";
  var scale_x, length, tic_inc, tic_min, tic_max, i, x;
  // pixels per unit
  scale_x = w / (this.end - this.start);
  // calcuate a good tic increment
  length = this.end - this.start;
  if (length > 50) {
    // multiple of 10
    tic_inc = Math.max(1, Math.round(length / 100)) * 10;
  } else if (length > 25) {
    tic_inc = 5;
  } else if (length > 10) {
    tic_inc = 2;
  } else {
    tic_inc = 1;
  }
  // work out the min and max values within the start and end
  tic_min = Math.round(this.start / tic_inc) * tic_inc;
  if (tic_min < this.start) {
    tic_min += tic_inc;
  }
  tic_max = Math.round(this.end / tic_inc) * tic_inc;
  if (tic_max > this.end) {
    tic_max -= tic_inc;
  }
  // draw the tics
  ctx.save();
  ctx.font = "14pt Helvetica";
  ctx.textBaseline = "top";
  ctx.textAlign = "center";
  for (i = tic_min; i <= tic_max; i+= tic_inc) {
    x = Math.round((i - this.start) * scale_x) + 0.5;
    ctx.fillText(""+i, x, 5);
    if (i == this.start || i == this.end) continue;
    ctx.beginPath();
    ctx.moveTo(x, -5);
    ctx.lineTo(x, 3);
    ctx.stroke();
  }
  
  ctx.restore();
};

/*
 * draw_top_axis
 *
 */
CentrimoGraph.prototype.draw_top_axis = function(ctx, w, h) {
  "use strict";
  var scale_x, length, tic_inc, tic_min, tic_max, i, x;
  // pixels per unit
  scale_x = w / (this.end - this.start);
  // calcuate a good tic increment
  length = this.end - this.start;
  if (length > 50) {
    // multiple of 10
    tic_inc = Math.max(1, Math.round(length / 100)) * 10;
  } else if (length > 25) {
    tic_inc = 5;
  } else {
    tic_inc = 1;
  }
  // work out the min and max values within the start and end
  tic_min = Math.round(this.start / tic_inc) * tic_inc;
  if (tic_min < this.start) {
    tic_min += tic_inc;
  }
  tic_max = Math.round(this.end / tic_inc) * tic_inc;
  if (tic_max > this.end) {
    tic_max -= tic_inc;
  }
  // draw the tics
  ctx.save();
  ctx.font = "14pt Helvetica";
  ctx.textBaseline = "top";
  ctx.textAlign = "center";
  for (i = tic_min; i <= tic_max; i+= tic_inc) {
    if (i == this.start || i == this.end) continue;
    x = Math.round((i - this.start) * scale_x) + 0.5;
    ctx.beginPath();
    ctx.moveTo(x, 0);
    ctx.lineTo(x, 8);
    ctx.stroke();
  }
  ctx.restore();
};

/*
 * draw_x_axis_label
 */
CentrimoGraph.prototype.draw_x_axis_label = function(ctx, w, h) {
  "use strict";
  ctx.save();
  ctx.font = "14pt Helvetica";
  ctx.textAlign = "center";
  ctx.textBaseline = "bottom";
  ctx.fillText((this.mirror ? 
        "Distance of Best Site from Sequence Center" : 
        "Position of Best Site in Sequence"), w/2, h);
  ctx.restore();
};

/*
 * draw_y_axis
 *
 * TODO some work needs to be done to allow moving the zero
 */
CentrimoGraph.prototype.draw_y_axis = function(ctx, w, h) {
  "use strict";
  var y_scale, p, y;
  ctx.save();
  ctx.font = "14pt Helvetica";
  ctx.textBaseline = "middle";
  ctx.textAlign = "right";

  y_scale = h / this.scale.max;
  for (p = 0; p < this.scale.max; p += this.scale.inc) {
    y = Math.round(h - p * y_scale) + 0.5;
    this.draw_y_tic(ctx, this.scale.digits, y, p);
  }
  this.draw_y_tic(ctx, this.scale.digits, 0.5, this.scale.max);

  ctx.restore();
};

/*
 * draw_y_tic
 */
CentrimoGraph.prototype.draw_y_tic = function(ctx, digits, y, p) {
  "use strict";
  ctx.beginPath();
  ctx.moveTo(5, y);
  ctx.lineTo(-3, y);
  ctx.stroke();
  ctx.fillText(p.toFixed(digits), -5, y);
};

/*
 * draw_y_axis_label
 */
CentrimoGraph.prototype.draw_y_axis_label = function(ctx, w, h) {
  "use strict";
  ctx.save();
  ctx.translate(0, h/2);
  ctx.rotate(-Math.PI / 2);
  ctx.font = "14pt Helvetica";
  ctx.textBaseline = "top";
  ctx.textAlign = "center";
  ctx.fillText("Probability", 0, 0);
  ctx.restore();
};

/*
 * draw_lines
 */
CentrimoGraph.prototype.draw_lines = function(ctx, w, h, tic_size) {
  "use strict";
  var scale_y, scale_x, i, line, x_points, y_points, j, x, y, dasher;
  if (typeof tic_size === "undefined") {
    tic_size = 0;
  }
  scale_y = h / this.scale.max;
  scale_x = w / (this.end - this.start);
  // draw the central line
  if (0 > this.start && 0 < this.end) {
    ctx.save();
    ctx.translate((-this.start) * scale_x, 0);
    ctx.strokeStyle = '#DDD';
    ctx.beginPath();
    ctx.moveTo(0.5, tic_size);
    ctx.lineTo(0.5, h - tic_size);
    ctx.stroke();
    ctx.restore();
  }
  // draw the lines
  ctx.save();
  ctx.lineJoin = "bevel";
  ctx.miterLimit = 0;
  for (i = 0; i < this.lines.length; i++) {
    line = this.lines[i];
    ctx.lineWidth = line.thickness;
    x_points = line.xlist;
    y_points = line.ylist;
    if (x_points.length > 1) {
      ctx.strokeStyle = line.colour;
      ctx.beginPath();
      for (j = 0; j < x_points.length; j++) {
        if (isNaN(x_points[j])) {
          throw new Error("X NaN!");
        }
        if (isNaN(y_points[j])) {
          throw new Error("Y NaN!");
        }
        x = (x_points[j] - this.start) * scale_x;
        y = h - y_points[j] * scale_y;
        if (j === 0) {
          dasher = new Dasher(ctx, x, y, line.dash);
        } else {
          dasher.dashTo(x, y);
        }
      }
      ctx.stroke();
    } else if (x_points.length == 1) {
      if (isNaN(x_points[0])) {
        throw new Error("X NaN!");
      }
      if (isNaN(y_points[0])) {
        throw new Error("Y NaN!");
      }
      x = (x_points[0] - this.start) * scale_x;
      y = h - y_points[0] * scale_y;
      ctx.fillStyle = line.colour;
      ctx.beginPath();
      ctx.arc(x, y, 2, 0, 2*Math.PI, false);
      ctx.fill();
    }
  }
  ctx.restore();
};

/*
 * measure_legend
 */
CentrimoGraph.prototype.measure_legend = function(ctx) {
  "use strict";
  var lw, lh, sq_w, id_w, pv_w, lines, i, line, len;
  // initilise the line height to the size of the padding
  lh = 2 * LEGEND_PAD;
  // calculate the column widths
  sq_w = LEGEND_LINE - 2;
  id_w = 0;
  pv_w = 0;
  ctx.save();
  ctx.font = "" + LEGEND_LINE + "px " + LEGEND_FONT; 
  for (i = 0; i < this.lines.length; i++) {
    line = this.lines[i];
    if (!line.show_on_legend) {
      continue;
    }
    lh += LEGEND_LINE * 1.2;
    len = ctx.measureText(line.name).width;
    if (id_w < len) {
      id_w = len;
    }
    len = ctx.measureText(line.sig).width;
    if (pv_w < len) {
      pv_w = len;
    }
  }
  lh = Math.round(lh);
  ctx.restore();
  // legend width is a function of the column widths
  lw = id_w + pv_w + sq_w + 4 * LEGEND_PAD;

  return new CentrimoLegendMetrics(lw, lh, id_w, pv_w, sq_w);
};

/*
 * draw_legend
 */
CentrimoGraph.prototype.draw_legend = function(ctx, metrics) {
  "use strict";
  var ln_h, pad, w, h, id_w, pv_w, sq_w, id_x, pv_x, sq_x, i, line;
  if (this.lines.length === 0) {
    return;
  }
  ln_h = LEGEND_LINE;
  pad = LEGEND_PAD;
  w = metrics.width;
  h = metrics.height;
  id_w = metrics.id_width;
  pv_w = metrics.pv_width;
  sq_w = metrics.sq_width;
  id_x = 0;
  pv_x = id_x + id_w + pad;
  sq_x = pv_x + pv_w + pad;

  ctx.save();
  ctx.font = "" + ln_h + "px " + LEGEND_FONT;
  ctx.textAlign = "left";
  ctx.textBaseline = "alphabetic";
  // draw it
  ctx.save();
  ctx.fillStyle = 'white';
  ctx.fillRect(0.5, 0.5, w-1, h-1);
  ctx.strokeStyle = 'black';
  ctx.strokeRect(0.5, 0.5, w-1, h-1);
  ctx.restore();
  ctx.translate(pad, pad);
  ctx.fillStyle = "black";
  for (i = 0; i < this.lines.length; i++) {
    line = this.lines[i];
    if (!line.show_on_legend) {
      continue;
    }
    ctx.translate(0, ln_h);
    ctx.save();
    ctx.fillStyle = line.colour;
    ctx.fillRect(sq_x, -sq_w + 0.1 * ln_h, sq_w, sq_w);
    ctx.restore();
    ctx.fillText(line.name, id_x, 0);
    ctx.fillText(line.sig, pv_x, 0);
    ctx.translate(0, 0.2 * ln_h);
  }
  ctx.restore();
};

/*
 * triangular_weights
 * Returns an array of length n containing normalized weights.
 *
 */
function triangular_weights(n) {
  "use strict";
  var weights, half, half_i, sum, i, unscaled_weight;
  weights = [];
  half = n/2;
  half_i = Math.floor(half);
  sum = 0;
  for (i = 0; i < half_i; i++) {
    unscaled_weight = (i + 0.5) / half;
    weights[i] = unscaled_weight;
    weights[n - i -1] = unscaled_weight;
    sum += 2 * unscaled_weight;
  }
  if (n % 2 == 1) {
    weights[half_i] = 1;
    sum += 1;
  }
  // normalize
  for (i = 0; i < n; i++) {
    weights[i] /= sum;
  }
  return weights;
}

/*
 * uniform_weights
 * Returns an array of length n containing normalized weights.
 *
 */
function uniform_weights(n) {
  "use strict";
  var weights, weight, i;
  weights = [];
  weight = 1.0 / n;
  for (i = 0; i < n; i++) {
    weights[i] = weight;
  }
  return weights;
}
