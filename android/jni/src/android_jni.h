#ifndef SDLPAL_JNI_H
#define SDLPAL_JNI_H

extern char externalStoragePath[255];
extern char midiInterFile[255];

void* JNI_mediaplayer_load(const char *)
void JNI_mediaplayer_free(void *)
void JNI_mediaplayer_play(void *);
void JNI_mediaplayer_stop(void *);
int JNI_mediaplayer_isplaying(void *);
void JNI_mediaplayer_setvolume(void *, int);

#endif // SDLPAL_JNI_H