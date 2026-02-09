#ifndef _CONFIG_H
#define _CONFIG_H

// Needs to be 20 khz!
#define SAMPLE_RATE_HZ 20000
#define CHANNELS 1
#define GAIN 30

// perform 1 sec of audio sampling every minute
// note that sampling process is started every minute
// all sampling and data writing is completed within that minute
// empirically, writing ~20k samples takes 550 ms or 0.5 sec
// so we basically do 1.5 sec of work every minute
#define SAMPLE_DURATION_MS 3000
#define START_SAMPLING_EVERY_MS (60 * 1000)

// filename is limited to 8.3 format
static const String FILENAME = "/audiodat.bin";

#define DEBUG false

#endif // _CONFIG_H