#ifndef _FILEIO_H
#define _FILEIO_H

// SD card
#include <SPI.h>
#include <SD.h>

#include "config.h"

File dataFile;

void createFileIfNeeded() {
  bool fileExists = SD.exists(FILENAME);
  if(DEBUG && fileExists) {
    if(!SD.remove(FILENAME)) {
      Serial.println(F("Failed to delete existing log file"));
      failLoop();
    }

    Serial.println(F("Existing log file deleted"));
  }

  dataFile = SD.open(FILENAME, FILE_WRITE);
  if(!dataFile) {
    if(DEBUG) {
      Serial.println(F("Could not open data file for writing."));
    }
    failLoop();
  }

  if(DEBUG) {
    Serial.println("Data file opened for writing.");
  }

  if(!fileExists) {
    dataFile.println(F("timestamp,win_std_dev,prediction"));
    if(DEBUG) {
      Serial.println("Initialized data file with header");
    }
  }
}

#endif // _FILEIO_H