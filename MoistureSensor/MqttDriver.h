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

// This class implements the homie convention (https://homieiot.github.io/specification/) 
// to allow home automation systems like OpenHAB to autodetect it.

#ifndef HEADER_MQTTDRIVER
#define HEADER_MQTTDRIVER
#include "Client.h"

static const char* PROPERTY_RAW = "raw";
//static const char* PROPERTY_VOLTAGE = "voltage";
static const char* PROPERTY_RESISTANCE = "resistance";
static const char* PROPERTY_NEXTRUN = "next-run";
static const char* PROPERTY_BUILD = "build"; 

class MqttDriver {
public:
    void begin(Client* client, const char* clientName, int nodes);
    bool connect();
    void disconnect();
    bool isConnected();
    bool processMessages();
    void publishDeviceProperty(const char* propertyName, const char* payload);
    void publishProperty(int nodeNumber, const char* property, const char* payload);
    void setState(const char* state);

protected:
    const char* _clientName = 0;
    int _nodes = 1;
    static const int TOPIC_BUFFER_SIZE = 100;
    char _topicBuffer[TOPIC_BUFFER_SIZE] = { 0 };

    bool announceDevice();
    void announceNode(const char* baseTopic, const char* name, const char* type, const char* properties);
    void announceProperty(const char* baseTopic, const char* name, const char* dataType, const char* format, const char* unit);
    void callback(const char* topic, byte* payload, unsigned int length);
    bool publishEntity(const char* baseTopic, const char* entity, const char* payload);
};

#endif
