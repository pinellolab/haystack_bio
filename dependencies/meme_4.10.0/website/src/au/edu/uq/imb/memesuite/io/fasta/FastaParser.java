package au.edu.uq.imb.memesuite.io.fasta;

import java.io.IOException;
import java.io.InputStream;

import static au.edu.uq.imb.memesuite.io.fasta.FastaParseException.Reason.*;

public class FastaParser {
  private FastaHandler out;
  private InputStream in;
  // states
  private final ProcessStart STATE_START;
  private final ProcessName STATE_NAME;
  private final ProcessDescription STATE_DESCRIPTION;
  private final ProcessSequence STATE_SEQUENCE;
  private final ProcessWeights STATE_WEIGHTS;
  private final ProcessPostWeights STATE_POST_WEIGHTS;
  // newline calculation
  private boolean prevNl;
  private boolean prevBegin;
  private byte prevByte;
  private int cLen; // codepoint length
  private int cPos; // position in codepoint, first is 0
  private int cVal; // codepoint
  // position in file
  private long pos; // byte position, first byte is byte 0
  private long line;
  private long column;
  // current state
  private FastaState state;
  private int idOffset;
  private boolean weightsSeq;
  
  private static final int[] minimumCodepointValue = {
      -1, // padding value, not used
      0, // 1 byte used so no minimum 
      0x7F, // 2 bytes, must use more than 7 bits 
      0x7FF, // 3 bytes, must use more than 11 bits
      0xFFFF, // 4 bytes, must use more than 16 bits 
      0x1FFFFF, // 5 bytes, must use more than 21 bits
      0x3FFFFFF}; // 6 bytes, must use more than 26 bits

  protected FastaParser(InputStream in, FastaHandler out) {
    this.out = out;
    this.in = in;
    this.STATE_START = new ProcessStart();
    this.STATE_NAME = new ProcessName();
    this.STATE_DESCRIPTION = new ProcessDescription();
    this.STATE_SEQUENCE = new ProcessSequence();
    this.STATE_WEIGHTS = new ProcessWeights();
    this.STATE_POST_WEIGHTS = new ProcessPostWeights();
  }

  private void doParse() throws IOException, FastaException {
    byte[] data = new byte[100];
    int readCount;
    int offset;
    // setup so first character is at column 0 of line 0
    prevNl = true;
    prevBegin = false;
    prevByte = 0x0;
    cLen = 0;
    cPos = 1;
    pos = -1;
    line = -1;
    column = 0; // this could be anything, it will be reset to zero
    state = this.STATE_START;
    while (true) {
      offset = 0;
      readCount = in.read(data);
      if (readCount == -1) break;
      while (offset < readCount) {
        offset += state.process(data, offset, readCount - offset);
      }
    }
    // Note: if your static code analysis tells you this is always false then it's wrong...
    if (state != STATE_START) out.endSequence();
  }

  public static void parse(InputStream in, FastaHandler out) throws IOException, FastaException {
    try {
      // notify the handler that we're starting
      out.startFasta();
      // parse the file
      FastaParser fp = new FastaParser(in, out);
      fp.doParse();
      // notify the handler that we're stopping
      try { out.endFasta(); } finally { out = null; }
      // close the file
      try { in.close(); } finally { in = null; }
    } finally {
      // ensure everything is closed.
      if (out != null) {
        try {
          out.endFasta();
        } catch (IOException e) {/* ignore */
        } catch (FastaException e) {/* ignore */ }
      }
      if (in != null) {
        try {
          in.close();
        } catch (IOException e) { /* ignore */ }
      }
    }
  }

