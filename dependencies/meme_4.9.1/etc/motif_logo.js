//======================================================================
// start Alphabet object
//======================================================================
var Alphabet = function (alphabet, bg) {
  "use strict";
  var is_letter, is_prob, pos, letter, parts, i, freq;
  //variable prototype
  this.freqs = new Array();
  this.alphabet = new Array();
  this.letter_count = 0;
  //construct
  is_letter = /^\w$/;
  is_prob = /^((1(\.0+)?)|(0(\.\d+)?))$/;
  for (pos = 0; pos < alphabet.length; pos++) {
    letter = alphabet.charAt(pos);
    if (is_letter.test(letter)) {
      this.alphabet[this.letter_count] = letter.toUpperCase();
      this.freqs[this.letter_count] = -1;
      this.letter_count++;
    }
  }
  if (typeof bg !== "undefined") {
    parts = bg.split(/\s+/);
    for (i = 0, pos = 0; (i + 1) < parts.length; i += 2) {
      letter = parts[i];
      freq = parts[i+1];
      if (is_letter.test(letter) && is_prob.test(freq)) {
        letter = letter.toUpperCase();          //find the letter it matches
        for (;pos < this.letter_count; pos++) {
          if (this.alphabet[pos] == letter) {
            break;
          }
        }
        if (pos >= this.letter_count) {
          throw new Error("NOT_IN_ALPHABET");
        }
        this.freqs[pos] = (+freq);
      }
    }
  } else {
    //assume uniform background
    freq = 1.0 / this.letter_count;
    for (pos = 0; pos < this.letter_count; pos++) {
      this.freqs[pos] = freq;
    }
  }
};


Alphabet.prototype.get_ic = function() {
  "use strict";
  if (this.is_nucleotide()) {
    return 2;
  } else {
    return Math.log(20) / Math.LN2;
  }
};

Alphabet.prototype.get_size = function() {
  "use strict";
  return this.letter_count;
};

Alphabet.prototype.get_letter = function(alph_index) {
  "use strict";
  if (alph_index < 0 || alph_index >= this.letter_count) {
    throw new Error("BAD_ALPHABET_INDEX");
  }
  return this.alphabet[alph_index];
};

Alphabet.prototype.get_bg_freq = function(alph_index) {
  "use strict";
  if (alph_index < 0 || alph_index >= this.letter_count) {
    throw new Error("BAD_ALPHABET_INDEX");
  }
  if (this.freqs[alph_index] == -1) {
    throw new Error("BG_FREQ_NOT_SET");
  }
  return this.freqs[alph_index];
};

Alphabet.prototype.get_colour = function(alph_index) {
  "use strict";
  var red, blue, orange, green, yellow, purple, magenta, pink, turquoise;
  red = "rgb(204,0,0)";
  blue = "rgb(0,0,204)";
  orange = "rgb(255,179,0)";
  green = "rgb(0,128,0)";
  yellow = "rgb(255,255,0)";
  purple = "rgb(204,0,204)";
  magenta = "rgb(255,0,255)";
  pink = "rgb(255,204,204)";
  turquoise = "rgb(51,230,204)";
  if (alph_index < 0 || alph_index >= this.letter_count) {
    throw new Error("BAD_ALPHABET_INDEX");
  }
  if (this.is_nucleotide()) {
    switch (this.alphabet[alph_index]) {
      case "A":
        return red;
      case "C":
        return blue;
      case "G":
        return orange;
      case "T":
        return green;
      default:
        throw new Error("Invalid nucleotide letter");
    }
  } else {
    switch (this.alphabet[alph_index]) {
      case "A":
      case "C":
      case "F":
      case "I":
      case "L":
      case "V":
      case "W":
      case "M":
        return blue;
      case "N":
      case "Q":
      case "S":
      case "T":
        return green;
      case "D":
      case "E":
        return magenta;
      case "K":
      case "R":
        return red;
      case "H":
        return pink;
      case "G":
        return orange;
      case "P":
        return yellow;
      case "Y":
        return turquoise;
      default:
        throw new Error("Invalid protein letter");
    }
  }
  return "black";
};

Alphabet.prototype.is_ambig = function(alph_index) {
  "use strict";
  if (alph_index < 0 || alph_index >= this.letter_count) {
    throw new Error("BAD_ALPHABET_INDEX");
  }
  if (this.is_nucleotide()) {
    return ("ACGT".indexOf(this.alphabet[alph_index]) == -1);
  } else {
    return ("ACDEFGHIKLMNPQRSTVWY".indexOf(this.alphabet[alph_index]) == -1);
  }
};

Alphabet.prototype.get_index = function(letter) {
  "use strict";
  var i;
  for (i = 0; i < this.letter_count; i++) {
    if (this.alphabet[i] == letter.toUpperCase()) {
      return i;
    }
  }
  throw new Error("UNKNOWN_LETTER");
};

Alphabet.prototype.is_nucleotide = function() {
  "use strict";
  //TODO basic method, make better
  if (this.letter_count < 20) {
    return true;
  }
  return false;
};

Alphabet.prototype.toString = function() {
  "use strict";
  return (this.is_nucleotide() ? "Nucleotide" : "Protein") + 
    " Alphabet " + (this.alphabet.join(""));
};

//======================================================================
// end Alphabet object
//======================================================================

//======================================================================
// start Symbol object
//======================================================================
var Symbol = function(alph_index, scale, alphabet) {
  "use strict";
  //variable prototype
  this.symbol = alphabet.get_letter(alph_index);
  this.scale = scale;
  this.colour = alphabet.get_colour(alph_index);
};

Symbol.prototype.get_symbol = function() {
  "use strict";
  return this.symbol;
};

Symbol.prototype.get_scale = function() {
  "use strict";
  return this.scale;
};

Symbol.prototype.get_colour = function() {
  "use strict";
  return this.colour;
};

Symbol.prototype.toString = function() {
  "use strict";
  return this.symbol + " " + (Math.round(this.scale*1000)/10) + "%";
};

function compare_symbol(sym1, sym2) {
  "use strict";
  if (sym1.get_scale() < sym2.get_scale()) {
    return -1;
  } else if (sym1.get_scale() > sym2.get_scale()) {
    return 1;
  } else {
    return 0;
  }
}
//======================================================================
// end Symbol object
//======================================================================

