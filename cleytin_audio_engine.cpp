#include "cleytin_audio_engine.h"

CleytinAudioEngine::CleytinAudioEngine() {
    this->audios = new std::vector<CleytinAudio*>;
    this->buff = new uint16_t[CLEYTIN_AUDIO_BUFFER_SIZE];
    this->mutex = new pthread_mutex_t;
    this->stopFlag = false;
}

CleytinAudioEngine::~CleytinAudioEngine() {
    this->stop();
    i2s_del_channel(this->tx_handle);

    for (size_t i = 0; i < this->audios->size(); i++)
    {
        delete this->audios->at(i);
    }
    delete this->audios;
    delete this->mutex;
    delete this->buff;
}

void CleytinAudioEngine::init() {
    this->chan_cfg = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_AUTO, I2S_ROLE_MASTER);
    i2s_new_channel(&chan_cfg, &this->tx_handle, NULL);
    this->std_cfg = this->getStdConfig(CLEYTIN_AUDIO_SAMPLE_RATE, CLEYTIN_AUDIO_BIT_SAMPLE, CLEYTIN_AUDIO_N_CHANNELS);
    i2s_channel_init_std_mode(this->tx_handle, &this->std_cfg);
    
    int r = pthread_mutex_init(this->mutex, NULL);
    if(r) {
        printf("Falha ao criar mutex: %d\n", r);
        return;
    }

    esp_pthread_cfg_t attr = esp_pthread_get_default_config();
    attr.pin_to_core = 1;
    esp_pthread_set_cfg(&attr);
    pthread_create(&this->thread, NULL, c_loop, this);
}

void CleytinAudioEngine::stop() {
    this->stopFlag = true;
    if(pthread_join(this->thread, NULL)) {
        printf("Falha ao parar thread de loop de audio\n");
    }
}

i2s_std_config_t CleytinAudioEngine::getStdConfig(uint32_t sampleRate, uint16_t bitsPerSample, uint16_t numChannels) {
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

void CleytinAudioEngine::loop() {
    i2s_channel_enable(this->tx_handle);
    size_t bytesWritten = 0;
    
    while(!this->stopFlag) {
        memset(this->buff, 0x00, CLEYTIN_AUDIO_BUFFER_SIZE * 2);
        this->lockMutex();

        this->clearAudios();
        for (size_t i = 0; i < this->audios->size(); i++) {
            CleytinAudio *audio = this->audios->at(i);
            uint32_t sampleCursor = audio->getSampleCursor();
            audio->setSampleCursor(sampleCursor + (bytesWritten / (CLEYTIN_AUDIO_BIT_SAMPLE / 8)));
            if(!audio->isPlaying() || sampleCursor >= audio->getNSamples()) {
                continue;
            }
            uint16_t *audioBuff = (uint16_t *)audio->getBuff();
            audioBuff += sampleCursor;

            uint32_t remainingLen = audio->getNSamples() - sampleCursor;
            uint32_t copyUntil = remainingLen < CLEYTIN_AUDIO_BUFFER_SIZE ? remainingLen : CLEYTIN_AUDIO_BUFFER_SIZE;
            for (size_t j = 0; j < copyUntil; j++) {
                uint32_t sample = (uint32_t) audioBuff[j];
                sample += (uint32_t) this->buff[j];
                if(sample > 0xFFFF) {
                    sample = 0xFFFF;
                }
                this->buff[j] = (uint16_t) sample;
            }
        }

        this->unlockMutex();
        i2s_channel_write(this->tx_handle, this->buff, CLEYTIN_AUDIO_BUFFER_SIZE, &bytesWritten, CLEYTIN_AUDIO_WAIT_TIMEOUT);
    }

    i2s_channel_disable(this->tx_handle);
}

void CleytinAudioEngine::clearAudios() {
    std::vector<CleytinAudio*> *newList = new std::vector<CleytinAudio*>;
    for (size_t i = 0; i < this->audios->size(); i++)
    {
        if(this->audios->at(i)->mustRemove()) {
            delete this->audios->at(i);
            continue;
        }
        newList->push_back(this->audios->at(i));
    }
    delete this->audios;
    this->audios = newList;
}

WavReadError CleytinAudioEngine::createAudio(const uint8_t* buff, CleytinAudio **audio) {
    const WavHeader *header = (WavHeader *)buff;
    const uint8_t* data = buff + 44; // O header tem 44 bytes, os dados começam depois disso

    WavReadError err = this->validateWavHeader(header);
    if(err != WAV_READ_OK) {
        return err;
    }

    *audio = new CleytinAudio(
        data,
        this->mutex,
        header->dataSize,
        header->bitsPerSample,
        header->sampleRate
    );
    this->audios->push_back(*audio);
    return WAV_READ_OK;
}

WavReadError CleytinAudioEngine::playOnce(const uint8_t* buff) {
    CleytinAudio *audio = NULL;
    WavReadError r = this->createAudio(buff, &audio);
    if(r != WAV_READ_OK) {
        return r;
    }
    audio->setAutoRemove(true);
    audio->play();
    return WAV_READ_OK;
}

WavReadError CleytinAudioEngine::validateWavHeader(const WavHeader *header) {
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
    if(header->numChannels != CLEYTIN_AUDIO_N_CHANNELS) {
        return WAV_READ_INVALID_NUM_CHANNELS;
    }
    if(header->bitsPerSample != CLEYTIN_AUDIO_BIT_SAMPLE) {
        return WAV_READ_INVALID_BIT_RATE;
    }
    if(header->sampleRate != CLEYTIN_AUDIO_SAMPLE_RATE) {
        return WAV_READ_INVALID_SAMPLE_RATE;
    }
    return WAV_READ_OK;
}

void CleytinAudioEngine::lockMutex() {
    int r = pthread_mutex_lock(this->mutex);
    if(r) {
        printf("[CleytinAudioEngine] Erro ao travar mutex: %d\n", r);
        return;
    }
}

void CleytinAudioEngine::unlockMutex() {
    int r = pthread_mutex_unlock(this->mutex);
    if(r) {
        printf("[CleytinAudioEngine] Erro ao destravar mutex: %d\n", r);
        return;
    }
}
