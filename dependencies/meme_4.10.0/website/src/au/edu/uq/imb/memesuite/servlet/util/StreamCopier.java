package au.edu.uq.imb.memesuite.servlet.util;

import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.util.concurrent.Callable;

/**
 * Copies data from a source stream to a destination stream
 */
public final class StreamCopier implements Callable<Void>, Runnable {
  private InputStream source;
  private OutputStream sink;
  private boolean closeSource;
  private boolean closeSink;

  public StreamCopier(InputStream source, OutputStream sink, boolean closeSource, boolean closeSink) {
    this.source = source;
    this.sink = sink;
    this.closeSource = closeSource;
    this.closeSink = closeSink;
  }

  @Override
  public Void call() throws IOException {
    byte[] buffer = new byte[10240]; // 10 KB
    int read;
    try {
      while ((read = source.read(buffer)) != -1) {
        sink.write(buffer, 0, read);
      }
      sink.flush();
      if (closeSource) { source.close(); source = null; }
      if (closeSink) { sink.close(); sink = null; }
    } finally {
      if (closeSource) WebUtils.closeQuietly(source);
      if (closeSink) WebUtils.closeQuietly(sink);
    }
    return null;
  }

  @Override
  public void run() {
    try {
      call();
    } catch (IOException e) {
      e.printStackTrace();
    }
  }

  public static void copy(InputStream source, OutputStream sink, boolean closeSource, boolean closeSink) {
    new Thread(new StreamCopier(source, sink, closeSource, closeSink)).start();
  }

  public static void toSystemOut(InputStream source, boolean closeSource) {
    new Thread(new StreamCopier(source, System.out,  closeSource, false)).start();
  }

  public static void toSystemErr(InputStream source, boolean closeSource) {
    new Thread(new StreamCopier(source, System.err,  closeSource, false)).start();
  }
}
