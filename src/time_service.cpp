#include "time_service.h"

#include <time.h>

#include "wifi_service.h"

namespace TimeService
{

    static bool synchronized = false;
    static unsigned long lastSyncAttempt = 0;

    void begin()
    {
        configTime(
            7 * 3600,
            0,
            "pool.ntp.org",
            "time.google.com");
    }

    void loop()
    {
        if (!WifiService::isConnected())
        {
            synchronized = false;
            return;
        }

        if (synchronized)
        {
            return;
        }

        const unsigned long now = millis();

        if (now - lastSyncAttempt < 5000)
        {
            return;
        }

        lastSyncAttempt = now;

        struct tm timeInfo;

        if (getLocalTime(&timeInfo, 1000))
        {
            synchronized = true;
            Serial.println("NTP synchronized");
        }
        else
        {
            Serial.println("Waiting for NTP...");
        }
    }

    bool isSynchronized()
    {
        return synchronized;
    }

    String getIso8601()
    {
        if (!synchronized)
        {
            return "";
        }

        struct tm timeInfo;

        if (!getLocalTime(&timeInfo))
        {
            return "";
        }

        char buffer[32];

        strftime(
            buffer,
            sizeof(buffer),
            "%Y-%m-%dT%H:%M:%S+07:00",
            &timeInfo);

        return String(buffer);
    }

}