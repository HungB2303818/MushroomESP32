#include "sensing_service.h"

#include <Arduino.h>
#include <ArduinoJson.h>
#include <DHT.h>

#include <math.h>
#include <new>
#include <string.h>

#include "app_config.h"
#include "mqtt_service.h"
#include "time_service.h"

namespace SensingService
{
    /*
     * Thời điểm publish gần nhất của từng sensor.
     *
     * Mỗi sensor có chu kỳ gửi dữ liệu riêng nên không sử dụng
     * một biến lastPublishTime chung cho toàn bộ hệ thống.
     */
    static unsigned long lastPublishTimes[AppConfig::SENSOR_COUNT] = {};

    /*
     * Mỗi sensor DHT có một đối tượng DHT tương ứng.
     *
     * Các phần tử không phải DHT sẽ giữ giá trị nullptr.
     * Đối tượng chỉ được cấp phát một lần trong begin().
     */
    static DHT *dhtInstances[AppConfig::SENSOR_COUNT] = {};

    // =====================================================
    // TÌM ĐẠI LƯỢNG ĐO TRONG SENSOR
    // =====================================================

    static const AppConfig::MeasurementConfig *
    findMeasurement(
        const AppConfig::SensorConfig &sensor,
        const char *observedProperty)
    {
        if (observedProperty == nullptr)
        {
            return nullptr;
        }

        for (
            size_t i = 0;
            i < sensor.measurementCount;
            i++)
        {
            const AppConfig::MeasurementConfig &measurement =
                sensor.measurements[i];

            if (
                strcmp(
                    measurement.observedProperty,
                    observedProperty) == 0)
            {
                return &measurement;
            }
        }

        return nullptr;
    }

    // =====================================================
    // KIỂM TRA GIÁ TRỊ CẢM BIẾN
    // =====================================================

    static bool isMeasurementValueValid(
        const AppConfig::MeasurementConfig &measurement,
        float value)
    {
        if (!isfinite(value))
        {
            return false;
        }

        if (value < measurement.minimumValue)
        {
            return false;
        }

        if (value > measurement.maximumValue)
        {
            return false;
        }

        return true;
    }

    // =====================================================
    // PUBLISH GIÁ TRỊ CẢM BIẾN
    //
    // Topic được tạo tự động:
    // sensor/{device_code}/{observed_property}
    // =====================================================

    static bool publishValue(
        const AppConfig::SensorConfig &sensor,
        const AppConfig::MeasurementConfig &measurement,
        float value)
    {
        if (
            !isMeasurementValueValid(
                measurement,
                value))
        {
            Serial.print(
                "Invalid sensing value: ");

            Serial.print(sensor.deviceCode);
            Serial.print(" / ");

            Serial.print(
                measurement.observedProperty);

            Serial.print(" = ");
            Serial.println(value);

            return false;
        }

        char topic[AppConfig::Mqtt::TOPIC_MAX_LENGTH];

        const bool topicCreated =
            AppConfig::buildSensorTopic(
                topic,
                sizeof(topic),
                sensor.deviceCode,
                measurement.observedProperty);

        if (!topicCreated)
        {
            Serial.print(
                "Failed to build sensing topic for: ");

            Serial.println(sensor.deviceCode);

            return false;
        }

        JsonDocument doc;

        doc["device_code"] =
            sensor.deviceCode;

        doc["observed_property"] =
            measurement.observedProperty;

        doc["value"] =
            value;

        doc["unit_symbol"] =
            measurement.unitSymbol;

        doc["observed_at"] =
            TimeService::getIso8601();

        String payload;
        serializeJson(doc, payload);

        const bool success =
            MqttService::publish(
                topic,
                payload,
                AppConfig::Mqtt::SENSOR_QOS,
                AppConfig::Mqtt::SENSOR_RETAIN);

        if (success)
        {
            Serial.print("Published [");
            Serial.print(topic);
            Serial.print("]: ");
            Serial.println(payload);
        }
        else
        {
            Serial.print(
                "Sensing publish failed [");

            Serial.print(topic);
            Serial.println("]");
        }

        return success;
    }

    // =====================================================
    // ĐỌC TRUNG BÌNH ADC
    // =====================================================

