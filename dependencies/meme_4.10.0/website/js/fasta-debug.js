
//******************************************************************************
// Fasta Checker
//******************************************************************************
var FastaChecker = function (handler) {
  "use strict";
  var a, dna_only, protein_only;
  // store a reference to the handler
  this.handler = handler;
  // check if only one alphabet is allowed (count RNA & DNA as one)
  if (typeof this.handler.allow_alphabets == "number") {
    a = this.handler.allow_alphabets;
    dna_only = (a == AlphType.DNA || a == AlphType.RNA || a == (AlphType.DNA | AlphType.RNA));
    protein_only = (a == AlphType.PROTEIN);
  } else {
    dna_only = false; protein_only = false;
  }
  // enable the MEME extension "WEIGHTS"
  this.enable_weights = (typeof this.handler.enable_weights == "boolean" ? this.handler.enable_weights : false);
  // allow sequence characters which are a wildcard and are commonly used for masking
  this.allow_sequence_masking = (typeof this.allow_sequence_masking == "boolean" ? this.allow_sequence_masking : true);
  // allow ambiguous sequence characters
  this.allow_ambigs = (typeof this.handler.allow_ambigs == "boolean" ? this.handler.allow_ambigs : true);
  if (this.allow_ambigs) this.allow_sequence_masking = true;
  // allow characters which indicate a gap inserted for the purpose of alignment
  this.allow_gaps = (typeof this.handler.allow_gaps == "boolean" ? this.handler.allow_gaps : false);
  // only allow the sequence to be represented with uppercase letters
  this.only_uppercase = (typeof this.handler.only_uppercase == "boolean" ? this.handler.only_uppercase : false);
  // setup a charCode mapping for the allowed characters in the sequence
  this.alpha_map = FastaChecker.make_alpha_map(dna_only, protein_only);
  // allow_sequence_masking the allowed bit-set for alphabet characters
  this.alpha_bitmask = (this.allow_sequence_masking ? 0 : FastaChecker.FASTA_MASK) |
      (this.allow_ambigs ? 0 : FastaChecker.FASTA_AMBIG) | 
      (this.allow_gaps ? 0 : FastaChecker.FASTA_GAP) | 
      (this.only_uppercase ? FastaChecker.FASTA_LC : 0);
  this.letter_map = FastaChecker.make_letter_map();
  this.letter_counts = [0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0];
  // UTF-8 reader
  this.decoder = new LineDecoder();
  // current parsing function
  this.process = this._process_start;
  // other stuff
  this.seq_byte_offset = 0;
  this.seq_line = 0;
  this.seq_name = "";
  this.seq_desc_len = 0;
  this.seq_offset = 0;
  this.after_comment = null;
  this.weight = "";
  this.weight_byte_offset = 0;
  this.weight_column = 0;
  this.weight_count = 0;
  // abort flag
  this.give_up = false;
};

//******************************************************************************
// Static variables used by the FastaChecker object
//******************************************************************************
FastaChecker.FASTA_UNDEFINED = 0; // Undefined or unset
FastaChecker.FASTA_NON_ALPHA = 1; // A character not in the alphabet and not whitespace
FastaChecker.FASTA_ALPHA = 2; // A character that is in the current alphabet
FastaChecker.FASTA_NUL = 3; // A character that should never appear (eg NUL)
// the following 4 can be combined (OR-ed) with FASTA_ALPHA
FastaChecker.FASTA_LC = 4; // In the alphabet but lowercase
FastaChecker.FASTA_MASK = 8; // Ambiguous catch all character, used for masking.
FastaChecker.FASTA_AMBIG = 16; // In the alphabet but ambiguous
FastaChecker.FASTA_GAP = 32; // In the alphabet but gap, not valid with UC or AMBIG
// other characters
FastaChecker.FASTA_CHEVRON = 64; // NB a cheveron ('>') is also a FASTA_NON_ALPHA
FastaChecker.FASTA_COMMENT = 128; // NB a comment (';') is also a FASTA_NON_ALPHA
FastaChecker.FASTA_WHITESPACE = 256;
FastaChecker.FASTA_NEWLINE = 512;

// constants representing encoding errors
FastaChecker.ENCODE_MISSING = 1; // missing an expected continuation character
FastaChecker.ENCODE_NOT_COMPACT = 2; // code point uses more bytes than required
FastaChecker.ENCODE_ILLEGAL = 3; // a byte of FE or FF
FastaChecker.ENCODE_NUL = 4; // a NUL strongly suggests that it's not UTF-8

//******************************************************************************
// Static functions used by the FastaChecker object
//******************************************************************************
FastaChecker.add_to_letter_map = function (map, letters, flags) {
  "use strict";
  var i, lc_letters;
  lc_letters = letters.toLowerCase();
  for (i = 0; i < letters.length; i++) {
    map[letters.charCodeAt(i)] = flags;
    if (letters[i] != lc_letters[i]) {
      map[lc_letters.charCodeAt(i)] = flags | FastaChecker.FASTA_LC;
    }
  }
};

