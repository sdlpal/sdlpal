/* -*- mode: c; tab-width: 4; c-basic-offset: 4; c-file-style: "linux" -*- */
//
// Copyright (c) 2009-2011, Wei Mingzhi <whistler_wmz@users.sf.net>.
// Copyright (c) 2011-2018, SDLPAL development team.
// All rights reserved.
//
// This file is part of SDLPAL.
//
// SDLPAL is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
//

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
#include "generated.h"

#include <unistd.h>
#include <sys/stat.h>
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
 * Class:     com_sdlpal_sdlpal_MainActivity
 * Method:    setAppPath
 * Signature: (Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;)V
 */
EXTERN_C_LINKAGE
JNIEXPORT void JNICALL Java_com_sdlpal_sdlpal_MainActivity_setAppPath(JNIEnv *env, jclass cls, jstring base_path, jstring data_path, jstring cache_path)
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
 * Class:     com_sdlpal_sdlpal_PalActivity
 * Method:    setScreenSize
 * Signature: (II)V
 */
EXTERN_C_LINKAGE
JNIEXPORT void JNICALL Java_com_sdlpal_sdlpal_PalActivity_setScreenSize(JNIEnv *env, jclass cls, int width, int height)
{
    g_screenWidth = width;
    g_screenHeight = height;
}

/*
 * Class:     com_sdlpal_sdlpal_SettingsActivity
 * Method:    getGitRevision
 * Signature: (V)Ljava/lang/String;
 */
EXTERN_C_LINKAGE
JNIEXPORT jstring JNICALL Java_com_sdlpal_sdlpal_SettingsActivity_getGitRevision(JNIEnv *env, jclass cls)
{
    return env->NewStringUTF(PAL_GIT_REVISION);
}

/*
 * Class:     com_sdlpal_sdlpal_SettingsActivity
 * Method:    loadConfigFile
 * Signature: (V)Z
 */
EXTERN_C_LINKAGE
JNIEXPORT jboolean JNICALL Java_com_sdlpal_sdlpal_SettingsActivity_loadConfigFile(JNIEnv *env, jclass cls)
{
    PAL_LoadConfig(TRUE);
    return gConfig.fLaunchSetting ? JNI_TRUE : JNI_FALSE;
}

/*
 * Class:     com_sdlpal_sdlpal_SettingsActivity
 * Method:    saveConfigFile
 * Signature: (V)Z
 */
EXTERN_C_LINKAGE
JNIEXPORT jboolean JNICALL Java_com_sdlpal_sdlpal_SettingsActivity_saveConfigFile(JNIEnv *env, jclass cls)
{
    return PAL_SaveConfig() ? JNI_TRUE : JNI_FALSE;
}

/*
 * Class:     com_sdlpal_sdlpal_SettingsActivity
 * Method:    getConfigBoolean
 * Signature: (Ljava/lang/String;Z)Z
 */
EXTERN_C_LINKAGE
JNIEXPORT jboolean JNICALL Java_com_sdlpal_sdlpal_SettingsActivity_getConfigBoolean(JNIEnv *env, jclass cls, jstring j_str, jboolean defval)
{
    PALCFG_ITEM item = PAL_ConfigIndex(jstring_to_utf8(env, j_str).c_str());
    return item >= 0 ? PAL_GetConfigBoolean(item, defval) : JNI_FALSE;
}

/*
 * Class:     com_sdlpal_sdlpal_SettingsActivity
 * Method:    getConfigInt
 * Signature: (Ljava/lang/String;Z)I
 */
EXTERN_C_LINKAGE
JNIEXPORT int JNICALL Java_com_sdlpal_sdlpal_SettingsActivity_getConfigInt(JNIEnv *env, jclass cls, jstring j_str, jboolean defval)
{
    PALCFG_ITEM item = PAL_ConfigIndex(jstring_to_utf8(env, j_str).c_str());
    return item >= 0 ? (int)PAL_GetConfigNumber(item, defval) : 0;
}

/*
 * Class:     com_sdlpal_sdlpal_SettingsActivity
 * Method:    getConfigString
 * Signature: (Ljava/lang/String;Z)ILjava/lang/String;
 */
EXTERN_C_LINKAGE
JNIEXPORT jstring JNICALL Java_com_sdlpal_sdlpal_SettingsActivity_getConfigString(JNIEnv *env, jclass cls, jstring j_str, jboolean defval)
{
    PALCFG_ITEM item = PAL_ConfigIndex(jstring_to_utf8(env, j_str).c_str());
    return item >= 0 ? env->NewStringUTF(PAL_GetConfigString(item, defval)) : nullptr;
}

/*
 * Class:     com_sdlpal_sdlpal_SettingsActivity
 * Method:    setConfigBoolean
 * Signature: (Ljava/lang/String;Z)Z
 */
EXTERN_C_LINKAGE
JNIEXPORT jboolean JNICALL Java_com_sdlpal_sdlpal_SettingsActivity_setConfigBoolean(JNIEnv *env, jclass cls, jstring j_str, jboolean val)
{
    PALCFG_ITEM item = PAL_ConfigIndex(jstring_to_utf8(env, j_str).c_str());
    return item >= 0 ? PAL_SetConfigBoolean(item, val ? TRUE : FALSE) : JNI_FALSE;
}