  private void updateColumn(byte currByte) throws FastaException, IOException {
    pos++;
    if (cPos >= cLen) {
      // update the line and column
      boolean currNl = (currByte == '\r' || currByte == '\n');
      boolean currBegin = false;
      // see if the previous character was a newline
      if (prevNl) {
        // the previous character is a newline
        // see what the current character is
        if (!currNl) {
          // not a newline, so line is incremented by previous newline
          line++;
          column = 0;
        } else {
          // current character is a newline, so line is only incremented if
          // the previous is an identical newline or is not a newline beginning
          if (currByte == prevByte || !prevBegin) {
            line++;
            column = 0;
            // so by deduction the current character is a newline beginning
            currBegin = true;
          }
        }
      } else {
        // previous character is not a newline so we can increment the column
        column++;
        if (currNl) {
          // so by deduction the current character is a newline beginning
          currBegin = true;
        }
      }
      prevByte = currByte;
      prevNl = currNl;
      prevBegin = currBegin;

      //check the encoding
      cPos = 1;
      // should be a start byte
      if ((currByte & 0x80) == 0) { // 0xxx xxxx  == ASCII == 7 bits
        cLen = 1;
        cVal = currByte & 0x7F;
      } else if ((currByte & 0xE0) == 0xC0) { // 110x xxxx == 2 byte == 11 bits
        cLen = 2;
        cVal = (currByte & 0x1F) << 6;
      } else if ((currByte & 0xF0) == 0xE0) { // 1110 xxxx == 3 bytes == 16 bits
        cLen = 3;
        cVal = (currByte & 0x0F) << 12;
      } else if ((currByte & 0xF8) == 0xF0) { // 1111 0xxx == 4 bytes == 21 bits
        cLen = 4;
        cVal = (currByte & 0x07) << 18;
      } else if ((currByte & 0xFC) == 0xF8) { // 1111 10xx == 5 bytes == 26 bits
        cLen = 5;
        cVal = (currByte & 0x03) << 24;
      } else if ((currByte & 0xFE) == 0xFC) { // 1111 110x == 6 bytes == 31 bits
        cLen = 6;
        cVal = (currByte & 0x01) << 30;
      } else { // Illegal Encoding!
        cLen = 0;
        if ((currByte & 0xC0) == 0x80) { // 10xx xxxx (not allowed here!)
          out.error(new FastaParseException(pos, line, column, BAD_UTF8_ORPHAN_CONTINUATION_BYTE));
        } else if (currByte == 0xFE || currByte == 0xFF) { // 1111 111x (not allowed!)
          out.error(new FastaParseException(pos, line, column, BAD_UTF8_FE_OR_FF_BYTE));
        }
      }
    } else {
      cPos++;
      // should be a continuation byte
      if ((currByte & 0xC0) == 0x80) { // 10xx xxxx
        // found a continuation byte
        cVal |= (currByte & 0x3F) << (6 * (cLen - cPos - 1));
        if (cPos == cLen && cVal <= minimumCodepointValue[cLen]) {
          // codepoint uses more bytes than needed
          out.error(new FastaParseException(pos - cLen + 1, line, column, BAD_UTF8_OVERLONG_ENCODING));
        }
      } else {
        // non-continuation byte is encoding error!
        out.error(new FastaParseException(pos - cPos + 1, line, column, BAD_UTF8_INCOMPLETE_ENCODING));
        cPos = 1;
        cLen = 0;
      }
    }
  }

  private boolean isASCIISpace(byte c) {
    // this aims to match the isspace function in c
    return (c == ' ' || c == '\f' || c == '\n' || c == '\r' || c == '\t' || c== '\013');
  }

  private void paramCheck(byte[] buffer, int start, int length) {
    assert(buffer != null);
    assert(start >= 0 && start <= buffer.length);
    assert(length >= 0);
    assert(start + length <= buffer.length);
  }

  private interface FastaState {
    int process(byte[] buffer, int start, int length) throws FastaException, IOException;
  }

  private class ProcessStart implements FastaState {
    private boolean errorCharsBeforeStart;
    public int process(byte[] buffer, int start, int length) throws FastaException, IOException {
      paramCheck(buffer, start, length);
      int end = start + length;
      int i;
      for (i = start; i < end; i++) {
        FastaParser.this.updateColumn(buffer[i]);
        if (FastaParser.this.column == 0 && buffer[i] == '>') {
          // found the start of a sequence
          FastaParser.this.state = FastaParser.this.STATE_NAME;
          FastaParser.this.idOffset = 0;
          FastaParser.this.weightsSeq = true; // start by assuming it is and disprove
          return i - start + 1;
        } else if (!isASCIISpace(buffer[i])) {
          // there shouldn't be anything but whitespace before the first sequence
          if (!this.errorCharsBeforeStart) {
            FastaParser.this.out.error(new FastaParseException(pos, line, column, TEXT_BEFORE_SEQUENCES));
            this.errorCharsBeforeStart = true;
          }
        }
      }
      return length;
    }
  }