FastaChecker.make_alpha_map = function (only_allow_dna, only_allow_protein) {
  "use strict";
  var i, alpha_map;
  alpha_map = new Int16Array(128);
  for (i = 0; i < alpha_map.length; i++) alpha_map[i] = FastaChecker.FASTA_NON_ALPHA;
  // mark some characters as illegal
  alpha_map[0x0] = FastaChecker.FASTA_NUL; // NUL should not appear in UTF-8 FASTA files
  if (only_allow_dna) {
    // allow U for RNA as that's how the MEME Suite does it.
    FastaChecker.add_to_letter_map(alpha_map, "ACGTU", FastaChecker.FASTA_ALPHA);
    FastaChecker.add_to_letter_map(alpha_map, "N", FastaChecker.FASTA_ALPHA | FastaChecker.FASTA_MASK);
    FastaChecker.add_to_letter_map(alpha_map, "RYKMSWBDHV", FastaChecker.FASTA_ALPHA | FastaChecker.FASTA_AMBIG);
  } else if (only_allow_protein) {
    //technically O and U are also in the protein alphabet, 
    //but they are unsupported by the MEME Suite.
    FastaChecker.add_to_letter_map(alpha_map, "ACDEFGHIKLMNPQRSTVWY", FastaChecker.FASTA_ALPHA);
    FastaChecker.add_to_letter_map(alpha_map, "X", FastaChecker.FASTA_ALPHA | FastaChecker.FASTA_MASK);
    FastaChecker.add_to_letter_map(alpha_map, "BZ", FastaChecker.FASTA_ALPHA | FastaChecker.FASTA_AMBIG);
  } else {
    // J is the only character not in the combined DNA + protein
    // the proteins O and U are not supported by the MEME Suite
    // but U stays because it's in RNA
    FastaChecker.add_to_letter_map(alpha_map, "ABCDEFGHIKLMNPQRSTUVWXYZ", FastaChecker.FASTA_ALPHA);
  }
  FastaChecker.add_to_letter_map(alpha_map, ".-", FastaChecker.FASTA_ALPHA | FastaChecker.FASTA_GAP);
  FastaChecker.add_to_letter_map(alpha_map, " \t\f", FastaChecker.FASTA_WHITESPACE);
  FastaChecker.add_to_letter_map(alpha_map, "\n\r", FastaChecker.FASTA_NEWLINE);
  alpha_map[">".charCodeAt(0)] = FastaChecker.FASTA_CHEVRON | FastaChecker.FASTA_NON_ALPHA;
  alpha_map[";".charCodeAt(0)] = FastaChecker.FASTA_COMMENT | FastaChecker.FASTA_NON_ALPHA;
  return alpha_map;
};

FastaChecker.make_letter_map = function () {
  "use strict";
  var i, start_lc, start_uc, index_map;
  index_map = new Int8Array(128);
  for (i = 0; i < index_map.length; i++) index_map[i] = 0;
  start_lc = "a".charCodeAt(0);
  start_uc = "A".charCodeAt(0);
  for (i = 0; i < 26; i++) {
    index_map[i + start_lc] = i+1;
    index_map[i + start_uc] = i+1;
  }
  return index_map;
};

//******************************************************************************
// Private functions
//******************************************************************************

// 
// Reports the sequence name, description length and sequence length to the handler.
//
FastaChecker.prototype._report_seq_info = function () {
  "use strict";
  if (typeof this.handler.info_seq == "function") {
    this.handler.info_seq(this.seq_byte_offset, this.seq_line, this.seq_name,
        this.seq_desc_len, this.seq_offset);
  }
};

FastaChecker.prototype._process_start = function (char_code, char_type) {
  "use strict";
  if (this.decoder.column() === 0 && ((char_type & FastaChecker.FASTA_CHEVRON) != 0)) {
    this.process = this._process_name;
    this.seq_name = "";
    this.seq_offset = 0;
    this.seq_byte_offset = this.decoder.offset();
    this.seq_line = this.decoder.line();
  } else if (this.decoder.column() === 0 && ((char_type & FastaChecker.FASTA_COMMENT) != 0)) {
    this.after_comment = this._process_start;
    this.process = this._process_comment;
    if (typeof this.handler.info_comment == "function") {
      this.handler.info_comment(this.decoder.offset(), this.decoder.line());
    }
  } else if ((char_type & (FastaChecker.FASTA_ALPHA | FastaChecker.FASTA_NON_ALPHA)) != 0) {
    if ((char_type & FastaChecker.FASTA_CHEVRON) != 0) {
      // possible misplaced sequence start
      if (typeof this.handler.warn_chevron == "function") {
        this.handler.warn_chevron(this.decoder.offset(), this.decoder.line(), this.decoder.column());
      }
    }
    if (typeof this.handler.warn_junk == "function") {
      this.handler.warn_junk(this.decoder.offset(), this.decoder.line(), this.decoder.column(), 
          char_type, String.fromCharCode(char_code));
    }
  }
};

