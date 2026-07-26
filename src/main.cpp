#include <Arduino.h>

#include "actuator_service.h"
#include "mqtt_service.h"
#include "sensing_service.h"
#include "tasking_service.h"
#include "time_service.h"
#include "wifi_service.h"

void setup()
{
  Serial.begin(115200);
  delay(500);

  Serial.println();
  Serial.println("ESP32 Mushroom System");

  ActuatorService::begin();
  WifiService::begin();
  TimeService::begin();

  MqttService::begin(
      TaskingService::handleMessage);

  SensingService::begin();
}

void loop()
{
  WifiService::loop();

  if (!WifiService::isConnected())
  {
    return;
  }

  TimeService::loop();
  MqttService::loop();

  if (!MqttService::isConnected())
  {
    return;
  }

  SensingService::loop();
}