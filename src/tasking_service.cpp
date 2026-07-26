#include "tasking_service.h"

#include <ArduinoJson.h>

#include "actuator_service.h"
#include "app_config.h"
#include "mqtt_service.h"
#include "time_service.h"

namespace TaskingService
{

    constexpr size_t TASK_HISTORY_SIZE = 10;

    static long processedTaskIds[TASK_HISTORY_SIZE] = {};
    static size_t historyIndex = 0;

    static bool wasProcessed(long taskId)
    {
        for (long savedTaskId : processedTaskIds)
        {
            if (savedTaskId == taskId)
            {
                return true;
            }
        }

        return false;
    }

    static void rememberTask(long taskId)
    {
        processedTaskIds[historyIndex] = taskId;

        historyIndex =
            (historyIndex + 1) %
            TASK_HISTORY_SIZE;
    }

    static const char *getStatusTopic(
        const String &deviceCode)
    {
        /*
         * Buffer static để chuỗi vẫn tồn tại
         * sau khi hàm kết thúc.
         */
        static char statusTopic[AppConfig::Mqtt::TOPIC_MAX_LENGTH];

        const AppConfig::ActuatorConfig *actuator =
            AppConfig::findActuator(
                deviceCode.c_str());

        if (actuator == nullptr)
        {
            Serial.print(
                "Unknown actuator device_code: ");

            Serial.println(deviceCode);

            return nullptr;
        }

        if (!actuator->enabled)
        {
            Serial.print(
                "Actuator is disabled: ");

            Serial.println(deviceCode);

            return nullptr;
        }

        const bool topicCreated =
            AppConfig::buildActuatorStatusTopic(
                statusTopic,
                sizeof(statusTopic),
                actuator->deviceCode);

        if (!topicCreated)
        {
            Serial.print(
                "Failed to build status topic: ");

            Serial.println(deviceCode);

            return nullptr;
        }

        return statusTopic;
    }

    static void publishStatus(
        long taskId,
        const String &deviceCode,
        const String &currentValue,
        const char *status,
        const char *errorCode = nullptr)
    {
        const char *statusTopic =
            getStatusTopic(deviceCode);

        if (statusTopic == nullptr)
        {
            return;
        }

        JsonDocument doc;

        doc["task_id"] = taskId;
        doc["device_code"] = deviceCode;
        doc["capability_code"] = "ON_OFF";
        doc["current_value"] = currentValue;
        doc["status"] = status;
        doc["reported_at"] =
            TimeService::getIso8601();

        if (errorCode != nullptr)
        {
            doc["error_code"] = errorCode;
        }

        String response;
        serializeJson(doc, response);

        MqttService::publish(
            statusTopic,
            response,
            1,
            true);
    }

    void handleMessage(
        const String &topic,
        const String &payload)
    {
        JsonDocument doc;

        const DeserializationError error =
            deserializeJson(doc, payload);

        if (error)
        {
            Serial.println("Invalid task JSON");
            return;
        }

        const long taskId = doc["task_id"] | -1;

        const String deviceCode =
            doc["device_code"] | "";

        const String capabilityCode =
            doc["capability_code"] | "";

        const String targetValue =
            doc["target_value"] | "";

        if (
            taskId <= 0 ||
            deviceCode.isEmpty() ||
            capabilityCode.isEmpty() ||
            targetValue.isEmpty())
        {
            Serial.println("Missing task fields");
            return;
        }

        if (capabilityCode != "ON_OFF")
        {
            publishStatus(
                taskId,
                deviceCode,
                ActuatorService::getState(deviceCode),
                "FAILED",
                "UNSUPPORTED_CAPABILITY");
            return;
        }

        if (wasProcessed(taskId))
        {
            publishStatus(
                taskId,
                deviceCode,
                ActuatorService::getState(deviceCode),
                "COMPLETED");
            return;
        }

        const ActuatorService::Result result =
            ActuatorService::setState(
                deviceCode,
                targetValue);

        if (
            result !=
            ActuatorService::Result::SUCCESS)
        {
            publishStatus(
                taskId,
                deviceCode,
                ActuatorService::getState(deviceCode),
                "FAILED",
                "ACTUATOR_CONTROL_FAILED");
            return;
        }

        rememberTask(taskId);

        publishStatus(
            taskId,
            deviceCode,
            targetValue,
            "COMPLETED");

        Serial.printf(
            "Task %ld completed: %s -> %s\n",
            taskId,
            deviceCode.c_str(),
            targetValue.c_str());
    }

}