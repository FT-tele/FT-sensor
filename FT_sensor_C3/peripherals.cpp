#include <Arduino.h>

#include <Adafruit_BMP280.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <TinyGPS++.h>



#include "Fourier.h"
#include "Sensor.h"
#include "VariableDefine.h"





//-------------------------------------------------------static


//-----------------------------GPS


TinyGPSPlus gps;

double Latitude = 0;
double Longitude = 0;
double Altitude = 0;
double Speed = 0;
uint8_t pathIndex = 0;
//StaticJsonDocument<200> GPSjson;

// Variables to track distance
double TotalDistanceMeters = 0.0;
char timeBuffer[9];
//-----------------------------OLED



//-----------------------------RGB led


const uint32_t colorLevels[16] = {
  0x000000, 0x00FF00, 0x33FF00, 0x66FF00,
  0x99FF00, 0xCCFF00, 0xFFFF00, 0xFFCC00,
  0xFF9900, 0xFF6600, 0xFF3300, 0xFF0000,  // Pure GREEN to Red

  0x0000FF,  // blue
  0x00FFFF,  // Aqua
  0xCC00FF,  // purple
  0xFFFFFF   // Pure White
};

//-------------------------------------------------------ISR


//-----------------------------timer


void onTimer(void *arg) {
  taskReady = true;  // Set a flag when the timer fires
}

//-----------------------------button

void IRAM_ATTR switchWifiMode() {
  TurnOnWifi = !TurnOnWifi;
  Serial.printf("  reboot to %d ", TurnOnWifi);
  NeedReboot = true;
}





//-------------------------------------------------------PSRAM

//-----------------------------MAX3010x


//-----------------------------GPS



//-----------------------------OLED



//-------------------------------------------------------task