//======================================================================
// start Pspm object
//======================================================================
var Pspm = function(matrix, name, ltrim, rtrim, nsites, evalue) {
  "use strict";
  var row, col, data, row_sum, delta, evalue_re;
  if (typeof name !== "string") {
    name = "";
  }
  this.name = name;
  //construct
  if (matrix instanceof Pspm) {
    // copy constructor
    this.alph_length = matrix.alph_length;
    this.motif_length = matrix.motif_length;
    this.name = matrix.name;
    this.nsites = matrix.nsites;
    this.evalue = matrix.evalue;
    this.ltrim = matrix.ltrim;
    this.rtrim = matrix.rtrim;
    this.pspm = [];
    for (row = 0; row < matrix.motif_length; row++) {
      this.pspm[row] = [];
      for (col = 0; col < matrix.alph_length; col++) {
        this.pspm[row][col] = matrix.pspm[row][col];
      }
    }
  } else {
    // check parameters
    if (typeof ltrim === "undefined") {
      ltrim = 0;
    } else if (typeof ltrim !== "number" || ltrim % 1 !== 0 || ltrim < 0) {
      throw new Error("ltrim must be a non-negative integer, got: " + ltrim);
    }
    if (typeof rtrim === "undefined") {
      rtrim = 0;
    } else if (typeof rtrim !== "number" || rtrim % 1 !== 0 || rtrim < 0) {
      throw new Error("rtrim must be a non-negative integer, got: " + rtrim);
    }
    if (typeof nsites !== "undefined") {
      if (typeof nsites !== "number" || nsites <= 0) {
        throw new Error("nsites must be a positive number, got: " + nsites);
      }
    }
    if (typeof evalue !== "undefined") {
      if (typeof evalue === "number") {
        if (evalue < 0) {
          throw new Error("evalue must be a non-negative number, got: " + evalue);
        }
      } else if (typeof evalue === "string") {
        evalue_re = /^((?:[+]?[0-9]*\.?[0-9]+(?:[eE][-+]?[0-9]+)?)|inf)$/;
        if (!evalue_re.test(evalue)) {
          throw new Error("evalue must be a non-negative number, got: " + evalue);
        }
      } else {
        throw new Error("evalue must be a non-negative number, got: " + evalue);
      }
    }
    // set properties
    this.name = name;
    this.nsites = nsites;
    this.evalue = evalue;
    this.ltrim = ltrim;
    this.rtrim = rtrim;
    if (typeof matrix === "string") {
      // string constructor
      data = parse_pspm_string(matrix);
      this.alph_length = data["alph_length"];
      this.motif_length = data["motif_length"];
      this.pspm = data["pspm"];
      if (typeof this.evalue === "undefined") {
        if (typeof data["evalue"] !== "undefined") {
          this.evalue = data["evalue"];
        } else {
          this.evalue = 0;
        }
      }
      if (typeof this.nsites === "undefined") {
        if (typeof data["nsites"] === "number") {
          this.nsites = data["nsites"];
        } else {
          this.nsites = 20;
        }
      }
    } else {
      // assume pspm is a nested array
      this.motif_length = matrix.length;
      this.alph_length = (matrix.length > 0 ? matrix[0].length : 0);
      if (typeof this.nsites === "undefined") {
        this.nsites = 20;
      }
      if (typeof this.evalue === "undefined") {
        this.evalue = 0;
      }
      this.pspm = [];
      // copy pspm and check
      for (row = 0; row < this.motif_length; row++) {
        if (this.alph_length != matrix[row].length) {
          throw new Error("COLUMN_MISMATCH");
        }
        this.pspm[row] = [];
        row_sum = 0;
        for (col = 0; col < this.alph_length; col++) {
          this.pspm[row][col] = matrix[row][col];
          row_sum += this.pspm[row][col];
        }
        delta = 0.1;
        if (isNaN(row_sum) || (row_sum > 1 && (row_sum - 1) > delta) || 
            (row_sum < 1 && (1 - row_sum) > delta)) {
          throw new Error("INVALID_SUM");
        }
      }
    }
  }
};

Pspm.prototype.copy = function() {
  "use strict";
  return new Pspm(this);
};

Pspm.prototype.reverse_complement = function(alphabet) {
  "use strict";
  var x, y, temp, a_index, c_index, g_index, t_index, i, row, temp_trim;
  if (this.alph_length != alphabet.get_size()) {
    throw new Error("ALPHABET_MISMATCH");
  }
  if (!alphabet.is_nucleotide()) {
    throw new Error("NO_PROTEIN_RC");
  }
  //reverse
  x = 0;
  y = this.motif_length-1;
  while (x < y) {
    temp = this.pspm[x];
    this.pspm[x] = this.pspm[y];
    this.pspm[y] = temp;
    x++;
    y--;
  }
  //complement
  a_index = alphabet.get_index("A");
  c_index = alphabet.get_index("C");
  g_index = alphabet.get_index("G");
  t_index = alphabet.get_index("T");
  for (i = 0; i < this.motif_length; i++) {
    row = this.pspm[i];
    //swap A and T
    temp = row[a_index];
    row[a_index] = row[t_index];
    row[t_index] = temp;
    //swap C and G
    temp = row[c_index];
    row[c_index] = row[g_index];
    row[g_index] = temp;
  }
  //swap triming
  temp_trim = this.ltrim;
  this.ltrim = this.rtrim;
  this.rtrim = temp_trim;
  //note that ambigs are ignored because they don't effect motifs
  return this; //allow function chaining...
};

Pspm.prototype.get_stack = function(position, alphabet) {
  "use strict";
  var row, stack_ic, alphabet_ic, stack, i, sym;
  if (this.alph_length != alphabet.get_size()) {
    throw new Error("ALPHABET_MISMATCH");
  }
  row = this.pspm[position];
  stack_ic = this.get_stack_ic(position, alphabet);
  alphabet_ic = alphabet.get_ic();
  stack = [];
  for (i = 0; i < this.alph_length; i++) {
    if (alphabet.is_ambig(i)) {
      continue;
    }
    sym = new Symbol(i, row[i]*stack_ic/alphabet_ic, alphabet);
    if (sym.get_scale() <= 0) {
      continue;
    }
    stack.push(sym);
  }
  stack.sort(compare_symbol);
  return stack;
};

