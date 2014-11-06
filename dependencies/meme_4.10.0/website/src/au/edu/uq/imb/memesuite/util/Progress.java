package au.edu.uq.imb.memesuite.util;

/**
 * This class is for making status messages that include a progress indication.
 * It is used with the MultiSourceStatus.
 */
public class Progress {
  private String threadName;
  private String taskName;
  private long taskDone;
  private long taskTotal;
  private long lastSeenDone;
  private int ticker;
  private MultiSourceStatus.StatusSource updater;


  public Progress(MultiSourceStatus statusManager, String threadName) {
    this.threadName = threadName;
    taskName = "";
    taskDone = 0;
    taskTotal = -1;
    lastSeenDone = -1;
    ticker = 0;
    updater = statusManager.addStatusSource(this);
  }

  public void setName(String threadName) {
    synchronized (this) {
      this.threadName = threadName;
      taskName = "";
      taskDone = 0;
      taskTotal = -1;
      lastSeenDone = -1;
      ticker = 0;
    }
    updater.update();
  }

  public void setName(String threadName, String task, long done, long total) {
    synchronized (this) {
      this.threadName = threadName;
      this.taskName = task;
      this.taskDone = done;
      this.taskTotal = total;
      lastSeenDone = -1;
      ticker = 0;
    }
    updater.update();
  }

  public void setTask(String task, long done, long total) {
    synchronized (this) {
      this.taskName = task;
      this.taskDone = done;
      this.taskTotal = total;
      ticker = 0;
      lastSeenDone = -1;
    }
    updater.update();
  }

  public void setTask(String task) {
    this.setTask(task, 0, -1);
  }

  public void setTaskProgress(long done, long total) {
    synchronized (this) {
      this.taskDone = done;
      this.taskTotal = total;
      ticker = 0;
      lastSeenDone = -1;
    }
    updater.update();
  }

  public void setTaskPartDone(long done) {
    synchronized (this) {
      this.taskDone = done;
    }
    updater.update();
  }

  public void addTaskPartDone(long doneAdditional) {
    synchronized (this) {
      this.taskDone += doneAdditional;
    }
    updater.update();
  }

  public void addTaskPartDone() {
    this.addTaskPartDone(1);
  }

  public void complete() {
    updater.done();
  }

  public synchronized String toString() {
    StringBuilder builder = new StringBuilder();
    builder.append(threadName);
    if (!taskName.equals("")) {
      builder.append(" - ");
      builder.append(taskName);
    }
    if (taskTotal > 0) {
      // output percentage
      builder.append(String.format(" %.1f%%", (double) taskDone / taskTotal * 100));
    }
    // output cycling dots to show that something has changed, hide for 0% and 100%
    if (taskDone > 0 && taskDone != taskTotal) {
      builder.append(' ');
      for (int i = 0; i <= ticker; i++) builder.append('.');
    }
    if (lastSeenDone != taskDone) {
      lastSeenDone = taskDone;
      ticker = (ticker + 1) % 4;
    }
    return builder.toString();
  }
}
