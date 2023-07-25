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
    this->std_cfg = this->getStdConfig(44100, 16, 1);
    i2s_channel_init_std_mode(this->tx_handle, &this->std_cfg);
}

i2s_std_config_t CleytinAudio::getStdConfig(uint32_t sampleRate, uint16_t bitsPerSample, uint16_t numChannels) {
    i2s_data_bit_width_t dataBitWidth = i2s_data_bit_width_t::I2S_DATA_BIT_WIDTH_16BIT;
    i2s_slot_mode_t slotMode = i2s_slot_mode_t::I2S_SLOT_MODE_STEREO;
    if(numChannels == 1) {
        slotMode = i2s_slot_mode_t::I2S_SLOT_MODE_MONO;
    }

    switch (bitsPerSample) {
    case 8:
        dataBitWidth = i2s_data_bit_width_t::I2S_DATA_BIT_WIDTH_8BIT;
        break;
    case 16:
        dataBitWidth = i2s_data_bit_width_t::I2S_DATA_BIT_WIDTH_16BIT;
        break;
    case 24:
        dataBitWidth = i2s_data_bit_width_t::I2S_DATA_BIT_WIDTH_24BIT;
        break;
    case 32:
        dataBitWidth = i2s_data_bit_width_t::I2S_DATA_BIT_WIDTH_32BIT;
        break;
    
    default:
        dataBitWidth = i2s_data_bit_width_t::I2S_DATA_BIT_WIDTH_16BIT;
        break;
    }

    i2s_std_config_t r = {
        .clk_cfg = I2S_STD_CLK_DEFAULT_CONFIG(sampleRate),
        .slot_cfg = I2S_STD_MSB_SLOT_DEFAULT_CONFIG(dataBitWidth, slotMode),
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

    return r;
}

WavReadError CleytinAudio::playWav(const uint8_t* buff) {
    WavHeader *header = (WavHeader *)buff;
    const uint8_t* data = buff + 44; // O header tem 44 bytes, os dados começam depois disso

    WavReadError err = this->validateWavHeader(header);
    if(err != WAV_READ_OK) {
        return err;
    }

    this->std_cfg = this->getStdConfig(header->sampleRate, header->bitsPerSample, header->numChannels);
    i2s_channel_reconfig_std_clock(this->tx_handle, &this->std_cfg.clk_cfg);
    i2s_channel_reconfig_std_slot(this->tx_handle, &this->std_cfg.slot_cfg);

    i2s_channel_enable(this->tx_handle);
    i2s_channel_write(this->tx_handle, data, header->dataSize, this->bytes_written, 1000);
    i2s_channel_disable(this->tx_handle);
    return WAV_READ_OK;
}

WavReadError CleytinAudio::validateWavHeader(WavHeader *header) {
    if(
        header->format[0] != 'W' ||
        header->format[1] != 'A' ||
        header->format[2] != 'V' ||
        header->format[3] != 'E'
    ) {
        // O formato deve ser WAVE, o arquivo pode estar corrompido
        return WAV_READ_INVALID_DATA;
    }
    if(header->formatID != 1) {
        // O formato deve ser 1, sem compressão
        return WAV_READ_INVALID_FORMAT;
    }
    if(header->numChannels > 2) {
        // Só são suportados mono(1) e estéreo(2)
        return WAV_READ_INVALID_NUM_CHANNELS;
    }
    if(
        header->bitsPerSample != 8 &&
        header->bitsPerSample != 16 &&
        header->bitsPerSample != 24 &&
        header->bitsPerSample != 32
    ) {
        // Só são suportados 8, 16, 24 ou 32 bits por sample
        return WAV_READ_INVALID_BIT_RATE;
    }
    return WAV_READ_OK;
}
