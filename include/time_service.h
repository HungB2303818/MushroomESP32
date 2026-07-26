#ifndef TIME_SERVICE_H
#define TIME_SERVICE_H

#include <Arduino.h>

namespace TimeService
{

    void begin();
    void loop();

    bool isSynchronized();
    String getIso8601();

}

#endif