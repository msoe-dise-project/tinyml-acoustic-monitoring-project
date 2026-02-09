/*
  Read Audio

  Useful reference:
  https://docs.arduino.cc/tutorials/nano-rp2040-connect/rp2040-microphone-basics/
*/

#include <PDM.h>

// SD card
#include <SPI.h>
#include <SD.h>

#include "config.h"

// magic constant from https://docs.arduino.cc/tutorials/nano-connector-carrier/getting-started-nano-connector-carrier/#example-project-motion-logger
#define SD_PIN 4

File dataFile;

constexpr uint32_t MS_PER_SEC = 1000;
constexpr uint32_t SEC_PER_MIN = 60;

// total batch duration (in ms): 3.2 ms
// 64 samples per batch
// time between samples in a batch: 0.05 ms
constexpr uint32_t SAMPLE_BYTES = sizeof(int16_t);
// extra space to handle timing errors
constexpr uint32_t N_BUFFER_SAMPLES = (SAMPLE_DURATION_MS * SAMPLE_RATE_HZ) / MS_PER_SEC + 2048;
constexpr uint32_t BUFFER_BYTES = N_BUFFER_SAMPLES * SAMPLE_BYTES;

// note that only the entries up to and including index samplesRead - 1 are valid
int16_t audioSamplesBuffer[N_BUFFER_SAMPLES];
uint32_t samplesRead = 0;

// used to flip blink
int LED_STATE = HIGH;

unsigned long lastSamplingTime = 0;
unsigned long lastWriteTime = 0;

void blink() {
  digitalWrite(LED_BUILTIN, LED_STATE);
  if(LED_STATE == HIGH) {
    LED_STATE = LOW;
  } else {
    LED_STATE = HIGH;
  }
}

// when we fail, just loop infinitely
// and blink the LED quickly
void failLoop() {
  while(true) {
    blink();
    delay(100);
  }
}

void createFileIfNeeded() {
  if(DEBUG && SD.exists(FILENAME)) {
    if(SD.remove(FILENAME)) {
      Serial.println(F("Existing log file deleted"));
    } else {
      Serial.println(F("Failed to delete existing log file"));
    }
  }
  if(!SD.exists(FILENAME)) {
    if(DEBUG) {
      Serial.println(F("Log file does not exist. Creating."));
    }
    dataFile = SD.open(FILENAME, FILE_WRITE);
    if(dataFile) {
      dataFile.close();
      if(DEBUG) {
        Serial.println(F("Log file created"));
      }
    } else {
      if(DEBUG) {
        Serial.println(F("Could not open data file for writing."));
      }
      failLoop();
    }
  } else {
    if(DEBUG) {
      Serial.println(F("Log file already exists."));
    }
  }
}

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

  if (!SD.begin(SD_PIN)) {
    if(DEBUG) {
      Serial.println(F("Could not find SD device"));
    }
    failLoop();
  } else {
    if(DEBUG) {
      Serial.println(F("SD device initialized"));
    }
  }

  createFileIfNeeded();

  dataFile = SD.open(FILENAME, FILE_WRITE);
  if(dataFile) {
    if(DEBUG) {
      Serial.println(F("Log file opened for appending"));
    }
  } else {
    if(DEBUG) {
      Serial.println(F("Could not open data file for writing."));
    }
    failLoop();
  }

  // setup complete
  digitalWrite(LED_BUILTIN, LOW);
}