Pspm.prototype.get_stack_ic = function(position, alphabet) {
  "use strict";
  var row, H, i;
  if (this.alph_length != alphabet.get_size()) {
    throw new Error("ALPHABET_MISMATCH");
  }
  row = this.pspm[position];
  H = 0;
  for (i = 0; i < this.alph_length; i++) {
    if (alphabet.is_ambig(i)) {
      continue;
    }
    if (row[i] === 0) {
      continue;
    }
    H -= (row[i] * (Math.log(row[i]) / Math.LN2));
  }
  return alphabet.get_ic() - H;
};

Pspm.prototype.get_error = function(alphabet) {
  "use strict";
  var asize;
  if (this.nsites === 0) {
    return 0;
  }
  if (alphabet.is_nucleotide()) {
    asize = 4;
  } else {
    asize = 20;
  }
  return (asize-1) / (2 * Math.log(2)*this.nsites);
};

Pspm.prototype.get_motif_length = function() {
  "use strict";
  return this.motif_length;
};

Pspm.prototype.get_alph_length = function() {
  "use strict";
  return this.alph_length;
};

Pspm.prototype.get_left_trim = function() {
  "use strict";
  return this.ltrim;
};

Pspm.prototype.get_right_trim = function() {
  "use strict";
  return this.rtrim;
};

Pspm.prototype.as_pspm = function() {
  "use strict";
  var out, row, col;
  out = "letter-probability matrix: alength= " + this.alph_length + 
      " w= " + this.motif_length + " nsites= " + this.nsites + 
      " E= " + (typeof this.evalue === "number" ? 
          this.evalue.toExponential() : this.evalue) + "\n";
  for (row = 0; row < this.motif_length; row++) {
    for (col = 0; col < this.alph_length; col++) {
      if (col !== 0) {
        out += " ";
      }
      out += this.pspm[row][col].toFixed(6);
    }
    out += "\n";
  }
  return out;
};

Pspm.prototype.as_pssm = function(alphabet, pseudo) {
  "use strict";
  var out, log2, total, row, col, p, bg, p2, score;
  if (typeof pseudo === "undefined") {
    pseudo = 0.1;
  } else if (typeof pseudo !== "number") {
    throw new Error("Expected number for pseudocount");
  }
  out = "log-odds matrix: alength= " + this.alph_length + 
      " w= " + this.motif_length + 
      " E= " + (typeof this.evalue == "number" ? 
          this.evalue.toExponential() : this.evalue) + "\n";
  log2 = Math.log(2);
  total = this.nsites + pseudo;
  for (row = 0; row < this.motif_length; row++) {
    for (col = 0; col < this.alph_length; col++) {
      if (col !== 0) {
        out += " ";
      }
      p = this.pspm[row][col];
      // to avoid log of zero we add a pseudo count
      bg = alphabet.get_bg_freq(col);
      p2 = (p * this.nsites + bg * pseudo) / total;
      // now calculate the score
      score = -10000;
      if (p2 > 0) {
        score = Math.round((Math.log(p2 / bg) / log2) * 100);
      }
      out += score;
    }
    out += "\n";
  }
  return out;
};

Pspm.prototype.toString = function() {
  "use strict";
  var str, i, row;
  str = "";
  for (i = 0; i < this.pspm.length; i++) {
    row = this.pspm[i];
    str += row.join("\t") + "\n";
  }
  return str;
};

function parse_pspm_properties(str) {
  "use strict";
  var parts, i, eqpos, before, after, properties, prop, num, num_re;
  num_re = /^((?:[+]?[0-9]*\.?[0-9]+(?:[eE][-+]?[0-9]+)?)|inf)$/;
  parts = trim(str).split(/\s+/);
  // split up words containing =
  for (i = 0; i < parts.length;) {
    eqpos = parts[i].indexOf("=");
    if (eqpos != -1) {
      before = parts[i].substr(0, eqpos);
      after = parts[i].substr(eqpos+1);
      if (before.length > 0 && after.length > 0) {
        parts.splice(i, 1, before, "=", after);
        i += 3;
      } else if (before.length > 0) {
        parts.splice(i, 1, before, "=");
        i += 2;
      } else if (after.length > 0) {
        parts.splice(i, 1, "=", after);
        i += 2;
      } else {
        parts.splice(i, 1, "=");
        i++;
      }
    } else {
      i++;
    }
  }
  properties = {};
  for (i = 0; i < parts.length; i += 3) {
    if (parts.length - i < 3) {
      throw new Error("Expected PSPM property was incomplete. "+
          "Remaing parts are: " + parts.slice(i).join(" "));
    }
    if (parts[i+1] !== "=") {
      throw new Error("Expected '=' in PSPM property between key and " +
          "value but got " + parts[i+1]); 
    }
    prop = parts[i].toLowerCase();
    num = parts[i+2];
    if (!num_re.test(num)) {
      throw new Error("Expected numeric value for PSPM property '" + 
          prop + "' but got '" + num + "'");
    }
    properties[prop] = num;
  }
  return properties;
}