FastaChecker.prototype._process_comment = function (char_code, char_type) {
  if ((char_type & FastaChecker.FASTA_NEWLINE) != 0) {
    this.process = this.after_comment;
  }
};

FastaChecker.prototype._process_name = function (char_code, char_type) {
  "use strict";
  if ((char_type & (FastaChecker.FASTA_WHITESPACE | FastaChecker.FASTA_NEWLINE)) != 0) {
    // end of the sequence name
    if (this.enable_weights && this.seq_name == "WEIGHTS") {
      if ((char_type & FastaChecker.FASTA_NEWLINE) != 0) {
        if (typeof this.handler.info_weights == "function") {
          this.handler.info_weights(this.seq_byte_offset, this.seq_line, 0);
        }
        this.process = this._process_post_weights;
      } else {
        this.process = this._process_weights;
        this.weight = "";
        this.weight_byte_offset = this.decoder.offset() + 1;
        this.weight_column = this.decoder.column() + 1;
        this.weight_count = 0;
      }
      return;
    }
    if ((char_type & FastaChecker.FASTA_WHITESPACE) != 0) {
      this.seq_desc_len = 0;
      this.process = this._process_description;
    } else { //newline!
      this.process = this._process_sequence;
    }
  } else {
    if ((char_type & FastaChecker.FASTA_CHEVRON) != 0) {
      // possible misplaced sequence start
      if (typeof this.handler.warn_chevron == "function") {
        this.handler.warn_chevron(this.decoder.offset(), this.decoder.line(), this.decoder.column());
      }
    }
    this.seq_name += String.fromCharCode(char_code);
  }
};

FastaChecker.prototype._process_description = function (char_code, char_type) {
  "use strict";
  if ((char_type & FastaChecker.FASTA_NEWLINE) != 0) {
    // end of the description
    this.process = this._process_sequence;
  } else {
    this.seq_desc_len++;
    if ((char_type & FastaChecker.FASTA_CHEVRON) != 0) {
      // possible misplaced sequence start
      if (typeof this.handler.warn_chevron == "function") {
        this.handler.warn_chevron(this.decoder.offset(), this.decoder.line(), this.decoder.column());
      }
    }
  }
};

FastaChecker.prototype._process_weights = function (char_code, char_type) {
  "use strict";
  if ((char_type & (FastaChecker.FASTA_WHITESPACE | FastaChecker.FASTA_NEWLINE)) != 0) {
    this.weight_count++;
    var is_weight = /^(:?[10](?:\.\d*)?)?$/;
    var weight = parseFloat(this.weight);
    if (!is_weight.test(this.weight) || weight > 1 || weight == 0) {
      if (typeof this.handler.warn_weight == "function") {
        this.handler.warn_weight(this.weight_byte_offset, this.decoder.line(), this.weight_column, this.weight);
      }
    }
    this.weight = "";
    this.weight_byte_offset = this.byte_offst + 1;
    this.weight_column = this.decoder.column() + 1;
    if ((char_type & FastaChecker.FASTA_NEWLINE) != 0) {
      if (typeof this.handler.info_weights == "function") {
        this.handler.info_weights(this.seq_byte_offset, this.seq_line, this.weight_count);
      }
      this.process = this._process_post_weights;
    }
  } else {
    this.weight += String.fromCharCode(char_code);
  }
};

FastaChecker.prototype._process_post_weights = function (char_code, char_type) {
  "use strict";
  if (this.decoder.column() === 0 && (char_type & FastaChecker.FASTA_CHEVRON) != 0) {
    // new sequence
    this.process = this._process_name;
    this.seq_name = "";
    this.seq_offset = 0;
    this.seq_byte_offset = this.decoder.offset();
    this.seq_line = this.decoder.line();
  } else if (this.decoder.column() === 0 && (char_type & FastaChecker.FASTA_COMMENT) != 0) {
    // comment
    this.after_comment = this._process_post_weights;
    this.process = this._process_comment;
    if (typeof this.handler.info_comment == "function") {
      this.handler.info_comment(this.decoder.offset(), this.decoder.line());
    }
  } else if ((char_type & (FastaChecker.FASTA_ALPHA | FastaChecker.FASTA_NON_ALPHA)) != 0) {
    // text before sequence
    if ((char_type & FastaChecker.FASTA_CHEVRON) != 0) {
      // possible misplaced sequence start
      if (typeof this.handler.warn_chevron == "function") {
        this.handler.warn_chevron(this.decoder.offset(), this.decoder.line(), this.decoder.column());
      }
    }
    if (typeof this.handler.warn_junk == "function") {
      this.handler.warn_junk(this.decoder.offset(), this.decoder.line(), this.decoder.column(), 
          char_type, String.fromCharCode(char_code));
    }
  }
};