bool readAudioSamples() {
  unsigned long startTime = millis();
  unsigned long elapsedMs = 0;
  samplesRead = 0;

  while(elapsedMs < SAMPLE_DURATION_MS) {
    int bytesAvailable = PDM.available();

    if(bytesAvailable == 0) {
      continue;
    }

    // avoid reading past end of buffer
    int remainingBufferBytes = BUFFER_BYTES - samplesRead * SAMPLE_BYTES;
    if(remainingBufferBytes < bytesAvailable) {
      bytesAvailable = remainingBufferBytes;
    }

    int bytesRead = PDM.read(&audioSamplesBuffer[samplesRead], bytesAvailable);
    samplesRead += bytesRead / SAMPLE_BYTES;
    elapsedMs = millis() - startTime;
  }

  return samplesRead > 0;
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

bool writeDataBlock() {
  // reused variables
  byte array[4] = {0, 0, 0, 0};
  uint32_t bytesFilled;
  uint32_t bytesWritten;

  // write block header
  // magic number
  const uint32_t magicNumber = 0xDEADBEEF;
  bytesFilled = toBytes(magicNumber, array);
  if(bytesFilled != sizeof(uint32_t)) {
    if(DEBUG) {
      Serial.print(F("Failed to convert magic number to bytes. Only "));
      Serial.print(bytesFilled);
      Serial.println(F(" bytes filled."));
    }
    return false;
  }
  bytesWritten = dataFile.write(array, bytesFilled);
  if(bytesWritten != sizeof(uint32_t)) {
    if(DEBUG) {
      Serial.print(F("Failed to write magic number. Only "));
      Serial.print(bytesWritten);
      Serial.println(F(" bytes written."));
    }
    return false;
  }

  // sample rate
  const uint32_t sampleRate = SAMPLE_RATE_HZ;
  bytesFilled = toBytes(sampleRate, array);
  if(bytesFilled != sizeof(uint32_t)) {
    if(DEBUG) {
      Serial.print(F("Failed to convert sample rate to bytes. Only "));
      Serial.print(bytesFilled);
      Serial.println(F(" bytes filled."));
    }
    return false;
  }
  bytesWritten = dataFile.write(array, bytesFilled);
  if(bytesWritten != sizeof(uint32_t)) {
    if(DEBUG) {
      Serial.print(F("Failed to write sample rate. Only "));
      Serial.print(bytesWritten);
      Serial.println(F(" bytes written."));
    }
    return false;
  }

  // num samples
  bytesFilled = toBytes(samplesRead, array);
  if(bytesFilled != sizeof(uint32_t)) {
    if(DEBUG) {
      Serial.print(F("Failed to convert numbers of samples to bytes. Only "));
      Serial.print(bytesFilled);
      Serial.println(F(" bytes filled."));
    }
    return false;
  }
  bytesWritten = dataFile.write(array, bytesFilled);
  if(bytesWritten != sizeof(uint32_t)) {
    if(DEBUG) {
      Serial.print(F("Failed to write number of samples. Only "));
      Serial.print(bytesWritten);
      Serial.println(F(" bytes written."));
    }
    return false;
  }

  for(int i = 0; i < samplesRead; i++) {
    bytesFilled = toBytes(audioSamplesBuffer[i], array);
    if(bytesFilled != sizeof(int16_t)) {
      if(DEBUG) {
        Serial.print(F("Failed to convert sample to bytes. Only "));
        Serial.print(bytesFilled);
        Serial.println(F(" bytes filled."));
      }
      return false;
    }
    bytesWritten = dataFile.write(array, bytesFilled);
    if(bytesWritten != sizeof(int16_t)) {
      if(DEBUG) {
        Serial.print(F("Failed to write number sample. Only "));
        Serial.print(bytesWritten);
        Serial.println(F(" bytes written."));
      }
      return false;
    }
  }

  // "empty" buffer
  samplesRead = 0;

  dataFile.flush();

  return true;
}

bool enoughTimeElapsedSinceLastSampling() {
  unsigned long elapsed = millis() - lastSamplingTime;
  return lastSamplingTime == 0 || elapsed >= START_SAMPLING_EVERY_MS;
}

// the loop function runs over and over again forever
void loop() {
  if(enoughTimeElapsedSinceLastSampling()) {
    digitalWrite(LED_BUILTIN, HIGH);
    lastSamplingTime = millis();

    if(readAudioSamples()) {
      if(DEBUG) {
        Serial.print(F("Read "));
        Serial.print(samplesRead);
        Serial.println(F(" audio samples."));
      }
      unsigned long start = millis();
      if(writeDataBlock() && DEBUG) {
        Serial.println(F("Data block written."));
      } else if (DEBUG) {
        Serial.println(F("Failed to write data block"));
      }
      unsigned long elapsed = millis() - start;
      if(DEBUG) {
        Serial.print(F("Took "));
        Serial.print(elapsed);
        Serial.println(F(" ms to write data block."));
      }
      digitalWrite(LED_BUILTIN, LOW);
    }
  }
}