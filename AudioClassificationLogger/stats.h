#ifndef _STATS_H
#define _STATS_H

typedef struct {
  unsigned long nSamples;
  float mean;
  float stddev;
  int16_t max;
  int16_t min;
} SummaryStats;

bool calculateSampleSummaryStats(SummaryStats &stats, uint32_t nSamples, float* data) {
  // need at least 2 samples to calculate std dev
  if(nSamples < 2) {
    return false;
  }

  stats.nSamples = nSamples;
  float sum = 0.0;
  stats.min = 10000;
  stats.max = 0;
  for(int i = 0; i < nSamples; i++) {
    sum += realSamples[i];
    if (realSamples[i] < stats.min) {
      stats.min = realSamples[i];
    }
    if(realSamples[i] > stats.max) {
      stats.max = realSamples[i];
    }
  }

  stats.mean = sum / nSamples;
  
  float diff_sq_sum = 0.0;
  for(int i = 0; i < nSamples; i++) {
    diff_sq_sum += sq(realSamples[i] - stats.mean);
  }

  // corrected for bias
  stats.stddev = sqrt(diff_sq_sum / (nSamples - 1));

  return true;
}

#endif // _STATS_H