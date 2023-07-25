#ifndef CLEYTIN_AUDIO_H
#define CLEYTIN_AUDIO_H

#include "driver/i2s_std.h"

class CleytinAudio {
public:
    CleytinAudio();
    ~CleytinAudio();
    void init();
    void playWav(const uint8_t* buff);

private:
    i2s_chan_handle_t tx_handle;
    i2s_chan_config_t chan_cfg;
    i2s_std_config_t std_cfg;
    size_t *bytes_written;
};


#endif
