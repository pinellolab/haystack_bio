package au.edu.uq.imb.memesuite.io.fasta;

public class FastaParseException extends FastaException {
  private long position;
  private long line;
  private long column;
  private Reason reason;

  public enum Reason {
    BAD_UTF8_ORPHAN_CONTINUATION_BYTE,
    BAD_UTF8_FE_OR_FF_BYTE,
    BAD_UTF8_OVERLONG_ENCODING,
    BAD_UTF8_INCOMPLETE_ENCODING,
    TEXT_BEFORE_SEQUENCES,
    WEIGHTS_OUT_OF_RANGE,
    WEIGHTS_NAN,
    WEIGHTS_BUFFER_OVERFLOW,
    WEIGHTS_HAVE_SEQUENCE,
    SEQUENCE_UNSUPPORTED
  }

  public FastaParseException(long position, long line, long column, Reason reason) {
    super();
    this.position = position;
    this.line = line;
    this.column = column;
    this.reason = reason;
  }

  public FastaParseException(long position, long line, long column, Reason reason, String message) {
    super(message);
    this.position = position;
    this.line = line;
    this.column = column;
    this.reason = reason;
  }

  public FastaParseException(long position, long line, long column, Reason reason, String message, Throwable cause) {
    super(message, cause);
    this.position = position;
    this.line = line;
    this.column = column;
    this.reason = reason;
  }

  public FastaParseException(long position, long line, long column, Reason reason, Throwable cause) {
    super(cause);
    this.position = position;
    this.line = line;
    this.column = column;
    this.reason = reason;
  }

  public long getPosition() {
    return position;
  }

  public long getLine() {
    return line;
  }

  public long getColumn() {
    return column;
  }

  public Reason getReason() {
    return reason;
  }
}
