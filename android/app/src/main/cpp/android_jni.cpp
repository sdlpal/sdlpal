#include <jni.h>
#include <android/log.h>
#define TAG "sdlpal-jni"

#define LOGV(...) __android_log_print(ANDROID_LOG_VERBOSE, TAG,__VA_ARGS__)
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG  , TAG,__VA_ARGS__)
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO   , TAG,__VA_ARGS__)
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN   , TAG,__VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR  , TAG,__VA_ARGS__)

#define EXTERN_C_LINKAGE extern "C"

#include "palcommon.h"
#include "global.h"
#include "palcfg.h"
#include "util.h"

#include <string>

static std::string g_basepath, g_configpath, g_cachepath, g_midipath;
static int g_screenWidth = 640, g_screenHeight = 400;
const char* midiInterFile;

static std::string jstring_to_utf8(JNIEnv* env, jstring j_str)
{
    jsize length = env->GetStringUTFLength(j_str);
    const char * const base = env->GetStringUTFChars(j_str, NULL);
    if (base == NULL) return "";
    std::string value(base, length);
    env->ReleaseStringUTFChars(j_str, base);
    return value;
}

static JavaVM* gJVM;

EXTERN_C_LINKAGE jint JNI_OnLoad(JavaVM* vm, void* reserved)
{
    JNIEnv* env;
    if (vm->GetEnv((void**)&env, JNI_VERSION_1_2) != JNI_OK) {
        return -1;
    }
    gJVM = vm;
    return JNI_VERSION_1_2;
}

static JNIEnv* getJNIEnv()
{
    JNIEnv* env;
    if (gJVM->GetEnv((void**)&env, JNI_VERSION_1_2) != JNI_OK) {
        return NULL;
    }
    return env;
}

/*
 * Class:     io_github_sdlpal_MainActivity
 * Method:    setAppPath
 * Signature: (Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;)V
 */
EXTERN_C_LINKAGE
JNIEXPORT void JNICALL Java_io_github_sdlpal_MainActivity_setAppPath(JNIEnv *env, jclass cls, jstring base_path, jstring data_path, jstring cache_path)
{
    g_basepath = jstring_to_utf8(env, base_path);
    g_configpath = jstring_to_utf8(env, data_path);
    g_cachepath = jstring_to_utf8(env, cache_path);
    LOGV("got basepath:%s,configpath:%s,cachepath:%s\n",g_basepath.c_str(),g_configpath.c_str(),g_cachepath.c_str());
    if (*g_basepath.rbegin() != '/') g_basepath.append("/");
    if (*g_configpath.rbegin() != '/') g_configpath.append("/");
    if (*g_cachepath.rbegin() != '/') g_cachepath.append("/");
    g_midipath = g_cachepath + "intermediates.mid";
    midiInterFile = g_midipath.c_str();
}

/*
 * Class:     io_github_sdlpal_PalActivity
 * Method:    setScreenSize
 * Signature: (II)V
 */
EXTERN_C_LINKAGE
JNIEXPORT void JNICALL Java_io_github_sdlpal_PalActivity_setScreenSize(JNIEnv *env, jclass cls, int width, int height)
{
    g_screenWidth = width;
    g_screenHeight = height;
}

/*
 * Class:     io_github_sdlpal_SettingsActivity
 * Method:    loadConfigFile
 * Signature: (V)Z
 */
EXTERN_C_LINKAGE
JNIEXPORT jboolean JNICALL Java_io_github_sdlpal_SettingsActivity_loadConfigFile(JNIEnv *env, jclass cls)
{
    PAL_LoadConfig(TRUE);
    return gConfig.fLaunchSetting ? JNI_TRUE : JNI_FALSE;
}

/*
 * Class:     io_github_sdlpal_SettingsActivity
 * Method:    saveConfigFile
 * Signature: (V)Z
 */
EXTERN_C_LINKAGE
JNIEXPORT jboolean JNICALL Java_io_github_sdlpal_SettingsActivity_saveConfigFile(JNIEnv *env, jclass cls)
{
    return PAL_SaveConfig() ? JNI_TRUE : JNI_FALSE;
}

