
#include <Arduino.h>
#include <Wire.h>
#define SENSOR_RATE 100
#define RATE_SIZE 4

#define OLED_PREVIEW 0
#define OLED_MSG 1
#define OLED_PATH 2
#define OLED_HBR 3

void onTimer(void *arg);
void IRAM_ATTR switchWifiMode();
void gpsTask(void *pvParameters);
void SensorTask(void *pvParameters);
 