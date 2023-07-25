#ifndef CLEYTIN_AUDIO_H
#define CLEYTIN_AUDIO_H

#include "driver/i2s_std.h"

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
  uint32_t byteRate;          // 8, 16, 24 ou 32
  uint16_t blockAlign;
  uint16_t bitsPerSample;     // Quantos bits de informação por sample

  char dataSectionID[4];      // String "data"
  uint32_t dataSize;          // Tamanho dos dados
};

enum WavReadError {
    WAV_READ_OK                     = 0,
    WAV_READ_INVALID_FORMAT         = 1,
    WAV_READ_INVALID_NUM_CHANNELS   = 2,
    WAV_READ_INVALID_BIT_RATE       = 3,
    WAV_READ_INVALID_DATA           = 4,
};

class CleytinAudio {
public:
    CleytinAudio();
    ~CleytinAudio();
    void init();
    WavReadError playWav(const uint8_t* buff);

private:
    i2s_chan_handle_t tx_handle;
    i2s_chan_config_t chan_cfg;
    i2s_std_config_t std_cfg;
    size_t *bytes_written;

    WavReadError validateWavHeader(WavHeader *header);
    i2s_std_config_t getStdConfig(uint32_t sampleRate, uint16_t bitsPerSample, uint16_t numChannels);
};


#endif