/*
 * Class:     io_github_sdlpal_SettingsActivity
 * Method:    getConfigBoolean
 * Signature: (Ljava/lang/String;Z)Z
 */
EXTERN_C_LINKAGE
JNIEXPORT jboolean JNICALL Java_io_github_sdlpal_SettingsActivity_getConfigBoolean(JNIEnv *env, jclass cls, jstring j_str, jboolean defval)
{
    PALCFG_ITEM item = PAL_ConfigIndex(jstring_to_utf8(env, j_str).c_str());
    return item >= 0 ? PAL_GetConfigBoolean(item, defval) : JNI_FALSE;
}

/*
 * Class:     io_github_sdlpal_SettingsActivity
 * Method:    getConfigInt
 * Signature: (Ljava/lang/String;Z)I
 */
EXTERN_C_LINKAGE
JNIEXPORT int JNICALL Java_io_github_sdlpal_SettingsActivity_getConfigInt(JNIEnv *env, jclass cls, jstring j_str, jboolean defval)
{
    PALCFG_ITEM item = PAL_ConfigIndex(jstring_to_utf8(env, j_str).c_str());
    return item >= 0 ? (int)PAL_GetConfigNumber(item, defval) : 0;
}

/*
 * Class:     io_github_sdlpal_SettingsActivity
 * Method:    getConfigString
 * Signature: (Ljava/lang/String;Z)ILjava/lang/String;
 */
EXTERN_C_LINKAGE
JNIEXPORT jstring JNICALL Java_io_github_sdlpal_SettingsActivity_getConfigString(JNIEnv *env, jclass cls, jstring j_str, jboolean defval)
{
    PALCFG_ITEM item = PAL_ConfigIndex(jstring_to_utf8(env, j_str).c_str());
    return item >= 0 ? env->NewStringUTF(PAL_GetConfigString(item, defval)) : nullptr;
}

/*
 * Class:     io_github_sdlpal_SettingsActivity
 * Method:    setConfigBoolean
 * Signature: (Ljava/lang/String;Z)Z
 */
EXTERN_C_LINKAGE
JNIEXPORT jboolean JNICALL Java_io_github_sdlpal_SettingsActivity_setConfigBoolean(JNIEnv *env, jclass cls, jstring j_str, jboolean val)
{
    PALCFG_ITEM item = PAL_ConfigIndex(jstring_to_utf8(env, j_str).c_str());
    return item >= 0 ? PAL_SetConfigBoolean(item, val ? TRUE : FALSE) : JNI_FALSE;
}

/*
 * Class:     io_github_sdlpal_SettingsActivity
 * Method:    setConfigInt
 * Signature: (Ljava/lang/String;I)Z
 */
EXTERN_C_LINKAGE
JNIEXPORT jboolean JNICALL Java_io_github_sdlpal_SettingsActivity_setConfigInt(JNIEnv *env, jclass cls, jstring j_str, int val)
{
    PALCFG_ITEM item = PAL_ConfigIndex(jstring_to_utf8(env, j_str).c_str());
    return item >= 0 ? PAL_SetConfigNumber(item, (long)val) : JNI_FALSE;
}

/*
 * Class:     io_github_sdlpal_SettingsActivity
 * Method:    setConfigString
 * Signature: (Ljava/lang/String;Ljava/lang/String;)I
 */
EXTERN_C_LINKAGE
JNIEXPORT jboolean JNICALL Java_io_github_sdlpal_SettingsActivity_setConfigString(JNIEnv *env, jclass cls, jstring j_str, jstring v_str)
{
    PALCFG_ITEM item = PAL_ConfigIndex(jstring_to_utf8(env, j_str).c_str());
    return item >= 0 ? PAL_SetConfigString(item, v_str ? jstring_to_utf8(env, v_str).c_str() : nullptr) : JNI_FALSE;
}

