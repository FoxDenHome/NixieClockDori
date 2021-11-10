#include "variables.h"
#include "config.h"

DisplayTask_Clock displayClock;
DisplayTask_Date displayDate;
DisplayTask_Stopwatch displayStopwatch;
DisplayTask_Countdown displayCountdown;
DisplayTask_Flash displayFlash;
DisplayTask_Temperature displayTemp;

WifiSerial wifiSerial(WIFI_SERIAL);
HostSerial hostSerial(HOST_SERIAL);
