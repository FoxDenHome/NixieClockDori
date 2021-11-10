#pragma once

#include <Arduino.h>

#include "serial_host.h"
#include "serial_wifi.h"

#include "DisplayTask.h"
#include "DisplayTask_Clock.h"
#include "DisplayTask_Date.h"
#include "DisplayTask_Stopwatch.h"
#include "DisplayTask_Countdown.h"
#include "DisplayTask_Flash.h"
#include "DisplayTask_Temperature.h"

extern DisplayTask_Clock displayClock;
extern DisplayTask_Date displayDate;
extern DisplayTask_Stopwatch displayStopwatch;
extern DisplayTask_Countdown displayCountdown;
extern DisplayTask_Flash displayFlash;
extern DisplayTask_Temperature displayTemp;

extern WifiSerial wifiSerial;
extern HostSerial hostSerial;
