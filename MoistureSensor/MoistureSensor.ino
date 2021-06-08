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

// Detect moisture in e.g. gypsum walls using a NodeMCU 8266 (Amica).
// Screw 2 screws in the wall you want to measure at about 3 cm distance, and wire them.
// Dry gypsum walls should have a very high resistance (well over 1 MΩ), and lower resistance implies more moisture. 
// Thanks to Enam (https://forum.arduino.cc/index.php?topic=195258.0) for the inspiration.

// After the measurement, it sends the results over MQTT and goes into deep sleep until the next measurement.
// Also connect D0 (GPIO16/WAKE) to Reset (RST) to enable wake up from deep sleep (remove while uploading).
// This implies you cannot use LED_BUILTIN_AUX as that's D0 too - switching it on (LOW) would reset the device.

#include "secrets.h"
// Contains the configuration data and secrets we don't want to share in GitHub. Contents:
//   #ifndef SECRETS_H
//   #define SECRETS_H
//   static const char* SECRET_SSID = "wifi-ssid";
//   static const char* SECRET_WIFI_PASSWORD = "wifi-password";
//   static const char* CONFIG_DEVICE_NAME = "name-of-this-device";
//   static const char* CONFIG_MQTT_BROKER = "name-of-mqtt-broker";
//   static const int   CONFIG_MQTT_PORT = mqtt-broker-port;
//   static const char* SECRET_MQTT_USER = "mqtt-user";
//   static const char* SECRET_MQTT_PASSWORD = "mqtt-ueer-password";
//   static const char* CONFIG_BASE_FIRMWARE_URL = "URL-of-OTA-images-with-trailing-slash/";
//   static const char* CONFIG_ROOTCA_CERTIFICATE PROGMEM = R"rootca(
//   -----BEGIN CERTIFICATE-----
//   Encoded Root CA certificate
//   -----END CERTIFICATE-----
//   )rootca";
//   static const char* CONFIG_DEVICE_CERTIFICATE PROGMEM = R"certdef(
//   -----BEGIN CERTIFICATE-----
//   Encoded device certificate
//   -----END CERTIFICATE-----
//   )certdef";
//   static const char* SECRET_DEVICE_PRIVATE_KEY PROGMEM = R"certkey(
//   -----BEGIN PRIVATE KEY-----
//   Encoded device private key
//   -----END PRIVATE KEY-----
//   )certkey";
//   #endif

#include "WifiDriver.h"
#include "MqttDriver.h"
#include "FirmwareManager.h"
#include "Scheduler.h"
#include "SensorManager.h"

Scheduler scheduler;
SensorManager sensorManager;
FirmwareManager firmwareManager;
WifiDriver wifiDriver;
MqttDriver mqttDriver;

const int BUILD_NUMBER = 14;
const int SENSOR_COUNT = 2;
const long MEASURE_INTERVAL_SECONDS = 900;

int sensorValue;
float V_out;
float R_wall;
float smoothSensorValue = 0.0;
unsigned long startTime;
time_t nextRunTimestamp;

void publishNextRun(time_t nextRunTimestamp) {
  Serial.printf("Next run:     %s", ctime(&nextRunTimestamp));
  const int BUFFER_SIZE = 30;
  char dateBuffer[BUFFER_SIZE];
  strftime(dateBuffer, BUFFER_SIZE, "%FT%TZ", gmtime(&nextRunTimestamp));
  mqttDriver.publishDeviceProperty(PROPERTY_NEXTRUN, dateBuffer);    
}

void waitCallback() {
  digitalWrite(LED_BUILTIN, time(nullptr) % 2 == 0);
  // keep the MQTT connection active
  mqttDriver.processMessages();  
}

void setup() {
  startTime = micros();
  Serial.begin(115200);
  delay(250);
  sensorManager.begin(SENSOR_COUNT);
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);
  wifiDriver.begin();
  wifiDriver.printStatus();
  scheduler.begin(MEASURE_INTERVAL_SECONDS); 
  mqttDriver.begin(wifiDriver.client(), CONFIG_DEVICE_NAME, SENSOR_COUNT); 
  if (!mqttDriver.isConnected()) {
    Serial.println("Could not connect to MQTT broker. Rebooting...");
    ESP.restart();
  }
  Serial.printf("Build: %d\n", BUILD_NUMBER);
  char buildString[20];
  sprintf(buildString, "%d", BUILD_NUMBER);
  mqttDriver.publishDeviceProperty(PROPERTY_BUILD, buildString);

  // The clock during deep sleep is not very accurate. Wait for the right time to start measuring
  nextRunTimestamp = scheduler.getNextRunTimestamp();
  publishNextRun(nextRunTimestamp);
  scheduler.waitForNextRun(waitCallback);
}

void loop() {
  if (mqttDriver.connect()) {
    for (int sensorNumber = 0; sensorNumber < SENSOR_COUNT; sensorNumber++) {
      sensorManager.read(sensorNumber);      
      sensorManager.printResult();
      
      // Send the results over MQTT
      char numberBuffer[20];
      sprintf(numberBuffer, "%.1f", sensorManager.pinValue());
      mqttDriver.publishProperty(sensorNumber, PROPERTY_RAW, numberBuffer);
      sprintf(numberBuffer, "%.0f", sensorManager.resistance());
      mqttDriver.publishProperty(sensorNumber, PROPERTY_RESISTANCE, numberBuffer);
      // keep the MQTT connection active
      mqttDriver.processMessages();
    }
  }
  nextRunTimestamp = scheduler.setNextRunTimestamp();
  publishNextRun(nextRunTimestamp);
  mqttDriver.disconnect();
  firmwareManager.begin(wifiDriver.client(), CONFIG_BASE_FIRMWARE_URL, CONFIG_DEVICE_NAME);
  firmwareManager.tryUpdateFrom(BUILD_NUMBER);
  scheduler.waitForNextRun(waitCallback);
}