function parse_pspm_string(pspm_string) {
  "use strict";
  var header_re, lines, first_line, line_num, col_num, alph_length, 
      motif_length, nsites, evalue, pspm, i, line, match, props, parts,
      j, prob;
  header_re = /^letter-probability\s+matrix:(.*)$/i;
  lines = pspm_string.split(/\n/);
  first_line = true;
  line_num = 0;
  col_num = 0;
  alph_length;
  motif_length;
  nsites;
  evalue;
  pspm = [];
  for (i = 0; i < lines.length; i++) {
    line = trim(lines[i]);
    if (line.length === 0) { 
      continue;
    }
    // check the first line for a header though allow matrices without it
    if (first_line) {
      first_line = false;
      match = header_re.exec(line);
      if (match !== null) {
        props = parse_pspm_properties(match[1]);
        if (props.hasOwnProperty("alength")) {
          alph_length = parseFloat(props["alength"]);
          if (alph_length != 4 && alph_length != 20) {
            throw new Error("PSPM property alength should be 4 or 20" +
                " but got " + alph_length);
          }
        }
        if (props.hasOwnProperty("w")) {
          motif_length = parseFloat(props["w"]);
          if (motif_length % 1 !== 0 || motif_length < 1) {
            throw new Error("PSPM property w should be an integer larger " +
                "than zero but got " + motif_length);
          }
        }
        if (props.hasOwnProperty("nsites")) {
          nsites = parseFloat(props["nsites"]);
          if (nsites <= 0) {
            throw new Error("PSPM property nsites should be larger than " +
                "zero but got " + nsites);
          }
        }
        if (props.hasOwnProperty("e")) {
          evalue = props["e"];
          if (evalue < 0) {
            throw new Error("PSPM property evalue should be " +
                "non-negative but got " + evalue);
          }
        }
        continue;
      }
    }
    pspm[line_num] = [];
    col_num = 0;
    parts = line.split(/\s+/);
    for (j = 0; j < parts.length; j++) {
      prob = parseFloat(parts[j]);
      if (prob != parts[j] || prob < 0 || prob > 1) {
        throw new Error("Expected probability but got '" + parts[j] + "'"); 
      }
      pspm[line_num][col_num] = prob;
      col_num++;
    }
    line_num++;
  }
  if (typeof motif_length === "number") {
    if (pspm.length != motif_length) {
      throw new Error("Expected PSPM to have a motif length of " + 
          motif_length + " but it was actually " + pspm.length);
    }
  } else {
    motif_length = pspm.length;
  }
  if (typeof alph_length !== "number") {
    alph_length = pspm[0].length;
    if (alph_length != 4 && alph_length != 20) {
      throw new Error("Expected length of first row in the PSPM to be " +
          "either 4 or 20 but got " + alph_length);
    }
  }
  for (i = 0; i < pspm.length; i++) {
    if (pspm[i].length != alph_length) {
      throw new Error("Expected PSPM row " + i + " to have a length of " + 
          alph_length + " but the length was " + pspm[i].length);
    }
  }
  return {"pspm": pspm, "motif_length": motif_length, 
    "alph_length": alph_length, "nsites": nsites, "evalue": evalue};
}
//======================================================================
// end Pspm object
//======================================================================

//======================================================================
// start Logo object
//======================================================================

var Logo = function(alphabet, fine_text) {
  "use strict";
  this.alphabet = alphabet;
  this.fine_text = fine_text;
  this.pspm_list = [];
  this.pspm_column = [];
  this.rows = 0;
  this.columns = 0;
};

Logo.prototype.add_pspm = function(pspm, column) {
  "use strict";
  var col;
  if (typeof column === "undefined") {
    column = 0;
  } else if (column < 0) {
    throw new Error("Column index out of bounds.");
  }
  this.pspm_list[this.rows] = pspm;
  this.pspm_column[this.rows] = column;
  this.rows++;
  col = column + pspm.get_motif_length();
  if (col > this.columns) {
    this.columns = col;
  }
};

Logo.prototype.get_columns = function() {
  "use strict";
  return this.columns;
};

Logo.prototype.get_rows = function() {
  "use strict";
  return this.rows;
};

Logo.prototype.get_pspm = function(row_index) {
  "use strict";
  if (row_index < 0 || row_index >= this.rows) {
    throw new Error("INDEX_OUT_OF_BOUNDS");
  }
  return this.pspm_list[row_index];
};

Logo.prototype.get_offset = function(row_index) {
  "use strict";
  if (row_index < 0 || row_index >= this.rows) {
    throw new Error("INDEX_OUT_OF_BOUNDS");
  }
  return this.pspm_column[row_index];
};

//======================================================================
// end Logo object
//======================================================================

//======================================================================
// start RasterizedAlphabet
//======================================================================

// Rasterize Alphabet
// 1) Measure width of text at default font for all symbols in alphabet
// 2) sort in width ascending
// 3) Drop the top and bottom 10% (designed to ignore outliers like 'W' and 'I')
// 4) Calculate the average as the maximum scaling factor (designed to stop I becoming a rectangular blob).
// 5) Assume scale of zero would result in width of zero, interpolate scale required to make perfect width font
// 6) Draw text onto temp canvas at calculated scale
// 7) Find bounds of drawn text
// 8) Paint on to another canvas at the desired height (but only scaling width to fit if larger).
var RasterizedAlphabet = function(alphabet, font, target_width) {
  "use strict";
  var default_size, safety_pad, canvas, ctx, middle, baseline, widths, count,
      letters, i, letter, size, tenpercent, avg_width, scale, 
      target_height, raster;
  //variable prototypes
  this.lookup = []; //a map of letter to index
  this.rasters = []; //a list of rasters
  this.dimensions = []; //a list of dimensions

  //construct
  default_size = 60; // size of square to assume as the default width
  safety_pad = 20; // pixels to pad around so we don't miss the edges
  // create a canvas to do our rasterizing on
  canvas = document.createElement("canvas");
  // assume the default font would fit in a canvas of 100 by 100
  canvas.width = default_size + 2 * safety_pad;
  canvas.height = default_size + 2 * safety_pad;
  // check for canvas support before attempting anything
  if (!canvas.getContext) {
    throw new Error("NO_CANVAS_SUPPORT");
  }
  ctx = canvas.getContext('2d');
  // check for html5 text drawing support
  if (!supports_text(ctx)) {
    throw new Error("NO_CANVAS_TEXT_SUPPORT");
  }
  // calculate the middle
  middle = Math.round(canvas.width / 2);
  // calculate the baseline
  baseline = Math.round(canvas.height - safety_pad);
  // list of widths
  widths = [];
  count = 0;
  letters = [];
  //now measure each letter in the alphabet
  for (i = 0; i < alphabet.get_size(); ++i) {
    if (alphabet.is_ambig(i)) {
      continue; //skip ambigs as they're never rendered
    }
    letter = alphabet.get_letter(i);
    letters.push(letter);
    this.lookup[letter] = count;
    //clear the canvas
    canvas.width = canvas.width;
    // get the context and prepare to draw our width test
    ctx = canvas.getContext('2d');
    ctx.font = font;
    ctx.fillStyle = alphabet.get_colour(i);
    ctx.textAlign = "center";
    ctx.translate(middle, baseline);
    // draw the test text
    ctx.fillText(letter, 0, 0);
    //measure
    size = canvas_bounds(ctx, canvas.width, canvas.height);
    if (size.width === 0) {
      throw new Error("INVISIBLE_LETTER"); //maybe the fill was white on white?
    }
    widths.push(size.width);
    this.dimensions[count] = size;
    count++;
  }
  //sort the widths
  widths.sort(function(a,b) {return a - b;});
  //drop 10% of the items off each end
  tenpercent = Math.floor(widths.length / 10);
  for (i = 0; i < tenpercent; ++i) {
    widths.pop();
    widths.shift();
  }
  //calculate average width
  avg_width = 0;
  for (i = 0; i < widths.length; ++i) {
    avg_width += widths[i];
  }
  avg_width /= widths.length;
  // calculate scales
  for (i = 0; i < this.dimensions.length; ++i) {
    size = this.dimensions[i];
    // calculate scale
    scale = target_width / Math.max(avg_width, size.width);
    // estimate scaled height
    target_height = size.height * scale;
    // create an approprately sized canvas
    raster = document.createElement("canvas");
    raster.width = target_width; // if it goes over the edge too bad...
    raster.height = target_height + safety_pad * 2;
    // calculate the middle
    middle = Math.round(raster.width / 2);
    // calculate the baseline
    baseline = Math.round(raster.height - safety_pad);
    // get the context and prepare to draw the rasterized text
    ctx = raster.getContext('2d');
    ctx.font = font;
    ctx.fillStyle = alphabet.get_colour(i);
    ctx.textAlign = "center";
    ctx.translate(middle, baseline);
    ctx.save();
    ctx.scale(scale, scale);
    // draw the rasterized text
    ctx.fillText(letters[i], 0, 0);
    ctx.restore();
    this.rasters[i] = raster;
    this.dimensions[i] = canvas_bounds(ctx, raster.width, raster.height);
  }
};

