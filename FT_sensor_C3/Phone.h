

//esp32 s3

#define AUDIO_SPEAKER_BCLK 9  // Speaker BCLK pin
#define AUDIO_SPEAKER_LRC 13   // Speaker LRC (WS) pin
#define AUDIO_SPEAKER_DIN 8   // Speaker DIN pin 

#define AUDIO_SAMPLE_RATE 48000  //48000  // 48kHz sample rate 
 
#define CHANNELS 1
 
#define FRAME_SIZE 2880  // 2880 samples per frame
 


bool initI2S();
 

void listenTask(void *pvParameters);