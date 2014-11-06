package au.edu.uq.imb.memesuite.util;

import java.math.BigDecimal;

/**
 * Calculate mean and standard deviation without storing values.
 * Uses recurrence formulas
 * M_k = M_{k-1} + (x_k - M_{k-1}) / k
 * S_k = S_{k-1} + (x_k - M_{k-1}) * (x_k - M_k)
 * Based on C++ implementation at http://www.johndcook.com/standard_deviation.html
 */
public class SampleStats {
  private long count;
  private double mean;
  private double min;
  private double max;
  private double oldMean;
  private double oldS;
  private double newS;
  private BigDecimal total;

  public SampleStats(boolean recordTotal) {
    this.count = 0;
    this.mean = 0;
    this.min = 0;
    this.max = 0;
    if (recordTotal) {
      this.total = new BigDecimal(0);
    } else {
      this.total = null;
    }
  }

  /**
   * Update the calculation of mean and standard deviation with a new sample.
   * @param x The sample
   */
  public void update(double x) {
    this.count++;
    if (this.count == 1) {
      this.oldMean = x;
      this.mean = x;
      this.oldS = 0;
      this.min = x;
      this.max = x;
    } else {
      // update mean and variance calculation
      this.mean = this.oldMean + ((x - this.oldMean) / this.count);
      this.newS = this.oldS + ((x - this.oldMean) * (x - this.mean));
      //set up for next iteration
      this.oldMean = this.mean;
      this.oldS = this.newS;
      // update min and max
      if (x < this.min) {
        this.min = x;
      } else if (x > this.max) {
        this.max = x;
      }
    }
    if (this.total != null) {
      this.total = this.total.add(new BigDecimal(x));
    }
  }

  /**
   * Return the number of samples seen.
   * @return The number of samples
   */
  public long getCount() {
    return this.count;
  }

  /**
   * Return the summed total to sample seen.
   * @return The summed samples
   */
  public BigDecimal getTotal() {
    return this.total;
  }

  /**
   * Return the smallest sample seen.
   * @return The smallest sample
   */
  public double getSmallest() {
    return this.min;
  }

  /**
   * Return the largest sample seen.
   * @return The largest sample
   */
  public double getLargest() {
    return this.max;
  }

  /**
   * Returns the mean of all the samples seen or 0 when none have been seen.
   * @return The meadian of all samples
   */
  public double getMean() {
    return this.mean;
  }

  /**
   * Returns the variance of all the samples seen or 0 when less than 2 samples
   * have been seen.
   * @return The variance of all samples
   */
  public double getVariance() {
    return (this.count > 1) ? (this.newS / (this.count - 1)) : 0;
  }

  /**
   * Returns the standard deviation of all the samples seen or 0 when less than
   * 2 samples have been seen.
   * @return The standard deviation of all samples
   */
  public double getStandardDeviation() {
    return Math.sqrt(this.getVariance());
  }
}
