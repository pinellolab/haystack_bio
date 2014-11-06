package au.edu.uq.imb.memesuite.io.fasta;

import au.edu.uq.imb.memesuite.data.SequenceStats;

import java.io.IOException;
import java.io.OutputStream;
import java.nio.charset.Charset;
import java.text.DecimalFormat;
import java.text.NumberFormat;

public class FastaWriter implements FastaHandler {
  private OutputStream out;
  private SequenceStats stats;
  private int lineLen;
  private int idLen;
  private int descLen;
  private boolean keepWeights;
  private NumberFormat weightFormat;
  // state information
  private boolean firstLine;
  private int outLineLen;
  private int outIdLen;
  private int outDescLen;
  private static final Charset utf8 = Charset.forName("UTF-8");

  public FastaWriter(OutputStream output) {
    this.out = output;
    this.lineLen = 100;
    this.idLen = 0;
    this.descLen = 0;
    this.keepWeights = true;
    this.stats = new SequenceStats();
    this.weightFormat = new DecimalFormat("#.###");
  }

  /**
   * Sets the line length used for sequence data.
   * This does not effect the id or description of the sequence.
   * @param lineLen the line length.
   */
  public void setLineLen(int lineLen) {
    this.lineLen = lineLen;
  }

  public int getLineLen() {
    return lineLen;
  }

  public void setMaxIdLen(int idLen) {
    this.idLen = idLen;
  }

  public int getMaxIdLen() {
    return idLen;
  }

  public void setMaxDescLen(int descLen) {
    this.descLen = descLen;
  }

  public int getMaxDescLen() {
    return descLen;
  }

  public void setKeepWeights(boolean keepWeights) {
    this.keepWeights = keepWeights;
  }

  public boolean getKeepWeights() {
    return keepWeights;
  }

  public void setStatsRecorder(SequenceStats stats) {
    this.stats = stats;
  }

  public SequenceStats getStatsRecorder() {
    return stats;
  }

  public void startFasta() { 
    this.firstLine = true;
  }

  public void startWeights() throws IOException {
    if (this.keepWeights) {
      if (!this.firstLine) this.out.write('\n');
      this.firstLine = false;
      this.out.write(">WEIGHTS".getBytes(utf8));
    }
  }

  public void weight(double value) throws IOException {
    if (this.keepWeights) {
      this.out.write(' ');
      this.out.write(this.weightFormat.format(value).getBytes(utf8));
    }
  }

  public void endWeights() throws IOException {
    if (this.keepWeights) {
      this.out.write('\n');
    }
  }

  public void startSequence() throws IOException { 
    if (!this.firstLine) this.out.write('\n');
    this.firstLine = false;
    this.out.write('>');
    this.outIdLen = 0;
    this.outDescLen = 0;
    this.outLineLen = 0;
  }
  
  public void identifier(byte[] ch, int start, int length) throws IOException {
    if (ch == null) throw new IllegalArgumentException("Argument ch is null");
    if (start < 0 || start > ch.length) throw new IllegalArgumentException("Argument start invalid.");
    if (length < 0 || (start + length) > ch.length) throw new IllegalArgumentException("Argument length invalid.");
    int len = (this.idLen <= 0 ? length : Math.min(length, this.idLen - this.outIdLen));
    if (len > 0) {
      this.out.write(ch, start, len);
      this.outIdLen += len;
    }
  }

  public void description(byte[] ch, int start, int length) throws IOException {
    if (ch == null) throw new IllegalArgumentException("Argument ch is null");
    if (start < 0 || start > ch.length) throw new IllegalArgumentException("Argument start invalid.");
    if (length < 0 || (start + length) > ch.length) throw new IllegalArgumentException("Argument length invalid.");
    if (this.outDescLen == 0 && length > 0) this.out.write(' ');
    int len = (this.descLen <= 0 ? length : Math.min(length, this.descLen - this.outDescLen));
    if (len > 0) {
      this.out.write(ch, start, len);
      this.outDescLen += len;
    }
  }

  public void sequence(byte[] ch, int start, int length) throws IOException {
    if (ch == null) throw new IllegalArgumentException("Argument ch is null");
    if (start < 0 || start > ch.length) throw new IllegalArgumentException("Argument start invalid.");
    if (length < 0 || (start + length) > ch.length) throw new IllegalArgumentException("Argument length invalid.");
    if (this.stats != null) this.stats.addSeqPart(ch, start, length);
    while (length > 0) {
      if (this.outLineLen == 0) this.out.write('\n');
      int space = this.lineLen - this.outLineLen;
      if (length < space) {
        this.out.write(ch, start, length);
        this.outLineLen += length;
        start += length;
        length = 0;
      } else {
        this.out.write(ch, start, space);
        this.outLineLen = 0;
        start += space;
        length -= space;
      }
    }
  }

  public void endSequence() throws IOException {
    if (this.stats != null) this.stats.endSeq();
    this.out.write('\n');
  }

  public void endFasta() throws IOException {
    out.flush();
  }

  public void warning(FastaParseException e) {
    // ignore
  }

  public void error(FastaParseException e) throws FastaParseException {
    throw e;
  }
}
