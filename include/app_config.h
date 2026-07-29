#ifndef APP_CONFIG_H
#define APP_CONFIG_H

#include <Arduino.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

namespace AppConfig
{
    // =====================================================
    // MQTT
    // =====================================================

    namespace Mqtt
    {
        // constexpr char HOST[] = "10.13.146.19";
        constexpr char HOST[] = "192.168.1.157";

        constexpr uint16_t PORT = 1883;

        /*
         * Đây là mã của ESP32 MQTT client,
         * không phải device_code của sensor hoặc actuator.
         */
        constexpr char CLIENT_ID[] = "ESP32_MUSHROOM_01";

        constexpr uint8_t SENSOR_QOS = 0;
        constexpr uint8_t COMMAND_QOS = 1;
        constexpr uint8_t STATUS_QOS = 1;

        constexpr bool SENSOR_RETAIN = false;
        constexpr bool COMMAND_RETAIN = false;
        constexpr bool STATUS_RETAIN = true;

        constexpr size_t TOPIC_MAX_LENGTH = 100;
    }

    // =====================================================
    // THỜI GIAN
    // =====================================================

    namespace Timing
    {
        constexpr unsigned long WIFI_RECONNECT_MS = 5000;
        constexpr unsigned long MQTT_RECONNECT_MS = 5000;

        constexpr unsigned long DEFAULT_SENSING_INTERVAL_MS =
            30000;
    }

    // =====================================================
    // KHAI BÁO CHÂN GPIO
    // =====================================================

    namespace Pins
    {
        constexpr uint8_t DHT11_SENSOR = 33;
        constexpr uint8_t LIGHT_SENSOR = 35;
        constexpr uint8_t MOISTURE_SENSOR = 34;

        constexpr uint8_t MIST_PUMP = 25;
        constexpr uint8_t VENT_FAN = 27;
        constexpr uint8_t GROW_LIGHT = 26;
    }

    // =====================================================
    // LOẠI SENSOR
    // =====================================================

    enum class SensorDriver : uint8_t
    {
        DHT11,
        ANALOG_LIGHT,
        ANALOG_MOISTURE
    };

    // =====================================================
    // LOẠI ACTUATOR
    // =====================================================

    enum class ActuatorType : uint8_t
    {
        PUMP,
        FAN,
        LIGHT
    };

    // =====================================================
    // CẤU HÌNH ĐẠI LƯỢNG ĐO
    // =====================================================

    struct MeasurementConfig
    {
        const char *observedProperty;
        const char *unitSymbol;

        float minimumValue;
        float maximumValue;
    };

    // =====================================================
    // CẤU HÌNH SENSOR
    // =====================================================

    struct SensorConfig
    {
        const char *deviceCode;
        const char *name;

        SensorDriver driver;

        uint8_t pin;
        bool enabled;

        unsigned long publishIntervalMs;

        const MeasurementConfig *measurements;
        size_t measurementCount;

        /*
         * Hai mốc hiệu chuẩn dành cho cảm biến analog:
         *
         * rawAtZeroPercent:
         * Giá trị ADC tương ứng 0%.
         *
         * rawAtHundredPercent:
         * Giá trị ADC tương ứng 100%.
         *
         * DHT11 không sử dụng hai giá trị này.
         */
        int rawAtZeroPercent;
        int rawAtHundredPercent;
    };

    // =====================================================
    // CẤU HÌNH ACTUATOR
    // =====================================================

    struct ActuatorConfig
    {
        const char *deviceCode;
        const char *name;

        ActuatorType type;

        uint8_t pin;

        /*
         * true:
         * HIGH = ON, LOW = OFF.
         *
         * false:
         * LOW = ON, HIGH = OFF.
         */
        bool activeHigh;

        bool enabled;
    };

    // =====================================================
    // ĐẠI LƯỢNG CỦA DHT11
    // =====================================================

    constexpr MeasurementConfig DHT11_MEASUREMENTS[] = {
        {"temperature",
         "°C",
         -20.0f,
         80.0f},
        {"humidity",
         "%",
         0.0f,
         100.0f}};

