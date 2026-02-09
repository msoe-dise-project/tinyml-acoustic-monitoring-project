/*
  Audio Classification Validation
  
  Implements a pipeline using DSP and ML techniques to predict whether a dish washer
  is running from 1 second recordings of audio.  This implementation is designed to
  batch process prerecorded audio found in an 'audiodata.bin' file stored on the SD card
  and write the results out to a text file named 'predict.csv'.
*/

// SD card
#include <SPI.h>
#include <SD.h>

#include <arduinoFFT.h>

#include "config.h"
#include "model.h"

// magic constant from https://docs.arduino.cc/tutorials/nano-connector-carrier/getting-started-nano-connector-carrier/#example-project-motion-logger
#define SD_PIN 4

File inFile;
File outFile;

#define N_SAMPLES 16384
const uint32_t N_FFT_VALUES = N_SAMPLES / 2 + 1;

constexpr size_t SAMPLE_SIZE_BYTES = sizeof(int16_t);
float realSamples[N_SAMPLES];
float imagSamples[N_SAMPLES];

// used to flip blink
int LED_STATE = HIGH;

// imagSamples is ignored unless COMPLEX_INPUT is defined
ArduinoFFT<float> FFT(realSamples, imagSamples, N_SAMPLES, 20000);

uint32_t blockId = 0;

void blink() {
  digitalWrite(LED_BUILTIN, LED_STATE);
  if (LED_STATE == HIGH) {
    LED_STATE = LOW;
  } else {
    LED_STATE = HIGH;
  }
}

// when we fail, just loop infinitely
// and blink the LED quickly
void failLoop() {
  outFile.close();
  inFile.close();
  while (true) {
    blink();
    delay(100);
  }
}

void setup() {
  // initialize digital pin LED_BUILTIN as an output.
  pinMode(LED_BUILTIN, OUTPUT);
  // setup started
  digitalWrite(LED_BUILTIN, HIGH);

  if (DEBUG) {
    Serial.begin(9600);
    // wait for serial port to connect
    while (!Serial)
      ;

    Serial.println(F("Hello world!"));
  }

  if (!SD.begin(SD_PIN)) {
    if (DEBUG) {
      Serial.println(F("Could not find SD device"));
    }
    failLoop();
  } else {
    if (DEBUG) {
      Serial.println(F("SD device initialized"));
    }
  }

  if (!SD.exists(IN_FILENAME)) {
    if (DEBUG) {
      Serial.println(F("Cannot find input file."));
    }
    failLoop();
  }

  inFile = SD.open(IN_FILENAME, FILE_READ);
  if (inFile) {
    if (DEBUG) {
      Serial.println(F("Opened data file for reading"));
    }
  } else {
    if (DEBUG) {
      Serial.println(F("Could not open data file for reading."));
    }
    failLoop();
  }

  if (SD.exists(OUT_FILENAME)) {
    if(DEBUG) {
      Serial.println(F("Old output file exists.  Removing."));
    }
    if (SD.remove(OUT_FILENAME)) {
      if(DEBUG) {
        Serial.println(F("Old output file deleted"));
      }
    } else {
      if(DEBUG) {
       Serial.println(F("Failed to delete existing output file"));
      }
      failLoop();
    }
  }

  outFile = SD.open(OUT_FILENAME, FILE_WRITE);
  if (outFile) {
    if (DEBUG) {
      Serial.println(F("Opened output file for writing"));
    }
    outFile.println("prediction\n");
  } else {
    if (DEBUG) {
      Serial.println(F("Could not open output file for writing."));
    }
    failLoop();
  }

  // setup complete
  digitalWrite(LED_BUILTIN, LOW);
}

// returns number of bytes used
// from: https://stackoverflow.com/questions/3784263/converting-an-int-into-a-4-byte-char-array-c
uint32_t toBytes(int16_t value, byte* array) {
  array[0] = (value >> 8) & 0xFF;
  array[1] = value & 0xFF;

  return 2;
}

// returns number of bytes used
// from: https://stackoverflow.com/questions/3784263/converting-an-int-into-a-4-byte-char-array-c
uint32_t toBytes(uint32_t value, byte* array) {
  array[0] = (value >> 24) & 0xFF;
  array[1] = (value >> 16) & 0xFF;
  array[2] = (value >> 8) & 0xFF;
  array[3] = value & 0xFF;

  return 4;
}

// assumes input has 2 bytes
int16_t fromBytes_int16(byte* array) {
  int16_t value = (array[1] & 0xFF) + ((array[0] & 0xFF) << 8);
  return value;
}

// assumes input has 4 bytes
uint32_t fromBytes_uint32(byte* array) {
  uint32_t value = (array[3] & 0xFF) + ((array[2] & 0xFF) << 8) + ((array[1] & 0xFF) << 16) + ((array[0] & 0xFF) << 24);
  return value;
}

