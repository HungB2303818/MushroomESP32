#ifndef TASKING_SERVICE_H
#define TASKING_SERVICE_H

#include <Arduino.h>

namespace TaskingService
{

    void handleMessage(
        const String &topic,
        const String &payload);

}

#endif