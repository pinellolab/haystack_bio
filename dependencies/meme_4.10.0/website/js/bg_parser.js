
var BgCharType = {
  JUNK:         1<<0, 
  WHITESPACE:   1<<1, 
  DNA:          1<<2, 
  PROTEIN:      1<<3, 
  NUMBER_START: 1<<4,
  NUMBER_MID:   1<<5,
  COMMENT:      1<<6
};

var BgErrorType = {
  FORMAT: 1,
  ENCODING: 2,
  ALPHABET_MISMATCH: 3,
  NUMBER_BEFORE_CHAIN: 4,
  JUNK: 5,
  NO_GAP: 6,
  DUPLICATE: 7,
  MISSING_CHAIN: 8,
  INCORRECT_SUM: 9,
  BAD_PROBABILITY: 10,
  MISSING_PROBABILITY: 11
};

var BgAbortError = function(message) {
  "use strict";
  this.message = message;
  this.stack = Error().stack;
};
BgAbortError.prototype = Object.create(Error.prototype);
BgAbortError.prototype.name = "BgAbortError";
BgAbortError.prototype.constructor = BgAbortError;

var BgParserUtil = {};

BgParserUtil.add_to_map = function (map, letters, flags) {
  "use strict";
  var i, lc_letters;
  lc_letters = letters.toLowerCase();
  for (i = 0; i < letters.length; i++) {
    map[letters.charCodeAt(i)] = flags;
    if (letters[i] != lc_letters[i]) {
      map[lc_letters.charCodeAt(i)] = flags;
    }
  }
};

BgParserUtil.make_type_map = function () {
  "use strict";
  var i, map;
  map = new Int16Array(128);
  for (i = 0; i < map.length; i++) map[i] = BgCharType.JUNK;
  BgParserUtil.add_to_map(map, " \t\f\n\r", BgCharType.WHITESPACE);
  BgParserUtil.add_to_map(map, "ACGT", BgCharType.DNA | BgCharType.PROTEIN);
  BgParserUtil.add_to_map(map, "DFHIKLMNPQRSVWY", BgCharType.PROTEIN);
  BgParserUtil.add_to_map(map, "0123456789", BgCharType.NUMBER_START | BgCharType.NUMBER_MID);
  BgParserUtil.add_to_map(map, "-+.", BgCharType.NUMBER_MID);
  BgParserUtil.add_to_map(map, "E", BgCharType.NUMBER_MID | BgCharType.PROTEIN);
  BgParserUtil.add_to_map(map, "#", BgCharType.COMMENT);
  return map;
};

BgParserUtil.make_alph_map = function (alphabet) {
  "use strict";
  var i, map, lc_alphabet;
  lc_alphabet = alphabet.toLowerCase();
  map = new Int16Array(128);
  for (i = 0; i < map.length; i++) map[i] = 0;
  for (i = 0; i < alphabet.length; i++) {
    map[alphabet.charCodeAt(i)] = i + 1;
    map[lc_alphabet.charCodeAt(i)] = i + 1;
  }
  return map;
};

BgParserUtil.TYPE_MAP = BgParserUtil.make_type_map();
BgParserUtil.DNA_MAP = BgParserUtil.make_alph_map("ACGT");
BgParserUtil.PROTEIN_MAP = BgParserUtil.make_alph_map("ACDEFGHIKLMNPQRSTVWY");


var BgParser = function (handler) {
  "use strict";
  var i;
  this.handler = handler;
  this.give_up = false;
  this.alphabet = AlphType.UNKNOWN;
  this.decoder = new LineDecoder();
  this.process = this._process_start;
  this.chain_type = BgCharType.DNA | BgCharType.PROTEIN;
  this.chain = "";
  this.chain_index = 0;
  this.chain_len = 1; // expected length of chains
  this.probs_check_start = 0;
  this.prob = "";
  // initially record as if protein, will re-adjust if it seems to be DNA
  this.alen = 20;
  this.amap = BgParserUtil.PROTEIN_MAP;
  this.probs = new Float32Array(20);
  for (i = 0; i < this.probs.length; i++) this.probs[i] = -1;
};

