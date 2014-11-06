function rad2deg(rad) {
  return rad * 180 / Math.PI;
}

function parse_line_join(line_join) {
  line_join = line_join.toLowerCase();
  if (line_join == "bevel") {
    return 2;
  } else if (line_join == "round") {
    return 1;
  } else { // miter
    return 0;
  }
}

function parse_line_cap(line_cap) {
  line_cap = line_cap.toLowerCase();
  if (line_cap == "square") {
    return 2;
  } else if (line_cap == "round") {
    return 1;
  } else { // butt 
    return 0;
  }
}

function parse_text_align(text_align) {
  text_align = text_align.toLowerCase();
  if (text_align == "center") {
    return "center";
  } else if (text_align == "end" || text_align == "right") {
    return "right"; 
  } else { // start or left
    return "left";
  }
}

function parse_text_baseline(text_baseline) {
  text_baseline = text_baseline.toLowerCase();
  if (text_baseline == "top" || text_baseline == "hanging") {
    return "top";
  } else if (text_baseline == "middle") {
    return "middle";
  } else if (text_baseline == "bottom") {
    return "bottom";
  } else { // alphabetic or ideographic
    return "alphabetic";
  }
}

function parse_colour(colour) {
  var hex6_re = /^#([0-9a-f][0-9a-f])([0-9a-f][0-9a-f])([0-9a-f][0-9a-f])$/;
  var hex3_re = /^#([0-9a-f])([0-9a-f])([0-9a-f])$/;
  var rgb_re = /^\s*rgb\s*\(\s*(\d{1,3})(%?)\s*,\s*(\d{1,3})(%?)\s*,\s*(\d{1,3})(%?)\s*\)\s*$/;
  var rgba_re = /^\s*rgba\s*\(\s*(\d{1,3})(%?)\s*,\s*(\d{1,3})(%?)\s*,\s*(\d{1,3})(%?)\s*,\s*(\d{1,3})(%?)\s*\)\s*$/;
  if (colour === undefined || colour == "") {
    return {'red': 0, 'green': 0, 'blue': 0, 'alpha': 255};
  }
  colour = colour.replace(/^\s\s*/, '').replace(/\s\s*$/, ''); //trim
  colour = colour.toLowerCase();
  if (colour == "maroon") {// #800000
    return {'red': 128, 'green': 0, 'blue': 0, 'alpha': 255};
  } else if (colour == "red") { // #FF0000
    return {'red': 255, 'green': 0, 'blue': 0, 'alpha': 255};
  } else if (colour == "orange") { // FFA500
    return {'red': 255, 'green': 165, 'blue': 0, 'alpha': 255};
  } else if (colour == "yellow") { // #FFFF00
    return {'red': 255, 'green': 255, 'blue': 0, 'alpha': 255};
  } else if (colour == "olive") { // #808000
    return {'red': 128, 'green': 128, 'blue': 0, 'alpha': 255};
  } else if (colour == "purple") { // #800080
    return {'red': 128, 'green': 0, 'blue': 128, 'alpha': 255};
  } else if (colour == "fuchsia" || colour == "magenta") { // #FF00FF
    return {'red': 255, 'green': 0, 'blue': 255, 'alpha': 255};
  } else if (colour == "white") { // #FFFFFF
    return {'red': 255, 'green': 255, 'blue': 255, 'alpha': 255};
  } else if (colour == "lime") { // #00FF00
    return {'red': 0, 'green': 255, 'blue': 0, 'alpha': 255};
  } else if (colour == "green") { // #008000
    return {'red': 0, 'green': 128, 'blue': 0, 'alpha': 255};
  } else if (colour == "navy") { // #000080
    return {'red': 0, 'green': 0, 'blue': 128, 'alpha': 255};
  } else if (colour == "blue") { // #0000FF
    return {'red': 0, 'green': 0, 'blue': 255, 'alpha': 255};
  } else if (colour == "aqua" || colour == "cyan") { // #00FFFF
    return {'red': 0, 'green': 255, 'blue': 255, 'alpha': 255};
  } else if (colour == "teal") { // #008080
    return {'red': 0, 'green': 128, 'blue': 128, 'alpha': 255};
  } else if (colour == "black") { // #000000
    return {'red': 0, 'green': 0, 'blue': 0, 'alpha': 255};
  } else if (colour == "silver") { // #C0C0C0
    return {'red': 192, 'green': 192, 'blue': 192, 'alpha': 255};
  } else if (colour == "gray") { // #808080
    return {'red': 128, 'green': 128, 'blue': 128, 'alpha': 255};
  }
  var matches;
  matches = hex6_re.exec(colour);
  if (matches) {
    var red = parseInt(matches[1], 16);
    var green = parseInt(matches[2], 16);
    var blue = parseInt(matches[3], 16);
    return {'red': red, 'green': green, 'blue': blue, 'alpha': 255};
  }
  matches = hex3_re.exec(colour);
  if (matches) {
    var red = parseInt(matches[1] + matches[1], 16);
    var green = parseInt(matches[2] + matches[2], 16);
    var blue = parseInt(matches[3] + matches[3], 16);
    return {'red': red, 'green': green, 'blue': blue, 'alpha': 255};
  }
  matches = rgb_re.exec(colour);
  if (matches) {
    var red = parseInt(matches[1]);
    if (matches[2] == "%") red = Math.round((red * 255) / 100);
    var green = parseInt(matches[3]);
    if (matches[4] == "%") green = Math.round((green * 255) / 100);
    var blue = parseInt(matches[5]);
    if (matches[6] == "%") blue = Math.round((blue * 255) / 100);
    return {'red': red, 'green': green, 'blue': blue, 'alpha': 255};
  }
  matches = rgba_re.exec(colour);
  if (matches) {
    var red = parseInt(matches[1]);
    if (matches[2] == "%") red = Math.round((red * 255) / 100);
    var green = parseInt(matches[3]);
    if (matches[4] == "%") green = Math.round((green * 255) / 100);
    var blue = parseInt(matches[5]);
    if (matches[6] == "%") blue = Math.round((blue * 255) / 100);
    var alpha = parseInt(matches[7]);
    if (matches[8] == "%") alpha = Math.round((alpha * 255) / 100);
    return {'red': red, 'green': green, 'blue': blue, 'alpha': alpha};
  }
  // default to black
  throw new Error("Failed to parse colour: " + colour);
}

