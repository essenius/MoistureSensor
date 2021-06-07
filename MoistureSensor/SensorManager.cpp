// Copyright 2021 Rik Essenius
// 
//   Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file
//   except in compliance with the License. You may obtain a copy of the License at
// 
//       http://www.apache.org/licenses/LICENSE-2.0
// 
//    Unless required by applicable law or agreed to in writing, software distributed under the License
//    is distributed on an "AS IS" BASIS WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//    See the License for the specific language governing permissions and limitations under the License.

#include <ESP.h>
#include "SensorManager.h"

const double ALPHA = 0.2;
const int SAMPLE_COUNT = 25;
const int CURRENT_DIRECTION_PIN = D1;
const int INHIBIT_PIN = D2;
const int SENSOR_SELECT_PIN = D5;
const int ANALOG_IN_PIN = A0;
const int SAMPLE_TIME_MILLIS = 10; // for pos and neg, so total 20ms, i.e. 50 Hz 

// The overall resistance in the ADC voltage divider (100kΩ + 220kΩ) and the wall create another voltage divider. This results in 
// fairly accurate measurements between 15 kΩ and about 1 MΩ. The walls should have a higher resistance than 1MΩ, but accuracy is 
// also not that critical. Knowing the resistance is above that is enough.

const double R_REF = 320000.0; 
const double V_IN = 3.3;

void SensorManager::begin(int sensorCount) {
  _sensorCount = sensorCount;
  pinMode(INHIBIT_PIN, OUTPUT);
  pinMode(CURRENT_DIRECTION_PIN, OUTPUT);
  pinMode(SENSOR_SELECT_PIN, OUTPUT);
  digitalWrite(INHIBIT_PIN, HIGH);
}

void SensorManager::printResult() {
   Serial.printf("Sensor: %d, Pin: %.1f, Corrected: %.1f, V_out: %.3f V, R_wall:%.3f MΩ\n", _sensorNumber, _rawPinValue, _correctedPinValue, _vOut, _resistance/1e6);
}

float SensorManager::pinValue() {
  return _correctedPinValue;
}

void SensorManager::read(int sensorNumber) {
  _sensorNumber = sensorNumber;
  // This assumes we have at most 2 sensors, called 0 and 1.
  digitalWrite(SENSOR_SELECT_PIN,_sensorNumber == 1);
  digitalWrite(INHIBIT_PIN, LOW);
  for (int i = 0; i < SAMPLE_COUNT * 2; i++) {
   digitalWrite(LED_BUILTIN, i > SAMPLE_COUNT * 1.5 ); // Led ON during the first 3 quarters of the measurement.
    unsigned long lastMeasureTime = millis();
    // simulate alternating current to reduce corrosion, and read on directon LOW (i.e. using X0/Y0 on the mux)
    bool doRead = i % 2 == 0;
    digitalWrite(CURRENT_DIRECTION_PIN, !doRead);
    while (millis() - lastMeasureTime < SAMPLE_TIME_MILLIS) { 
      yield(); 
    }
    if (doRead) {
      int sensorValue = analogRead(ANALOG_IN_PIN);
      // Initialize at first value, after that do a low pass filter (averaging effect)
      _rawPinValue = i==0 ? sensorValue : sensorValue * ALPHA + _rawPinValue * (1 - ALPHA);
    }
  }
  // cut the power on the sensor, so we keep the AC symmetrical 
  digitalWrite(INHIBIT_PIN, HIGH);
  // empirically calibrated correction factors (using resistors)
  _correctedPinValue = _rawPinValue < 1024 ? max(_rawPinValue * 0.95 - 5.7, 0.0) : 1024;
  _vOut = _correctedPinValue * V_IN / 1024.0;
  _resistance = R_REF * (V_IN - _vOut) / _vOut;
}

float SensorManager::resistance() {
  return _resistance;
}