BgParser.prototype._signal_stop = function () {
  if (typeof this.handler.progress == "function") this.handler.progress(1.0, this.alphabet);
  if (typeof this.handler.end == "function") this.handler.end(this.alphabet);
};

BgParser.prototype._error = function (offset, line, column, code, message, data) {
  if (typeof this.handler.error == "function") {
    this.handler.error(offset, line, column, code, message, data);
  }
};

BgParser.prototype._error_here = function (code, message, data) {
  this._error(this.decoder.offset(), this.decoder.line(), this.decoder.column(),
      code, message, data);
};

BgParser.prototype._process_start = function (code, type) {
  "use strict";
  var error_code, error_message;
  // dispatch to the correct mode
  if ((type & BgCharType.COMMENT) != 0) {
    //nothing in line yet so we can just read the comment
    this.process = this._process_comment;
    return true;
  } else if ((type & BgCharType.WHITESPACE) != 0) {
    // whitespace at the start of the line can be safely ignored
    return true;
  } else if ((type & this.chain_type) != 0) {
    this.process = this._process_chain;
    this.chain = "";
    this.chain_index = 0;
    return false;
  } else {
    // report error and treat as comment
    if ((type & (BgCharType.DNA | BgCharType.PROTEIN)) != 0) { // not in alphabet
      this._error_here(BgErrorType.ALPHABET_MISMATCH,
          "Found a letter which does not match the previously established alphabet.",
          {"letter": String.fromCharCode(code)});
    } else if ((type & (BgCharType.NUMBER_START | BgCharType.NUMBER_MID)) != 0) { // number in wrong place
      this._error_here(BgErrorType.NUMBER_BEFORE_CHAIN, 
          "Found a number before the letter chain was established.", {});
    } else { // junk
      this._error_here(BgErrorType.JUNK, 
          "Found a character that was not part of a state chain", {});
    }
    // skip until new line
    this.process = this._process_comment;
    return true;
  }
}

BgParser.prototype._process_comment = function (code, type) {
  // Blindly accept characters until we get to a new line
  if (this.decoder.column() == 0) {
    this.process = this._process_start;
    return false;
  }
  return true;
};

BgParser.prototype._determine_alphabet = function () {
  "use strict";
  var is_dna, temp_probs, i;
  if (this.chain_len == 1) {
    // check to see if any protein-only letters were used
    is_dna = true;
    for (i = 0; i < 16; i++) {
      if (this.probs[BgParserUtil.PROTEIN_MAP["DEFHIKLMNPQRSVWY".charCodeAt(i)]-1] != -1) {
        is_dna = false;
        break;
      }
    }
    if (is_dna) {
      this.chain_type = BgCharType.DNA;
      this.amap = BgParserUtil.DNA_MAP;
      this.alen = 4;
      // recalculate the chain index for the DNA alphabet
      this.chain_index = this.amap[this.chain.charCodeAt(0)] * this.alen + this.amap[this.chain.charCodeAt(1)];
      // move the probabilities for ACGT to their normal position
      temp_probs = new Float32Array(4);
      for (i = 0; i < 4; i++) temp_probs[i] = this.probs[BgParserUtil.PROTEIN_MAP["ACGT".charCodeAt(i)]-1];
      this.probs = temp_probs;
      this.alphabet = AlphType.DNA;
    } else {
      this.chain_type = BgCharType.PROTEIN;
      this.alphabet = AlphType.PROTEIN;
    }
  }
};

BgParser.prototype._check_probabilities = function (min_index) {
  "use strict";
  var sum, i, missing_list;
  sum = 0;
  missing_list = [];
  for (i = min_index; i < this.probs.length; i++) {
    if (this.probs[i] == -1) {
      missing_list.push(i);
      continue;
    }
    sum += this.probs[i];
  }
  if (missing_list.length > 0) {
    this._error_here(BgErrorType.MISSING_CHAIN,
        "Missing " + missing_list.length + " chain value(s).", {missing: missing_list});
    if (missing_list.length == (this.probs.length - min_index)) {
      throw new BgAbortError("Missing all entries for a chain length");
    }
  } else {
    // check that the sum of all the states is 1
    if (Math.abs(sum - 1.0) > 0.1) {
      this._error_here(BgErrorType.INCORRECT_SUM,
          "Summed probability of all length " + this.chain_len +
          " chains was " + sum + " when it should have been 1.0",
          {chain_length: this.chain_len, total: sum});
    }
  }
};