function colour_equals(colour1, colour2) {
  if (colour1.red != colour2.red) return false;
  if (colour1.green != colour2.green) return false;
  if (colour1.blue != colour2.blue) return false;
  if (colour1.alpha != colour2.alpha) return false;
  return true;
}

// splits a font string into words
function split_words(str) {
  var words = [];
  var start = -1;
  var space = /\s/;
  var single_quote = false;
  var double_quote = false;
  // read words
  for (var i = 0; i < str.length; i++) {
    if (start == -1) {
      if (!space.test(str.charAt(i))) {
        switch (str.charAt(i)) {
          case "'":
            single_quote = true;
            start = i + 1;
            break;
          case '"':
            double_quote = true;
            start = i + 1;
            break;
          default:
            start = i;
        }
      }
    } else {
      if (!single_quote && !double_quote) {
        if (space.test(str.charAt(i))) {
          var len = i - start;
          if (len > 0) words.push(str.substr(start, len));
          start = -1;
        } else if (str.charAt(i) == "'" || str.charAt(i) == '"') {
          throw new Error("Quote in the middle of an unquoted word!");
        }
      } else if (single_quote) {
        if (str.charAt(i) == "'") {
          var len = i - start;
          if (len > 0) words.push(str.substr(start, len));
          start = -1;
          single_quote = false;
        }
      } else if (double_quote) {
        if (str.charAt(i) == '"') {
          var len = i - start;
          if (len > 0) words.push(str.substr(start, len));
          start = -1;
          double_quote = false;
        }
      }
    }
  }
  if (start != -1) {
    if (single_quote || double_quote) throw new Error("Unterminated quoted region");
    words.push(str.substr(start));
  }
  return words;
}

