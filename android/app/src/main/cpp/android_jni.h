#ifndef SDLPAL_JNI_H
#define SDLPAL_JNI_H

#ifdef __cplusplus
extern "C" {
#endif

extern char *midiInterFile;

void* JNI_mediaplayer_load(const char *);
void JNI_mediaplayer_free(void *);
void JNI_mediaplayer_play(void *, int);
void JNI_mediaplayer_stop(void *);
int JNI_mediaplayer_isplaying(void *);
void JNI_mediaplayer_setvolume(void *, int);

#ifdef __cplusplus
}
#endif

#endif // SDLPAL_JNI_H