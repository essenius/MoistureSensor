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

#include <time.h>
#include <ESP.h>
#include <fs.h>

#include "Scheduler.h"

const time_t NON_SYNCED_TIME_UPPER_LIMIT = 16 * 3600;
const char* NEXT_RUN_TIMESTAMP_FILE = "/nextrun_timestamp.txt";

const long STARTUP_TIME_SECONDS = 15;
const long MAX_WAIT_SECONDS_WITHOUT_SLEEP = 60; // must be larger than STARTUP_TIME_SECONDS

// deepSleep is not very accurate. Empirically determined correction factor
const double TIME_CORRECTION_FACTOR = 1.062;

// Sync time from the Internet (i.e. do this after wifi has become active)
void Scheduler::begin(long measureIntervalSeconds) {
  bool SPIFFSmounted = SPIFFS.begin();
  if (!SPIFFSmounted) {
    Serial.println("Could not mount SPIFFS");
  }
  _measureIntervalSeconds = measureIntervalSeconds;
  configTime(0, 0, "pool.ntp.org", "time.nist.gov");
  time_t now = time(nullptr);
  Serial.print("Current time: ");
  // if current time epoch is very low, the ntp time has not kicked in yet
  while (now < NON_SYNCED_TIME_UPPER_LIMIT) {
    delay(500);
    Serial.print(".");
    now = time(nullptr);
  }
  struct tm timeinfo;
  gmtime_r(&now, &timeinfo);
  Serial.print(asctime(&timeinfo));
}

// Read time stamp (epoch) from file
time_t Scheduler::readTimestamp() {
  File file = SPIFFS.open(NEXT_RUN_TIMESTAMP_FILE, "r");
  if (!file) {
    return 0;
  }
  time_t returnValue = file.readStringUntil('\n').toInt();
  file.close();
  return returnValue;  
}

// Write time stamp (epoch) to file
void Scheduler::writeTimestamp(time_t timestamp) {
  File file = SPIFFS.open(NEXT_RUN_TIMESTAMP_FILE, "w");
  if (!file) {
      Serial.printf("open file '%s' failed\n", NEXT_RUN_TIMESTAMP_FILE);
  }
  file.println(timestamp);
  file.close();
  return;
}

void Scheduler::waitForNextRun(std::function<void(void)> callback) {
  long sleepTimeSeconds = _nextRunTimestamp - time(nullptr);  
  Serial.printf("Wait time: %ld\n",sleepTimeSeconds);
  if (sleepTimeSeconds > MAX_WAIT_SECONDS_WITHOUT_SLEEP) {
    Serial.printf("Deep sleep for %d seconds\n", sleepTimeSeconds);
    ESP.deepSleep((sleepTimeSeconds - STARTUP_TIME_SECONDS) * 1000L * 1000L * TIME_CORRECTION_FACTOR);
  }
  Serial.printf("Normal wait for %d seconds\n", _nextRunTimestamp - time(nullptr));
  while (time(nullptr) < _nextRunTimestamp) {
    digitalWrite(LED_BUILTIN, time(nullptr) % 2 == 0);
    callback();
    //mqttDriver.processMessages();
    delay(10);
  }
  Serial.println("done waiting");
}

time_t Scheduler::getNextRunTimestamp() {
  _nextRunTimestamp = readTimestamp();
  Serial.printf("nextRunTimestamp: %s", ctime(&_nextRunTimestamp));
  // If we are much too late or the file wasn't there, wait for the next start of a minute
  if (time(nullptr) > _nextRunTimestamp + _measureIntervalSeconds) {
    _nextRunTimestamp = (time(nullptr)/60 + 1) * 60;
  Serial.printf("updated nextRunTimestamp to %s", ctime(&_nextRunTimestamp));
  }
  return _nextRunTimestamp;
}

time_t Scheduler::setNextRunTimestamp() {
  _nextRunTimestamp += _measureIntervalSeconds;
  writeTimestamp(_nextRunTimestamp);
  return _nextRunTimestamp;
}