    static int readAnalogAverage(
        uint8_t pin)
    {
        constexpr uint8_t SAMPLE_COUNT = 10;
        constexpr unsigned long SAMPLE_DELAY_MS = 5;

        unsigned long total = 0;

        for (
            uint8_t i = 0;
            i < SAMPLE_COUNT;
            i++)
        {
            total += analogRead(pin);

            delay(SAMPLE_DELAY_MS);
        }

        return static_cast<int>(
            total / SAMPLE_COUNT);
    }

    // =====================================================
    // ĐỌC SENSOR DHT11
    // =====================================================

    static void readAndPublishDht11(
        size_t sensorIndex,
        const AppConfig::SensorConfig &sensor)
    {
        DHT *dht = dhtInstances[sensorIndex];

        if (dht == nullptr)
        {
            Serial.print(
                "DHT instance is not initialized: ");

            Serial.println(sensor.deviceCode);

            return;
        }

        const float humidity =
            dht->readHumidity();

        const float temperature =
            dht->readTemperature();

        /*
         * DHT trả về NaN khi không đọc được dữ liệu.
         *
         * Không return khỏi toàn bộ SensingService::loop(),
         * vì các sensor khác vẫn cần tiếp tục hoạt động.
         */
        if (
            isnan(temperature) ||
            isnan(humidity))
        {
            Serial.print(
                "Failed to read DHT11: ");

            Serial.println(sensor.deviceCode);

            return;
        }

        Serial.println();
        Serial.print("DHT11 reading [");
        Serial.print(sensor.deviceCode);
        Serial.println("]");

        Serial.print("Temperature: ");
        Serial.print(temperature, 1);
        Serial.println(" °C");

        Serial.print("Humidity: ");
        Serial.print(humidity, 1);
        Serial.println(" %");

        const AppConfig::MeasurementConfig *
            temperatureMeasurement =
                findMeasurement(
                    sensor,
                    "temperature");

        if (temperatureMeasurement != nullptr)
        {
            publishValue(
                sensor,
                *temperatureMeasurement,
                temperature);
        }
        else
        {
            Serial.print(
                "Missing measurement configuration: ");

            Serial.print(sensor.deviceCode);
            Serial.println(" / temperature");
        }

        const AppConfig::MeasurementConfig *
            humidityMeasurement =
                findMeasurement(
                    sensor,
                    "humidity");

        if (humidityMeasurement != nullptr)
        {
            publishValue(
                sensor,
                *humidityMeasurement,
                humidity);
        }
        else
        {
            Serial.print(
                "Missing measurement configuration: ");

            Serial.print(sensor.deviceCode);
            Serial.println(" / humidity");
        }
    }

    // =====================================================
    // ĐỌC SENSOR ANALOG
    //
    // Dùng chung cho:
    // - Cảm biến ánh sáng.
    // - Cảm biến độ ẩm giá thể.
    // - Các sensor analog 0–100% khác.
    // =====================================================

    static void readAndPublishAnalog(
        const AppConfig::SensorConfig &sensor)
    {
        if (
            sensor.measurements == nullptr ||
            sensor.measurementCount == 0)
        {
            Serial.print(
                "Sensor has no measurement: ");

            Serial.println(sensor.deviceCode);

            return;
        }

        const int rawValue =
            readAnalogAverage(sensor.pin);

        const float percent =
            AppConfig::convertAnalogToPercent(
                rawValue,
                sensor);

        Serial.println();
        Serial.print("Analog sensor reading [");
        Serial.print(sensor.deviceCode);
        Serial.println("]");

        Serial.print("GPIO: ");
        Serial.println(sensor.pin);

        Serial.print("Raw ADC: ");
        Serial.println(rawValue);

        Serial.print("Converted value: ");
        Serial.print(percent, 1);
        Serial.println(" %");

        /*
         * Sensor analog hiện tại chỉ có một đại lượng đo.
         *
         * Vòng lặp vẫn được dùng để cấu trúc có thể mở rộng
         * nếu một module analog cung cấp nhiều đại lượng.
         */
        for (
            size_t i = 0;
            i < sensor.measurementCount;
            i++)
        {
            const AppConfig::MeasurementConfig &measurement =
                sensor.measurements[i];

            publishValue(
                sensor,
                measurement,
                percent);
        }
    }