//
// This is used when reading sequence data and hence only valid sequence
// data or the start of a new sequence is allowed.
//
FastaChecker.prototype._process_sequence = function (char_code, char_type) {
  "use strict";
  if ((char_type & FastaChecker.FASTA_ALPHA) != 0) { // An alphabet character
    if ((char_type & this.alpha_bitmask) != 0) {
      // though not allowed by current settings
      if (typeof this.handler.warn_seq == "function") {
        this.handler.warn_seq(this.decoder.offset(), this.decoder.line(), this.decoder.column(), 
            this.seq_offset, this.seq_line, this.seq_name, char_type,
            String.fromCharCode(char_code));
      }
    }
    this.seq_offset++;
    this.letter_counts[this.letter_map[char_code]]++;
  } else if (this.decoder.column() === 0 && (char_type & FastaChecker.FASTA_CHEVRON) != 0) {
    // new sequence
    this._report_seq_info();
    this.process = this._process_name;
    this.seq_name = "";
    this.seq_offset = 0;
    this.seq_byte_offset = this.decoder.offset();
    this.seq_line = this.decoder.line();
  } else if (this.decoder.column() === 0 && (char_type & FastaChecker.FASTA_COMMENT) != 0) {
    // comment
    this.after_comment = this._process_sequence;
    this.process = this._process_comment;
    if (typeof this.handler.info_comment == "function") {
      this.handler.info_comment(this.decoder.offset(), this.decoder.line());
    }
  } else if ((char_type & FastaChecker.FASTA_NON_ALPHA) != 0) {
    // non-alpha character, should not be in a sequence!
    if ((char_type & FastaChecker.FASTA_CHEVRON) != 0) {
      // possible misplaced sequence start
      if (typeof this.handler.warn_chevron == "function") {
        this.handler.warn_chevron(this.decoder.offset(), this.decoder.line(), this.decoder.column());
      }
    }
    if (typeof this.handler.warn_seq == "function") {
      this.handler.warn_seq(this.decoder.offset(), this.decoder.line(), this.decoder.column(),
          this.seq_offset, this.seq_line, this.seq_name, char_type,
          String.fromCharCode(char_code));
    }
    this.seq_offset++;
    if (char_code < 128) {
      this.letter_counts[this.letter_map[char_code]]++;
    } else {
      this.letter_counts[0]++;
    }
  }
  // white space is ignored...
};

// When we're done, call the approprate functions on the handler
FastaChecker.prototype._signal_stop = function() {
  alphabet = this.guess_alphabet();
  if (typeof this.handler.progress == "function") this.handler.progress(1.0, alphabet);
  if (typeof this.handler.end == "function") this.handler.end(alphabet);
};

//******************************************************************************
// Public functions
//******************************************************************************

