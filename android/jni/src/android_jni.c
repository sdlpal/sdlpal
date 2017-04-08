#include <jni.h>
#include "palcommon.h"
#include "global.h"
#include "palcfg.h"

char externalStoragePath[255];
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