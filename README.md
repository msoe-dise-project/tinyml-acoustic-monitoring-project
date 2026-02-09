# Illustrating TinyML on Microcontrollers with an Acoustic Monitoring Project

[![Creative Commons Attribution-ShareAlike 4.0 International license badge](https://mirrors.creativecommons.org/presskit/buttons/80x15/png/by-sa.png)](https://creativecommons.org/licenses/by-sa/4.0/)

This repository contains the code and model artifacts for the paper "Illustrating
TinyML on Microcontrollers with an Acoustic Monitoring Project."  The paper describes 
the end-to-end process of developing a real-time system capable of determining the state
(e.g., running, wash or rinse cycles, etc.) of an appliance from live audio recordings.
To achieve its technical goals, the project integrates concepts from embedded systems,
machine learning, and digital signal processing.  Educationally, the project is meant to
serve as a detailed, realistic demonstration that can be the basis of a sequence of labs
or lectures for a class.

## Hardware and Software Materials
The system was implemented using the following hardware:

* [Arduino Nano RP2040 Connect]()
* [Arduino Nano Connector]()
* SD card
* Power adapter
* micro-USB to USB-C cable

and software:

* [Arduino Environment](https://github.com/arduino/ArduinoCore-mbed)
* [ArduinoFFT](https://github.com/kosme/arduinoFFT) library
* [Python](https://www.python.org/) environment including
  [Jupyter Notebooks](https://jupyter.org/) and the [Numpy](https://numpy.org/),
  [Pandas](https://pandas.pydata.org/), [Matplotlib](https://matplotlib.org/),
  [Seaborn](https://seaborn.pydata.org/), and [scikit-learn](https://scikit-learn.org/stable/)
  libraries for data exploration and model development

## This Repository
This repository containers the Arduino sketches (in C/C++) used as firmware on the device and the notebooks
used for model development and export.  The Arduino sketches can be compiled and loaded using the
[Arduino IDE](https://docs.arduino.cc/software/ide/) (version 2).

### Arduino Sketches
The repository contains three Arduino sketches:

1. `AudioLogger`: reads 3 second audio samples from the onboard microphone every minute and writes the samples to
    a binary file (custom file format) on an SD card.
1. `AudioClassificationValidation`: implementation of the prediction pipeline in firmware that performs batch evaluation
    of audio samples stored on the SD card. This was used to validate that the on-device implementation matched
    the Python prototype implementation. 
1. `AudioClassificationLogger`: implementation of the prediction pipeline that collects and classifies windows of audio
    samples in real time.  Both the predictions and standard deviations of the windows are written to a file, so the
    predictions can be evaluated for accuracy.

### Notebooks
The repository contains four notebooks in the `Notebooks` directory:

1. `Dish Washer Audio Data combined exploration.ipynb`: Notebook for exploring and visualizating the collected audio data.
1. `Dish Washer Audio Data combined.ipynb`: Notebook for model development and export.
1. `Dish Washer Audio Data MCU Pipeline evaluation.ipynb`: Notebook used to evaluate the predictions made by the
   `AudioClassificationValidation` firmware to confirm that the on-device prediction pipeline implementation matched
   the Python prototype.
1. `Dish Washer Audio Data Complete Evaluation.ipynb`: Notebook for evaluating the predictions on the audio data
   collected and classified in real time.

## Licensing
The items in this repository are made available under the [Creative Commons Attribution-ShareAlike 4.0 International](https://creativecommons.org/licenses/by-sa/4.0/) license.
Please see the LICENSE file for a full copy of the license text.