FastaChecker.prototype.process_blob = function (blob, offset, chunk_size) {
  "use strict";
  var reader, me, alphabet;
  if (this.give_up) return;
  // set default parameter values
  if (typeof offset === "undefined") offset = 0;
  if (typeof chunk_size === "undefined") chunk_size = 1 << 10;
  // update the progress
  if (offset === 0) {
    if (typeof this.handler.begin == "function") this.handler.begin(blob.size);
  }
  if (typeof this.handler.progress == "function") {
    this.handler.progress(offset / blob.size, this.guess_alphabet());
  }
  // setup the reader
  me = this; // so we can access 'this' inside the closure
  reader = new FileReader();
  reader.onload = function(evt) {
    "use strict";
    var chunk, format, code_data, code_type;
    if (me.give_up) return;
    // process the loaded chunk
    chunk = new Uint8Array(reader.result);
    if (offset == 0) { // check for common (but unreadable) file types
      if ((format = unusable_format(chunk, 400, blob.name)) != null) {
        // report error and stop scan as we don't have a chance of understanding this file
        if (typeof me.handler.error_format == "function") 
          me.handler.error_format(format.type, format.name);
        me._signal_stop();
        return;
      }
    }
    try {
      // validate the loaded chunk
      me.decoder.set_source(chunk, (offset + chunk_size) >= blob.size);
      while ((code_data = me.decoder.next()) != null) {
        code_type = code_data < 128 ? me.alpha_map[code_data] : FastaChecker.FASTA_NON_ALPHA;
        me.process(code_data, code_type);
      }
    } catch (e) {
      if (e instanceof EncodingError) {
        // report error and stop scan as something is wrong with the encoding
        if (typeof me.handler.error_encoding == "function")
          me.handler.error_encoding(e.offset, e.code, e.message);
        me._signal_stop();
        return;
      } else {
        throw e;
      }
    }
    if ((offset + chunk_size) >= blob.size) {
      if (me.process === me._process_comment) me.process = me.after_comment;
      if (me.process === me._process_name) {
        if (me.enable_weights && me.seq_name == "WEIGHTS") {
          if (typeof me.handler.info_weights == "function") {
            me.handler.info_weights(me.seq_byte_offset, me.seq_line, 0);
          }
          me.process = me._process_post_weights;
        }
      }
      if (me.process === me._process_name || 
          me.process === me._process_description ||
          me.process === me._process_sequence) {
        me._report_seq_info();
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

FastaChecker.prototype.cancel = function () {
  "use strict";
  this.give_up = true;
  this.handler = {};
};

FastaChecker.prototype.guess_alphabet = function () {
  "use strict";
  var i, count_total, count_ACGTUN;
  // sum up all counts to get a total
  count_total = 0;
  for (i = 0; i < this.letter_counts.length; i++) {
    count_total += this.letter_counts[i];
  }
  if (count_total == 0) {
    return AlphType.UNKNOWN;
  }
  //sum up counts for A, C, G, T, N and U
  count_ACGTUN = this.letter_counts[1] + this.letter_counts[3] + 
    this.letter_counts[7] + this.letter_counts[20] + this.letter_counts[14] + 
    this.letter_counts[21];
  // if the fraction of ACGTU and N is larger than 90 percent assume DNA, otherwise protein
  if ((count_ACGTUN / count_total) > 0.9) {
    // check if T is more common than U, prefer DNA when equal
    if (this.letter_counts[20] >= this.letter_counts[21]) {
      return AlphType.DNA;
    } else {
      return AlphType.RNA;
    }
  } else {
    return AlphType.PROTEIN;
  }
};


//******************************************************************************
// Fasta Handler
//******************************************************************************
var FastaHandler = function (options) {
  this.configure(options);
  this.reset();
};

FastaHandler.prototype.configure = function (options) {
  "use strict";
  var alphabets, alphabet_name;
  if (typeof options != "object" || options == null) options = {};
  // configure file size
  if (typeof options.file_max == "number") {
    this.max_file_size = options.file_max;
  } else {
    this.max_file_size = null; // 20 MB
  }
  // configure alphabets
  if ((alphabets = options.alphabets) != null) {
    this.allow_alphabets = 0;
    for (alphabet_name in alphabets) {
      if (!alphabets.hasOwnProperty(alphabet_name)) continue;
      this.allow_alphabets |= AlphUtil.from_name(alphabet_name);
    }
  } else {
    this.allow_alphabets = AlphType.DNA | AlphType.RNA | AlphType.PROTEIN;
  }
  // enable MEME weights extension?
  if (typeof options.weights == "boolean") {
    this.enable_weights = options.weights;
  } else {
    this.enable_weights = true;
  }
  // allow masking characters like N in DNA?
  if (typeof options.mask == "boolean") {
    this.allow_sequence_masking = options.masks;
  } else {
    this.allow_sequence_masking = true;
  }
  // allow other ambiguous characters?
  if (typeof options.ambigs == "boolean") {
    this.allow_ambigs = options.ambigs;
  } else {
    this.allow_ambigs = true;
  }
  // allow gap characters?
  if (typeof options.gaps == "boolean") {
    this.allow_gaps = options.gaps;
  } else {
    this.allow_gaps = false;
  }
  // require all sequence to be uppercase (can help with debugging)?
  if (typeof options.uppercase == "boolean") {
    this.only_uppercase = options.uppercase;
  } else {
    this.only_uppercase = false;
  }
  // specify a maximum name length?
  if (typeof options.max_name_len == "number" && options.max_name_len >= 1) {
    this.max_name_length = options.max_name_len;
  } else {
    this.max_name_length = null;
  }
  // specify a maximum description length?
  if (typeof options.max_desc_len == "number" && options.max_desc_len >= 1) {
    this.max_desc_length = options.max_desc_len;
  } else {
    this.max_desc_length = null;
  }
  // specify a minimum sequence length?
  if (typeof options.min_seq_len == "number" && options.min_seq_len >= 0) {
    this.min_seq_length = options.min_seq_len;
  } else {
    this.min_seq_length = null;
  }
  // specify a maximum sequence length?
  if (typeof options.max_seq_len == "number" && options.max_seq_len >= 1) {
    this.max_seq_length = options.max_seq_len;
  } else {
    this.max_seq_length = null;
  }
  // check that the minimum is not larger than the maximum
  if (this.min_seq_length != null && this.max_seq_length != null &&
      this.min_seq_length > this.max_seq_length) {
    throw new Error("Option min_seq must not be larger than option max_seq.");
  }
  // specifiy the maximum number of sequences
  if (typeof options.max_seq_count == "number" && options.max_seq_count >= 1) {
    this.max_seq_count = options.max_seq_count;
  } else {
    this.max_seq_count = null;
  }
  // specify the maximum total sequence length
  if (typeof options.max_seq_total == "number" && options.max_seq_total >= 1) {
    this.max_seq_total = options.max_seq_total;
  } else {
    this.max_seq_total = null;
  }
};

FastaHandler.prototype.reset = function () {
  // have the file details changed?
  this.updated = false;
  // the part of the file processed
  this.fraction = 0;
  // fasta details
  this.file_size = 0;
  // these indicate when not UTF-8
  this.unusable_format_type = 0;
  this.unusable_format_name = null;
  this.encoding_error = null;
  // the lookup for sequence IDs
  this.name_lookup = {};
  // the count of sequences
  this.sequence_count = 0;
  this.sequence_total = 0;
  // the alphabet of the sequences
  this.alphabet = AlphType.UNKNOWN;
  // keep track of problems found
  this.missing_name = new FileFaults();
  this.long_name = new FileFaults();
  this.duplicate_name = new FileFaults();
  this.long_description = new FileFaults();
  this.short_sequence = new FileFaults();
  this.long_sequence = new FileFaults();
  this.comment = new FileFaults();
  this.junk = new FileFaults();
  this.bad_weight = new FileFaults();
  this.chevron = new FileFaults();
  // problems within the sequence data
  this.seq_errors = 0;// total count
  this.seq_gap = new FileFaults(); // gap characters
  this.seq_ambig = new FileFaults(); // ambiguous characters
  this.seq_mask = new FileFaults(); // masking character - Eg. N for DNA
  this.seq_lc = new FileFaults(); // lower case
  this.seq_non_alpha = new FileFaults();
  this.count_seqs_with_error = 0;
  this.last_seq_error_line = -1;
};

FastaHandler.prototype.summary = function () {
  "use strict";
  var error, warning, messages, reason, reasons, letters, add;
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
  // alphabet warning
  if (this.sequence_count === 0) {
    add(true, "No sequences found");
  } else if (this.alphabet != AlphType.UNKNOWN &&
      (this.allow_alphabets & this.alphabet) == 0) {
    add(false, "Sequences look like " +
        AlphUtil.display(this.alphabet) + " but " + 
        AlphUtil.display(this.allow_alphabets) + " was expected.");
  }
  // junk
  if (this.junk.faults() > 0) {
    reason = "Found some text that is not part of any sequence or weights on " +
      (this.junk.line_count == 1 ? "line " : "lines ") + this.junk.lines_str();
    add(true, "Junk text found", [reason]);
  }
  // weights
  if (this.bad_weight.faults() > 0) {
    if (this.bad_weight.faults() == 1) {
      reason = "Found a sequence weight that was not in the correct " +
        "range (0 < weight \u2264 1) on line " + this.bad_weight.lines_str();
    } else {
      reason = "Found " + this.bad_weight.faults() + " sequences weights that " +
        "were not in the correct range (0 < weight \u2264 1) on " +
        (this.bad_weight.line_count == 1 ? "line " : "lines ") +
        this.bad_weight.lines_str();
    }
    add(true, "Sequence weights out of range", [reason]);
  }
  // missing sequence identifiers
  if (this.missing_name.faults() > 0) {
    if (this.missing_name.faults() == 1) {
      reason = "Missed a sequence identifier on line ";
    } else {
      reason = "Missed " + this.missing_name.faults() +
        " sequence identifiers on lines ";
    }
    reason += this.missing_name.lines_str();
    add(true, "Sequence identifier missing - all sequences must have identifiers", [reason]);
  }
  // duplicated sequence identifiers
  if (this.duplicate_name.faults() > 0) {
    if (this.duplicate_name.faults() == 1) {
      reason = "Found a duplicate sequence identifier on line ";
    } else {
      reason = "Found " + this.duplicate_name.faults() + " duplicated sequence identifiers on lines ";
    }
    reason += this.duplicate_name.lines_str();
    add(true, "Sequence identifier duplicated - identifiers must be unique", [reason]);
  }
  // sequence length
  if (this.short_sequence.faults() > 0 || this.long_sequence.faults() > 0) {
    reasons = [];
    if (this.short_sequence.faults() > 0) {
      if (this.short_sequence.faults() == 1) {
        reason = "Found a sequence shorter than the minimum allowed length " + 
          "of " + this.min_seq_length + " on line ";
      } else {
        reason = "Found " + this.short_sequence.faults() + " sequences shorter " +
          "than the minimum allowed length of " + this.min_seq_length + " on lines ";
      }
      reason += this.short_sequence.lines_str();
      reasons.push(reason);
    }
    if (this.long_sequence.faults() > 0) {
      if (this.long_sequence.faults() == 1) {
        reason = "Found a sequence longer than the maximum allowed length " + 
          "of " + this.max_seq_length + " on line ";
      } else {
        reason = "Found " + this.long_sequence.faults() + " sequences longer " +
          "than the maximum allowed length of " + this.max_seq_length + " on lines ";
      }
      reason += this.long_sequence.lines_str();
      reasons.push(reason);
    }
    add(true, "Sequence length out of bounds", reasons);
  }
  // sequence data problems
  if (this.seq_errors > 0) {
    reasons = [];
    // report total errors
    reasons.push("Total of " + this.seq_errors + " bad characters in " +
        this.count_seqs_with_error + " sequences");
    // report gap characters
    if (this.seq_gap.faults() > 0) {
      if (this.seq_gap.faults() == 1) {
        reason = "Found a disallowed gap character " + 
          this.seq_gap.letters_sep() + " on ";
      } else {
        reason = "Found " + this.seq_gap.faults() + " disallowed gap " +
          "characters " + this.seq_gap.letters_sep() + " on ";
      }
      reason += (this.seq_gap.line_count == 1 ? "line " : "lines ") +
        this.seq_gap.lines_str();
      reasons.push(reason);
    }
    // report ambiguous characters
    if (this.seq_ambig.faults() > 0) {
      if (this.seq_ambig.faults() == 1) {
        reason = "Found a disallowed ambiguous character " + 
          this.seq_ambig.letters_sep() + " on ";
      } else {
        reason = "Found " + this.seq_ambig.faults() + " disallowed ambiguous " +
          "characters " + this.seq_ambig.letters_sep() + " on ";
      }
      reason += (this.seq_ambig.line_count == 1 ? "line " : "lines ") +
        this.seq_ambig.lines_str();
      reasons.push(reason);
    }
    // report masking characters
    if (this.seq_mask.faults() > 0) {
      if (this.seq_mask.faults() == 1) {
        reason = "Found a disallowed masking character " + 
          this.seq_mask.letters_sep() + " on ";
      } else {
        reason = "Found " + this.seq_mask.faults() + " disallowed masking " +
          "characters " + this.seq_mask.letters_sep() + " on ";
      }
      reason += (this.seq_mask.line_count == 1 ? "line " : "lines ") +
        this.seq_mask.lines_str();
      reasons.push(reason);
    }
    // report lowercase characters
    if (this.seq_lc.faults() > 0) {
      if (this.seq_lc.faults() == 1) {
        reason = "Found a disallowed lowercase character " + 
          this.seq_lc.letters_sep() + " on ";
      } else {
        reason = "Found " + this.seq_lc.faults() + " disallowed lowercase " +
          "characters " + this.seq_lc.letters_sep() + " on ";
      }
      reason += (this.seq_lc.line_count == 1 ? "line " : "lines ") +
        this.seq_lc.lines_str();
      reasons.push(reason);
    }
    // report non-alpha characters
    if (this.seq_non_alpha.faults() > 0) {
      letters = this.seq_non_alpha.letters_sep();
      if (this.seq_non_alpha.faults() == 1) {
        reason = "Found a character " + (letters.length > 0 ? letters + " " : "") +
          "not in the " + AlphUtil.display(this.allow_alphabets) + " alphabet on ";
      } else {
        reason = "Found " + this.seq_non_alpha.faults() + " characters " + 
          (letters.length > 0 ? "(including " + letters + ") " : "") + 
          "not in the " + AlphUtil.display(this.allow_alphabets) + " alphabet on ";
      }
      reason += (this.seq_non_alpha.line_count == 1 ? "line " : "lines ") +
        this.seq_non_alpha.lines_str();
      reasons.push(reason);
    }
    add(true, "Sequence contains bad characters", reasons);
  }
  // report comments
  if (this.comment.faults() > 0) {
    if (this.comment.faults() == 1) {
      reason = "Found a comment on line ";
    } else {
      reason = "Found " + this.comment.faults() + " comments on lines ";
    }
    reason += this.comment.lines_str();
    add(true, "Unsupported comments - comment lines must be removed", [reason]);
  }
  // report unusual chevrons
  if (this.chevron.faults() > 0) {
    if (this.chevron.faults() == 1) {
      reason = "Found a '>' character on line " + this.chevron.lines_str() +
        " which is not at the begining";
    } else {
      reason = "Found " + this.chevron.faults() + " '>' characters on lines " + 
        this.chevron.lines_str() + " where the '>' is not at the begining";
    }
    add(false, "Potential malformed sequence start", [reason]);
  }
  // report long sequence identifiers
  if (this.long_name.faults() > 0) {
    if (this.long_name.faults() == 1) {
      reason = "Found a sequence with a long identifier on line ";
    } else {
      reason = "Found " + this.long_name.faults() + 
        " sequences with long identifiers on lines ";
    }
    reason += this.long_name.lines_str();
    add(false, "Long sequence identifiers may cause problems", [reason]);

  }
  // report long sequence descriptions
  if (this.long_description.faults() > 0) {
    if (this.long_description.faults() == 1) {
      reason = "Found a sequence with a long description on line ";
    } else {
      reason = "Found " + this.long_description.faults() + 
        " sequences with long descriptions on lines ";
    }
    reason += this.long_description.lines_str();
    add(false, "Long sequence descriptions may cause problems", [reason]);
  }
  // report sequence count excess
  if (this.max_seq_count != null && this.sequence_count > this.max_seq_count) {
    add(true, "Too many sequences", ["Found " + this.sequence_count + 
        " sequences but this only accepts up to " + this.max_seq_count]);
  }
  // report sequence length total excess
  if (this.max_seq_total != null && this.sequence_total > this.max_seq_total) {
    add(true, "Combined sequence length exceeds maximum", 
        ["Found sequences with lengths totaling " + this.sequence_total +
        " but this only accepts a total length up to " + this.max_seq_total]);
  }
  // clear updated state
  this.updated = false;
  // return state
  return {"error": error, "warning": warning, "alphabet": this.alphabet, "messages": messages};
};

FastaHandler.prototype.guess_alphabet = function () {
  return this.alphabet;
}

// tracks the progress of reading the file
FastaHandler.prototype.progress = function (fraction, alphabet) {
  "use strict";
  this.fraction = fraction;
  if (this.alphabet != alphabet) {
    this.alphabet = alphabet;
    this.updated = true;
  }
};

// Reading of the file has begun
FastaHandler.prototype.begin = function (file_size) {
  "use strict";
  this.reset();
  this.file_size = file_size;
  this.updated = true;
};

// Reading of the file has finished (perhaps early due to an error)
FastaHandler.prototype.end = function (alphabet) {
  "use strict";
  this.alphabet = alphabet;
  this.updated = true;
};

// Notes the existence of a sequence
FastaHandler.prototype.info_seq = function (offset, line, name, desc_len, len) {
  "use strict";
  var reason, below_max;
  // check if we're going to overstep the maximum sequence count
  this.sequence_count++;
  if (this.max_seq_count != null && this.sequence_count > this.max_seq_count) {
    this.updated = true;
  }
  // check if we're going to overstep the maximum total sequence length
  this.sequence_total += len;
  if (this.max_seq_total != null && this.sequence_total > this.max_seq_total) {
    this.updated = true;
  }
  // check for missing name
  if (name.length == 0) {
    this.missing_name.add(line);
    this.updated = true;
  } else {
    // check for long name
    if (this.max_name_length != null && name.length > this.max_name_length) {
      this.long_name.add(line);
      this.updated = true;
    }
    // check for duplicate name
    if (this.name_lookup[name]) {
      this.duplicate_name.add(line);
      this.updated = true;
    }
    // store name to check for duplicates later
    this.name_lookup[name] = true;
  }
  // check for long description
  if (this.max_desc_length != null && desc_len > this.max_desc_length) {
    this.long_description.add(line);
    this.updated = true;
  }
  // check sequence length
  if (this.min_seq_length != null && len < this.min_seq_length) {
    this.short_sequence.add(line);
    this.updated = true;
  } else if (this.max_seq_length != null && len > this.max_seq_length) {
    this.long_sequence.add(line);
    this.updated = true;
  }
};

// Notes the existence of a comment
FastaHandler.prototype.info_comment = function (offset, line) {
  "use strict";
  this.comment.add(line);
};

// Notes the existence of a set of weights (MEME extension to FASTA)
FastaHandler.prototype.info_weights = function (offset, line, num_weights) {
  "use strict";
};

// Warns about a character not associated with a sequence, comment or weights
FastaHandler.prototype.warn_junk = function (offset, line, column, type, character) {
  "use strict";
  this.junk.add(line);
  this.updated = true;
};

// Warns about a bad character in a sequence
FastaHandler.prototype.warn_seq = function (offset, line, column,
    seq_offset, seq_line, seq_name, type, character) {
  "use strict";
  var param, lines;
  this.seq_errors++;
  if ((type & FastaChecker.FASTA_ALPHA) !== 0) {
    if (!this.allow_gap && (type & FastaChecker.FASTA_GAP) != 0) {
      seq_gap.add(line, character);
    } else if (!this.allow_ambig && (type & FastaChecker.FASTA_AMBIG) != 0) {
      seq_ambig.add(line, character);
    } else if (!this.allow_mask && (type & FastaChecker.FASTA_MASK) != 0) {
      seq_mask.add(line, character);
    } else if (!this.allow_lc && (type & FastaChecker.FASTA_LC) != 0) {
      seq_lc.add(line, character);
    } else {
      throw new Error("Should not get here");
    }
  } else {
    this.seq_non_alpha.add(line, character);
  }
  // count the number of sequences with errors
  if (this.last_seq_error_line != seq_line) {
    this.count_seqs_with_error++;
    this.last_seq_error_line = seq_line;
  }
  this.updated = true;
};

// Warns about a bad weight (weights are a MEME extension to FASTA)
FastaHandler.prototype.warn_weight = function (offset, line, column, weight_text) {
  "use strict";
  this.bad_weight.add(line);
  this.updated = true;
};

// Warns about an oddly placed chevron character; 
// note that this same character may trigger a call to warn_junk, warn_seq or warn_weight
FastaHandler.prototype.warn_chevron = function (offset, line, column) {
  "use strict";
  this.chevron.add(line);
  this.updated = true;
};

// Parsing has stopped due to an unreadable file format
FastaHandler.prototype.error_format = function (type, name) {
  "use strict";
  this.unusable_format_type = type;
  this.unusable_format_name = name;
  this.updated = true;
};

// Parsing has stopped due to an error in the file encoding
FastaHandler.prototype.error_encoding = function (offset, type, reason) {
  "use strict";
  this.encoding_error = reason;
  this.updated = true;
};
