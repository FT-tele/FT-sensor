
#include <esp_heap_caps.h>

#include "esp_mac.h"

#include <FastLED.h>
#include "WiFi.h"

#include "AES.h"
#include "Fourier.h"
#include "Phone.h"
#include "Sensor.h"
#include "VariableDefine.h"

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

CRGB leds[1];


//wifi_mode_t mode;





void setup() {


  Serial.begin(115200);

  initInfrast();


  //Serial.print("\n   setup   boot \n");

  esp_efuse_mac_get_default(FavoriteMAC[0]);
  esp_read_mac(FavoriteMAC[0], ESP_MAC_WIFI_SOFTAP);
  Serial.println(" FavoriteMAC   .");
  for (int i = 0; i < 6; i++) Serial.printf("%d:%02X ", i, FavoriteMAC[0][i]);
  //vTaskDelay(pdMS_TO_TICKS(3000));
  listenQueue = xQueueCreate(KEY, sizeof(uint8_t));
  loraQueue = xQueueCreate(KEY, sizeof(uint8_t));

  wsEventQueue = xQueueCreate(HKEY, sizeof(uint8_t));

  pinMode(BEEPER, OUTPUT);
  pinMode(MENU_BTN, INPUT_PULLUP);

  attachInterrupt(digitalPinToInterrupt(MENU_BTN), switchWifiMode, RISING);  // Swap to release


  if (FTconfig.EnableGPS == 1) {

    xTaskCreatePinnedToCore(gpsTask, "gpsTask", 4096, NULL, 5, &gpsTaskHandle, CORE1);
    //vTaskDelay(2000 / portTICK_PERIOD_MS);
  }



  Serial.printf("  \n    esp_get_free_heap_size: %d\n", esp_get_free_heap_size());
  Serial.printf("\n   esp_get_minimum_free_heap_size: %d\n", esp_get_minimum_free_heap_size());



  xTaskCreatePinnedToCore(electricTask, "electricTask", 4096, NULL, 8, &electricTaskHandle, CORE1);
  //vTaskDelay(5000 / portTICK_PERIOD_MS);




  xTaskCreatePinnedToCore(transformTask, "transformTask", 4096, NULL, 9, &transformTaskHandle, CORE1);
  //vTaskDelay(5000 / portTICK_PERIOD_MS);










  loraInit();

  GenerateKeyPairs(myPrivate, myPublic);

  esp_timer_create_args_t timer_args = {
    .callback = &onTimer,
    .arg = NULL,
    .dispatch_method = ESP_TIMER_TASK,
    .name = "15min_timer"
  };

  if (FTconfig.PeripheralsMode == 2) {

    //Serial.println(" ceate SensorTask");
    Wire.begin(SDA_PIN, SCK_PIN);
    Wire.setClock(400000);

    xTaskCreatePinnedToCore(SensorTask, "SensorTask", 4096, NULL, 4, &sensorTaskHandle, CORE0);
    vTaskDelay(6000 / portTICK_PERIOD_MS);

    pinMode(IN_OR_OUT, INPUT_PULLDOWN);
  }


  if (FTconfig.PeripheralsMode == 3) {

    Wire.begin(SDA_PIN, SCK_PIN);
    Wire.setClock(400000);

    xTaskCreatePinnedToCore(SensorTask, "SensorTask", 4096, NULL, 4, &sensorTaskHandle, CORE0);
    vTaskDelay(6000 / portTICK_PERIOD_MS);
    Serial.println("\n analog start \n");
  }

  esp_timer_create(&timer_args, &timer);
  esp_timer_start_periodic(timer, 30000000);
  if (TurnOnWifi) {

    wifiMode();
    xTaskCreatePinnedToCore(httpdTask, "httpdTask", 4096, NULL, 4, &httpdTaskHandle, CORE0);

    xTaskCreatePinnedToCore(sessionCipherTask, "sessionCipherTask", 8192, NULL, 9, &sessionCipherTaskHandle, CORE1);


    xTaskCreatePinnedToCore(websocketTask, "websocketTask", 4096, NULL, 7, &websocketTaskHandle, CORE0);
  } else {




    if (FTconfig.PeripheralsMode == 1)  // 1 speaker;
    {

      initI2S();


      xTaskCreatePinnedToCore(listenTask, "listenTask", 20480, NULL, 10, &listenTaskHandle, CORE1);
      //vTaskDelay(5000 / portTICK_PERIOD_MS);

      pinMode(IN_OR_OUT, OUTPUT);

      FastLED.addLeds<NEOPIXEL, LED_PIN>(leds, 1);
      FastLED.setBrightness(BRIGHTNESS);

      //Serial.println(" ceate speaker");
    }
  }





  //Serial.printf("  heap: %d\n", esp_get_free_heap_size());
  //Serial.printf("Minimum: %d\n", esp_get_minimum_free_heap_size());
}

void loop() {





  vTaskDelay(1000 / portTICK_PERIOD_MS);

  if (taskReady && NeedReboot) {

    //Serial.printf("   \n FTconfig.Mode to %d ", FTconfig.Mode);
    if (FTconfig.WifiMode == 0) {

      FTconfig.WifiMode = 1;
    } else {

      FTconfig.WifiMode = 0;
    }
    saveConfig();

    vTaskDelay(2000 / portTICK_PERIOD_MS);
    ESP.restart();
  }

  if (FTconfig.PeripheralsMode == 1) {
    vTaskDelay(2000 / portTICK_PERIOD_MS);


    leds[0] = CRGB(LedRed, LedGreen, LedBlue);  // Set LED color
    FastLED.show();
  }
}