function add_intercepts(ctx2d, eps_callback) {
  ctx2d.eps_callback = eps_callback;
  ctx2d.save = function() {
    this.eps_callback.save();
    Object.getPrototypeOf(this).save.call(this);
  };
  ctx2d.restore = function() {
    this.eps_callback.restore();
    Object.getPrototypeOf(this).restore.call(this);
  };
  ctx2d.beginPath = function() {
    this.eps_callback.beginPath();
    Object.getPrototypeOf(this).beginPath.call(this);
  };
  ctx2d.closePath = function() {
    this.eps_callback.closePath();
    Object.getPrototypeOf(this).closePath.call(this);
  };
  ctx2d.moveTo = function(x, y) {
    this.eps_callback.moveTo(x, y);
    Object.getPrototypeOf(this).moveTo.call(this, x, y);
  };
  ctx2d.lineTo = function(x, y) {
    this.eps_callback.lineTo(x, y);
    Object.getPrototypeOf(this).lineTo.call(this, x, y);
  };
  ctx2d.rect = function(x, y, w, h) {
    this.eps_callback.rect(x, y, w, h);
    Object.getPrototypeOf(this).rect.call(this, x, y, w, h);
  };
  ctx2d.arc = function(x, y, radius, startAngle, endAngle, anticlockwise) {
    this.eps_callback.arc(x, y, radius, startAngle, endAngle, anticlockwise);
    Object.getPrototypeOf(this).arc.call(this, x, y, radius, startAngle, endAngle, anticlockwise);
  };
  ctx2d.arcTo = function(cpx1, cpy1, cpx2, cpy2, radius) {
    this.eps_callback.arcTo(cpx1, cpy1, cpx2, cpy2, radius);
    Object.getPrototypeOf(this).arcTo.call(this, cpx1, cpy1, cpx2, cpy2, radius);
  };
  ctx2d.quadraticArcTo = function(cpx, cpy, x, y) {
    this.eps_callback.quadraticArcTo(cpx, cpy, x, y);
    Object.getPrototypeOf(this).quadraticArcTo.call(this, cpx, cpy, x, y);
  };
  ctx2d.bezierCurveTo = function(cp1x, cp1y, cp2x, cp2y, x, y) {
    this.eps_callback.bezierCurveTo(cp1x, cp1y, cp2x, cp2y, x, y);
    Object.getPrototypeOf(this).bezierCurveTo.call(this, cp1x, cp1y, cp2x, cp2y, x, y);
  };
  ctx2d.stroke = function() {
    this.eps_callback.stroke();
    Object.getPrototypeOf(this).stroke.call(this);
  };
  ctx2d.fill = function() {
    this.eps_callback.fill();
    Object.getPrototypeOf(this).fill.call(this);
  };
  ctx2d.clip = function() {
    this.eps_callback.clip();
    Object.getPrototypeOf(this).clip.call(this);
  };
  ctx2d.fillRect = function(x, y, width, height) {
    this.eps_callback.fillRect(x, y, width, height);
    Object.getPrototypeOf(this).fillRect.call(this, x, y, width, height);
  };
  ctx2d.strokeRect = function(x, y, width, height) {
    this.eps_callback.strokeRect(x, y, width, height);
    Object.getPrototypeOf(this).strokeRect.call(this, x, y, width, height);
  };
  ctx2d.clearRect = function(x, y, width, height) {
    this.eps_callback.clearRect(x, y, width, height);
    Object.getPrototypeOf(this).clearRect.call(this, x, y, width, height);
  };
  ctx2d.fillText = function(string, x, y) {
    this.eps_callback.fillText(string, x, y);
    Object.getPrototypeOf(this).fillText.call(this, string, x, y);
  };
  ctx2d.translate = function(dx, dy) {
    this.eps_callback.translate(dx, dy);
    Object.getPrototypeOf(this).translate.call(this, dx, dy);
  };
  ctx2d.rotate = function(angle) {
    this.eps_callback.rotate(angle);
    Object.getPrototypeOf(this).rotate.call(this, angle);
  };
  ctx2d.scale = function(sx, sy) {
    this.eps_callback.scale(sx, sy);
    Object.getPrototypeOf(this).scale.call(this, sx, sy);
  };
  ctx2d.transform = function(m11, m12, m21, m22, dx, dy) {
    this.eps_callback.transform(m11, m12, m21, m22, dx, dy);
    Object.getPrototypeOf(this).transform.call(this, m11, m12, m21, m22, dx, dy);
  };
  ctx2d.setTransform = function(m11, m12, m21, m22, dx, dy) {
    this.eps_callback.setTransform(m11, m12, m21, m22, dx, dy);
    Object.getPrototypeOf(this).setTransform.call(this, m11, m12, m21, m22, dx, dy);
  };
}

