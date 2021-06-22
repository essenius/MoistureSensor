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

const double ALPHA = 0.1;
const int STARTUP_COUNT = 16;
const int SAMPLE_COUNT = 16;
const int SAMPLE_TIME_MILLIS = 10; 
const int CURRENT_DIRECTION_PIN = D1;
const int INHIBIT_PIN = D2;
const int SENSOR_SELECT_PIN = D5;
const int ANALOG_IN_PIN = A0;

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
  _comment[0] = 0;
  _samples[0] = 0;
}

const char* SensorManager::comment() {
  return _comment;
}

void SensorManager::printResult() {
  Serial.print(_comment);
}

float SensorManager::pinValue() {
  return _rawPinValue;
}

const char* SensorManager::samples() {
  return _samples;
}

void SensorManager::read(int sensorNumber) {
  _sensorNumber = sensorNumber;
  // switch on led
  digitalWrite(LED_BUILTIN, LOW); 
  // This assumes we have at most 2 sensors, called 0 and 1.
  digitalWrite(SENSOR_SELECT_PIN,_sensorNumber == 1);
  // switch mux to connect to ADC
  digitalWrite(CURRENT_DIRECTION_PIN, LOW);
  // Power up the sensor (enable the muxes)
  digitalWrite(INHIBIT_PIN, LOW);
  _samples[0] = 0;

  // skip the first measurements to let it settle in
  for (int i = 0; i < STARTUP_COUNT; i++) {
    analogRead(ANALOG_IN_PIN);
    delay(SAMPLE_TIME_MILLIS);
  }

  // read a number of samples
  for (int i = 0; i < SAMPLE_COUNT; i++) {
    int sensorValue = analogRead(ANALOG_IN_PIN);
    char numberBuffer[10];
    sprintf(numberBuffer,"%d,",sensorValue);
    strcat(_samples, numberBuffer);
    // Initialize at first value, after that do a low pass filter (averaging effect)
    _rawPinValue = i==0 ? sensorValue : sensorValue * ALPHA + _rawPinValue * (1 - ALPHA);
    delay(SAMPLE_TIME_MILLIS);
  }

  // reverse the current over the wall the same amount of time to reduce corrosion of the sensor
  digitalWrite(LED_BUILTIN, HIGH);
  digitalWrite(CURRENT_DIRECTION_PIN, HIGH);
  delay (SAMPLE_TIME_MILLIS * (STARTUP_COUNT + SAMPLE_COUNT));

  // cut the power on the sensor 
  digitalWrite(INHIBIT_PIN, HIGH);

  // empirically calibrated correction factors (using resistors). Minimum is 1 to avoid division by zero.
  // TODO: correct. The internal voltage divider has a max of 3.2V, not 3.3V.
  _correctedPinValue = _rawPinValue < 1024 ? max(_rawPinValue * 0.95 - 5.7, 1.0) : 1024;
  _vOut = _correctedPinValue * V_IN / 1024.0;
  _resistance = R_REF * (V_IN - _vOut) / _vOut;
  sprintf(_comment, "Sensor: %d, Pin: %.1f, Corrected: %.1f, V_out: %.3f V, R_wall:%.3f MΩ\n", _sensorNumber, _rawPinValue, _correctedPinValue, _vOut, _resistance/1e6);
}

float SensorManager::resistance() {
  return _resistance;
}