function canvas_bounds(ctx, cwidth, cheight) {
  "use strict";
  var data, r, c, top_line, bottom_line, left_line, right_line, 
      txt_width, txt_height;
  data = ctx.getImageData(0, 0, cwidth, cheight).data;
  r = 0; c = 0; // r: row, c: column
  top_line = -1; bottom_line = -1; left_line = -1; right_line = -1;
  txt_width = 0; txt_height = 0;
  // Find the top-most line with a non-white pixel
  for (r = 0; r < cheight; r++) {
    for (c = 0; c < cwidth; c++) {
      if (data[r * cwidth * 4 + c * 4 + 3]) {
        top_line = r;
        break;
      }
    }
    if (top_line != -1) {
      break;
    }
  }
  
  //find the last line with a non-white pixel
  if (top_line != -1) {
    for (r = cheight-1; r >= top_line; r--) {
      for(c = 0; c < cwidth; c++) {
        if(data[r * cwidth * 4 + c * 4 + 3]) {
          bottom_line = r;
          break;
        }
      }
      if (bottom_line != -1) {
        break;
      }
    }
    txt_height = bottom_line - top_line + 1;
  }

  // Find the left-most line with a non-white pixel
  for (c = 0; c < cwidth; c++) {
    for (r = 0; r < cheight; r++) {
      if (data[r * cwidth * 4 + c * 4 + 3]) {
        left_line = c;
        break;
      }
    }
    if (left_line != -1) {
      break;
    }
  }

  //find the right most line with a non-white pixel
  if (left_line != -1) {
    for (c = cwidth-1; c >= left_line; c--) {
      for(r = 0; r < cheight; r++) {
        if(data[r * cwidth * 4 + c * 4 + 3]) {
          right_line = c;
          break;
        }
      }
      if (right_line != -1) {
        break;
      }
    }
    txt_width = right_line - left_line + 1;
  }

  //return the bounds
  return {bound_top: top_line, bound_bottom: bottom_line, 
    bound_left: left_line, bound_right: right_line, width: txt_width, 
    height: txt_height};
}

RasterizedAlphabet.prototype.draw = function(ctx, letter, dx, dy, dWidth, dHeight) {
  "use strict";
  var index, raster, size;
  index = this.lookup[letter];
  raster = this.rasters[index];
  size = this.dimensions[index];
  ctx.drawImage(raster, 0, size.bound_top -1, raster.width, size.height+1, dx, dy, dWidth, dHeight);
};

//======================================================================
// end RasterizedAlphabet
//======================================================================

//======================================================================
// start LogoMetrics object
//======================================================================

