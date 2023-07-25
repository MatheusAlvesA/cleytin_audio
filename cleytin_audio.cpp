#include "cleytin_audio.h"

CleytinAudio::CleytinAudio() {
    this->bytes_written = new size_t;
}

CleytinAudio::~CleytinAudio() {
    i2s_channel_disable(this->tx_handle);
    i2s_del_channel(this->tx_handle);
    delete this->bytes_written;
}

void CleytinAudio::init() {
    this->chan_cfg = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_AUTO, I2S_ROLE_MASTER);
    i2s_new_channel(&chan_cfg, &this->tx_handle, NULL);
    this->std_cfg = {
        .clk_cfg = I2S_STD_CLK_DEFAULT_CONFIG(44100),
        .slot_cfg = I2S_STD_MSB_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_MONO),
        .gpio_cfg = {
            .mclk = I2S_GPIO_UNUSED,
            .bclk = GPIO_NUM_21,
            .ws = GPIO_NUM_19,
            .dout = GPIO_NUM_33,
            .din = I2S_GPIO_UNUSED,
            .invert_flags = {
                .mclk_inv = false,
                .bclk_inv = false,
                .ws_inv = false,
            },
        },
    };
    i2s_channel_init_std_mode(this->tx_handle, &std_cfg);
}

void CleytinAudio::playWav(const uint8_t* buff) {
    i2s_channel_enable(this->tx_handle);
    i2s_channel_write(this->tx_handle, buff + 44, 12324, this->bytes_written, 1000);
    i2s_channel_disable(this->tx_handle);
}
