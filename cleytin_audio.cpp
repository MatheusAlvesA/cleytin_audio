#include "cleytin_audio.h"

CleytinAudio::CleytinAudio(
    const uint8_t* buff,
    pthread_mutex_t *engineMutex,
    uint32_t buffSize,
    uint32_t bitsPerSample,
    uint32_t sampleRate
) {
    this->buff = buff;
    this->playing = false;
    this->sampleCursor = 0;
    this->toBeRemoved = false;
    this->engineMutex = engineMutex;
    this->buffSize = buffSize;
    this->bitsPerSample = bitsPerSample;
    this->sampleRate = sampleRate;
    this->autoRemove = false;
    this->loop = false;
}

void CleytinAudio::play() {
    this->lockMutex("CleytinAudio::play");
    this->playing = true;
    this->unlockMutex("CleytinAudio::play");
}

void CleytinAudio::pause() {
    this->lockMutex("CleytinAudio::pause");
    this->playing = false;
    this->unlockMutex("CleytinAudio::pause");
}

void CleytinAudio::stop() {
    this->lockMutex("CleytinAudio::stop");
    this->playing = false;
    this->sampleCursor = 0;
    this->unlockMutex("CleytinAudio::stop");
}

void CleytinAudio::remove() {
    this->lockMutex("CleytinAudio::remove");
    this->toBeRemoved = true;
    this->unlockMutex("CleytinAudio::remove");
}

bool CleytinAudio::isPlaying() {
    return this->playing;
}

bool CleytinAudio::mustRemove() {
    return this->toBeRemoved;
}

void CleytinAudio::setAutoRemove(bool autoRemove) {
    this->lockMutex("CleytinAudio::setAutoRemove");
    this->autoRemove = autoRemove;
    this->unlockMutex("CleytinAudio::setAutoRemove");
}

void CleytinAudio::setLoop(bool loop) {
    this->lockMutex("CleytinAudio::setLoop");
    this->loop = loop;
    this->unlockMutex("CleytinAudio::setLoop");
}

uint32_t CleytinAudio::getPlayTimeMs() {
    uint32_t sampleRateMS = this->sampleRate / 1000;
    uint32_t nSamples = this->sampleCursor * (this->bitsPerSample / 8);

    return nSamples / sampleRateMS;
}

void CleytinAudio::setPlayTimeMs(uint32_t time) {
    this->lockMutex("CleytinAudio::setPlayTimeMs");
    uint32_t sampleRateMS = this->sampleRate / 1000;
    this->sampleCursor = time * sampleRateMS;
    this->unlockMutex("CleytinAudio::setPlayTimeMs");
}

uint32_t CleytinAudio::getSampleCursor() {
    return this->sampleCursor;
}

uint32_t CleytinAudio::getNSamples() {
    return this->buffSize / (this->bitsPerSample / 8);
}

void CleytinAudio::setSampleCursor(uint32_t cursor) {
    this->sampleCursor = cursor;
    uint32_t buffCursor = cursor * (this->bitsPerSample / 8);

    if(buffCursor < this->buffSize) {
        return;
    }

    this->playing = false;
    if(this->autoRemove) {
        this->toBeRemoved = true;
    }
    if(this->loop) {
        this->sampleCursor = 0;
        this->playing = true;
    }
}

bool CleytinAudio::getLoop() {
    return this->loop;
}

uint32_t CleytinAudio::getDurationMs() {
    uint32_t sampleRateMS = this->sampleRate / 1000;
    uint32_t nSamples = this->buffSize / (this->bitsPerSample / 8);

    return nSamples / sampleRateMS;
}

const uint8_t* CleytinAudio::getBuff() {
    return this->buff;
}

void CleytinAudio::lockMutex(const char* funcName) {
    int r = pthread_mutex_lock(this->engineMutex);
    if(r) {
        printf("[%s] Erro ao travar mutex: %d\n", funcName, r);
        return;
    }
}

void CleytinAudio::unlockMutex(const char* funcName) {
    int r = pthread_mutex_unlock(this->engineMutex);
    if(r) {
        printf("[%s] Erro ao destravar mutex: %d\n", funcName, r);
        return;
    }
}