var LogoMetrics = function(ctx, logo_columns, logo_rows, allow_space_for_names) {
  "use strict";
  var i, row_height;
  if (typeof allow_space_for_names === "undefined") {
    allow_space_for_names = false;
  }
  //variable prototypes
  this.pad_top = 5;
  this.pad_left = 10;
  this.pad_right = 5;
  this.pad_bottom = 0;
  this.pad_middle = 20;
  this.name_height = 14;
  this.name_font = "bold " + this.name_height + "px Times, sans-serif";
  this.name_spacer = 0;
  this.y_label = "bits";
  this.y_label_height = 12;
  this.y_label_font = "bold " + this.y_label_height + "px Helvetica, sans-serif";
  this.y_label_spacer = 3;
  this.y_num_height = 12;
  this.y_num_width = 0;
  this.y_num_font = "bold " + this.y_num_height + "px Helvetica, sans-serif";
  this.y_tic_width = 5;
  this.stack_pad_left = 0;
  this.stack_font = "bold 25px Helvetica, sans-serif";
  this.stack_height = 90;
  this.stack_width = 26;
  this.stacks_pad_right = 5;
  this.x_num_above = 2;
  this.x_num_height = 12;
  this.x_num_width = 0;
  this.x_num_font = "bold " + this.x_num_height + "px Helvetica, sans-serif";
  this.fine_txt_height = 6;
  this.fine_txt_above = 2;
  this.fine_txt_font = "normal " + this.fine_txt_height + "px Helvetica, sans-serif";
  this.letter_metrics = new Array();
  this.summed_width = 0;
  this.summed_height = 0;
  //calculate the width of the y axis numbers
  ctx.font = this.y_num_font;
  for (i = 0; i <= 2; i++) {
    this.y_num_width = Math.max(this.y_num_width, ctx.measureText("" + i).width);
  }
  //calculate the width of the x axis numbers (but they are rotated so it becomes height)
  ctx.font = this.x_num_font;
  for (i = 1; i <= logo_columns; i++) {
    this.x_num_width = Math.max(this.x_num_width, ctx.measureText("" + i).width);
  }
  
  //calculate how much vertical space we want to draw this
  //first we add the padding at the top and bottom since that's always there
  this.summed_height += this.pad_top + this.pad_bottom;
  //all except the last row have the same amount of space allocated to them
  if (logo_rows > 1) {
    row_height = this.stack_height + this.pad_middle;
    if (allow_space_for_names) {
      row_height += this.name_height;
      //the label is allowed to overlap into the spacer
      row_height += Math.max(this.y_num_height/2, this.name_spacer); 
      //the label is allowed to overlap the space used by the other label
      row_height += Math.max(this.y_num_height/2, this.x_num_height + this.x_num_above); 
    } else {
      row_height += this.y_num_height/2; 
      //the label is allowed to overlap the space used by the other label
      row_height += Math.max(this.y_num_height/2, this.x_num_height + this.x_num_above); 
    }
    this.summed_height += row_height * (logo_rows - 1);
  }
  //the last row has the name and fine text below it but no padding
  this.summed_height += this.stack_height + this.y_num_height/2;
  if (allow_space_for_names) {
    this.summed_height += this.fine_txt_height + this.fine_txt_above + this.name_height;
    this.summed_height += Math.max(this.y_num_height/2, 
        this.x_num_height + this.x_num_above + this.name_spacer);
  } else {
    this.summed_height += Math.max(this.y_num_height/2, 
        this.x_num_height + this.x_num_above + this.fine_txt_height + this.fine_txt_above);
  }

  //calculate how much horizontal space we want to draw this
  //first add the padding at the left and right since that's always there
  this.summed_width += this.pad_left + this.pad_right;
  //add on the space for the y-axis label
  this.summed_width += this.y_label_height + this.y_label_spacer;
  //add on the space for the y-axis
  this.summed_width += this.y_num_width + this.y_tic_width;
  //add on the space for the stacks
  this.summed_width += (this.stack_pad_left + this.stack_width) * logo_columns;
  //add on the padding after the stacks (an offset from the fine text)
  this.summed_width += this.stacks_pad_right;

};

//======================================================================
// end LogoMetrics object
//======================================================================

//found this trick at http://talideon.com/weblog/2005/02/detecting-broken-images-js.cfm
function image_ok(img) {
  "use strict";
  // During the onload event, IE correctly identifies any images that
  // weren't downloaded as not complete. Others should too. Gecko-based
  // browsers act like NS4 in that they report this incorrectly.
  if (!img.complete) {
    return false;
  }
  // However, they do have two very useful properties: naturalWidth and
  // naturalHeight. These give the true size of the image. If it failed
  // to load, either of these should be zero.
  if (typeof img.naturalWidth !== "undefined" && img.naturalWidth === 0) {
    return false;
  }
  // No other way of checking: assume it's ok.
  return true;
}
  
function supports_text(ctx) {
  "use strict";
  if (!ctx.fillText) {
    return false;
  }
  if (!ctx.measureText) {
    return false;
  }
  return true;
}

//draws the scale, returns the width
function draw_scale(ctx, metrics, alphabet_ic) {
  "use strict";
  var tic_height, i;
  tic_height = metrics.stack_height / alphabet_ic;
  ctx.save();
  ctx.lineWidth = 1.5;
  ctx.translate(metrics.y_label_height, metrics.y_num_height/2);
  //draw the axis label
  ctx.save();
  ctx.font = metrics.y_label_font;
  ctx.translate(0, metrics.stack_height/2);
  ctx.save();
  ctx.rotate(-(Math.PI / 2));
  ctx.textAlign = "center";
  ctx.fillText("bits", 0, 0);
  ctx.restore();
  ctx.restore();

  ctx.translate(metrics.y_label_spacer + metrics.y_num_width, 0);

  //draw the axis tics
  ctx.save();
  ctx.translate(0, metrics.stack_height);
  ctx.font = metrics.y_num_font;
  ctx.textAlign = "right";
  ctx.textBaseline = "middle";
  for (i = 0; i <= alphabet_ic; i++) {
    //draw the number
    ctx.fillText("" + i, 0, 0);
    //draw the tic
    ctx.beginPath();
    ctx.moveTo(0, 0);
    ctx.lineTo(metrics.y_tic_width, 0);
    ctx.stroke();
    //prepare for next tic
    ctx.translate(0, -tic_height);
  }
  ctx.restore();

  ctx.translate(metrics.y_tic_width, 0);

  ctx.beginPath();
  ctx.moveTo(0, 0);
  ctx.lineTo(0, metrics.stack_height);
  ctx.stroke();

  ctx.restore();
}

function draw_stack_num(ctx, metrics, row_index) {
  "use strict";
  ctx.save();
  ctx.font = metrics.x_num_font;
  ctx.translate(metrics.stack_width / 2, metrics.stack_height + metrics.x_num_above);
  ctx.save();
  ctx.rotate(-(Math.PI / 2));
  ctx.textBaseline = "middle";
  ctx.textAlign = "right";
  ctx.fillText("" + (row_index + 1), 0, 0);
  ctx.restore();
  ctx.restore();
}

function draw_stack(ctx, metrics, symbols, raster) {
  "use strict";
  var preferred_pad, sym_min, i, sym, sym_height, pad;
  preferred_pad = 0;
  sym_min = 5;

  ctx.save();//1
  ctx.translate(0, metrics.stack_height);
  for (i = 0; i < symbols.length; i++) {
    sym = symbols[i];
    sym_height = metrics.stack_height * sym.get_scale();
    
    pad = preferred_pad;
    if (sym_height - pad < sym_min) {
      pad = Math.min(pad, Math.max(0, sym_height - sym_min));
    }
    sym_height -= pad;

    //translate to the correct position
    ctx.translate(0, -(pad/2 + sym_height));
    //draw
    raster.draw(ctx, sym.get_symbol(), 0, 0, metrics.stack_width, sym_height);
    //translate past the padding
    ctx.translate(0, -(pad/2));
  }
  ctx.restore();//1
}

