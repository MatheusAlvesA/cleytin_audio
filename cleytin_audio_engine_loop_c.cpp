#include "cleytin_audio_engine_loop_c.h"

void* c_loop(void *ptrEngine) {
    CleytinAudioEngine *engine = (CleytinAudioEngine *) ptrEngine;
    engine->loop();
    return NULL;
}
