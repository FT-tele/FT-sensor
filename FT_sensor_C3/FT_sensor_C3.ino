
#include <esp_heap_caps.h>

#include "esp_mac.h"
#include "WiFi.h"

#include "AES.h"
#include "Fourier.h"
#include "Phone.h"
#include "Sensor.h"
#include "VariableDefine.h"



wifi_mode_t mode;





void setup() {


  Serial.begin(115200);

  initInfrast();


  Serial.print("\n   setup   boot \n");

  esp_efuse_mac_get_default(FavoriteMAC[0]);
  esp_read_mac(FavoriteMAC[0], ESP_MAC_WIFI_SOFTAP);
  Serial.println(" FavoriteMAC   .");
  for (int i = 0; i < 6; i++) Serial.printf("%d:%02X ", i, FavoriteMAC[0][i]);
  //vTaskDelay(pdMS_TO_TICKS(3000));
  listenQueue = xQueueCreate(KEY, sizeof(uint8_t));
  loraQueue = xQueueCreate(KEY, sizeof(uint8_t));

  wsEventQueue = xQueueCreate(HKEY, sizeof(uint8_t));

  pinMode(BEEPER, OUTPUT);
  pinMode(BEEPER2, OUTPUT);
  pinMode(MENU_BTN, INPUT_PULLUP);

  attachInterrupt(digitalPinToInterrupt(MENU_BTN), switchWifiMode, RISING);  // Swap to release


  if (EnableGPS == 1) {

    xTaskCreatePinnedToCore(gpsTask, "gpsTask", 4096, NULL, 5, &gpsTaskHandle, CORE1);
    //vTaskDelay(2000 / portTICK_PERIOD_MS);
  }



  Serial.printf("  \n    Free heap: %d\n", esp_get_free_heap_size());
  Serial.printf("\n Minimum WiFi.mode  free heap: %d\n", esp_get_minimum_free_heap_size());



  xTaskCreatePinnedToCore(electricTask, "electricTask", 4096, NULL, 8, &electricTaskHandle, CORE1);
  //vTaskDelay(5000 / portTICK_PERIOD_MS);




  xTaskCreatePinnedToCore(transformTask, "transformTask", 4096, NULL, 9, &transformTaskHandle, CORE1);
  //vTaskDelay(5000 / portTICK_PERIOD_MS);










  loraInit();

  GenerateKeyPairs(myPrivate, myPublic);
  Serial.println("GenerateKeyPairs  successfully.");

  esp_timer_create_args_t timer_args = {
    .callback = &onTimer,
    .arg = NULL,
    .dispatch_method = ESP_TIMER_TASK,
    .name = "15min_timer"
  };

  esp_timer_create(&timer_args, &timer);
  esp_timer_start_periodic(timer, 30000000);
  if (TurnOnWifi == 1) {

    WiFi.mode(WIFI_AP);
    xTaskCreatePinnedToCore(httpdTask, "httpdTask", 4096, NULL, 4, &httpdTaskHandle, CORE0);

    xTaskCreatePinnedToCore(sessionCipherTask, "sessionCipherTask", 8192, NULL, 9, &sessionCipherTaskHandle, CORE1);


    xTaskCreatePinnedToCore(websocketTask, "websocketTask", 4096, NULL, 7, &websocketTaskHandle, CORE0);
  } else {

    switch (PeripheralsMode) {
      case 2:  //1 bmp280 & mpu6050
        {

          Wire.begin(SDA_PIN, SCK_PIN);
          Wire.setClock(400000);

          xTaskCreatePinnedToCore(SensorTask, "SensorTask", 4096, NULL, 4, &sensorTaskHandle, CORE0);
          vTaskDelay(6000 / portTICK_PERIOD_MS);
        }
        break;
      case 1:  // 1 speaker;
        {
 
          initI2S();


          xTaskCreatePinnedToCore(listenTask, "listenTask", 20480, NULL, 10, &listenTaskHandle, CORE1);
          //vTaskDelay(5000 / portTICK_PERIOD_MS);
        }
        break;
    }
  }





  Serial.printf("  heap: %d\n", esp_get_free_heap_size());
  Serial.printf("Minimum: %d\n", esp_get_minimum_free_heap_size());
}

void loop() {





  vTaskDelay(5000 / portTICK_PERIOD_MS);

  if (taskReady && NeedReboot) {
    if (FTconfig.Mode == !TurnOnWifi) {
      FTconfig.Mode = TurnOnWifi;
      saveConfig();
    }
    ESP.restart();
  }
}
