#ifndef CLEYTIN_AUDIO_ENGINE_H
#define CLEYTIN_AUDIO_ENGINE_H

#include <pthread.h>
#include <vector>
#include <string.h>
#include "esp_pthread.h"
#include "driver/i2s_std.h"
#include "cleytin_audio.h"
#include "cleytin_audio_engine_loop_c.h"

#ifndef CLEYTIN_AUDIO_SAMPLE_RATE
#define CLEYTIN_AUDIO_SAMPLE_RATE 44100
#endif
#ifndef CLEYTIN_AUDIO_BIT_SAMPLE
#define CLEYTIN_AUDIO_BIT_SAMPLE 16
#endif
#ifndef CLEYTIN_AUDIO_N_CHANNELS
#define CLEYTIN_AUDIO_N_CHANNELS 1
#endif
#ifndef CLEYTIN_AUDIO_WAIT_TIMEOUT
#define CLEYTIN_AUDIO_WAIT_TIMEOUT 1000
#endif
#define CLEYTIN_AUDIO_BUFFER_SIZE ((8 * 1024) / (CLEYTIN_AUDIO_BIT_SAMPLE / 8)) //8KB

struct WavHeader
{
  char startSectionID[4];     // String "RIFF"
  uint32_t size;              // Tamanho do arquivo em bytes, menos 8
  char format[4];             // String "WAVE"
  
  char formatSectionID[4];    // String "fmt"
  uint32_t formatSize;        // Tamanho dessa seção, menos 8
  uint16_t formatID;          // Id do formato dos dados, 1 para sem compressão, outros formatos não são suportados
  uint16_t numChannels;       // Número de canais, 1=mono e 3=estéreo, outros formatos não são suportados
  uint32_t sampleRate;        // 44100, 16000, 8000 etc.
  uint32_t byteRate;
  uint16_t blockAlign;
  uint16_t bitsPerSample;     // Quantos bits de informação por sample 8, 16, 24 ou 32

  char dataSectionID[4];      // String "data"
  uint32_t dataSize;          // Tamanho dos dados
};

enum WavReadError {
    WAV_READ_OK                     = 0,
    WAV_READ_INVALID_FORMAT         = 1,
    WAV_READ_INVALID_NUM_CHANNELS   = 2,
    WAV_READ_INVALID_BIT_RATE       = 3,
    WAV_READ_INVALID_SAMPLE_RATE    = 4,
    WAV_READ_INVALID_DATA           = 5,
};

class CleytinAudioEngine {
public:
    CleytinAudioEngine();
    ~CleytinAudioEngine();
    void init();
    WavReadError createAudio(const uint8_t* buff, CleytinAudio **audio);
    WavReadError playOnce(const uint8_t* buff);
    void loop();
    void clear();

private:
    i2s_chan_handle_t tx_handle;
    i2s_chan_config_t chan_cfg;
    i2s_std_config_t std_cfg;
    pthread_mutex_t *mutex;
    pthread_t thread;
    std::vector<CleytinAudio*> *audios;
    bool stopFlag;
    uint16_t *buff;

    WavReadError validateWavHeader(const WavHeader *header);
    i2s_std_config_t getStdConfig(uint32_t sampleRate, uint16_t bitsPerSample, uint16_t numChannels);
    void clearAudios();
    void lockMutex();
    void unlockMutex();
    void stop();
};

#endif
