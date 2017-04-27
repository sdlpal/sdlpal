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

char externalStoragePath[255];
char midiInterFile[255];

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
JNIEnv *getJNIEnv() {
    JNIEnv* env;
    if ((*globalVM)->GetEnv(globalVM, (void**)&env, JNI_VERSION_1_2) != JNI_OK) {
        return NULL;
    }
    return env;
}

/* 
 * Class:     com_codeplex_sdlpal_PalActivity 
 * Method:    setExternalStorage 
 * Signature: (Ljava/lang/String;)V
 */  
JNIEXPORT void JNICALL Java_com_codeplex_sdlpal_PalActivity_setExternalStorage(JNIEnv *env, jclass cls, jstring j_str)  
{  
    const jchar* c_str= NULL;  
    char* pBuff = externalStoragePath;  
    c_str = (*env)->GetStringCritical(env,j_str,NULL);
    if (c_str == NULL)
    {  
        return;  
    }  
    while(*c_str)   
    {  
        *pBuff++ = *c_str++;  
    }
    (*env)->ReleaseStringCritical(env,j_str,c_str);
    strncat(externalStoragePath,"/sdlpal/",strnlen("/sdlpal/",255));
    return;  
}
/* 
 * Class:     com_codeplex_sdlpal_PalActivity 
 * Method:    setMIDIInterFile 
 * Signature: (Ljava/lang/String;)V
 */  
JNIEXPORT void JNICALL Java_com_codeplex_sdlpal_PalActivity_setMIDIInterFile(JNIEnv *env, jclass cls, jstring j_str)  
{  
    const jchar* c_str= NULL;  
    char* pBuff = midiInterFile;
    memset(midiInterFile,0,sizeof(midiInterFile));
    c_str = (*env)->GetStringCritical(env,j_str,NULL);
    if (c_str == NULL)
    {  
        return;  
    }  
    while(*c_str)   
    {  
        *pBuff++ = *c_str++;  
    }
    //*pBuff = '\0';
    (*env)->ReleaseStringCritical(env,j_str,c_str);
    LOGV("JNI got midi inter filename:%s", midiInterFile);
    return;  
}

void JNI_mediaplayer_load(){
    JNIEnv* env=getJNIEnv();
    jclass clazz = (*env)->FindClass(env, "com/codeplex/sdlpal/PalActivity");
    jmethodID mid = (*env)->GetStaticMethodID(env, clazz, "JNI_mediaplayer_load", "()V");
    (*env)->CallStaticVoidMethod(env,clazz,mid); 
}
void JNI_mediaplayer_play() {
    JNIEnv* env=getJNIEnv();
    jclass clazz = (*env)->FindClass(env, "com/codeplex/sdlpal/PalActivity");
    jmethodID mid = (*env)->GetStaticMethodID(env, clazz, "JNI_mediaplayer_play", "()V");
    (*env)->CallStaticVoidMethod(env,clazz,mid); 
}
void JNI_mediaplayer_stop() {
    JNIEnv* env=getJNIEnv();
    jclass clazz = (*env)->FindClass(env, "com/codeplex/sdlpal/PalActivity");
    jmethodID mid = (*env)->GetStaticMethodID(env, clazz, "JNI_mediaplayer_stop", "()V");
    (*env)->CallStaticVoidMethod(env,clazz,mid); 
}
int JNI_mediaplayer_isplaying() {
    return 0;
}
void JNI_mediaplayer_setvolume(int volume) {
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