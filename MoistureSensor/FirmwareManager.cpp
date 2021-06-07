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
#include <ESP8266HTTPClient.h>
#include <ESP8266httpUpdate.h>
#include <WiFiClient.h>

#include "FirmwareManager.h" 

const char* VERSION_EXTENSION = ".version";
const char* IMAGE_EXTENSION = ".bin";

void FirmwareManager::begin(WiFiClient* client, const char* baseUrl, const char* machineId) {
  _client = client;
  strcpy(_baseUrl, baseUrl);
  strcat(_baseUrl, machineId);
}

bool FirmwareManager::updateAvailableFor(int currentVersion) {
  char versionUrl[BASE_URL_SIZE];
  strcpy(versionUrl, _baseUrl);
  strcat(versionUrl, VERSION_EXTENSION);
  Serial.printf("Firmware version URL: %s\n", versionUrl);

  HTTPClient httpClient;
  httpClient.begin(*_client, versionUrl);
  bool returnValue = false;
  int httpCode = httpClient.GET();
  if (httpCode == 200) {
    int newVersion = httpClient.getString().toInt();
    Serial.printf("Current firmware version: %d; available version: %d\n", currentVersion, newVersion);
    returnValue = newVersion > currentVersion;
  } else {
    Serial.printf("Firmware version check failed with response code %d\n", httpCode);
  }
  httpClient.end();
  return returnValue;
}

void FirmwareManager::update() {
  Serial.println("Updating firmware");
  char imageUrl[BASE_URL_SIZE];
  strcpy(imageUrl, _baseUrl);
  strcat(imageUrl, IMAGE_EXTENSION);

  t_httpUpdate_return returnValue = ESPhttpUpdate.update(*_client, imageUrl);

  switch(returnValue) {
    case HTTP_UPDATE_FAILED:
      Serial.printf("HTTP_UPDATE_FAILED Error (%d): %s\n", ESPhttpUpdate.getLastError(), ESPhttpUpdate.getLastErrorString().c_str());
      break;

    case HTTP_UPDATE_NO_UPDATES:
      Serial.println("HTTP_UPDATE_NO_UPDATES");
      break;
  }
}

void FirmwareManager::tryUpdateFrom(int currentVersion) {
    if (updateAvailableFor(currentVersion)) {
    update();
  } else {
    Serial.println("No updates available");
  }
}
