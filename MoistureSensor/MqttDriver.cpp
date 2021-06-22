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
#include <PubSubClient.h>
#include "MqttDriver.h"
#include "secrets.h"

const char* BASE_TOPIC_TEMPLATE = "homie/%s/%s";
const char* TYPE_INTEGER = "integer";
const char* TYPE_FLOAT = "float";
const char* TYPE_DATETIME = "datetime";
const char* TYPE_STRING = "string";

// we should never see a TΩ
const char* RESISTANCE_RANGE = "0:1000000000000";
const int WILL_QOS = 1;
const bool WILL_RETAIN = true;
const bool MESSAGE_RETAIN = true;
const char* WILL_MESSAGE = "lost";
const char* PROPERTY_STATE = "$state";
const char* NODE_DEVICE = "device";

PubSubClient mqttClient;

bool MqttDriver::announceDevice() {
  char baseTopic[50];
  char payload[100];
  char sensorNumber[20];
  if (!publishEntity(_clientName, "$homie", "3.0.1")) {
      return false;
  }
  setState("init");
  publishEntity(_clientName, "$name", _clientName);
  strcpy(payload, NODE_DEVICE);
  for (int i = 0; i < _nodes; i++) {
      sprintf(sensorNumber,",%d",i);
      strcat(payload, sensorNumber);
  }
  publishEntity(_clientName, "$nodes", payload);
  publishEntity(_clientName, "$implementation", "esp8266");
  publishEntity(_clientName, "$extensions", "");
  sprintf(baseTopic, "%s/%s",_clientName, NODE_DEVICE);
  sprintf(payload, "%s,%s,%s", PROPERTY_MAC, PROPERTY_BUILD, PROPERTY_NEXTRUN);
  announceNode(baseTopic, NODE_DEVICE, NODE_DEVICE, payload);
  sprintf(payload, "%s,%s,%s,%s", PROPERTY_RAW, PROPERTY_RESISTANCE, PROPERTY_COMMENT, PROPERTY_SAMPLES);
  strcat(baseTopic, "/");
  strcat(baseTopic, PROPERTY_NEXTRUN);
  announceProperty(baseTopic, PROPERTY_NEXTRUN, TYPE_DATETIME, "", "");
  sprintf(baseTopic, "%s/%s/%s", _clientName, NODE_DEVICE, PROPERTY_MAC);
  announceProperty(baseTopic, PROPERTY_MAC, TYPE_STRING, "", "");
  sprintf(baseTopic, "%s/%s/%s", _clientName, NODE_DEVICE, PROPERTY_BUILD);
  announceProperty(baseTopic, PROPERTY_BUILD, TYPE_INTEGER, "", "");

  for (int i = 0; i < _nodes; i++) {
    sprintf(sensorNumber, "%i", i);
    sprintf(baseTopic, "%s/%s", _clientName, sensorNumber);
    announceNode(baseTopic, sensorNumber, "moisture sensor", payload);  
    strcat(baseTopic, "/");
    strcat(baseTopic, PROPERTY_RAW);
    announceProperty(baseTopic, PROPERTY_RAW, TYPE_INTEGER, "0-1024", "");
    sprintf(baseTopic, "%s/%s/%s", _clientName, sensorNumber, PROPERTY_RESISTANCE);
    announceProperty(baseTopic, PROPERTY_RESISTANCE, TYPE_INTEGER, RESISTANCE_RANGE, "Ω");
    sprintf(baseTopic, "%s/%s/%s", _clientName, sensorNumber, PROPERTY_COMMENT);
    announceProperty(baseTopic, PROPERTY_COMMENT, TYPE_STRING, "", "");    
    sprintf(baseTopic, "%s/%s/%s", _clientName, sensorNumber, PROPERTY_SAMPLES);
    announceProperty(baseTopic, PROPERTY_SAMPLES, TYPE_STRING, "", "");    
  }
  setState("ready");
  return true;
}

void MqttDriver::announceNode(const char* baseTopic, const char* name, const char* type, const char* properties) {
  publishEntity(baseTopic, "$name", name);
  publishEntity(baseTopic, "$type", type);
  publishEntity(baseTopic, "$properties", properties);
}

void MqttDriver::announceProperty(const char* baseTopic, const char* name, const char* dataType, const char* format, const char*  unit) {
  publishEntity(baseTopic, "$name", name);
  publishEntity(baseTopic, "$dataType", dataType);
  if (strlen(format) > 0) {
    publishEntity(baseTopic, "$format", format);
  }
  if (strlen(unit) > 0) {
    publishEntity(baseTopic, "$unit", unit);
  }
}

void MqttDriver::begin(Client* client, const char* clientName, int nodes) {
  mqttClient.setClient(*client);
  _clientName = clientName;
  mqttClient.setBufferSize(512);
  _nodes = nodes;

  mqttClient.setServer(CONFIG_MQTT_BROKER, CONFIG_MQTT_PORT);
  if (!connect()) {
    Serial.printf("Could not connect to MQTT broker: state %d\n", mqttClient.state());     
  }
}

bool MqttDriver::connect() {
  if (isConnected()) {
    return true;
  }
  sprintf(_topicBuffer, BASE_TOPIC_TEMPLATE, _clientName, PROPERTY_STATE);
  bool connectionSucceeded;
  if (strlen(SECRET_MQTT_USER) == 0) {
    connectionSucceeded = mqttClient.connect(CONFIG_DEVICE_NAME, _topicBuffer, WILL_QOS, WILL_RETAIN, WILL_MESSAGE);
  } else {
    connectionSucceeded = mqttClient.connect(CONFIG_DEVICE_NAME, SECRET_MQTT_USER, SECRET_MQTT_PASSWORD, _topicBuffer, WILL_QOS, WILL_RETAIN, WILL_MESSAGE);
  }
  if (connectionSucceeded) {
    if (!announceDevice()) {
      Serial.println("Could not announce device");
      connectionSucceeded = false; 
    }
  }  
  return connectionSucceeded;
}

void MqttDriver::disconnect() {
  if (isConnected()) {
    setState("disconnected");
    mqttClient.disconnect();    
  }
}

bool MqttDriver::isConnected() {
  return mqttClient.connected();
}

bool MqttDriver::publishEntity(const char* baseTopic, const char* entity, const char* payload) {
  if (!mqttClient.connected() && !connect()) {
    return false;
  }
  sprintf(_topicBuffer, BASE_TOPIC_TEMPLATE, baseTopic, entity);
  return mqttClient.publish(_topicBuffer, payload, MESSAGE_RETAIN);
}

bool MqttDriver::processMessages() {
  return mqttClient.loop();
}

void MqttDriver::publishDeviceProperty(const char* propertyName, const char* payload) {
  char baseTopic[50];
  sprintf(baseTopic, "%s/%s", _clientName, NODE_DEVICE);
  publishEntity(baseTopic, propertyName, payload);
}

void MqttDriver::publishProperty(int nodeNumber, const char* property, const char* payload) {
  char baseTopic[50];
  sprintf(baseTopic, "%s/%d", _clientName, nodeNumber);
  if (!publishEntity(baseTopic, property, payload)) {
    Serial.printf("Could not publish %s: %s\n", property, payload);
  }
}

void MqttDriver::setState(const char* state) {
    publishEntity(_clientName, PROPERTY_STATE, state);
}
