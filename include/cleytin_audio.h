#ifndef CLEYTIN_AUDIO_H
#define CLEYTIN_AUDIO_H

#include <pthread.h>
#include <cstdio>

class CleytinAudio {
public:
    CleytinAudio(
        const uint8_t* buff,
        pthread_mutex_t *engineMutex,
        uint32_t buffSize,
        uint32_t bitsPerSample,
        uint32_t sampleRate
    );
    void play();
    void pause();
    void stop();
    void remove();

    void setAutoRemove(bool autoRemove);
    void setLoop(bool loop);
    void setPlayTimeMs(uint32_t time);
    void setVolume(uint8_t volume);
    /**
     * @brief Configura o cursor de reprodução, não é thread safe
    */
    void setSampleCursor(uint32_t cursor);

    uint32_t getDurationMs();
    uint32_t getPlayTimeMs();
    uint32_t getSampleCursor();
    uint32_t getNSamples();
    uint8_t getVolume();
    bool getLoop();
    const uint8_t* getBuff();
    bool isPlaying();
    bool mustRemove();

private:
    const uint8_t* buff;
    bool playing;
    bool autoRemove;
    bool loop;
    uint32_t sampleCursor;
    uint32_t buffSize;
    bool toBeRemoved;
    uint32_t bitsPerSample;
    uint32_t sampleRate;
    uint8_t volume;
    pthread_mutex_t *engineMutex;

    void lockMutex(const char* funcName);
    void unlockMutex(const char* funcName);
};

#endif
