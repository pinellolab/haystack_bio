package au.edu.uq.imb.memesuite.util;

import java.io.BufferedReader;
import java.io.IOException;
import java.io.InputStreamReader;
import java.util.*;
import java.util.regex.Pattern;

import static au.edu.uq.imb.memesuite.util.MultiSourceStatus.CmdResult.*;

/**
 * Display status updates from multiple threads by cycling between which is
 * currently displayed on a timer.
 */
public final class MultiSourceStatus extends TimerTask {
  private BufferedReader console;
  private boolean commandMode;
  private boolean emptyQueueAndStop;
  private TreeMap<String,CmdHandler> cmdHandlers;
  private long changeRate;
  private long fastChangeRate;
  private PriorityQueue<StatusSource> statusQueue;
  private StatusSource displayed;
  private boolean updated;
  private long lastChanged;
  private Timer timer;

  private static final Pattern HELP_CMD_RE = Pattern.compile("^(?:help|\\?)(?:\\s.*)?$", Pattern.CASE_INSENSITIVE);
  private static final Pattern STATUS_CMD_RE = Pattern.compile("^status(?:\\s.*)?$", Pattern.CASE_INSENSITIVE);
  private static final CmdHelp HELP_INFO = new CmdHelp("help", "prints this help message");
  private static final CmdHelp STATUS_INFO = new CmdHelp("status", "returns to the display of status messages");
  private static final String STATUS_TEXT = "Status (press \u23CE Enter to input commands):";

  /**
   * Constructs a multi source status.
   * @param pollRate the rate (in milliseconds) at which the status messages are updated.
   * @param changeRate the rate (in milliseconds) at which the sources are cycled.
   * @param fastChangeRate the rate (in milliseconds) at which a done source is cycled.
   */
  public MultiSourceStatus(long pollRate, long changeRate,
      long fastChangeRate) {
    this.changeRate = changeRate;
    this.fastChangeRate = fastChangeRate;
    emptyQueueAndStop = false;
    statusQueue = new PriorityQueue<StatusSource>();
    displayed = null;
    updated = false;
    lastChanged = System.currentTimeMillis();
    console = new BufferedReader(new InputStreamReader(System.in));
    cmdHandlers = new TreeMap<String, CmdHandler>();
    cmdHandlers.put("", new InbuiltCommands());
    timer = new Timer(true);
    System.out.println(STATUS_TEXT);
    timer.scheduleAtFixedRate(this, 0, pollRate);
  }

  /**
   * Initiate the shutdown process but do not wait for it to complete.
   * As part of the shutdown process all future added status sources will be
   * silently rejected additionally all existing status sources will have their
   * state set to done so they are displayed for one final time. When all
   * existing status have been displayed stop the update timer.
   */
  public synchronized void shutdown() {
    emptyQueueAndStop = true;
    for (StatusSource src : statusQueue) {
      src.done();
    }
    //force a switch to status mode so we can display the final statuses
    if (commandMode) {
      System.out.println("\nStatus:");
      commandMode = false;
    }
  }

  /**
   * Same as shutdown except it blocks until the shutdown has completed.
   * As part of the shutdown process all future added status sources will be
   * silently rejected additionally all existing status sources will have their
   * state set to done so they are displayed for one final time. When all
   * existing status have been displayed stop the update timer.
   */
  public synchronized void shutdownAndWait() {
    shutdown();
    while (this.displayed != null) {
      try { this.wait(); } catch (InterruptedException e) { /* ignore */}
    }
  }

  /**
   * Registers a command handler.
   * @param name the name of the command handler so it can be deregistered later.
   * @param cmdHandler the command handler.
   */
  public synchronized void registerCmdHandler(String name, CmdHandler cmdHandler) {
    if (name == null) throw new NullPointerException("name is null");
    if (name.isEmpty()) throw new IllegalArgumentException("name is empty string.");
    if (cmdHandler == null) throw new NullPointerException("cmdHandler is null");
    cmdHandlers.put(name, cmdHandler);
  }

  /**
   * Deregisters a command handler.
   * @param name the name of the command handler to be deregistered.
   */
  public synchronized void deregisterCmdHandler(String name) {
    if (name == null) throw new NullPointerException("name is null");
    if (name.isEmpty()) throw new IllegalArgumentException("name is empty string.");
    cmdHandlers.remove(name);
  }

  /**
   * Add a status message source to the multi source status.
   * @param source the source to add.
   * @return an object to manage the source.
   */
  public synchronized StatusSource addStatusSource(Object source) {
    StatusSource statusSource = new StatusSource(source);
    if (emptyQueueAndStop) return statusSource; // ignore new entries
    if (displayed == null) {
      displayed = statusSource;
      updated = true;
    } else {
      statusQueue.add(statusSource);
    }
    return statusSource;
  }

  private void show(String display) {
    System.out.print("\r\033[K\r" + display);
  }

