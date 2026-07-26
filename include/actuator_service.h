#ifndef ACTUATOR_SERVICE_H
#define ACTUATOR_SERVICE_H

#include <Arduino.h>

namespace ActuatorService
{

    enum class Result
    {
        SUCCESS,
        UNKNOWN_DEVICE,
        INVALID_VALUE
    };

    void begin();

    Result setState(
        const String &deviceCode,
        const String &targetValue);

    String getState(const String &deviceCode);

}

#endif