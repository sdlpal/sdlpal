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

static int jstring_to_utf8(JNIEnv *env, jstring j_str, char *buffer, int capacity)
{
    jsize pos = 0, length = (*env)->GetStringLength(env, j_str);
    const jchar * const base = (*env)->GetStringCritical(env, j_str, NULL);
    if (base == NULL)
    {
        return 0;
    }
    // Convert at char boundary, no incomplete output can be generated
    for(const jchar *str = base;pos < length && pos < capacity - 1;str++)
    {
        if (*str > 4095 && pos < capacity - 3)
        {
            buffer[pos++] = 0xe0 | (*str >> 12);
            buffer[pos++] = 0x80 | ((*str >> 6) & 0x3f);
            buffer[pos++] = 0x80 | (*str & 0x3f);
        }
        else if (*str > 127 && pos < capacity - 2)
        {
            buffer[pos++] = 0xc0 | (*str >> 6);
            buffer[pos++] = 0x80 | (*str & 0x3f);
        }
        else if (*str <= 127)
        {
            buffer[pos++] = *str;
        }
        else
        {
            break;
        }
    }
    (*env)->ReleaseStringCritical(env, j_str, base);
    buffer[pos] = '\0';
    return pos;
}

static jstring jstring_from_utf8(JNIEnv *env, const char *str)
{
    jstring retval = NULL;
    jchar *temp = NULL;
    int wlen = 0, len = strlen(str);
    // Count length of the UTF-8 string, stop at any error
    for(int i = 0, state = 0, count = 0;i < len;i++)
    {
        if (state == 0)
        {
            if (str[i] < 127)
            {
                wlen++;
            }
            else if (str[i] >= 0xc0 && str[i] < 0xf0)
            {
                state = 1;
                count = (str[i] >> 5) - 5;
            }
            else
            {
                break;
            }
        }
        else
        {
            if (str[i] >= 0x80 && str[i] < 0xc0)
            {
                if (count == 0)
                {
                    state = 0;
                    wlen++;
                }
                else
                {
                    count--;
                }
            }
            else
            {
                break;
            }
        }
    }
    if (wlen == 0)
    {
        return (*env)->NewString(env, L"", 0);
    }

    temp = (jchar *)malloc(wlen * sizeof(jchar));
    for(int i = 0, j = 0;j < wlen;j++)
    {
        if (str[i] > 127)
        {
            // Trick here:
            // 2-byte form: 110x xxxx -> 0xxx xx000000
            // 3-byte form: 1110 xxxx -> 10xx xx000000
            temp[j] = (str[i++] & 0x3f) << 6;
            temp[j] |= str[i++] & 0x3f;
            if (temp[j] & 0x800)
            {
                // 3-byte form, the top-most bit will be dicarded during shift
                temp[j] <<= 6;
                temp[j] |= str[i++] & 0x3f;
            }
        }
        else
        {
            temp[j] = str[i++];
        }
    }
    retval = (*env)->NewString(env, temp, wlen);
    free(temp);
    return retval;
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
    jstring_to_utf8(env, j_str, externalStoragePath, 255 - 8);
    strncat(externalStoragePath, "/sdlpal/", 8);
    return;
}

/* 
 * Class:     io_github_sdlpal_PalActivity 
 * Method:    setMIDIInterFile 
 * Signature: (Ljava/lang/String;)V
 */  
JNIEXPORT void JNICALL Java_io_github_sdlpal_PalActivity_setMIDIInterFile(JNIEnv *env, jclass cls, jstring j_str)  
{
    jstring_to_utf8(env, j_str, midiInterFile, 255);
    LOGV("JNI got midi inter filename:%s", midiInterFile);
    return;
}

void JNI_mediaplayer_load(const char *filename)
{
    JNIEnv *env = getJNIEnv();
    jclass clazz = (*env)->FindClass(env, "io/github/sdlpal/PalActivity");
    jmethodID mid = (*env)->GetStaticMethodID(env, clazz, "JNI_mediaplayer_load", "(Ljava/lang/String;)V");
    (*env)->CallStaticVoidMethod(env, clazz, mid, jstring_from_utf8(env, filename));
}

void JNI_mediaplayer_play()
{
    JNIEnv *env = getJNIEnv();
    jclass clazz = (*env)->FindClass(env, "io/github/sdlpal/PalActivity");
    jmethodID mid = (*env)->GetStaticMethodID(env, clazz, "JNI_mediaplayer_play", "()V");
    (*env)->CallStaticVoidMethod(env, clazz, mid);
}

void JNI_mediaplayer_stop()
{
    JNIEnv *env = getJNIEnv();
    jclass clazz = (*env)->FindClass(env, "io/github/sdlpal/PalActivity");
    jmethodID mid = (*env)->GetStaticMethodID(env, clazz, "JNI_mediaplayer_stop", "()V");
    (*env)->CallStaticVoidMethod(env, clazz, mid);
}

int JNI_mediaplayer_isplaying()
{
    return 0;
}

void JNI_mediaplayer_setvolume(int volume)
{
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