  /**
   * Called by a Timer to update the status message.
   */
  public synchronized void run() {
    String command = null;
    try {
      if (console.ready()) {
        command = console.readLine();
      }
    } catch (IOException e) {
      e.printStackTrace();
    }
    if (command != null) {
      final String INPUT_CMD_TEXT = "\nInput command (type help for command list):";
      if (commandMode) {
        CmdResult status = UNHANDLED;
        for (CmdHandler cmdHandler : cmdHandlers.values())
          if ((status = cmdHandler.command(command)) != UNHANDLED) break;
        switch (status) {
          case STATUS_MODE:
            System.out.println();
            System.out.println(STATUS_TEXT);
            commandMode = false;
            updated = true;
            break;
          case SHUTDOWN:
            shutdown();
            return;
           case UNHANDLED:
             System.out.println("Unrecognised command");
          case COMMAND_MODE: // fall through intended
            System.out.println(INPUT_CMD_TEXT);
        }
      } else {
        if (emptyQueueAndStop) {
          // when shutting down we disallow any commands
          System.out.println("\nCommands disabled for shutdown, please wait!\n\nStatus:");
          updated = true;
        } else {
          // switch to command mode
          commandMode = true;
          System.out.println(INPUT_CMD_TEXT);
        }
      }
    }
    if (commandMode) return;
    if (displayed == null) {
      if (emptyQueueAndStop) {
        timer.cancel();
        this.notifyAll();
      }
      return;
    }
    long rate = (displayed.done ? fastChangeRate : changeRate);
    long now = System.currentTimeMillis();
    if (now - lastChanged > rate) {
      lastChanged = now;
      if (statusQueue.isEmpty()) {
        if (displayed.done) {
          displayed = null;
          return;
        }
      } else {
        StatusSource temp = statusQueue.poll();
        if (!displayed.done) statusQueue.add(displayed);
        displayed = temp;
        updated = true;
      }
    }
    if (updated) {
      show(displayed.getStatus());
      updated = false;
    }
  }

  private class InbuiltCommands implements CmdHandler {
    @Override
    public CmdResult command(String command) {
      if (HELP_CMD_RE.matcher(command).matches()) {
        List<CmdHelp> info = new ArrayList<CmdHelp>();
        for (CmdHandler cmdHandler : cmdHandlers.values()) info.addAll(cmdHandler.getHelp());
        System.out.println("\nCommands:");
        for (CmdHelp help : info) {
          System.out.println(help.usage);
          System.out.print("        ");
          System.out.println(help.description);
        }
        return COMMAND_MODE;
      } else if (STATUS_CMD_RE.matcher(command).matches()) {
        return STATUS_MODE;
      }
      return UNHANDLED;
    }

    @Override
    public List<CmdHelp> getHelp() {
      return Arrays.asList(HELP_INFO, STATUS_INFO);
    }
  }

  public enum CmdResult {
    /**
     * Indicates that the command handler did not understand the command.
     */
    UNHANDLED,
    /**
     * Indicates that the command handler understood the command and that
     * the system should remain in command mode.
     */
    COMMAND_MODE,
    /**
     * Indicates that the command handler understood the command and that
     * the system should switch to status display mode.
     */
    STATUS_MODE,
    /**
     * Indicates that the command handler understood the command and that
     * the system should stop reading commands or writing status messages
     * in preparation for shutdown.
     */
    SHUTDOWN
  }

  /**
   * Allows other objects to link into the commands that the
   * multi source status reads.
   */
  public interface CmdHandler {
    /**
     * Called when a new command is issued on the console.
     *
     * @param command the command that was read off the console
     * @return true when handled.
     */
    public CmdResult command(String command);

    /**
     * Returns a list of help objects for each of the commands handled.
     * @return a list of help objects
     */
    public List<CmdHelp> getHelp();
  }

  /**
   * Allows the multi source status to display help topics
   */
  public static final class CmdHelp {
    public final String usage;
    public final String description;

    public CmdHelp(String usage, String description) {
      this.usage = usage;
      this.description = description;
    }
  }
  /**
   * Keeps track of a status message source.
   */
  public final class StatusSource implements Comparable<StatusSource> {
    private long lastSeen;
    private Object source;
    private boolean done;

    private StatusSource(Object source) {
      lastSeen = System.currentTimeMillis();
      this.source = source;
    }

    /**
     * Called to indicated that the status message should be updated from
     * the source object.
     */
    public void update() {
      synchronized (MultiSourceStatus.this) {
        if (displayed == this) {
          updated = true;
        }
      }
    }

    /**
     * The status message will be displayed one final time before
     * begin removed from the cycle.
     */
    public void done() {
      synchronized (MultiSourceStatus.this) {
        done = true;
      }
    }

    /**
     * Returns the status message and updates the last seen time.
     * @return the status message.
     */
    private String getStatus() {
      lastSeen = System.currentTimeMillis();
      return source.toString();
    }

    @Override
    public int compareTo(StatusSource statusSource) {
      if (this.lastSeen < statusSource.lastSeen) {
        return -1;
      } else if (this.lastSeen > statusSource.lastSeen) {
        return 1;
      } else {
        return 0;
      }
    }
  }
}
