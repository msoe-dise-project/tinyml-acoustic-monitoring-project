/*
  Read Audio

  Useful reference:
  https://docs.arduino.cc/tutorials/nano-rp2040-connect/rp2040-microphone-basics/
*/

#include <PDM.h>

#include "config.h"
#include "fail.h"
// #include "fileio.h"
#include "model.h"
#include "stats.h"

constexpr uint32_t MS_PER_SEC = 1000;
constexpr uint32_t SEC_PER_MIN = 60;

// total batch duration (in ms): 3.2 ms
// 64 samples per batch
// time between samples in a batch: 0.05 ms

// used to treat realSamples as a circular buffer
uint32_t sampleIdx = 0;

// used to save LED state from last prediction
int PRED_LED_STATE = LOW;

unsigned long lastSamplingTime = 0;
unsigned long lastWriteTime = 0;

// the setup function runs once when you press reset or power the board
void setup() {
  // initialize digital pin LED_BUILTIN as an output.
  pinMode(LED_BUILTIN, OUTPUT);
  // setup started
  digitalWrite(LED_BUILTIN, HIGH);
  
  if(DEBUG) {
    Serial.begin(9600);
    // wait for serial port to connect
    while(!Serial);

    Serial.println(F("Hello world!"));
  }

  PDM.setGain(GAIN);

  if(!PDM.begin(CHANNELS, SAMPLE_RATE_HZ)) {
    if(DEBUG) {
      Serial.println(F("Could not initialize PDM"));
    }
    failLoop();
  } else if(DEBUG) {
    Serial.println(F("PDM initialize!"));
  }

  /*
  if (!SD.begin(SD_PIN)) {
    if(DEBUG) {
      Serial.println(F("Could not find SD device"));
    }
    failLoop();
  }

  if(DEBUG) {
    Serial.println(F("SD device initialized"));
  }
  */

  // createFileIfNeeded();

  // setup complete
  digitalWrite(LED_BUILTIN, LOW);
}

bool readAudioSamples() {
  constexpr uint32_t SAMPLE_BYTES = sizeof(int16_t);
  constexpr uint32_t N_BUFFER_SAMPLES = 128;
  constexpr uint32_t BUFFER_BYTES = N_BUFFER_SAMPLES * SAMPLE_BYTES;

  unsigned long startTime = millis();
  unsigned long elapsedMs = 0;
  int16_t tmpBuffer[N_BUFFER_SAMPLES];
  sampleIdx = 0;

  uint32_t nTotalSamplesRead = 0;
  while(elapsedMs < SAMPLE_DURATION_MS) {
    int bytesAvailable = PDM.available();

    if(bytesAvailable == 0) {
      continue;
    }

    // avoid reading past end of buffer
    if(BUFFER_BYTES < bytesAvailable) {
      bytesAvailable = BUFFER_BYTES;
    }

    int bytesRead = PDM.read(&tmpBuffer, bytesAvailable);
    uint16_t nSamplesRead = bytesRead / SAMPLE_BYTES;

    // copy from temp buffer to where we where
    // we do the actual processing.  treat as a 
    // circular buffer since we only want to keep
    // the last N_WIN_SAMPLES samples.
    for(int i = 0; i < nSamplesRead; i++) {
      realSamples[sampleIdx] = tmpBuffer[i];
      sampleIdx += 1;
      sampleIdx = sampleIdx % N_WIN_SAMPLES;
      nTotalSamplesRead += 1;
    }

    elapsedMs = millis() - startTime;
  }

  return nTotalSamplesRead > 0;
}

void rotateBuffer() {
  const uint32_t shiftSize = N_WIN_SAMPLES - sampleIdx;
  // we only need to swap the first shiftSize values
  for(uint32_t i = 0; i < shiftSize; i++) {
    uint32_t j = (i + shiftSize) % N_WIN_SAMPLES;
    float tmp = realSamples[i];
    realSamples[i] = realSamples[j];
    realSamples[j] = tmp;
  }
}

bool enoughTimeElapsedSinceLastSampling() {
  unsigned long elapsed = millis() - lastSamplingTime;
  return lastSamplingTime == 0 || elapsed >= START_SAMPLING_EVERY_MS;
}

// the loop function runs over and over again forever
void loop() {
  if(enoughTimeElapsedSinceLastSampling()) {
    lastSamplingTime = millis();

    if(readAudioSamples()) {
      if(DEBUG) {
        Serial.println(F("Read audio samples window."));
        Serial.print(F("Circular buffer index: "));
        Serial.println(sampleIdx);
      }

      rotateBuffer();

      SummaryStats stats;
      calculateSampleSummaryStats(stats, N_WIN_SAMPLES, realSamples);
      bool dishwasherActivityPrediction = runPredictionPipeline();

      /*
      dataFile.print(lastSamplingTime);
      dataFile.print(",");
      dataFile.print(stats.stddev, 4);
      dataFile.print(",");
      dataFile.println(dishwasherActivityPrediction);
      dataFile.flush();
      */

      if(DEBUG) {
        Serial.print(lastSamplingTime);
        Serial.print(" ");
        Serial.print(stats.stddev, 4);
        Serial.print(" ");
        Serial.println(dishwasherActivityPrediction);
      }

      if(dishwasherActivityPrediction) {
        PRED_LED_STATE = HIGH;
      } else {
        PRED_LED_STATE = LOW;
      }
    }
  }

  digitalWrite(LED_BUILTIN, PRED_LED_STATE);
}