BgParser.prototype._process_chain = function (code, type) {
  "use strict";
  var temp_probs, i;
  if ((type & BgCharType.WHITESPACE) != 0) {
    if (this.probs[this.chain_index - 1] != -1) {
      this._error_here(BgErrorType.DUPLICATE,
          "Found a duplicated letter chain " +
          this.chain + " = " + this.probs[this.chain_index - 1], {});
      // continue anyway
    }
    this.process = this._process_gap;
    return true;
  } else if ((type & this.chain_type) != 0) {
    // extend the chain
    this.chain += String.fromCharCode(code);
    this.chain_index = this.chain_index * this.alen + this.amap[code];
    // check chain length is within expected
    if (this.chain.length > this.chain_len) {
      // looks like we're using longer chains (higher level markov model)
      this._determine_alphabet();
      // so check we actually set all values for the states
      this._check_probabilities(this.probs_check_start);
      // now extend the probs array to store the next larger chain length
      this.probs_check_start = this.probs.length;
      this.chain_len++;
      temp_probs = new Float32Array(this.probs.length + Math.pow(this.alen, this.chain_len));
      temp_probs.set(this.probs);
      for (i = this.probs_check_start; i < temp_probs.length; i++) temp_probs[i] = -1;
      this.probs = temp_probs;
    }
    return true;
  } else {
    //error
    if ((type & (BgCharType.DNA | BgCharType.PROTEIN)) != 0) {
      this._error_here(BgErrorType.ALPHABET_MISMATCH,
          "Found a letter which does not match the previously established alphabet.",
          {"letter": String.fromCharCode(code)});
    } else if ((type & BgCharType.NUMBER_START) != 0) {
      this._error_here(BgErrorType.NO_GAP, 
          "There is no gap between the letter chain and the probability.", {});
      this.process = this._process_number;
      this.prob = "";
      return false;
    } else {
      this._error_here(BgErrorType.JUNK, 
          "Found a character that was not part of a state chain", {});
    }
    this.process = this._process_comment;
    return true;
  }
};

BgParser.prototype._process_gap = function (code, type) {
  "use strict";
  if (this.decoder.column() == 0) {
    this._error_here(BgErrorType.MISSING_PROBABILITY,
        "Did not find a probability to go with the chain", {});
    this.process = this._process_start;
    return false;
  } else if ((type & BgCharType.WHITESPACE) != 0) {
    return true;
  } else if ((type & BgCharType.NUMBER_START) != 0) {
    this.process = this._process_number;
    this.prob = "";
    return false;
  } else {
    // error
    this._error_here(BgErrorType.JUNK, 
        "Found a character that was not the start of a number", {});
    this.process = this._process_comment;
    return true;
  }
};

BgParser.prototype._store_number = function() {
  "use strict";
  var prob;
  if (!/^(?:0(?:\.[0-9]+)?|[1-9][0-9]*(?:\.[0-9]+)?|\.[0-9]+)(?:[eE][+-]?[0-9]+)?$/.test(this.prob)) {
    this._error_here(BgErrorType.BAD_PROBABILITY, "Number does not match expected format.", {});
    prob = 0;
  } else {
    prob = +this.prob;
    if (prob <= 0 || prob >= 1) {
      this._error_here(BgErrorType.BAD_PROBABILITY, "Number is not in expected range 0 < p < 1", {});
      prob = 0;
    }
  }
  this.probs[this.chain_index -1] = prob;
  this.process = this._process_whitespace;
};

