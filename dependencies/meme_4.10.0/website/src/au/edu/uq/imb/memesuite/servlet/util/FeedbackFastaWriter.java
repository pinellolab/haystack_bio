package au.edu.uq.imb.memesuite.servlet.util;

import au.edu.uq.imb.memesuite.io.fasta.FastaParseException;
import au.edu.uq.imb.memesuite.io.fasta.FastaWriter;
import au.edu.uq.imb.memesuite.servlet.util.FeedbackHandler;

import java.io.OutputStream;
import java.util.EnumSet;

public class FeedbackFastaWriter extends FastaWriter {
  private FeedbackHandler feedback;
  private String description;
  private EnumSet<FastaParseException.Reason> seen;

  public FeedbackFastaWriter(
      FeedbackHandler feedback, String description, OutputStream output) {
    super(output);
    this.feedback = feedback;
    this.description = description;
    seen = EnumSet.noneOf(FastaParseException.Reason.class);
  }

  private void appendPosition(FastaParseException e, StringBuilder builder) {
    builder.append(" at line ");
    builder.append(e.getLine());
    builder.append(" column ");
    builder.append(e.getColumn());
    builder.append(" (byte offset ");
    builder.append(e.getPosition());
    builder.append(")");
  }

  @Override
  public void error(FastaParseException e) {
    // ignore error types that we've already seen
    if (seen.contains(e.getReason())) return;
    seen.add(e.getReason());
    // customise the message based on the reason
    StringBuilder builder = new StringBuilder();
    switch (e.getReason()) {
      case BAD_UTF8_ORPHAN_CONTINUATION_BYTE:
        builder.append("There is an orphan continuation byte (UTF-8 encoding error) in the ");
        break;
      case BAD_UTF8_FE_OR_FF_BYTE:
        builder.append("There is a 0xFE or 0xFF byte (UTF-8 encoding error) in the ");
        break;
      case BAD_UTF8_OVERLONG_ENCODING:
        builder.append("There is an overlong encoding (UTF-8 encoding error) in the ");
        break;
      case BAD_UTF8_INCOMPLETE_ENCODING:
        builder.append("There is an incomplete encoding (UTF-8 encoding error) in the ");
        break;
      case TEXT_BEFORE_SEQUENCES:
        builder.append("There is text before the first sequence in the ");
        break;
      case WEIGHTS_OUT_OF_RANGE:
        builder.append("There is a weight which is not in the range 0 &lt; <b>weight</b> &le; 1 in the ");
        break;
      case WEIGHTS_NAN:
        builder.append("There is a weight which is not a number in the ");
        break;
      case WEIGHTS_BUFFER_OVERFLOW:
        builder.append("There is a weight which is too large to fit in the parsing buffer for the ");
        break;
      case WEIGHTS_HAVE_SEQUENCE:
        builder.append("There is text following a WEIGHTS section that is not part of a sequence in the ");
        break;
      case SEQUENCE_UNSUPPORTED:
        builder.append("Found unsupported sequence characters in the ");
        break;
      default: // this generic message should not be needed if I keep these up to date...
        builder.append("Encountered a problem parsing the ");
    }
    builder.append(description);
    appendPosition(e, builder);
    builder.append(".");
    this.feedback.whine(builder.toString());
  }
}
