#ifndef _MODEL_H
#define _MODEL_H

#include <cmath>

#include <arduinoFFT.h>

#include "config.h"
#include "model_weights.h"

static const uint32_t N_WIN_SAMPLES = 16384;
float realSamples[N_WIN_SAMPLES];
float imagSamples[N_WIN_SAMPLES];

ArduinoFFT<float> FFT(realSamples, imagSamples, N_WIN_SAMPLES, SAMPLE_RATE_HZ);

bool applyModel(float* features) {
  float s = MODEL_INTERCEPT;
  for(uint32_t i = 0; i < N_MODEL_FEATURES; i++) {
    float log_feature = log1p(features[i]);
    float scaled_feature = (log_feature - SCALER_MEANS[i]) / SCALER_STD_DEVS[i];
    s += MODEL_COEFS[i] * scaled_feature;
  }

  return s > 0;
}

bool runPredictionPipeline() {
// Reverse = Numpy forward
      FFT.compute(FFTDirection::Reverse);
      FFT.complexToMagnitude();
      bool prediction = applyModel(realSamples);

    // clear sample buffers when done
    for(uint32_t i = 0; i < N_WIN_SAMPLES; i++) {
      realSamples[i] = 0.0f;
      imagSamples[i] = 0.0f;
    }

  return prediction;
}

#endif // _MODEL_H