BgParser.prototype._process_number = function (code, type) {
  "use strict";
  var prob;
  if ((type & (BgCharType.NUMBER_START | BgCharType.NUMBER_MID)) != 0) {
    this.prob += String.fromCharCode(code);
    return true;
  } else if ((type & BgCharType.WHITESPACE) != 0) {
    this._store_number();
    return false;
  } else {
    this._error_here(BgErrorType.JUNK, "Expected a number.", {});
    this.process = this._process_comment;
    return true;
  }
};

BgParser.prototype._process_whitespace = function (code, type) {
  "use strict";
  if (this.decoder.column() == 0) {
    this.process = this._process_start;
    return false;
  } else if ((type & BgCharType.WHITESPACE) != 0) {
    return true;
  } else {
    this.error_here(BgErrorType.JUNK, "Expected whitespace", {});
    this.process = this._process_comment;
  }
};

BgParser.prototype.process_blob = function (blob, offset, chunk_size) {
  "use strict";
  var reader, me;
  if (this.give_up) return;
  // set default parameter values
  if (typeof offset === "undefined") offset = 0;
  if (typeof chunk_size === "undefined") chunk_size = 1 << 12;
  // update the progress
  if (offset === 0 && typeof this.handler.begin == "function")
    this.handler.begin(blob.size);
  if (typeof this.handler.progress == "function")
    this.handler.progress(offset / blob.size, this.alphabet);
  // setup the reader
  me = this; // so we can access 'this' inside the closure
  reader = new FileReader();
  reader.onload = function(evt) {
    "use strict";
    var chunk, format, code, type, consumed, i;
    if (me.give_up) return;
    // process the loaded chunk
    chunk = new Uint8Array(reader.result);
    if (offset == 0) {
      if ((format = unusable_format(chunk, 40, blob.name)) != null) {
        me._error(0, 0, 0, BgErrorType.FORMAT,
            "The file format is not correct for a background file",
            {"format_type": format.type, "format_name": format.name});
        me._signal_stop();
        return;
      }
    }
    try {
      me.decoder.set_source(chunk, (offset + chunk_size) >= blob.size);
      while ((code = me.decoder.next()) != null) {
        type = (code < 128 ? BgParserUtil.TYPE_MAP[code] : BgCharType.JUNK);
        for (i = 0; i < 20; i++) {
          consumed = me.process(code, type);
          if (consumed) break;
        }
        if (i == 20) throw new Error("Infinite loop protection activated!");
      }
    } catch (e) {
      if (e instanceof EncodingError) {
        // report error and stop scan as something is wrong with the encoding
        me._error(e.offset, this.decoder.line(), this.decoder.column(),
            BgErrorType.ENCODING, e.message, {enc_code: e.code});
        me._signal_stop();
        return;
      } else if (e instanceof BgAbortError) {
        me._signal_stop();
        return;
      } else {
        throw e;
      }
    }
    if ((offset + chunk_size) >= blob.size) {
      if (me.process == me._process_chain) {
        if (me.probs[me.chain_index - 1] != -1) {
          me._error_here(BgErrorType.DUPLICATE,
              "Found a duplicated letter chain " +
              me.chain + " = " + me.probs[me.chain_index - 1], {});
        }
        me._error_here(BgErrorType.MISSING_PROBABILITY,
            "Did not find a probability to go with the chain.", {});
      } else if (me.process == me._process_number) {
        me._store_number();
      }
      try {
        me._determine_alphabet();
        me._check_probabilities(me.probs_check_start);
      } catch (e) {
        if (!(e instanceof BgAbortError)) {
          throw e;
        }
      }
      me._signal_stop();
    } else {
      // start loading the next chunk
      me.process_blob(blob, offset + chunk_size, chunk_size);
    }
  };
  // read the next chunk
  reader.readAsArrayBuffer(blob.slice(offset, offset + chunk_size));
};

BgParser.prototype.cancel = function () {
  this.give_up = true;
  this.handler = {};
};

var BgHandler = function(options) {
  this.configure(options);
  this.reset();
};