    // =====================================================
    // ĐẠI LƯỢNG CẢM BIẾN ÁNH SÁNG
    // =====================================================

    constexpr MeasurementConfig LIGHT_MEASUREMENTS[] = {
        {"light_level",
         "%",
         0.0f,
         100.0f}};

    // =====================================================
    // ĐẠI LƯỢNG ĐỘ ẨM GIÁ THỂ
    // =====================================================

    constexpr MeasurementConfig MOISTURE_MEASUREMENTS[] = {
        {"substrate_moisture",
         "%",
         0.0f,
         100.0f}};

    // =====================================================
    // DANH SÁCH SENSOR
    // =====================================================

    constexpr SensorConfig SENSORS[] = {
        {"SENSOR_DHT11_01",
         "Cảm biến nhiệt độ và độ ẩm",

         SensorDriver::DHT11,

         Pins::DHT11_SENSOR,
         true,

         Timing::DEFAULT_SENSING_INTERVAL_MS,

         DHT11_MEASUREMENTS,
         sizeof(DHT11_MEASUREMENTS) /
             sizeof(DHT11_MEASUREMENTS[0]),

         0,
         0},

        {"SENSOR_LIGHT_01",
         "Cảm biến cường độ ánh sáng",

         SensorDriver::ANALOG_LIGHT,

         Pins::LIGHT_SENSOR,
         true,

         Timing::DEFAULT_SENSING_INTERVAL_MS,

         LIGHT_MEASUREMENTS,
         sizeof(LIGHT_MEASUREMENTS) /
             sizeof(LIGHT_MEASUREMENTS[0]),

         /*
          * Các giá trị này chỉ là giá trị ban đầu.
          * Cần đo thực tế để hiệu chuẩn.
          */
         4095,
         200},

        {"SENSOR_MOISTURE_01",
         "Cảm biến độ ẩm giá thể",

         SensorDriver::ANALOG_MOISTURE,

         /*
          * Đổi GPIO35 thành chân thực tế.
          */
         Pins::MOISTURE_SENSOR,

         /*
          * Để false cho đến khi đã nối cảm biến.
          */
         true,

         Timing::DEFAULT_SENSING_INTERVAL_MS,

         MOISTURE_MEASUREMENTS,
         sizeof(MOISTURE_MEASUREMENTS) /
             sizeof(MOISTURE_MEASUREMENTS[0]),

         /*
          * Ví dụ:
          * 3500 tương ứng khô 0%.
          * 1300 tương ứng ẩm 100%.
          *
          * Cần hiệu chuẩn lại theo giá thể thực tế.
          */
         3500,
         1300}};

    constexpr size_t SENSOR_COUNT =
        sizeof(SENSORS) / sizeof(SENSORS[0]);

    // =====================================================
    // DANH SÁCH ACTUATOR
    // =====================================================

    constexpr ActuatorConfig ACTUATORS[] = {
        {"MIST_PUMP_01",
         "Máy bơm phun sương",

         ActuatorType::PUMP,

         Pins::MIST_PUMP,

         /*
          * true phù hợp khi kiểm thử bằng LED.
          * Relay active-low thì đổi thành false.
          */
         true,

         true},

        {"VENT_FAN_01",
         "Quạt thông gió",

         ActuatorType::FAN,

         Pins::VENT_FAN,

         true,

         true},

        {"GROW_LIGHT_01",
         "Đèn trồng nấm",

         ActuatorType::LIGHT,

         Pins::GROW_LIGHT,

         true,

         true}};

    constexpr size_t ACTUATOR_COUNT =
        sizeof(ACTUATORS) / sizeof(ACTUATORS[0]);

    // =====================================================
    // TÌM SENSOR THEO DEVICE CODE
    // =====================================================

    inline const SensorConfig *findSensor(
        const char *deviceCode)
    {
        if (deviceCode == nullptr)
        {
            return nullptr;
        }

        for (size_t i = 0; i < SENSOR_COUNT; i++)
        {
            if (
                strcmp(
                    SENSORS[i].deviceCode,
                    deviceCode) == 0)
            {
                return &SENSORS[i];
            }
        }

        return nullptr;
    }

