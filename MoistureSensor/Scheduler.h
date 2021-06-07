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

// This class takes care of 
// * retrieving the next run time from a file
// * calculating the next run time based on the previous one (or taking the next start of a minute if absent or too long ago)
// * saving the next run time into the file
// * waiting for the next run time. It will do that via deep sleep if the wait is long enough (configured as a minute).

#ifndef HEADER_SCHEDULER
#define HEADER_SCHEDULER
#include <functional>

class Scheduler {
public:
    void begin(long measureIntervalSeconds);
    time_t getNextRunTimestamp();
    time_t setNextRunTimestamp();
    void waitForNextRun(std::function<void(void)> callback);
private:
    long _measureIntervalSeconds;
    time_t _nextRunTimestamp;
    void writeTimestamp(time_t timestamp);
    time_t readTimestamp();
};
#endif