function draw_dashed_line(ctx, pattern, start, x1, y1, x2, y2) {
  "use strict";
  var x, y, len, i, dx, dy, tlen, theta, mulx, muly, lx, ly;
  dx = x2 - x1;
  dy = y2 - y1;
  tlen = Math.pow(dx*dx + dy*dy, 0.5);
  theta = Math.atan2(dy,dx);
  mulx = Math.cos(theta);
  muly = Math.sin(theta);
  lx = [];
  ly = [];
  for (i = 0; i < pattern; ++i) {
    lx.push(pattern[i] * mulx);
    ly.push(pattern[i] * muly);
  }
  i = start;
  x = x1;
  y = y1;
  len = 0;
  ctx.beginPath();
  while (len + pattern[i] < tlen) {
    ctx.moveTo(x, y);
    x += lx[i];
    y += ly[i];
    ctx.lineTo(x, y);
    len += pattern[i];
    i = (i + 1) % pattern.length;
    x += lx[i];
    y += ly[i];
    len += pattern[i];
    i = (i + 1) % pattern.length;
  }
  if (len < tlen) {
    ctx.moveTo(x, y);
    x += mulx * (tlen - len);
    y += muly * (tlen - len);
    ctx.lineTo(x, y);
  }
  ctx.stroke();
}

function draw_trim_background(ctx, metrics, pspm, offset) {
  "use strict";
  var lwidth, rwidth, mwidth, rstart;
  lwidth = metrics.stack_width * pspm.get_left_trim();
  rwidth = metrics.stack_width * pspm.get_right_trim();
  mwidth = metrics.stack_width * pspm.get_motif_length();
  rstart = mwidth - rwidth;
  ctx.save();//s8
  ctx.translate(offset * metrics.stack_width, 0);
  ctx.fillStyle = "rgb(240, 240, 240)";
  if (pspm.get_left_trim() > 0) {
    ctx.fillRect(0, 0, lwidth, metrics.stack_height);
  }
  if (pspm.get_right_trim() > 0) {
    ctx.fillRect(rstart, 0, rwidth, metrics.stack_height);
  }
  ctx.fillStyle = "rgb(51, 51, 51)";
  if (pspm.get_left_trim() > 0) {
    draw_dashed_line(ctx, [3], 0, lwidth-0.5, 0, lwidth-0.5,  metrics.stack_height);
  }
  if (pspm.get_right_trim() > 0) {
    draw_dashed_line(ctx, [3], 0, rstart+0.5, 0, rstart+0.5,  metrics.stack_height);
  }
  ctx.restore();//s8
}

function size_logo_on_canvas(logo, canvas, show_names, scale) {
  "use strict";
  var draw_name, metrics;
  draw_name = (typeof show_names === "boolean" ? show_names : (logo.get_rows() > 1));
  if (canvas.width !== 0 && canvas.height !== 0) {
    return;
  }
  metrics = new LogoMetrics(canvas.getContext('2d'), 
      logo.get_columns(), logo.get_rows(), draw_name);
  if (typeof scale == "number") {
    //resize the canvas to fit the scaled logo
    canvas.width = metrics.summed_width * scale;
    canvas.height = metrics.summed_height * scale;
  } else {
    if (canvas.width === 0 && canvas.height === 0) {
      canvas.width = metrics.summed_width;
      canvas.height = metrics.summed_height;
    } else if (canvas.width === 0) {
      canvas.width = metrics.summed_width * (canvas.height / metrics.summed_height);
    } else if (canvas.height === 0) {
      canvas.height = metrics.summed_height * (canvas.width / metrics.summed_width);
    }
  }
}