    // =====================================================
    // TÌM ACTUATOR THEO DEVICE CODE
    // =====================================================

    inline const ActuatorConfig *findActuator(
        const char *deviceCode)
    {
        if (deviceCode == nullptr)
        {
            return nullptr;
        }

        for (size_t i = 0; i < ACTUATOR_COUNT; i++)
        {
            if (
                strcmp(
                    ACTUATORS[i].deviceCode,
                    deviceCode) == 0)
            {
                return &ACTUATORS[i];
            }
        }

        return nullptr;
    }

    // =====================================================
    // TẠO SENSING TOPIC
    //
    // sensor/{device_code}/{observed_property}
    // =====================================================

    inline bool buildSensorTopic(
        char *buffer,
        size_t bufferSize,
        const char *deviceCode,
        const char *observedProperty)
    {
        if (
            buffer == nullptr ||
            bufferSize == 0 ||
            deviceCode == nullptr ||
            observedProperty == nullptr)
        {
            return false;
        }

        const int written = snprintf(
            buffer,
            bufferSize,
            "sensor/%s/%s",
            deviceCode,
            observedProperty);

        return (
            written > 0 &&
            static_cast<size_t>(written) <
                bufferSize);
    }

    // =====================================================
    // TẠO ACTUATOR COMMAND TOPIC
    //
    // actuator/{device_code}/command
    // =====================================================

    inline bool buildActuatorCommandTopic(
        char *buffer,
        size_t bufferSize,
        const char *deviceCode)
    {
        if (
            buffer == nullptr ||
            bufferSize == 0 ||
            deviceCode == nullptr)
        {
            return false;
        }

        const int written = snprintf(
            buffer,
            bufferSize,
            "actuator/%s/command",
            deviceCode);

        return (
            written > 0 &&
            static_cast<size_t>(written) <
                bufferSize);
    }

    // =====================================================
    // TẠO ACTUATOR STATUS TOPIC
    //
    // actuator/{device_code}/status
    // =====================================================

    inline bool buildActuatorStatusTopic(
        char *buffer,
        size_t bufferSize,
        const char *deviceCode)
    {
        if (
            buffer == nullptr ||
            bufferSize == 0 ||
            deviceCode == nullptr)
        {
            return false;
        }

        const int written = snprintf(
            buffer,
            bufferSize,
            "actuator/%s/status",
            deviceCode);

        return (
            written > 0 &&
            static_cast<size_t>(written) <
                bufferSize);
    }

    // =====================================================
    // MỨC ĐIỆN BẬT ACTUATOR
    // =====================================================

    inline uint8_t getActuatorOnLevel(
        const ActuatorConfig &actuator)
    {
        return actuator.activeHigh
                   ? HIGH
                   : LOW;
    }

    // =====================================================
    // MỨC ĐIỆN TẮT ACTUATOR
    // =====================================================

    inline uint8_t getActuatorOffLevel(
        const ActuatorConfig &actuator)
    {
        return actuator.activeHigh
                   ? LOW
                   : HIGH;
    }

    // =====================================================
    // QUY ĐỔI ADC VỀ 0–100%
    //
    // Hỗ trợ cả:
    // - ADC tăng khi giá trị tăng.
    // - ADC giảm khi giá trị tăng.
    // =====================================================

    inline float convertAnalogToPercent(
        int rawValue,
        const SensorConfig &sensor)
    {
        const int rawZero =
            sensor.rawAtZeroPercent;

        const int rawHundred =
            sensor.rawAtHundredPercent;

        if (rawZero == rawHundred)
        {
            return 0.0f;
        }

        const float percent =
            static_cast<float>(
                rawValue - rawZero) *
            100.0f /
            static_cast<float>(
                rawHundred - rawZero);

        return constrain(
            percent,
            0.0f,
            100.0f);
    }
}

#endif