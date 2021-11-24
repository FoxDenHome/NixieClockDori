#pragma once

#define MQTT_PORT 1883

// Update NTP every 30 seconds
#define NTP_UPDATE_INTERVAL (30 * 1000)

// Reset if we haven't connected to WiFi for 1 hour
#define WIFI_TIMEOUT (1UL * 60UL * 60UL * 1000UL)
// Check for WiFi state every 10 seconds
#define WIFI_CHECK_INTERVAL (10UL * 1000UL)
