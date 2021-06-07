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

// This class connects to Wifi and prepares for using TLS connections
// The assumption is that you have your own Root CA certificate that has signed the certificates of the devices and the hosts you use.
// This way, you can use TLS without swithching on the insecure flag.
// The interface hides the TLS complexity by exposing a normal WiFiClient that MQTTDriver and FirmwareManager can use.
// So should you e.g. want to use normal HTTP instead, all you need to change is this class.

#ifndef HEADER_WIFIDRIVER
#define HEADER_WIFIDRIVER

#include "secrets.h"
#include "WiFiClient.h"

class WifiDriver {
public:
    void begin();
    WiFiClient* client();
    void printStatus();
};
#endif
