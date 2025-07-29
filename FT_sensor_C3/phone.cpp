#include <Arduino.h>

#include <driver/i2s.h>
#include <opus.h>
#include "Phone.h"

#include "AES.h"

#include "VariableDefine.h"



uint8_t cipherAudio = 0;

uint8_t PhoneIV[HKEY];
uint8_t PhoneKey[KEY];

int16_t audio_buffer[FRAME_SIZE];
int16_t opus_pcm[FRAME_SIZE];

// Speaker I2S configuration
i2s_config_t i2s_speaker_config = {
  .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX),
  .sample_rate = AUDIO_SAMPLE_RATE,
  .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
  .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
  .communication_format = I2S_COMM_FORMAT_I2S_MSB,
  .intr_alloc_flags = 0,
  .dma_buf_count = 8,
  .dma_buf_len = 512,
  .use_apll = false,
  .tx_desc_auto_clear = true,
  .fixed_mclk = 0
};

// Speaker I2S pin configuration
i2s_pin_config_t i2s_speaker_pin_config = {
  .mck_io_num = -1,
  .bck_io_num = AUDIO_SPEAKER_BCLK,
  .ws_io_num = AUDIO_SPEAKER_LRC,
  .data_out_num = AUDIO_SPEAKER_DIN,
  .data_in_num = I2S_PIN_NO_CHANGE
};
 ;



//-------------------------------------------------------api


bool initI2S() {

 
  // Initialize I2S driver for speaker
  if (i2s_driver_install(I2S_NUM_0, &i2s_speaker_config, 0, NULL) != ESP_OK) {
    //Serial.println("Failed to install I2S speaker driver");
    return false;
  }
  if (i2s_set_pin(I2S_NUM_0, &i2s_speaker_pin_config) != ESP_OK) {
    //Serial.println("Failed to set I2S speaker pins");
    return false;
  }
  //Serial.println("  I2S   init success \n");
  return true;
}


//-------------------------------------------------------task
 
// Audio processing task
void listenTask(void* pvParameters) {
  // Buffers

  uint8_t listenIdx;

  OpusDecoder* decoder;


  //int len = 0;
  int samples = 0;
  int err;
  decoder = opus_decoder_create(AUDIO_SAMPLE_RATE, CHANNELS, &err);
  if (err != OPUS_OK) {
    //Serial.printf("Opus decoder creation failed: %s\n", opus_strerror(err));
  }



  esp_err_t write_result;
  size_t bytes_written = 0;

  Serial.println(" listen task initialized");
  //vTaskSuspend(NULL);
  while (1) {

    if (xQueueReceive(listenQueue, &listenIdx, portMAX_DELAY)) {
      Serial.printf("\n listenIdx :%d ", listenIdx);

      samples = opus_decode(decoder, frame_buffer[listenIdx], frame_len[listenIdx], opus_pcm, FRAME_SIZE, 0);
      if (samples < 0) {
        Serial.printf("Opus decoding failed: %s\n", opus_strerror(samples));
      } else {



        write_result = i2s_write(I2S_NUM_0, opus_pcm, samples * sizeof(int16_t), &bytes_written, portMAX_DELAY);
        if (write_result != ESP_OK) {
          //Serial.println("Error writing to I2S speaker");
        }
      }
    }

    vTaskDelay(50 / portTICK_PERIOD_MS);
  }
}
