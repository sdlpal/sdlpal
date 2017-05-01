#include <jni.h>
#include <android/log.h>
#define TAG "sdlpal-jni"

#define LOGV(...) __android_log_print(ANDROID_LOG_VERBOSE, TAG,__VA_ARGS__)
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG  , TAG,__VA_ARGS__)
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO   , TAG,__VA_ARGS__)
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN   , TAG,__VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR  , TAG,__VA_ARGS__)

#include "palcommon.h"
#include "global.h"
#include "palcfg.h"

char externalStoragePath[1024];
char midiInterFile[1024];

static int jstring_to_utf8(JNIEnv *env, jstring j_str, char *buffer, int capacity)
{
    jsize length = (*env)->GetStringUTFLength(env, j_str);
    const char * const base = (*env)->GetStringUTFChars(env, j_str, NULL);
    if (base == NULL)
    {
        return 0;
    }
    if (capacity <= length)
        length = capacity - 1;
    strncpy(buffer, base, length);
    (*env)->ReleaseStringUTFChars(env, j_str, base);
    base[length] = '\0';
    return length;
}

JavaVM *globalVM;

jint JNI_OnLoad(JavaVM* vm, void* reserved)
{
    JNIEnv* env;
    if ((*vm)->GetEnv(vm, (void**)&env, JNI_VERSION_1_2) != JNI_OK) {
        return -1;
    }
    globalVM = vm;
    return JNI_VERSION_1_2;
}

JNIEnv *getJNIEnv()
{
    JNIEnv* env;
    if ((*globalVM)->GetEnv(globalVM, (void**)&env, JNI_VERSION_1_2) != JNI_OK) {
        return NULL;
    }
    return env;
}

/* 
 * Class:     io_github_sdlpal_PalActivity 
 * Method:    setExternalStorage 
 * Signature: (Ljava/lang/String;)V
 */  
JNIEXPORT void JNICALL Java_io_github_sdlpal_PalActivity_setExternalStorage(JNIEnv *env, jclass cls, jstring j_str)  
{
    jstring_to_utf8(env, j_str, externalStoragePath, sizeof(externalStoragePath) - 8);
    strcat(externalStoragePath, "/sdlpal/");
}

/* 
 * Class:     io_github_sdlpal_PalActivity 
 * Method:    setMIDIInterFile 
 * Signature: (Ljava/lang/String;)V
 */  
JNIEXPORT void JNICALL Java_io_github_sdlpal_PalActivity_setMIDIInterFile(JNIEnv *env, jclass cls, jstring j_str)  
{
    jstring_to_utf8(env, j_str, midiInterFile, sizeof(midiInterFile));
    LOGV("JNI got midi inter filename:%s", midiInterFile);
}

void* JNI_mediaplayer_load(const char *filename)
{
    JNIEnv *env = getJNIEnv();
    jclass clazz = (*env)->FindClass(env, "io/github/sdlpal/PalActivity");
    jmethodID mid = (*env)->GetStaticMethodID(env, clazz, "JNI_mediaplayer_load", "(Ljava/lang/String;)Landroid/media/MediaPlayer;");
    jstring str = (*env)->NewStringUTF(env, filename);
    jobject player = (*env)->CallStaticObjectMethod(env, clazz, mid, str);
    (*env)->DeleteLocalRef(env, str);
    return (*env)->NewGlobalRef(env, player);
}

void JNI_mediaplayer_free(void *player)
{
    JNIEnv *env = getJNIEnv();
    (*env)->DeleteGlobalRef(env, (jobject)player);
}

void JNI_mediaplayer_play(void *player, int looping)
{
    JNIEnv *env = getJNIEnv();
    jclass clazz = (*env)->FindClass(env, "android/media/MediaPlayer");
    (*env)->CallVoidMethod(env, (jobject)player, (*env)->GetMethodID(env, clazz, "setLooping", "(Z)V"), looping ? JNI_TRUE : JNI_FALSE);
    (*env)->CallVoidMethod(env, (jobject)player, (*env)->GetMethodID(env, clazz, "start", "()V"));
}

void JNI_mediaplayer_stop(void *player)
{
    JNIEnv *env = getJNIEnv();
    jclass clazz = (*env)->FindClass(env, "android/media/MediaPlayer");
    (*env)->CallVoidMethod(env, (jobject)player, (*env)->GetMethodID(env, clazz, "stop", "()V"));
}

int JNI_mediaplayer_isplaying(void *player)
{
    JNIEnv *env = getJNIEnv();
    jclass clazz = (*env)->FindClass(env, "android/media/MediaPlayer");
    return (*env)->CallBooleanMethod(env, (jobject)player, (*env)->GetMethodID(env, clazz, "isPlaying", "()Z"));
}

void JNI_mediaplayer_setvolume(void *player, int volume)
{
    float vol = (float)volume / 127.0f;
    JNIEnv *env = getJNIEnv();
    jclass clazz = (*env)->FindClass(env, "android/media/MediaPlayer");
    return (*env)->CallVoidMethod(env, (jobject)player, (*env)->GetMethodID(env, clazz, "setVolume", "(FF)V"), vol, vol);
}

LPCSTR
UTIL_BasePath(
   VOID
)
{
    return externalStoragePath;
}

LPCSTR
UTIL_SavePath(
   VOID
)
{
    return externalStoragePath;
}

BOOL
UTIL_GetScreenSize(
   DWORD *pdwScreenWidth,
   DWORD *pdwScreenHeight
)
{
    *pdwScreenWidth  = 640;
    *pdwScreenHeight = 400;
    return TRUE;
}

BOOL
UTIL_IsAbsolutePath(
	LPCSTR  lpszFileName
)
{
	return FALSE;
}

INT
UTIL_Platform_Init(
   int argc,
   char* argv[]
)
{
   gConfig.fLaunchSetting = FALSE;
   return 0;
}

VOID
UTIL_Platform_Quit(
   VOID
)
{
}
