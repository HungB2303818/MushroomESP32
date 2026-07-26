#ifndef MQTT_SERVICE_H
#define MQTT_SERVICE_H

#include <Arduino.h>

namespace MqttService
{

    using MessageHandler = void (*)(
        const String &topic,
        const String &payload);

    void begin(MessageHandler handler);
    void loop();

    bool isConnected();

    bool publish(
        const char *topic,
        const String &payload,
        uint8_t qos,
        bool retain);

}

#endif