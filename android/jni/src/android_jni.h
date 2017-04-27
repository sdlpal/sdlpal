#ifndef SDLPAL_JNI_H
#define SDLPAL_JNI_H

extern char externalStoragePath[255];
extern char midiInterFile[255];

void JNI_mediaplayer_load();
void JNI_mediaplayer_play();
void JNI_mediaplayer_stop();
int JNI_mediaplayer_isplaying();
void JNI_mediaplayer_setvolume(int);

#endif // SDLPAL_JNI_H