var EpsState = function(copy) {
  // canvas vars
  this.activeStyle = {'red': 0, 'green': 0, 'blue': 0, 'alpha': 255};
  this.fillStyle = {'red': 0, 'green': 0, 'blue': 0, 'alpha': 255};
  this.strokeStyle = {'red': 0, 'green': 0, 'blue': 0, 'alpha': 255};
  this.lineWidth = 1.0;
  this.lineCap = 0;
  this.lineJoin = 0;
  this.miterLimit = 10.0;
  this.font = undefined;
  this.textAlign = "left";
  this.textBaseline = "alphabetic";
  if (copy) {
    this.activeStyle = copy.activeStyle;
    this.fillStyle = copy.fillStyle;
    this.strokeStyle = copy.strokeStyle;
    this.lineWidth = copy.lineWidth;
    this.lineCap = copy.lineCap;
    this.lineJoin = copy.lineJoin;
    this.miterLimit = copy.miterLimit;
    this.font = copy.font;
    this.textAlign = copy.textAlign;
    this.textBaseline = copy.textBaseline;
  }
};


var EpsContext = function(ctx, width, height) {
  var title = "Image Title";
  var creator = "Image Creator";
  var date = new Date();
  // private parameters
  this.ctx = ctx;
  this.width = width;
  this.height = height;
  this.stack = [];
  this.current_state = new EpsState();
  this.font_lookup = {};
  this.indent = "";
  this.eps_text = 
    "%!PS-Adobe-3.0 EPSF-3.0\n" + 
    "%%Title: " + title + "\n" + 
    "%%Creator: " + creator + "\n" +
    "%%CreationDate: " + date.toUTCString() + "\n" + 
    "%%BoundingBox: 0 0 " + (width * 0.75) + " " + (height * 0.75) + "\n" +
    "%%Pages: 0\n" +
    "%%DocumentFonts:\n" +
    "%%EndComments\n" +
    "0.75 0.75 scale\n" +
    "0 " + height + " translate\n" + 
    "1 -1 scale\n";
  add_intercepts(ctx, this);
};

// look for differences between the current state and the settings to see 
// what has changed. Apply the changes to the eps file.
EpsContext.prototype.detect = function() {
  var state = this.current_state;
  var ctx = this.ctx;
  if (ctx.lineWidth != state.lineWidth) {
    state.lineWidth = ctx.lineWidth;
    this.eps_text += this.indent + state.lineWidth + " setlinewidth\n";
  }
  var cap = parse_line_cap(ctx.lineCap);
  if (cap != state.lineCap) {
    state.lineCap = cap;
    this.eps_text += this.indent + state.lineCap + " setlinecap\n";
  }
  var join = parse_line_join(ctx.lineJoin);
  if (join != state.lineJoin) {
    state.lineJoin = join;
    this.eps_text += this.indent + state.lineJoin + " setlinejoin\n";
  }
  var miterLimit = parseInt(ctx.miterLimit);
  if (miterLimit != state.miterLimit) {
    state.miterLimit = miterLimit;
    this.eps_text += this.indent + state.miterLimit + " setmiterlimit\n";
  }
  var strokeStyle = parse_colour(ctx.strokeStyle);
  if (!colour_equals(strokeStyle, state.strokeStyle)) {
    state.strokeStyle = strokeStyle;
    this.activateStyle(false);
  }
  var fillStyle = parse_colour(ctx.fillStyle);
  if (!colour_equals(fillStyle, state.fillStyle)) {
    state.fillStyle = fillStyle;
    this.activateStyle(true);
  }
  var textAlign = parse_text_align(ctx.textAlign);
  if (textAlign != state.textAlign) {
    state.textAlign = textAlign;
  }
  var textBaseline = parse_text_baseline(ctx.textBaseline);
  if (textBaseline != state.textBaseline) {
    state.textBaseline = textBaseline;
  }
  var font = this.lookup_font(ctx.font);
  if (font !== undefined && font != state.font) {
    state.font = font;
    this.eps_text += this.indent + "/" + font.name + " findfont " +  
      font.size + " scalefont setfont\n";
  }
};