BgHandler.prototype.configure = function (options) {
  "use strict";
  var alphabets, alphabet_name;
  if (typeof options != "object" || options == null) options = {};
  // configure file size
  if (typeof options.file_max == "number") {
    this.max_file_size = options.file_max;
  } else {
    this.max_file_size = 1 << 20;
  }
  // configure alphabets
  if ((alphabets = options.alphabets) != null) {
    this.allow_alphabets = 0;
    for (alphabet_name in alphabets) {
      if (!alphabets.hasOwnProperty(alphabet_name)) continue;
      this.allow_alphabets |= AlphUtil.from_name(alphabet_name);
    }
  } else {
    this.allow_alphabets = AlphType.DNA | AlphType.PROTEIN;
  }

};

BgHandler.prototype.reset = function () {
  // have the file details changed?
  this.updated = false;
  // the part of the file processed
  this.fraction = 0;
  // bg details
  this.file_size = 0;
  this.alphabet = AlphType.UNKNOWN;
  // these indicate when not UTF-8
  this.unusable_format_type = 0;
  this.unusable_format_name = null;
  this.encoding_error = null;
  // problems
  this.syntax_errors = new FileFaults();
  this.mismatches = new FileFaults();
  this.duplicates = new FileFaults();
  this.missing_chains = [];
  this.bad_sums = [];
};

BgHandler.prototype.summary = function () {
  var error, warning, messages, reason, reasons, letters, add, chain2str;
  var help;
  // setup
  error = false;
  warning = false;
  messages = [];
  // create closure to add messages
  add = function(is_error, message, reasons) {
    "use strict";
    messages.push({"is_error": is_error, "message": message, "reasons": reasons});
    if (is_error) error = true;
    else warning = true;
  };
  chain2str = function(alphabet, chain) {
    "use strict";
    var str, index;
    str = "";
    chain += 1;
    while (chain > 0) {
      index = chain % (alphabet.length + 1);
      str += alphabet.charAt(index - 1);
      chain = (chain - index) / alphabet.length;
    }
    return str;
  };
  // file size warning
  if (this.max_file_size != null && this.file_size > this.max_file_size) {
    add(false, "Large file", ["File is " + (Math.round(this.file_size / (1<<20) )) + "MB"])
  }
  // encoding or format error
  help = " - re-save as plain text; either Unicode UTF-8 (no Byte Order Mark) or ASCII";
  if (this.unusable_format_name != null) {
    switch (this.unusable_format_type) {
      case FileType.ENCODING:
        add(true, "Bad encoding \"" + this.unusable_format_name + "\"" + help);
        break;
      case FileType.BINARY:
        add(true, "Bad format \"" + this.unusable_format_name + "\"" + help);
        break;
      case FileType.COMPRESSED:
        add(true, "Bad format \"" + this.unusable_format_name + "\" - must be decompressed first");
        break;
    }
    return {"error": error, "warning": warning, "messages": messages};
  } else if (this.encoding_error != null) {
    // report anything else that indicates it's not UTF-8
    add(true, "Bad encoding" + help, [this.encoding_error]);
    return {"error": error, "warning": warning, "messages": messages};
  }
  // syntax errors
  if (this.syntax_errors.faults() > 0) {
    reason = "Incorrect syntax on " +
      (this.syntax_errors.line_count == 1 ? "line " : "lines ") + this.syntax_errors.lines_str();
    add(true, "Syntax errors", [reason]);
  }
  // report non-alpha characters
  if (this.mismatches.faults() > 0) {
    letters = this.mismatches.letters_sep();
    if (this.mismatches.faults() == 1) {
      reason = "Found a character " + (letters.length > 0 ? letters + " " : "") +
        "not in the detected " + AlphUtil.display(this.alphabet) + " alphabet on ";
    } else {
      reason = "Found " + this.mismatches.faults() + " characters " + 
        (letters.length > 0 ? "(including " + letters + ") " : "") + 
        "not in the detected " + AlphUtil.display(this.alphabet) + " alphabet on ";
    }
    reason += (this.mismatches.line_count == 1 ? "line " : "lines ") +
      this.mismatches.lines_str();
    add(true, "Alphabet is inconsistant", [reason]);
  }
  // report duplicated chains
  if (this.duplicates.faults() > 0) {
    if (duplicates.faults() == 1) {
      reason = "Found a duplicated entry on line " + this.duplicates.lines_str();
    } else {
      reason = "Found " + this.duplicates.line_count +
        " duplicated entries on lines " + this.duplicates.lines_str();
    }
    add(true, "Duplicated entries", [reason]);
  }
  // report missing chains
  if (this.missing_chains.length > 0) {
    letters = AlphUtil.letters(this.alphabet);
    if (this.missing_chains.length == 1) {
      reason = "Did not find the entry for " + 
        chain2str(letters, this.missing_chains[0]);
    } else {
      reason = "Did not find " + this.missing_chains.length + " entries for ";
      var end = Math.min(10, this.missing_chains.length - 1);
      for (i = 0; i < end; i++) {
        if (i != 0) reason += ", ";
        reason += chain2str(letters, this.missing_chains[i]);
      }
      if (i == (this.missing_chains.length - 1)) {
        reason += " and " + chain2str(letters, this.missing_chains[i]);
      } else {
        reason += ", ...";
      }
    }
    add(true, "Missing entries", [reason]);
  }
  // report incorrect sums
  if (this.bad_sums.length > 0) {
    if (this.bad_sums.length == 1) {
      reason = "The probabilities did not sum to 1 for the entries of length " + this.bad_sums[0];
    } else {
      reason = "The probabilities did not sum to 1 for multiple entry lengths ";
      end = Math.min(10, this.bad_sums.length - 1);
      for (i = 0; i < end; i++) {
        if (i != 0) reason += ", ";
        reason += this.bad_sums[i];
      }
      if (i == (this.bad_sums.length - 1)) {
        reason += " and " + this.bad_sums[i];
      } else {
        reason += ", ...";
      }
    }
    add(true, "Probabilities should sum to 1", [reason]);
  }
  if (this.alphabet != AlphType.UNKNOWN && ((this.alphabet & this.allow_alphabets) == 0)) {
    reason = "The background was " + AlphUtil.display(this.alphabet) + " but " 
      + AlphUtil.display(this.allow_alphabets) + " was expected";
    add(true, "Background is wrong alphabet", [reason])
  }

  return {"error": error, "warning": warning, 
    "alphabet": this.alphabet, "messages": messages};
};

