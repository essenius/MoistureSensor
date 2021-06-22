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
#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include "WifiDriver.h"

BearSSL::WiFiClientSecure wifiClient;
BearSSL::X509List caCert(CONFIG_ROOTCA_CERTIFICATE);
BearSSL::X509List clientCert(CONFIG_DEVICE_CERTIFICATE);
BearSSL::PrivateKey clientKey(SECRET_DEVICE_PRIVATE_KEY);

void WifiDriver::begin() {
  WiFi.mode(WIFI_STA);
  wifiClient.setClientRSACert(&clientCert, &clientKey);
  wifiClient.setTrustAnchors(&caCert);
  if (!WiFi.hostname(CONFIG_DEVICE_NAME)) {
    Serial.println("Could not set host name");
  }
  WiFi.begin(SECRET_SSID, SECRET_WIFI_PASSWORD);
  Serial.print("Connecting");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }  
}

WiFiClient* WifiDriver::client() {
  return &wifiClient;
}

const char* WifiDriver::macAddress() {
  uint8_t mac[6];
  WiFi.macAddress(mac);
  sprintf(_macAddress, "%02X%02X%02X%02X%02X%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]); 
  return _macAddress; 
}

void WifiDriver::printStatus() {
   Serial.print("Connected to SSID: ");
  Serial.print(WiFi.SSID());
  IPAddress ip = WiFi.localIP();
  Serial.print(", IP: ");
  Serial.print(ip);
  Serial.print(", name: ");
  Serial.println(WiFi.hostname());
  Serial.printf("Mac address: %s\n", macAddress()); 
}
