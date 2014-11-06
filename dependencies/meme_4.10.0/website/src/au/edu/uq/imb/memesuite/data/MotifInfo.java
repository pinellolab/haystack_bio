package au.edu.uq.imb.memesuite.data;

import au.edu.uq.imb.memesuite.util.JsonWr;

public interface MotifInfo extends JsonWr.JsonValue {
  Alphabet getAlphabet();
  int getMotifCount();
  int getTotalCols();
  int getMinCols();
  int getMaxCols();
  double getAverageCols();
  double getStandardDeviationCols();
}