EpsContext.prototype.activateStyle = function(useFillStyle) {
  var state = this.current_state;
  var style = (useFillStyle ? state.fillStyle : state.strokeStyle);
  if (!colour_equals(state.activeStyle, style)) {
    this.eps_text += this.indent + (style.red / 255) + " " + 
      (style.green / 255) + " " + (style.blue / 255) + " setrgbcolor\n";
    state.activeStyle = style;
  }
};


// saves the current state on the stack
EpsContext.prototype.save = function() {
  this.detect();
  // gsave
  this.eps_text += this.indent + "gsave\n";
  this.stack.push(new EpsState(this.current_state));
  this.indent += "  ";
};

// restores the last saved state
EpsContext.prototype.restore = function() {
  if (this.stack.length == 0) throw new Error("Call to restore not matched with call to save.");
  this.detect();
  this.current_state = this.stack.pop();
  this.indent = Array(this.stack.length + 1).join("  ");
  // grestore
  this.eps_text += this.indent + "grestore\n";
};

// start a path
EpsContext.prototype.beginPath = function() {
  this.detect();
  // newpath
  this.eps_text += this.indent + "newpath\n";
};

// join the current position to the path start
EpsContext.prototype.closePath = function() {
  this.detect();
  // closepath
  this.eps_text += this.indent + "closepath\n";
};

// move the current position
EpsContext.prototype.moveTo = function(x, y) {
  this.detect();
  // moveto
  this.eps_text += this.indent + x + " " + y + " moveto\n";
};

// join the current position to a new position and update the current position
EpsContext.prototype.lineTo = function(x, y) {
  this.detect();
  // lineto
  this.eps_text += this.indent + x + " " + y + " lineto\n";
};

// add a rectangle to the path
EpsContext.prototype.rect = function(x, y, width, height) {
  this.moveTo(x, y);
  this.lineTo(x + width, y);
  this.lineTo(x + width, y + height);
  this.lineTo(x, y + height);
  this.closePath();
};

// add an arc to the path
EpsContext.prototype.arc = function(x, y, radius, startAngle, endAngle, anticlockwise) {
  this.detect();
  if (anticlockwise) {
    // command "X Y RADIUS START_ANGLE END_ANGLE arc"
    this.eps_text += this.indent + x + " " + y + " " + radius + " " + rad2deg(startAngle) + " " + rad2deg(endAngle) + " arcn\n";
  } else {
    // command "X Y RADIUS START_ANGLE END_ANGLE arcn"
    this.eps_text += this.indent + x + " " + y + " " + radius + " " + rad2deg(startAngle) + " " + rad2deg(endAngle) + " arc\n";
  }
};

// imagine two lines, one going from the current position to point 1 and
// another going from point 1 to point 2. 
// Now imagine a circle of the given radius which touches the two lines at at 
// two tangental points T01 and T12. Add a line to the path that goes from
// the current position to T01 and add the arc which goes from T01 to T12.
// see http://www.dbp-consulting.com/tutorials/canvas/CanvasArcTo.html
EpsContext.prototype.arcTo = function(cpx1, cpy1, cpx2, cpy2, radius) {
  this.detect();
  // command "X1 Y1 X2 Y2 RADIUS arct"
  this.eps_text += this.indent + cpx1 + " " + cpy1 + " " + cpx2 + " " + cpy2 + " " + radius + " arct\n";
};

