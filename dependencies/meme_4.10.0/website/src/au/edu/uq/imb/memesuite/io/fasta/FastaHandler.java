package au.edu.uq.imb.memesuite.io.fasta;

import java.io.IOException;

/**
 * A set of callbacks to handle the parsing of a FASTA file.
 */
public interface FastaHandler {
  /**
   * The FASTA parser will call this method when it has started reading
   * a FASTA file. This will always be the first callback called.
   * @throws FastaException when the handler can not accept this and wants
   * the parsing stopped now.
   * @throws IOException when an IO problem occurs, for example if the handler
   * fails to open a file it wants to write the results to.
   */
  public void startFasta() throws FastaException, IOException;

  /**
   * The FASTA parser will call this method when it has finished reading
   * a FASTA file. This will always be the last callback called, even when
   * an exception has stopped the parse.
   * @throws FastaException when the handler can not accept this and wants
   * the parsing stopped now.
   * @throws IOException when an IO problem occurs, for example if the handler
   * fails to close a file it has written the results to.
   */
  public void endFasta() throws FastaException, IOException;

  /**
   * The FASTA parser will call this method when it has found a sequence entry
   * called WEIGHTS (which is a MEME specific extension).
   * @throws FastaException when the handler can not accept this and wants the
   * parsing stopped now.
   * @throws IOException when an IO problem occurs, for example if the handler
   * fails to write to a file it is recording the sequences in.
   */
  public void startWeights() throws FastaException, IOException;

  /**
   * The FASTA parser will call this method when it has read the last of the
   * weights on a >WEIGHTS line.
   * @throws FastaException when the handler can not accept this and wants
   * the parsing stopped now.
   * @throws IOException when an IO problem occurs, for example if the handler
   * fails to write to a file it is recording the sequences in.
   */
  public void endWeights() throws FastaException, IOException;

  /**
   * The FASTA parser will call this method when it has read a weight from a
   * WEIGHTS line.
   * @param value the value of the weight read.
   * @throws FastaException when the handler can not accept this and wants the
   * parsing stopped now.
   * @throws IOException when an IO problem occurs, for example if the handler
   * fails to write to a file it is recording the sequences in.
   */
  public void weight(double value)  throws FastaException, IOException;

  /**
   * The FASTA parser will call this method when it has found the start of a
   * sequence.
   * @throws FastaException when the handler can not accept this and wants the
   * parsing stopped now.
   * @throws IOException when an IO problem occurs, for example if the handler
   * fails to write to a file it is recording the sequences in.
   */
  public void startSequence() throws FastaException, IOException;

  /**
   * The FASTA parser will call this method when it has found the end of a
   * sequence.
   * @throws FastaException when the handler can not accept this and wants the
   * parsing stopped now.
   * @throws IOException when an IO problem occurs, for example if the handler
   * fails to write to a file it is recording the sequences in.
   */
  public void endSequence() throws FastaException, IOException;

  /**
   * The FASTA parser will call this method when it has read part of a sequence
   * identifier. Note that the data buffer should not be modified.
   * @param data the raw data buffer with text encoded in UTF-8.
   * @param start the start of the identifier part in the raw buffer.
   * @param length the length of the identifier part in the raw buffer.
   * @throws FastaException when the handler can not accept this and wants the
   * parsing stopped now.
   * @throws IOException when an IO problem occurs, for example if the handler
   * fails to write to a file it is recording the sequences in.
   */
  public void identifier(byte[] data, int start, int length) throws FastaException, IOException;

  /**
   * The FASTA parser will call this method when it has read part of a sequence
   * description. Note that the data buffer should not be modified.
   * @param data the raw data buffer with text encoded in UTF-8,
   * @param start the start of the description part in the raw buffer.
   * @param length the length of the description part in the raw buffer.
   * @throws FastaException when the handler can not accept this and wants the
   * parsing stopped now.
   * @throws IOException when an IO problem occurs, for example if the handler
   * fails to write to a file it is recording the sequences in.
   */
  public void description(byte[] data, int start, int length) throws FastaException, IOException;

  /**
   * The FASTA parser will call this method when it has read part of a sequence.
   * Note that the data buffer should not be modified.
   * @param data the raw data buffer with text encoded in UTF-8.
   * @param start the start of the sequence part in the raw buffer.
   * @param length the length of the sequence part in the raw buffer.
   * @throws FastaException when the handler can not accept this and wants the
   * parsing stopped now.
   * @throws IOException when an IO problem occurs, for example if the handler
   * fails to write to a file it is recording the sequences in.
   */
  public void sequence(byte[] data, int start, int length) throws FastaException, IOException;

  /**
   * The FASTA parser will call this method when it encounters something in
   * the FASTA file that it thinks is probably wrong.
   * @param e an exception describing the problem.
   * @throws FastaException when the handler wants the parsing stopped because
   * of the warning.
   * @throws IOException when an IO problem occurs, for example if the handler
   * fails to write to a file it is recording the sequences in.
   */
  public void warning(FastaParseException e) throws FastaException, IOException;

  /**
   * The FASTA parser will call this method when it encounters a bit of the
   * FASTA file that is definitely wrong.
   * @param e an exception describing the error.
   * @throws FastaException when the handler wants the parsing stopped because
   * of the error.
   * @throws IOException when an IO problem occurs, for example if the handler
   * fails to write to a file a file it is recording the sequences in.
   */
  public void error(FastaParseException e) throws FastaException, IOException;
}
