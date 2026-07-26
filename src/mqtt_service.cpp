#include "mqtt_service.h"

#include <Arduino.h>
#include <ArduinoMqttClient.h>
#include <WiFi.h>

#include "app_config.h"
#include "secrets.h"
#include "wifi_service.h"

namespace MqttService
{
    static WiFiClient wifiClient;
    static MqttClient mqttClient(wifiClient);

    static MessageHandler messageHandler = nullptr;

    static unsigned long lastReconnectAttempt = 0;

    // =====================================================
    // SUBSCRIBE COMMAND TOPIC CỦA MỘT ACTUATOR
    //
    // Topic:
    // actuator/{device_code}/command
    // =====================================================

    static bool subscribeActuatorCommand(
        const AppConfig::ActuatorConfig &actuator)
    {
        if (!actuator.enabled)
        {
            return true;
        }

        char commandTopic[AppConfig::Mqtt::TOPIC_MAX_LENGTH];

        const bool topicCreated =
            AppConfig::buildActuatorCommandTopic(
                commandTopic,
                sizeof(commandTopic),
                actuator.deviceCode);

        if (!topicCreated)
        {
            Serial.print(
                "Failed to build command topic for: ");

            Serial.println(actuator.deviceCode);

            return false;
        }

        const int result =
            mqttClient.subscribe(
                commandTopic,
                AppConfig::Mqtt::COMMAND_QOS);

        if (result == 0)
        {
            Serial.print(
                "MQTT subscribe failed [");

            Serial.print(commandTopic);
            Serial.println("]");

            return false;
        }

        Serial.print("Subscribed [");
        Serial.print(commandTopic);
        Serial.println("]");

        return true;
    }

    // =====================================================
    // SUBSCRIBE TOÀN BỘ COMMAND TOPIC
    // =====================================================

    static void subscribeCommandTopics()
    {
        Serial.println(
            "Subscribing actuator command topics...");

        size_t subscribedCount = 0;
        size_t failedCount = 0;

        for (
            size_t i = 0;
            i < AppConfig::ACTUATOR_COUNT;
            i++)
        {
            const AppConfig::ActuatorConfig &actuator =
                AppConfig::ACTUATORS[i];

            if (!actuator.enabled)
            {
                Serial.print(
                    "Skip disabled actuator: ");

                Serial.println(
                    actuator.deviceCode);

                continue;
            }

            if (subscribeActuatorCommand(actuator))
            {
                subscribedCount++;
            }
            else
            {
                failedCount++;
            }
        }

        Serial.print(
            "MQTT command topics subscribed: ");

        Serial.println(subscribedCount);

        if (failedCount > 0)
        {
            Serial.print(
                "MQTT command subscribe failures: ");

            Serial.println(failedCount);
        }
    }

    // =====================================================
    // XỬ LÝ MESSAGE MQTT NHẬN ĐƯỢC
    // =====================================================

    static void onMessage(
        int messageSize)
    {
        /*
         * messageSize hiện chưa cần sử dụng vì payload
         * được đọc đến khi mqttClient.available() = false.
         */
        (void)messageSize;

        const String topic =
            mqttClient.messageTopic();

        String payload;

        /*
         * Hạn chế việc cấp phát lại bộ nhớ String
         * khi đọc payload.
         */
        if (messageSize > 0)
        {
            payload.reserve(
                static_cast<unsigned int>(
                    messageSize));
        }

        while (mqttClient.available())
        {
            payload += static_cast<char>(
                mqttClient.read());
        }

        Serial.println();
        Serial.print("MQTT topic: ");
        Serial.println(topic);

        Serial.print("MQTT payload: ");
        Serial.println(payload);

        if (messageHandler == nullptr)
        {
            Serial.println(
                "MQTT message handler is not configured");

            return;
        }

        messageHandler(
            topic,
            payload);
    }

    // =====================================================
    // KẾT NỐI MQTT BROKER
    // =====================================================

    static bool connectBroker()
    {
        /*
         * Chỉ kết nối MQTT khi Wi-Fi đã hoạt động.
         */
        if (!WifiService::isConnected())
        {
            Serial.println(
                "MQTT skipped: WiFi not connected");

            return false;
        }

        /*
         * Không kết nối lại khi MQTT đang hoạt động.
         */
        if (mqttClient.connected())
        {
            return true;
        }

        mqttClient.setId(
            AppConfig::Mqtt::CLIENT_ID);

        Serial.print(
            "Connecting MQTT broker: ");

        Serial.print(
            AppConfig::Mqtt::HOST);

        Serial.print(":");

        Serial.println(
            AppConfig::Mqtt::PORT);

        const bool connected =
            mqttClient.connect(
                AppConfig::Mqtt::HOST,
                AppConfig::Mqtt::PORT);

        if (!connected)
        {
            Serial.print(
                "MQTT connection failed, error code: ");

            Serial.println(
                mqttClient.connectError());

            return false;
        }

        Serial.println("MQTT connected");

        /*
         * Sau mỗi lần reconnect, broker không tự giữ
         * subscription cũ nếu clean session được sử dụng.
         */
        subscribeCommandTopics();

        return true;
    }

    // =====================================================
    // KHỞI TẠO MQTT SERVICE
    // =====================================================

    void begin(
        MessageHandler handler)
    {
        messageHandler = handler;

        mqttClient.onMessage(
            onMessage);

        Serial.print("MQTT client ID: ");
        Serial.println(
            AppConfig::Mqtt::CLIENT_ID);

        Serial.println(
            "MQTT service initialized");
    }

    // =====================================================
    // VÒNG LẶP MQTT
    // =====================================================

    void loop()
    {
        if (!WifiService::isConnected())
        {
            return;
        }

        /*
         * poll() phải được gọi thường xuyên để:
         * - nhận message;
         * - duy trì kết nối;
         * - xử lý keep-alive.
         */
        if (mqttClient.connected())
        {
            mqttClient.poll();
            return;
        }

        const unsigned long now =
            millis();

        /*
         * Phép trừ unsigned long vẫn hoạt động đúng
         * khi millis() bị tràn.
         */
        if (
            now - lastReconnectAttempt <
            AppConfig::Timing::MQTT_RECONNECT_MS)
        {
            return;
        }

        lastReconnectAttempt = now;

        connectBroker();
    }

    // =====================================================
    // KIỂM TRA TRẠNG THÁI KẾT NỐI
    // =====================================================

    bool isConnected()
    {
        return mqttClient.connected();
    }

    // =====================================================
    // PUBLISH MQTT
    //
    // Hàm dùng chung cho:
    // - sensing;
    // - actuator status;
    // - availability nếu bổ sung sau.
    // =====================================================

    bool publish(
        const char *topic,
        const String &payload,
        uint8_t qos,
        bool retain)
    {
        if (!mqttClient.connected())
        {
            Serial.println(
                "MQTT publish skipped: not connected");

            return false;
        }

        if (
            topic == nullptr ||
            topic[0] == '\0')
        {
            Serial.println(
                "MQTT publish failed: empty topic");

            return false;
        }

        const bool started =
            mqttClient.beginMessage(
                topic,
                retain,
                qos);

        if (!started)
        {
            Serial.print(
                "MQTT beginMessage failed [");

            Serial.print(topic);
            Serial.println("]");

            return false;
        }

        mqttClient.print(payload);

        const int result =
            mqttClient.endMessage();

        if (result != 1)
        {
            Serial.print(
                "MQTT publish failed [");

            Serial.print(topic);
            Serial.println("]");

            return false;
        }

        return true;
    }
}