BgHandler.prototype.begin = function (file_size) {
  this.reset();
  this.file_size = file_size;
  this.updated = true;
};

BgHandler.prototype.end = function (alphabet) {
  this.alphabet = alphabet;
  this.updated = true;
};

BgHandler.prototype.progress = function (fraction, alphabet) {
  this.fraction = fraction;
  if (this.alphabet != alphabet) {
    this.alphabet = alphabet;
    this.updated = true;
  }
};

BgHandler.prototype.error = function (offset, line, column, code, message, data) {
  switch (code) {
    case BgErrorType.FORMAT:
      this.unusable_format_type = data["format_type"];
      this.unusable_format_name = data["format_name"];
      break;
    case BgErrorType.ENCODING:
      this.encoding_error = data["enc_code"];
      break;
    case BgErrorType.NUMBER_BEFORE_CHAIN:
    case BgErrorType.JUNK:
    case BgErrorType.NO_GAP:
    case BgErrorType.BAD_PROBABILITY:
    case BgErrorType.MISSING_PROBABILITY:
      this.syntax_errors.add(line);
      break;
    case BgErrorType.ALPHABET_MISMATCH:
      this.mismatches.add(line, data["letter"]);
      break;
    case BgErrorType.DUPLICATE:
      this.duplicates.add(line);
      break;
    case BgErrorType.MISSING_CHAIN:
      this.missing_chains.push.apply(this.missing_chains, data["missing"]);
      break;
    case BgErrorType.INCORRECT_SUM:
      this.bad_sums.push(data["chain_length"]);
      break;
  }
  this.updated = true;
};