EpsContext.prototype.quadraticArcTo = function(cpx, cpy, x, y) {
  /* 
   For the equations below the following variable name prefixes are used: 
     qp0 is the quadratic curve starting point (you must keep this from your 
        last point sent to moveTo(), lineTo(), or bezierCurveTo() ). 
     qp1 is the quadratic curve control point (this is the cpx,cpy you would 
        have sent to quadraticCurveTo() ). 
     qp2 is the quadratic curve ending point (this is the x,y arguments you 
        would have sent to quadraticCurveTo() ). 
   We will convert these points to compute the two needed cubic control points 
    (the starting/ending points are the same for both 
   the quadratic and cubic curves. 
 
   The exact equations for the two cubic control points are: 
     cp0 = qp0 and cp3 = qp2 
     cp1 = qp0 + (qp1 - qp0) * ratio 
     cp2 = cp1 + (qp2 - qp0) * (1 - ratio) 
     where ratio = (sqrt(2) - 1) * 4 / 3 exactly (approx. 0.5522847498307933984022516322796) 
                   if the quadratic is an approximation of an elliptic arc, 
                      and the cubic must approximate the same arc, or 
           ratio = 2.0 / 3.0 for keeping the same quadratic curve. 
 
   In the code below, we must compute both the x and y terms for each point separately. 
 
    cp1x = qp0x + (qp1x - qp0x) * ratio; 
    cp1y = qp0y + (qp1y - qp0y) * ratio; 
    cp2x = cp1x + (qp2x - qp0x) * (1 - ratio); 
    cp2y = cp1y + (qp2y - qp0y) * (1 - ratio); 
 
   We will now  
     a) replace the qp0x and qp0y variables with currentX and currentY 
        (which *you* must store for each moveTo/lineTo/bezierCurveTo) 
     b) replace the qp1x and qp1y variables with cpx and cpy (which we would 
        have passed to quadraticCurveTo) 
     c) replace the qp2x and qp2y variables with x and y. 
   which leaves us with:  
  */  
  var ratio = 2.0 / 3.0; // 0.5522847498307933984022516322796 if the Bezier is 
                        // approximating an elliptic arc with best fitting  
  var cp1x = this.currentX + (cpx - this.currentX) * ratio;  
  var cp1y = this.currentY + (cpy - this.currentY) * ratio;  
  var cp2x = cp1x + (x - this.currentX) * (1 - ratio);  
  var cp2y = cp1y + (y - this.currentY) * (1 - ratio);  
  
  // and now call cubic Bezier curve to function   
  this.bezierCurveTo( cp1x, cp1y, cp2x, cp2y, x, y );
};

// add a bezier curve to the path
EpsContext.prototype.bezierCurveTo = function(cp1x, cp1y, cp2x, cp2y, x, y) {
  this.detect();
  // command "CP1X CP1Y CP2X CP2Y X Y curveto"
  this.eps_text += this.indent + cp1x + " " + cp1y + " " + cp2x + " " + cp2y + " " + x + " " + y + " curveto";
};

// stroke the current path
EpsContext.prototype.stroke = function() {
  this.detect();
  this.activateStyle(false);
  // stroke
  this.eps_text += this.indent + "stroke\n";
};

// fill the current path
EpsContext.prototype.fill = function() {
  this.detect();
  this.activateStyle(true);
  // fill
  this.eps_text += this.indent + "fill\n";
};

// create a clipping region from the current path
EpsContext.prototype.clip = function() {
  this.detect();
  // clip
  this.eps_text += this.indent + "clip\n";
};

// Draws a filled rectangle
EpsContext.prototype.fillRect = function(x, y, width, height) {
  this.detect();
  this.activateStyle(true);
  // rectfill
  this.eps_text += this.indent + x + " " + y + " " + width + " " + height + " rectfill\n";
};

// Draws a rectangular outline
EpsContext.prototype.strokeRect = function(x, y, width, height) {
  this.detect();
  this.activateStyle(false);
  // rectstroke
  this.eps_text += this.indent + x + " " + y + " " + width + " " + height + " rectstroke\n";
};

// Clears the specified area and makes it transparent.
EpsContext.prototype.clearRect = function(x, y, width, height) {
  this.detect();
  // fill a rectangle with white in the cleared region
  // EPS doesn't do transparency so this is as close as it can get
  // command "1 setgray X Y WIDTH HEIGHT rectfill"
  this.eps_text += this.indent + "gsave\n";
  this.eps_text += this.indent + "  1 setgray\n";
  this.eps_text += this.indent + "  " + x + " " + y + " " + width + " " + height + " rectfill\n";
  this.eps_text += this.indent + "grestore\n";
};