bool readDataBlock() {
  unsigned long startTime = millis();

  byte array[sizeof(uint32_t)] = { 0, 0, 0, 0 };
  uint32_t bytesRead;

  bytesRead = inFile.read(array, sizeof(uint32_t));
  if (bytesRead != sizeof(uint32_t)) {
    if (DEBUG) {
      Serial.print("Read ");
      Serial.print(bytesRead);
      Serial.print(" but expected to read ");
      Serial.print(sizeof(uint32_t));
      Serial.println(" bytes.");
    }

    return false;
  }

  uint32_t magicNumber = fromBytes_uint32(array);
  if (magicNumber != 0xDEADBEEF) {
    if (DEBUG) {
      Serial.println("Magic number didn't match expected value.");
    }
    return false;
  }

  bytesRead = inFile.read(array, sizeof(uint32_t));
  if (bytesRead != sizeof(uint32_t)) {
    if (DEBUG) {
      Serial.print("Read ");
      Serial.print(bytesRead);
      Serial.print(" but expected to read ");
      Serial.print(sizeof(uint32_t));
      Serial.println(" bytes.");
    }

    return false;
  }

  uint32_t sampleRate = fromBytes_uint32(array);

  bytesRead = inFile.read(array, sizeof(uint32_t));
  if (bytesRead != sizeof(uint32_t)) {
    if (DEBUG) {
      Serial.print("Read ");
      Serial.print(bytesRead);
      Serial.print(" but expected to read ");
      Serial.print(sizeof(uint32_t));
      Serial.println(" bytes.");
    }

    return false;
  }

  uint32_t nSamples = fromBytes_uint32(array);

  if (nSamples < N_SAMPLES) {
    if (DEBUG) {
      Serial.println("Not enough samples.");
      Serial.print("Found ");
      Serial.print(nSamples);
      Serial.print(" but need a minimum of ");
      Serial.print(N_SAMPLES);
      Serial.println(" samples");
    }

    return false;
  }

  uint32_t nSamplesToDiscard = nSamples - N_SAMPLES;
  for (int i = 0; i < nSamplesToDiscard; i++) {
    // I hope the reading is buffered...
    bytesRead = inFile.read(array, SAMPLE_SIZE_BYTES);
    if (bytesRead != SAMPLE_SIZE_BYTES) {
      if (DEBUG) {
        Serial.println("Could not read complete sample.");
      }
      return false;
    }
  }

  for (int i = 0; i < N_SAMPLES; i++) {
    // I hope the reading is buffered...
    bytesRead = inFile.read(array, SAMPLE_SIZE_BYTES);
    if (bytesRead != SAMPLE_SIZE_BYTES) {
      if (DEBUG) {
        Serial.println("Could not read complete sample.");
      }
      return false;
    }

    int16_t sample = fromBytes_int16(array);

    realSamples[i] = sample;
  }

  unsigned long endTime = millis();
  unsigned long elapsed = endTime - startTime;

  if(DEBUG) {
    Serial.print("Read block in ");
    Serial.print(elapsed);
    Serial.println(" ms");
  }

  return true;
}

bool predict() {
  // Reverse = Numpy forward
  FFT.compute(FFTDirection::Reverse);
  FFT.complexToMagnitude();

  if(DEBUG) {
    Serial.println("First 8 FFT samples are: ");
    for(int i = 0; i < 8; i++) {
      Serial.print("\t");
      Serial.print(realSamples[i], 4);

      float log_feature = log1p(realSamples[i]);
      Serial.print(" ");
      Serial.print(log_feature, 4);

      float scaled_feature = (log_feature - SCALER_MEANS[i]) / SCALER_STD_DEVS[i];
      Serial.print(" ");
      Serial.println(scaled_feature, 4);
    }
  }

  if(N_FFT_VALUES != N_MODEL_FEATURES) {
    if(DEBUG) {
      Serial.print("Found ");
      Serial.print(N_FFT_VALUES);
      Serial.println(" features.");
      Serial.print("Expected ");
      Serial.print(N_MODEL_FEATURES);
      Serial.println(" features");
    }
    failLoop();
  }

  bool prediction = applyModel(realSamples);

  return prediction;
}

void loop() {
  if(DEBUG) {
    Serial.print("Processing block ");
    Serial.println(blockId);
  }

  // initialize arrays to zeros
  for (int i = 0; i < N_SAMPLES; i++) {
    realSamples[i] = 0;
    imagSamples[i] = 0;
  }

  if (!readDataBlock()) {
    failLoop();
  }

  unsigned long startTime = millis();
  bool isPredictedActive = predict();
  unsigned long endTime = millis();
  unsigned long elapsed = endTime - startTime;

  outFile.println(isPredictedActive);

  if(DEBUG) {
    Serial.print("Activity prediction: ");
    Serial.println(isPredictedActive);
    Serial.print("Prediction took ");
    Serial.print(elapsed);
    Serial.println(" ms");
  }

  blockId += 1;

  if(DEBUG && blockId == 10) {
    failLoop();
  }
}