void gpsTask(void *pvParameters) {

  bool located = false;
  int hour = 0, minute = 0, second = 0;
  int day = 0, month = 0, year = 0;
  int local_hour = 8;

  uint8_t buffer[128];
  size_t length;

  double lastLat = 0.0;
  double lastLon = 0.0;
  bool hasLastLocation = false;

  Serial1.begin(9600, SERIAL_8N1, RX_PIN, TX_PIN);

  unsigned long lastPathSaving = 0;
  vTaskDelay(3000 / portTICK_PERIOD_MS);
  Serial.println("gpsTask  successfully.");
  while (true) {


    length = Serial1.readBytes(buffer, sizeof(buffer));
    for (size_t i = 0; i < length; i++) {
      gps.encode(buffer[i]);
    }

    if (hasLastLocation) {

      double segment = TinyGPSPlus::distanceBetween(

        Latitude, Longitude, lastLat, lastLon);

      if (segment >= 5.0) {
        TotalDistanceMeters += segment;
      }
    }

    if (gps.location.isValid()) {
      located = true;
      Latitude = gps.location.lat();
      Longitude = gps.location.lng();

      lastLat = Latitude;
      lastLon = Longitude;
      hasLastLocation = true;
      if ((millis() - lastPathSaving) > LocationSaveFreq * MIN_MILL) {

        if (pathIndex == 31) {
          for (int i = 1; i < HKEY; i++) {
            PathGpsList[i - 1] = PathGpsList[i];
          }
          PathGpsList[pathIndex].latitude = int(Latitude * GPS_INT);
          PathGpsList[pathIndex].longitude = int(Longitude * GPS_INT);
          PathGpsList[pathIndex].altitude = float(Altitude);
          PathGpsList[pathIndex].speed = float(Speed);
          PathGpsList[pathIndex].relayNum = RelayNum;

        } else {
          PathGpsList[pathIndex].latitude = int(Latitude * GPS_INT);
          PathGpsList[pathIndex].longitude = int(Longitude * GPS_INT);
          PathGpsList[pathIndex].altitude = float(Altitude);
          PathGpsList[pathIndex].speed = float(Speed);
          PathGpsList[pathIndex].relayNum = RelayNum;
          pathIndex++;
        }


        lastPathSaving = millis();

        // pathIndex = (pathIndex + 1) & 15;

        GPSjson["T"] = 5;
        GPSjson["W"] = 0;  //self
        GPSjson["La"] = Latitude;
        GPSjson["Ln"] = Longitude;
        GPSjson["Al"] = Altitude;
        GPSjson["Sp"] = Speed;
        GPSjson["Ry"] = RelayNum;
        GPSjson["St"] = Satellites;
      }

    } else {
      located = false;
    }


    if (gps.altitude.isValid())
      Altitude = gps.altitude.meters();

    if (gps.speed.isValid())
      Speed = gps.speed.kmph();

    if (gps.satellites.isValid())
      Satellites = gps.satellites.value();

    if (gps.time.isValid()) {
      hour = gps.time.hour();
      minute = gps.time.minute();
      second = gps.time.second();

      local_hour = (hour + timeZone) % 24;
      sprintf(timeBuffer, "%02d:%02d:%02d", local_hour, minute, second);
    }

    if (gps.date.isValid()) {
      day = gps.date.day();
      month = gps.date.month();
      year = gps.date.year();
    }
  }


  vTaskDelay(pdMS_TO_TICKS(200));
}
//
void SensorTask(void *pvParameters) {


  Adafruit_BMP280 bmp;
  Adafruit_MPU6050 mpu;

  Wire.begin(SDA_PIN, SCK_PIN);

  // BMP280 initialization
  if (!bmp.begin(0x76)) {  // Try 0x77 if needed
    Serial.println("BMP280 not found!");
    vTaskDelete(NULL);
  }

  // MPU6050 initialization
  if (!mpu.begin()) {
    Serial.println("MPU6050 not found!");
    vTaskDelete(NULL);
  }

  // Configure MPU6050 ranges and filters
  mpu.setAccelerometerRange(MPU6050_RANGE_8_G);
  mpu.setGyroRange(MPU6050_RANGE_500_DEG);
  mpu.setFilterBandwidth(MPU6050_BAND_5_HZ);
  
  Serial.println(" SensorTask task initialized");

  while (1) {
    // BMP280 readings
    float temperature = bmp.readTemperature();
    float altitude = bmp.readAltitude(1013.25);  // Sea-level pressure (adjust as needed)

    // MPU6050 readings
    sensors_event_t accel, gyro, temp;
    mpu.getEvent(&accel, &gyro, &temp);

    // Display sensor data

    GPSjson["tmp"] = temperature;
    GPSjson["280alt"] = altitude;
    GPSjson["aX"] = accel.acceleration.x;
    GPSjson["aY"] = accel.acceleration.y;
    GPSjson["aZ"] = accel.acceleration.z;
    GPSjson["gX"] = gyro.gyro.x;
    GPSjson["gY"] = gyro.gyro.y;
    GPSjson["gZ"] = gyro.gyro.z;

    vTaskDelay(pdMS_TO_TICKS(1000));  // Delay 1 second 
    if (digitalRead(IN_OR_OUT) == HIGH) {

      //takingHBR = 1;
      ////alert : direction{trigger 0},type{1},srcMac_6,GPSjson
      String sosStr = "";
      memset(WsQueue[0].payloadData, 0, PKT);
      WsQueue[0].payloadData[0] = 2;
      WsQueue[0].payloadData[1] = 0;
      WsQueue[0].payloadData[2] = 1;
      WsQueue[0].payloadData[10] = '*';
      GPSjson["W"] = 1;  //sos gps
      GPSjson["ST"] = "sensor";
      //for (int i = 0; i < 6; i++) Serial.printf("%d:%02X ", i, FavoriteMAC[0][i]);
      memcpy(&WsQueue[0].payloadData[3], FavoriteMAC[0], 6);

      serializeJson(GPSjson, sosStr);
      WsQueue[0].pktLen = sosStr.length() + 1;
      sosStr.getBytes((unsigned char *)&WsQueue[0].payloadData[11], WsQueue[0].pktLen);
      WsQueue[0].pktLen = WsQueue[0].pktLen + 11;
      memcpy(SndPkt[PeripheralsMode].payloadData, WsQueue[0].payloadData, WsQueue[0].pktLen);
      SndPkt[PeripheralsMode].pktLen = WsQueue[0].pktLen;
      vTaskDelay(5000 / portTICK_PERIOD_MS);
      //for (int i = 0; i < SndPkt[takingHBR].pktLen; i++) Serial.printf("%d:%02X ", i, SndPkt[takingHBR].payloadData[i]);
      xQueueSend(loraQueue, &PeripheralsMode, portMAX_DELAY);
    }else{
      vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
  }
}
//