// Draws a filled string
EpsContext.prototype.fillText = function(string, x, y) {
  var state = this.current_state;
  this.detect();
  this.activateStyle(true);
  // lookup the font with "/FONT_NAME findfont" (leaves FONT on stack)
  // scale the font with "FONT SIZE scalefont" (leaves FONT on stack
  // set the font with "FONT setfont"
  // move the printing location with "X Y moveto"
  // various measurement commands will be needed to do center alignment etc
  // like "stringwidth"
  // show the text with "(STRING) show"
  string = string.replace(/\\/g, "\\\\");
  string = string.replace(/\(/g, "\\(");
  string = string.replace(/\)/g, "\\)");
  this.save();
  this.translate(x, y);
  this.scale(1, -1);
  this.eps_text += this.indent + "(" + string + ")\n";
  if (state.textBaseline != "alphabetic") {
    this.beginPath();
    this.moveTo(0,0);
    this.eps_text += this.indent + "dup true charpath flattenpath pathbbox %bounding box\n";
    this.eps_text += this.indent + "/ascent exch def pop /decent exch def pop\n";
    if (state.textBaseline == "top") {
      this.eps_text += this.indent + "0 ascent neg translate %vertical align top\n";
    } else if (state.textBaseline == "middle") {
      this.eps_text += this.indent + "0 ascent decent sub 2 div decent sub neg translate %vertical align middle\n";
    } else if (state.textBaseline == "bottom") {
      this.eps_text += this.indent + "0 decent neg translate %vertical align bottom\n";
    }
  }
  if (state.textAlign == "right") {
    this.eps_text += this.indent + "dup stringwidth pop neg 0 translate %right align\n"  
  } else if (state.textAlign == "center") {
    this.eps_text += this.indent + "dup stringwidth pop 2 div neg 0 translate %center align\n"  
  }
  this.moveTo(0,0);
  this.eps_text += this.indent + "show\n";
  this.restore();
};

// move the canvas origin
EpsContext.prototype.translate = function(dx, dy) {
  this.detect();
  // command "DELTA_X DELTA_Y translate"
  this.eps_text += this.indent + dx + " " + dy + " translate\n";
};

// rotate around the canvas origin
EpsContext.prototype.rotate = function(angle) {
  this.detect();
  // command "DEGREES rotate" (note need to convert angle from radians to degrees)
  this.eps_text += this.indent + rad2deg(angle) + " rotate\n";
};

// scale 
EpsContext.prototype.scale = function(scale_x, scale_y) {
  this.detect();
  // command "SCALE_X SCALE_Y scale
  this.eps_text += this.indent + scale_x + " " + scale_y + " scale\n";
};

// multiplies the current transform matrix by the matrix described by:
// m11    m21   dx
// m12    m22   dy
// 0      0     1
EpsContext.prototype.transform = function(m11, m12, m21, m22, dx, dy) {
  this.detect();
  // command "[M11 M12 M21 M22 DX DY] concat"
  this.eps_text += this.indent + "[" + m11 + " " + m12 + " " + m21 + " " + m22 + " " + dx + " " + dy + "] concat\n";
};

// reset transform matrix to the identity matrix then call transform
EpsContext.prototype.setTransform = function(m11, m12, m21, m22, dx, dy) {
  // command:
  // "[1 0 0 1 0 0] defaultmatrix setmatrix 0 height translate 1 -1 scale "
  // "[M11 M12 M21 M22 DX DY] concat"
  this.eps_text += this.indent + "[1 0 0 1 0 0] defaultmatrix setmatrix\n";
  this.eps_text += this.indent + "0 " + this.height + " translate\n";
  this.eps_text += this.indent + "1 -1 scale\n";
  this.transform(m11, m12, m21, m22, dx, dy);
};

EpsContext.prototype.register_font = function(font_str, eps_font, size) {
  font_str = font_str.replace(/\s+/g, ""); // remove spaces
  font_str = font_str.toLowerCase(); // lower case
  this.font_lookup[font_str] = {'name': eps_font, 'size': size};
};

EpsContext.prototype.lookup_font = function(font_str) {
  font_str = font_str.replace(/\s+/g, ""); // remove spaces
  font_str = font_str.toLowerCase(); // lower case
  return this.font_lookup[font_str];
};

// return the EPS text
EpsContext.prototype.eps = function() {
  return this.eps_text + this.indent + "showpage\n";
};
