#include "actuator_service.h"

#include <Arduino.h>

#include "app_config.h"

namespace ActuatorService
{
    /*
     * Lưu trạng thái logic của từng actuator.
     *
     * false = OFF
     * true  = ON
     *
     * Thứ tự phần tử tương ứng với AppConfig::ACTUATORS.
     */
    static bool actuatorStates[AppConfig::ACTUATOR_COUNT] = {};

    // =====================================================
    // TÌM VỊ TRÍ ACTUATOR TRONG DANH SÁCH CẤU HÌNH
    // =====================================================

    static int findActuatorIndex(
        const String &deviceCode)
    {
        String normalizedCode = deviceCode;

        normalizedCode.trim();
        normalizedCode.toUpperCase();

        for (
            size_t i = 0;
            i < AppConfig::ACTUATOR_COUNT;
            i++)
        {
            const AppConfig::ActuatorConfig &actuator =
                AppConfig::ACTUATORS[i];

            if (
                normalizedCode.equals(
                    actuator.deviceCode))
            {
                return static_cast<int>(i);
            }
        }

        return -1;
    }

    // =====================================================
    // KHỞI TẠO TOÀN BỘ ACTUATOR
    // =====================================================

    void begin()
    {
        Serial.println();
        Serial.println(
            "Initializing actuator service...");

        for (
            size_t i = 0;
            i < AppConfig::ACTUATOR_COUNT;
            i++)
        {
            const AppConfig::ActuatorConfig &actuator =
                AppConfig::ACTUATORS[i];

            /*
             * Trạng thái logic ban đầu luôn là OFF.
             */
            actuatorStates[i] = false;

            if (!actuator.enabled)
            {
                Serial.print(
                    "Skip disabled actuator: ");

                Serial.println(
                    actuator.deviceCode);

                continue;
            }

            pinMode(
                actuator.pin,
                OUTPUT);

            /*
             * Đưa actuator về trạng thái OFF an toàn.
             *
             * Hàm getActuatorOffLevel() tự xử lý:
             * - Relay active-high.
             * - Relay active-low.
             */
            digitalWrite(
                actuator.pin,
                AppConfig::getActuatorOffLevel(
                    actuator));

            Serial.print("Actuator initialized: ");
            Serial.print(actuator.deviceCode);

            Serial.print(" | GPIO ");
            Serial.print(actuator.pin);

            Serial.print(" | Initial state: OFF");

            Serial.print(" | Active ");
            Serial.println(
                actuator.activeHigh
                    ? "HIGH"
                    : "LOW");
        }

        Serial.print("Configured actuators: ");
        Serial.println(
            AppConfig::ACTUATOR_COUNT);

        Serial.println(
            "Actuator service initialized");
    }

    // =====================================================
    // THAY ĐỔI TRẠNG THÁI ACTUATOR
    // =====================================================

    Result setState(
        const String &deviceCode,
        const String &targetValue)
    {
        String normalizedTarget = targetValue;

        normalizedTarget.trim();
        normalizedTarget.toUpperCase();

        /*
         * Hiện tại capability ON_OFF chỉ cho phép
         * hai giá trị ON và OFF.
         */
        if (
            normalizedTarget != "ON" &&
            normalizedTarget != "OFF")
        {
            Serial.print(
                "Invalid actuator target value: ");

            Serial.println(targetValue);

            return Result::INVALID_VALUE;
        }

        const int actuatorIndex =
            findActuatorIndex(deviceCode);

        if (actuatorIndex < 0)
        {
            Serial.print(
                "Unknown actuator device_code: ");

            Serial.println(deviceCode);

            return Result::UNKNOWN_DEVICE;
        }

        const AppConfig::ActuatorConfig &actuator =
            AppConfig::ACTUATORS[actuatorIndex];

        if (!actuator.enabled)
        {
            Serial.print(
                "Actuator is disabled: ");

            Serial.println(
                actuator.deviceCode);

            /*
             * Giữ nguyên enum Result hiện tại.
             * Nếu muốn phân biệt, có thể bổ sung
             * Result::DEVICE_DISABLED sau.
             */
            return Result::UNKNOWN_DEVICE;
        }

        const bool turnOn =
            normalizedTarget == "ON";

        const uint8_t outputLevel =
            turnOn
                ? AppConfig::getActuatorOnLevel(
                      actuator)
                : AppConfig::getActuatorOffLevel(
                      actuator);

        digitalWrite(
            actuator.pin,
            outputLevel);

        actuatorStates[actuatorIndex] = turnOn;

        Serial.print("Actuator state changed: ");
        Serial.print(actuator.deviceCode);

        Serial.print(" -> ");
        Serial.print(
            turnOn
                ? "ON"
                : "OFF");

        Serial.print(" | GPIO level: ");
        Serial.println(outputLevel);

        return Result::SUCCESS;
    }

    // =====================================================
    // LẤY TRẠNG THÁI HIỆN TẠI
    // =====================================================

    String getState(
        const String &deviceCode)
    {
        const int actuatorIndex =
            findActuatorIndex(deviceCode);

        if (actuatorIndex < 0)
        {
            return "UNKNOWN";
        }

        const AppConfig::ActuatorConfig &actuator =
            AppConfig::ACTUATORS[actuatorIndex];

        if (!actuator.enabled)
        {
            return "UNKNOWN";
        }

        return actuatorStates[actuatorIndex]
                   ? "ON"
                   : "OFF";
    }
}