  private class ProcessName implements FastaState {
    public static final String WEIGHTS_ID = "WEIGHTS";
    public int process(byte[] buffer, int start, int length) throws FastaException, IOException {
      paramCheck(buffer, start, length);
      int end = start + length;
      int i;
      for (i = start; i < end; i++) {
        FastaParser.this.updateColumn(buffer[i]);
        // look for the end of the name
        if (isASCIISpace(buffer[i])) {
          // pass the final bit of the name to the callback
          int len = i - start;
          // check to see if it is really a WEIGHTS section
          if (FastaParser.this.weightsSeq) {
            if ((FastaParser.this.idOffset + len) == WEIGHTS_ID.length()) {
              if (!WEIGHTS_ID.regionMatches(FastaParser.this.idOffset,
                    new String(buffer, start, len), 0, len)) {
                FastaParser.this.weightsSeq = false;
              }
            } else {
              FastaParser.this.weightsSeq = false;
            }

            if (FastaParser.this.weightsSeq) {
              FastaParser.this.out.startWeights();
            } else {
              FastaParser.this.out.startSequence();
              if (FastaParser.this.idOffset > 0) FastaParser.this.out.identifier(
                  WEIGHTS_ID.getBytes("UTF-8"), 0, FastaParser.this.idOffset);
            }
          }
          FastaParser.this.idOffset += len;
          if (FastaParser.this.weightsSeq) {
            if (FastaParser.this.prevNl) {
              FastaParser.this.out.endWeights();
              FastaParser.this.state = FastaParser.this.STATE_POST_WEIGHTS;
            } else {
              FastaParser.this.state = FastaParser.this.STATE_WEIGHTS;
            }
          } else {
            if (len > 0) FastaParser.this.out.identifier(buffer, start, len);
            if (FastaParser.this.prevNl) {
              FastaParser.this.state = FastaParser.this.STATE_SEQUENCE;
            } else {
              FastaParser.this.state = FastaParser.this.STATE_DESCRIPTION;
            }
          }
          return i - start + 1;
        }
      }
      // did not find the end of the name so pass what we have to the callback
      if (length > 0) {
        if (FastaParser.this.weightsSeq) {
          if ((FastaParser.this.idOffset + length) <= WEIGHTS_ID.length()) {
            if (!WEIGHTS_ID.regionMatches(FastaParser.this.idOffset,
                  new String(buffer, start, length), 0, length)) {
              FastaParser.this.weightsSeq = false;
            }
          } else {
            FastaParser.this.weightsSeq = false;
          }
          if (!FastaParser.this.weightsSeq) FastaParser.this.out.startSequence();
        }
        FastaParser.this.idOffset += length;
        if (!FastaParser.this.weightsSeq) {
          FastaParser.this.out.identifier(buffer, start, length);
        }
      }
      return length;
    }
  }

  private class ProcessDescription implements FastaState {
    public int process(byte[] buffer, int start, int length) throws FastaException, IOException {
      paramCheck(buffer, start, length);
      int end = start + length;
      int i;
      for (i = start; i < end; i++) {
        FastaParser.this.updateColumn(buffer[i]);
        if (FastaParser.this.prevNl) {
          if (i > start) FastaParser.this.out.description(buffer, start, i - start);
          FastaParser.this.state = FastaParser.this.STATE_SEQUENCE;
          return i - start + 1;
        }
      }
      if (length > 0) FastaParser.this.out.description(buffer, start, length);
      return length;
    }
  }

