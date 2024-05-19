// Copyright 2021-2024 Rik Essenius
// 
//   Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file
//   except in compliance with the License. You may obtain a copy of the License at
// 
//       http://www.apache.org/licenses/LICENSE-2.0
// 
//    Unless required by applicable law or agreed to in writing, software distributed under the License
//    is distributed on an "AS IS" BASIS WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//    See the License for the specific language governing permissions and limitations under the License.

// This class takes a measurement for the sensors (can be 1 or 2 at the moment)
// We create a voltage divider circuit via the internal resistor of the ADC port (220k and 100k) and the wall.
// To limit corrosion we need to simulate AC. But because of those resistors internal to the ADC (connected to GND), 
// we can't simply reverse the current by switching between two ports.
// So instead we use a 4052 multiplexer with X0 on ADC, Y0 on +3.3V (measuring) 
// and X1 via a 320k resistor to 3.3V and Y1 to GND (reverse current). 
// We switch port A of the 4052 with D1, and inhibit with D2.
// The output Ports X and Y go to a second 4052 to multiplex the sensors. Two sensors are used, selected via port A on pin D5.
// If 4 are needed, connect Port B of the second 4052 with e.g. D6 and change the code so it switches right.

#ifndef HEADER_SENSORMANAGER
#define HEADER_SENSORMANAGER

class SensorManager {
public:
    void begin(int sensorCount);
    const char* comment();
    float pinValue();
    void printResult();
    void read(int sensorNumber);
    float resistance();
    const char* samples();
private:
    int _sensorCount;
    int _sensorNumber;
    float _rawPinValue;
    float _correctedPinValue;
    float _vOut;
    float _resistance;
    static const int COMMENT_SIZE = 512;
    char _comment[COMMENT_SIZE];
    char _samples[COMMENT_SIZE];
    
};
#endif
