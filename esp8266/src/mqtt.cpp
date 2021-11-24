#include "mqtt.h"

#include "config.h"
#include "eeprom.h"
#include "serial_arduino.h"

#include <PubSubClient.h>
#include <Arduino.h>
#include <ESP8266WiFi.h>

WiFiClient espMqttClient;
PubSubClient mqttClient(espMqttClient);

bool mqttEnabled = false;

char mqttTopicSub[EEPROM_LEN_STRING];
char mqttTopicPub[EEPROM_LEN_STRING];
char mqttBroker[EEPROM_LEN_STRING];

void mqttCallback(char *topic, byte *payload, unsigned int len) {
    char str[len + 1];
    memcpy(str, payload, len);
    str[len] = 0;
    arduinoSerial.send(String(str));
}

bool mqttEnsureConnected() {
    if (!WiFi.isConnected()) {
        return false;
    }

    if (!mqttEnabled) {
        return false;
    }

    if (mqttClient.connected()) {
        return true;
    }

    mqttClient.setServer(mqttBroker, MQTT_PORT);
    mqttClient.setCallback(mqttCallback);
    String clientId = "esp-nixie-clock-dori-" + String(WiFi.macAddress());

    if (!mqttClient.connect(clientId.c_str(), eepromRead(EEPROM_MQTT_USERNAME).c_str(), eepromRead(EEPROM_MQTT_PASSWORD).c_str())) {
        arduinoSerial.echoFirst("MQTT could not connect: ");
        arduinoSerial.sendEnd(String(mqttClient.state()));
        return false;
    }

    arduinoSerial.echo("MQTT connected");

    mqttClient.subscribe(mqttTopicSub);
    return true;
}

void mqttInit() {
    strcpy(mqttBroker, eepromRead(EEPROM_MQTT_BROKER).c_str());

    if (strlen(mqttBroker) < 1) {
        return;
    }

    String _mqttTopic = eepromRead(EEPROM_MQTT_TOPIC);
    String _mqttTopicSub = _mqttTopic + "/in";
    strcpy(mqttTopicSub, _mqttTopicSub.c_str());
    String _mqttTopicPub = _mqttTopic + "/out";
    strcpy(mqttTopicPub, _mqttTopicPub.c_str());

    mqttEnabled = true;
    mqttEnsureConnected();
}

void mqttLoop() {
    mqttClient.loop();
}

void mqttSend(const String& data) {
    if (!mqttEnsureConnected()) {
        return;
    }
    mqttClient.publish(mqttTopicPub, data.c_str());
}