  private class ProcessWeights implements FastaState {
    private StringBuilder numberBuffer;
    private boolean weightTooLong;
    private boolean weightNaN;
    private boolean weightOutOfRange;
    public ProcessWeights() {
      this.numberBuffer = new StringBuilder(20);
      this.numberBuffer.ensureCapacity(20);
      this.weightTooLong = false;
      this.weightNaN = false;
      this.weightOutOfRange = false;
    }
    public int process(byte[] buffer, int start, int length) throws FastaException, IOException {
      paramCheck(buffer, start, length);
      int end = start + length;
      int i;
      for (i = start; i < end; i++) {
        FastaParser.this.updateColumn(buffer[i]);
        // look for the end of the number
        if (isASCIISpace(buffer[i])) {
          if (this.numberBuffer.length() > 0) {
            double weight = 1;
            if (this.numberBuffer.length() <= 20) {
              try {
                weight = Double.valueOf(this.numberBuffer.toString());
                if (weight <= 0 || weight > 1) {
                  if (!this.weightOutOfRange) {
                    FastaParser.this.out.error(
                        new FastaParseException(pos, line, column, WEIGHTS_OUT_OF_RANGE));
                    this.weightOutOfRange = true;
                  }
                  weight = 1;
                }
              } catch (NumberFormatException e) {
                if (!this.weightNaN) {
                  FastaParser.this.out.error(
                      new FastaParseException(pos, line, column, WEIGHTS_NAN, e));
                  this.weightNaN = true;
                }
              }
            } else {
              if (!this.weightTooLong) {
                FastaParser.this.out.error(
                    new FastaParseException(pos, line, column, WEIGHTS_BUFFER_OVERFLOW));
                this.weightTooLong = true;
              }
            }
            // when there is something wrong with a weight it is set to 1
            FastaParser.this.out.weight(weight);
            // clear the number
            this.numberBuffer.setLength(0);
          }
        } else {
          // append to the number, allows length to go up to 21
          if (this.numberBuffer.length() <= 20) {
            this.numberBuffer.append(buffer[i]);
          }
        }
        if (FastaParser.this.prevNl) {
          if (i > start) FastaParser.this.out.endWeights();
          FastaParser.this.state = FastaParser.this.STATE_POST_WEIGHTS;
          return i - start + 1;
        }
      }
      return length;
    }
  }

  private class ProcessPostWeights implements FastaState {
    private boolean errorCharsPostWeights;
    public int process(byte[] buffer, int start, int length) throws FastaException, IOException {
      paramCheck(buffer, start, length);
      int end = start + length;
      int i;
      for (i = start; i < end; i++) {
        FastaParser.this.updateColumn(buffer[i]);
        if (FastaParser.this.column == 0 && buffer[i] == '>') {
          // start a new sequence.
          FastaParser.this.state = FastaParser.this.STATE_NAME;
          FastaParser.this.idOffset = 0;
          FastaParser.this.weightsSeq = true;
          return i - start + 1;
        } else if (!isASCIISpace(buffer[i])) {
          // there shouldn't be anything but whitespace before the first sequence
          if (!this.errorCharsPostWeights) {
            FastaParser.this.out.error(
                new FastaParseException(pos, line, column, WEIGHTS_HAVE_SEQUENCE));
            this.errorCharsPostWeights = true;
          }
        }
      }
      return length;
    }
  }

  private class ProcessSequence implements FastaState {
    private boolean errorUnsupportedChars = false;
    public int process(byte[] buffer, int start, int length) throws FastaException, IOException {
      paramCheck(buffer, start, length);

      int end = start + length;
      int i;
      int segStart = -1;
      for (i = start; i < end; i++) {
        FastaParser.this.updateColumn(buffer[i]);
        byte c = buffer[i];
        if (FastaParser.this.column == 0 && c == '>') {
          // end this sequence and start a new one.
          FastaParser.this.out.endSequence();
          FastaParser.this.state = FastaParser.this.STATE_NAME;
          FastaParser.this.idOffset = 0;
          FastaParser.this.weightsSeq = true;
          return i - start + 1;
        } else if (isASCIISpace(c)) {
          if (segStart != -1) {
            FastaParser.this.out.sequence(buffer, segStart, i - segStart);
            segStart = -1;
          }
        } else if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '.' || c == '-') {
          // sequence data
          if (segStart == -1) {
            segStart = i;
          }
        } else {
          // unsupported characters
          if (segStart != -1) {
            FastaParser.this.out.sequence(buffer, segStart, i - segStart);
            segStart = -1;
          }
          if (!this.errorUnsupportedChars) {
            FastaParser.this.out.error(
                new FastaParseException(pos, line, column, SEQUENCE_UNSUPPORTED));
            this.errorUnsupportedChars = true;
          }
        }
      }
      if (segStart != -1) {
        FastaParser.this.out.sequence(buffer, segStart, length - (segStart - start));
      }
      return length;
    }
  }

}