    // =====================================================
    // KHỞI TẠO MỘT SENSOR
    // =====================================================

    static void initializeSensor(
        size_t sensorIndex,
        const AppConfig::SensorConfig &sensor)
    {
        if (!sensor.enabled)
        {
            Serial.print("Sensor disabled: ");
            Serial.println(sensor.deviceCode);

            return;
        }

        Serial.println();
        Serial.print("Initializing sensor: ");
        Serial.println(sensor.deviceCode);

        Serial.print("Name: ");
        Serial.println(sensor.name);

        Serial.print("GPIO: ");
        Serial.println(sensor.pin);

        switch (sensor.driver)
        {
        case AppConfig::SensorDriver::DHT11:
        {
            DHT *dht = new (std::nothrow) DHT(
                sensor.pin,
                DHT11);

            if (dht == nullptr)
            {
                Serial.print(
                    "Failed to allocate DHT instance: ");

                Serial.println(sensor.deviceCode);

                return;
            }

            dhtInstances[sensorIndex] = dht;

            dht->begin();

            Serial.println(
                "DHT11 initialized");

            break;
        }

        case AppConfig::SensorDriver::ANALOG_LIGHT:
        case AppConfig::SensorDriver::ANALOG_MOISTURE:
        {
            pinMode(
                sensor.pin,
                INPUT);

            /*
             * Cấu hình suy hao riêng cho chân ADC.
             * Module phải được cấp bằng 3.3 V.
             */
            analogSetPinAttenuation(
                sensor.pin,
                ADC_11db);

            Serial.println(
                "Analog sensor initialized");

            break;
        }

        default:
        {
            Serial.print(
                "Unsupported sensor driver: ");

            Serial.println(sensor.deviceCode);

            break;
        }
        }
    }

    // =====================================================
    // KHỞI TẠO SENSING SERVICE
    // =====================================================

    void begin()
    {
        /*
         * ESP32 ADC 12 bit:
         * tập giá trị mặc định 0–4095.
         */
        analogReadResolution(12);

        Serial.println();
        Serial.println(
            "Initializing sensing service...");

        for (
            size_t i = 0;
            i < AppConfig::SENSOR_COUNT;
            i++)
        {
            initializeSensor(
                i,
                AppConfig::SENSORS[i]);
        }

        Serial.println();
        Serial.print("Configured sensors: ");
        Serial.println(
            AppConfig::SENSOR_COUNT);

        Serial.println(
            "Sensing service initialized");
    }

    // =====================================================
    // VÒNG LẶP SENSING
    // =====================================================

    void loop()
    {
        /*
         * Không publish khi MQTT chưa kết nối.
         */
        if (!MqttService::isConnected())
        {
            return;
        }

        /*
         * Không publish khi chưa có thời gian ISO 8601 hợp lệ.
         */
        if (!TimeService::isSynchronized())
        {
            return;
        }

        const unsigned long now = millis();

        for (
            size_t i = 0;
            i < AppConfig::SENSOR_COUNT;
            i++)
        {
            const AppConfig::SensorConfig &sensor =
                AppConfig::SENSORS[i];

            if (!sensor.enabled)
            {
                continue;
            }

            /*
             * Phép trừ unsigned long vẫn xử lý đúng
             * khi millis() bị tràn.
             */
            if (
                now - lastPublishTimes[i] <
                sensor.publishIntervalMs)
            {
                continue;
            }

            lastPublishTimes[i] = now;

            switch (sensor.driver)
            {
            case AppConfig::SensorDriver::DHT11:
            {
                readAndPublishDht11(
                    i,
                    sensor);

                break;
            }

            case AppConfig::SensorDriver::ANALOG_LIGHT:
            case AppConfig::SensorDriver::ANALOG_MOISTURE:
            {
                readAndPublishAnalog(sensor);

                break;
            }

            default:
            {
                Serial.print(
                    "Unsupported sensor driver: ");

                Serial.println(
                    sensor.deviceCode);

                break;
            }
            }
        }
    }
}