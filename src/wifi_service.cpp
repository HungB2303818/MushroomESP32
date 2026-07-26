#include "wifi_service.h"

#include <Arduino.h>
#include <WiFi.h>

#include "app_config.h"
#include "secrets.h"

namespace WifiService
{
    static unsigned long lastReconnectAttempt = 0;

    static bool connectionMessagePrinted = false;

    // =====================================================
    // BẮT ĐẦU KẾT NỐI WI-FI
    // =====================================================

    static void startConnection()
    {
        Serial.print("Connecting to WiFi: ");
        Serial.println(Secrets::WIFI_SSID);

        WiFi.begin(
            Secrets::WIFI_SSID,
            Secrets::WIFI_PASSWORD);
    }

    // =====================================================
    // IN THÔNG TIN KHI KẾT NỐI THÀNH CÔNG
    // =====================================================

    static void printConnectionInfo()
    {
        Serial.println();
        Serial.println("WiFi connected");

        Serial.print("SSID: ");
        Serial.println(WiFi.SSID());

        Serial.print("IP address: ");
        Serial.println(WiFi.localIP());

        Serial.print("Gateway: ");
        Serial.println(WiFi.gatewayIP());

        Serial.print("Signal strength: ");
        Serial.print(WiFi.RSSI());
        Serial.println(" dBm");
    }

    // =====================================================
    // KHỞI TẠO WI-FI SERVICE
    // =====================================================

    void begin()
    {
        connectionMessagePrinted = false;
        lastReconnectAttempt = 0;

        /*
         * ESP32 hoạt động ở chế độ Station,
         * kết nối vào router Wi-Fi hiện có.
         */
        WiFi.mode(WIFI_STA);

        /*
         * Tắt lưu cấu hình Wi-Fi vào flash mỗi lần gọi begin().
         * Giảm số lần ghi flash không cần thiết.
         */
        WiFi.persistent(false);

        /*
         * Service đang tự quản lý reconnect bằng millis(),
         * vì vậy không phụ thuộc vào auto reconnect.
         */
        WiFi.setAutoReconnect(false);

        startConnection();

        Serial.println("WiFi service initialized");
    }

    // =====================================================
    // VÒNG LẶP WI-FI
    // =====================================================

    void loop()
    {
        const wl_status_t status =
            WiFi.status();

        /*
         * Khi đã kết nối, chỉ in thông tin một lần.
         */
        if (status == WL_CONNECTED)
        {
            if (!connectionMessagePrinted)
            {
                connectionMessagePrinted = true;

                printConnectionInfo();
            }

            return;
        }

        /*
         * Mất kết nối thì cho phép in lại thông tin
         * ở lần kết nối thành công tiếp theo.
         */
        connectionMessagePrinted = false;

        const unsigned long now =
            millis();

        /*
         * Phép trừ unsigned long vẫn hoạt động đúng
         * khi millis() bị tràn.
         */
        if (
            now - lastReconnectAttempt <
            AppConfig::Timing::WIFI_RECONNECT_MS)
        {
            return;
        }

        lastReconnectAttempt = now;

        Serial.println();
        Serial.print("WiFi disconnected, status: ");
        Serial.println(
            static_cast<int>(status));

        Serial.println("Reconnecting WiFi...");

        /*
         * Ngắt kết nối cũ trước khi bắt đầu lại.
         */
        WiFi.disconnect();

        startConnection();
    }

    // =====================================================
    // KIỂM TRA KẾT NỐI
    // =====================================================

    bool isConnected()
    {
        return WiFi.status() ==
               WL_CONNECTED;
    }
}