EXTERN_C_LINKAGE
void* JNI_mediaplayer_load(const char *filename)
{
    JNIEnv* env = getJNIEnv();
    jclass clazz = env->FindClass("io/github/sdlpal/PalActivity");
    jmethodID mid = env->GetStaticMethodID(clazz, "JNI_mediaplayer_load", "(Ljava/lang/String;)Landroid/media/MediaPlayer;");
    jstring str = env->NewStringUTF(filename);
    jobject player = env->CallStaticObjectMethod(clazz, mid, str);
    env->DeleteLocalRef(str);
    return env->NewGlobalRef(player);
}

EXTERN_C_LINKAGE
void JNI_mediaplayer_free(void *player)
{
    getJNIEnv()->DeleteGlobalRef((jobject)player);
}

EXTERN_C_LINKAGE
void JNI_mediaplayer_play(void *player, int looping)
{
    JNIEnv* env = getJNIEnv();
    jclass clazz = env->FindClass("android/media/MediaPlayer");
    env->CallVoidMethod((jobject)player, env->GetMethodID(clazz, "setLooping", "(Z)V"), looping ? JNI_TRUE : JNI_FALSE);
    env->CallVoidMethod((jobject)player, env->GetMethodID(clazz, "start", "()V"));
}

EXTERN_C_LINKAGE
void JNI_mediaplayer_stop(void *player)
{
    JNIEnv* env = getJNIEnv();
    jclass clazz = env->FindClass("android/media/MediaPlayer");
    env->CallVoidMethod((jobject)player, env->GetMethodID(clazz, "stop", "()V"));
}

EXTERN_C_LINKAGE
int JNI_mediaplayer_isplaying(void *player)
{
    JNIEnv* env = getJNIEnv();
    jclass clazz = env->FindClass("android/media/MediaPlayer");
    return env->CallBooleanMethod((jobject)player, env->GetMethodID(clazz, "isPlaying", "()Z"));
}

EXTERN_C_LINKAGE
void JNI_mediaplayer_setvolume(void *player, int volume)
{
    float vol = (float)volume / 127.0f;
    JNIEnv* env = getJNIEnv();
    jclass clazz = env->FindClass("android/media/MediaPlayer");
    env->CallVoidMethod((jobject)player, env->GetMethodID(clazz, "setVolume", "(FF)V"), vol, vol);
}

EXTERN_C_LINKAGE
LPCSTR
UTIL_BasePath(
   VOID
)
{
    return g_basepath.c_str();
}

EXTERN_C_LINKAGE
LPCSTR
UTIL_ConfigPath(
   VOID
)
{
    return g_configpath.c_str();
}

EXTERN_C_LINKAGE
BOOL
UTIL_GetScreenSize(
   DWORD *pdwScreenWidth,
   DWORD *pdwScreenHeight
)
{
    if (*pdwScreenWidth) *pdwScreenWidth = g_screenWidth;
    if (*pdwScreenHeight) *pdwScreenHeight = g_screenHeight;
    return (pdwScreenWidth && pdwScreenHeight && *pdwScreenWidth && *pdwScreenHeight);
}

EXTERN_C_LINKAGE
BOOL
UTIL_IsAbsolutePath(
	LPCSTR  lpszFileName
)
{
	return lpszFileName[0] == '/';
}

EXTERN_C_LINKAGE
INT
UTIL_Platform_Init(
   int argc,
   char* argv[]
)
{
	UTIL_LogAddOutputCallback([](LOGLEVEL level, const char*, const char* str)->void {
		const static int level_mapping[] = {
			ANDROID_LOG_VERBOSE,
			ANDROID_LOG_DEBUG,
			ANDROID_LOG_INFO,
			ANDROID_LOG_WARN,
			ANDROID_LOG_ERROR
		};
		__android_log_print(level_mapping[level], TAG, "%s", str);
	}, PAL_DEFAULT_LOGLEVEL);

    FILE *fp = fopen((g_cachepath + "running").c_str(), "w");
    if (fp) fclose(fp);
    return 0;
}

EXTERN_C_LINKAGE
VOID
UTIL_Platform_Quit(
   VOID
)
{
    unlink((g_cachepath + "running").c_str());
    exit(0);
}