/*
 * Class:     com_sdlpal_sdlpal_SettingsActivity
 * Method:    setConfigInt
 * Signature: (Ljava/lang/String;I)Z
 */
EXTERN_C_LINKAGE
JNIEXPORT jboolean JNICALL Java_com_sdlpal_sdlpal_SettingsActivity_setConfigInt(JNIEnv *env, jclass cls, jstring j_str, int val)
{
    PALCFG_ITEM item = PAL_ConfigIndex(jstring_to_utf8(env, j_str).c_str());
    return item >= 0 ? PAL_SetConfigNumber(item, (long)val) : JNI_FALSE;
}

/*
 * Class:     com_sdlpal_sdlpal_SettingsActivity
 * Method:    setConfigString
 * Signature: (Ljava/lang/String;Ljava/lang/String;)I
 */
EXTERN_C_LINKAGE
JNIEXPORT jboolean JNICALL Java_com_sdlpal_sdlpal_SettingsActivity_setConfigString(JNIEnv *env, jclass cls, jstring j_str, jstring v_str)
{
    PALCFG_ITEM item = PAL_ConfigIndex(jstring_to_utf8(env, j_str).c_str());
    return item >= 0 ? PAL_SetConfigString(item, v_str ? jstring_to_utf8(env, v_str).c_str() : nullptr) : JNI_FALSE;
}

/*
 * Class:     com_sdlpal_sdlpal_SettingsActivity
 * Method:    checkResourceFiles
 * Signature: (Ljava/lang/String;Ljava/lang/String;)Z
 */
EXTERN_C_LINKAGE
JNIEXPORT jboolean JNICALL Java_com_sdlpal_sdlpal_SettingsActivity_checkResourceFiles(JNIEnv *env, jclass cls, jstring path_str, jstring msg_str)
{
    return PAL_MISSING_REQUIRED(UTIL_CheckResourceFiles(
        jstring_to_utf8(env, path_str).c_str(),
        jstring_to_utf8(env, msg_str).c_str()
    )) ? JNI_FALSE : JNI_TRUE;
}

/*
 * Class:     com_sdlpal_sdlpal_SettingsActivity
 * Method:    isDirWritable
 * Signature: (Ljava/lang/String;)Z
 */
EXTERN_C_LINKAGE
JNIEXPORT jboolean JNICALL Java_com_sdlpal_sdlpal_SettingsActivity_isDirWritable(JNIEnv *env, jclass cls, jstring path)
{
    std::string str_path = jstring_to_utf8(env, path);
    mkdir(str_path.c_str(), 0755);
    str_path += "/test";
    FILE *fp = fopen(str_path.c_str(), "wb");
    if (fp == NULL) {
        return JNI_FALSE;
    }
    fclose(fp);
    unlink(str_path.c_str());
    return JNI_TRUE;
}

EXTERN_C_LINKAGE
void* JNI_mediaplayer_load(const char *filename)
{
    JNIEnv* env = getJNIEnv();
    jclass clazz = env->FindClass("com/sdlpal/sdlpal/PalActivity");
    jmethodID mid = env->GetStaticMethodID(clazz, "JNI_mediaplayer_load", "(Ljava/lang/String;)Landroid/media/MediaPlayer;");
    jstring str = env->NewStringUTF(filename);
    jobject player_local = env->CallStaticObjectMethod(clazz, mid, str);
    jobject player = env->NewGlobalRef(player_local);
    env->DeleteLocalRef(str);
    env->DeleteLocalRef(player_local);
    env->DeleteLocalRef(clazz);
    return player;
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
    env->DeleteLocalRef(clazz);
}

EXTERN_C_LINKAGE
void JNI_mediaplayer_stop(void *player)
{
    JNIEnv* env = getJNIEnv();
    jclass clazz = env->FindClass("android/media/MediaPlayer");
    env->CallVoidMethod((jobject)player, env->GetMethodID(clazz, "stop", "()V"));
    env->DeleteLocalRef(clazz);
}

EXTERN_C_LINKAGE
int JNI_mediaplayer_isplaying(void *player)
{
    JNIEnv* env = getJNIEnv();
    jclass clazz = env->FindClass("android/media/MediaPlayer");
    int playing = env->CallBooleanMethod((jobject)player, env->GetMethodID(clazz, "isPlaying", "()Z"));
    env->DeleteLocalRef(clazz);
    return playing;
}

EXTERN_C_LINKAGE
void JNI_mediaplayer_setvolume(void *player, int volume)
{
    float vol = (float)volume / 127.0f;
    JNIEnv* env = getJNIEnv();
    jclass clazz = env->FindClass("android/media/MediaPlayer");
    env->CallVoidMethod((jobject)player, env->GetMethodID(clazz, "setVolume", "(FF)V"), vol, vol);
    env->DeleteLocalRef(clazz);
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
	UTIL_LogAddOutputCallback([](LOGLEVEL level, const char* str, const char*)->void {
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
}