function draw_logo_on_canvas(logo, canvas, show_names, scale) {
  "use strict";
  var draw_name, ctx, metrics, raster, pspm_i, pspm, 
      offset, col_index, motif_position;
  draw_name = (typeof show_names === "boolean" ? show_names : (logo.get_rows() > 1));
  ctx = canvas.getContext('2d');
  //assume that the user wants the canvas scaled equally so calculate what the best width for this image should be
  metrics = new LogoMetrics(ctx, logo.get_columns(), logo.get_rows(), draw_name);
  if (typeof scale == "number") {
    //resize the canvas to fit the scaled logo
    canvas.width = metrics.summed_width * scale;
    canvas.height = metrics.summed_height * scale;
  } else {
    if (canvas.width === 0 && canvas.height === 0) {
      scale = 1;
      canvas.width = metrics.summed_width;
      canvas.height = metrics.summed_height;
    } else if (canvas.width === 0) {
      scale = canvas.height / metrics.summed_height;
      canvas.width = metrics.summed_width * scale;
    } else if (canvas.height === 0) {
      scale = canvas.width / metrics.summed_width;
      canvas.height = metrics.summed_height * scale;
    } else {
      scale = Math.min(canvas.width / metrics.summed_width, canvas.height / metrics.summed_height);
    }
  }
  // cache the raster based on the assumption that we will be drawing a lot
  // of logos the same size
  if (typeof draw_logo_on_canvas.raster_scale === "number" && 
      Math.abs(draw_logo_on_canvas.raster_scale - scale) < 0.1) {
    raster = draw_logo_on_canvas.raster_cache;
  } else {
    raster = new RasterizedAlphabet(logo.alphabet, metrics.stack_font, metrics.stack_width * scale * 2);
    draw_logo_on_canvas.raster_cache = raster;
    draw_logo_on_canvas.raster_scale = scale;
  }
  ctx = canvas.getContext('2d');
  ctx.save();//s1
  ctx.scale(scale, scale);
  ctx.save();//s2
  ctx.save();//s7
  //create margin
  ctx.translate(metrics.pad_left, metrics.pad_top);
  for (pspm_i = 0; pspm_i < logo.get_rows(); ++pspm_i) {
    pspm = logo.get_pspm(pspm_i);
    offset = logo.get_offset(pspm_i);
    //optionally draw name if this isn't the last row or is the only row 
    if (draw_name && (logo.get_rows() == 1 || pspm_i != (logo.get_rows()-1))) {
      ctx.save();//s4
      ctx.translate(metrics.summed_width/2, metrics.name_height);
      ctx.font = metrics.name_font;
      ctx.textAlign = "center";
      ctx.fillText(pspm.name, 0, 0);
      ctx.restore();//s4
      ctx.translate(0, metrics.name_height + 
          Math.min(0, metrics.name_spacer - metrics.y_num_height/2));
    }
    //draw scale
    draw_scale(ctx, metrics, logo.alphabet.get_ic());
    ctx.save();//s5
    //translate across past the scale
    ctx.translate(metrics.y_label_height + metrics.y_label_spacer + 
        metrics.y_num_width + metrics.y_tic_width, 0);
    //draw the trimming background
    if (pspm.get_left_trim() > 0 || pspm.get_right_trim() > 0) {
      draw_trim_background(ctx, metrics, pspm, offset);
    }
    //draw letters
    ctx.translate(0, metrics.y_num_height / 2);
    for (col_index = 0; col_index < logo.get_columns(); col_index++) {
      ctx.translate(metrics.stack_pad_left,0);
      if (col_index >= offset && col_index < (offset + pspm.get_motif_length())) {
        motif_position = col_index - offset;
        draw_stack_num(ctx, metrics, motif_position);
        draw_stack(ctx, metrics, pspm.get_stack(motif_position, logo.alphabet), raster);
      }
      ctx.translate(metrics.stack_width, 0);
    }
    ctx.restore();//s5
    ////optionally draw name if this is the last row but isn't the only row 
    if (draw_name && (logo.get_rows() != 1 && pspm_i == (logo.get_rows()-1))) {
      //translate vertically past the stack and axis's        
      ctx.translate(0, metrics.y_num_height/2 + metrics.stack_height + 
          Math.max(metrics.y_num_height/2, metrics.x_num_above + metrics.x_num_width + metrics.name_spacer));

      ctx.save();//s6
      ctx.translate(metrics.summed_width/2, metrics.name_height);
      ctx.font = metrics.name_font;
      ctx.textAlign = "center";
      ctx.fillText(pspm.name, 0, 0);
      ctx.restore();//s6
      ctx.translate(0, metrics.name_height);
    } else {
      //translate vertically past the stack and axis's        
      ctx.translate(0, metrics.y_num_height/2 + metrics.stack_height + 
          Math.max(metrics.y_num_height/2, metrics.x_num_above + metrics.x_num_width));
    }
    //if not the last row then add middle padding
    if (pspm_i != (logo.get_rows() -1)) {
      ctx.translate(0, metrics.pad_middle);
    }
  }
  ctx.restore();//s7
  ctx.translate(metrics.summed_width - metrics.pad_right, metrics.summed_height - metrics.pad_bottom);
  ctx.font = metrics.fine_txt_font;
  ctx.textAlign = "right";
  ctx.fillText(logo.fine_text, 0,0);
  ctx.restore();//s2
  ctx.restore();//s1
}

function create_canvas(c_width, c_height, c_id, c_title, c_display) {
  "use strict";
  var canvas = document.createElement("canvas");
  //check for canvas support before attempting anything
  if (!canvas.getContext) {
    return null;
  }
  var ctx = canvas.getContext('2d');
  //check for html5 text drawing support
  if (!supports_text(ctx)) {
    return null;
  }
  //size the canvas
  canvas.width = c_width;
  canvas.height = c_height;
  canvas.id = c_id;
  canvas.title = c_title;
  canvas.style.display = c_display;
  return canvas;
}

function logo_1(alphabet, fine_text, pspm) {
  "use strict";
  var logo = new Logo(alphabet, fine_text);
  logo.add_pspm(pspm);
  return logo;
}

function logo_2(alphabet, fine_text, target, query, query_offset) {
  "use strict";
  var logo = new Logo(alphabet, fine_text);
  if (query_offset < 0) {
    logo.add_pspm(target, -query_offset);
    logo.add_pspm(query);
  } else {
    logo.add_pspm(target);
    logo.add_pspm(query, query_offset);
  }      
  return logo;
}

/*
 * Specifies an alternate source for an image.
 * If the image with the image_id specified has
 * not loaded then a generated logo will be used 
 * to replace it.
 *
 * Note that the image must either have dimensions
 * or a scale must be set.
 */
function alternate_logo(logo, image_id, scale) {
  "use strict";
  var image = document.getElementById(image_id);
  if (!image) {
    alert("Can't find specified image id (" +  image_id + ")");
    return;
  }
  //if the image has loaded then there is no reason to use the canvas
  if (image_ok(image)) {
    return;
  }
  //the image has failed to load so replace it with a canvas if we can.
  var canvas = create_canvas(image.width, image.height, image_id, image.title, image.style.display);
  if (canvas === null) {
    return;
  }
  //draw the logo on the canvas
  draw_logo_on_canvas(logo, canvas, null, scale);
  //replace the image with the canvas
  image.parentNode.replaceChild(canvas, image);
}

/*
 * Specifes that the element with the specified id
 * should be replaced with a generated logo.
 */
function replace_logo(logo, replace_id, scale, title_txt, display_style) {
  "use strict";
  var element = document.getElementById(replace_id);
  if (!replace_id) {
    alert("Can't find specified id (" + replace_id + ")");
    return;
  }
  //found the element!
  var canvas = create_canvas(50, 120, replace_id, title_txt, display_style);
  if (canvas === null) {
    return;
  }
  //draw the logo on the canvas
  draw_logo_on_canvas(logo, canvas, null, scale);
  //replace the element with the canvas
  element.parentNode.replaceChild(canvas, element);
}

/*
 * Fast string trimming implementation found at
 * http://blog.stevenlevithan.com/archives/faster-trim-javascript
 *
 * Note that regex is good at removing leading space but
 * bad at removing trailing space as it has to first go through
 * the whole string.
 */
function trim (str) {
  "use strict";
  var ws, i;
  str = str.replace(/^\s\s*/, '');
  ws = /\s/; i = str.length;
  while (ws.test(str.charAt(--i)));
  return str.slice(0, i + 1);
}
