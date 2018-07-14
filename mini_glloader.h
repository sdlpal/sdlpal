//
//  mini_glloader.h
//  Pal
//
//  Created by palxex on 27/2/18.
//

#ifndef mini_glloader_h
#define mini_glloader_h

#if __IOS__
#include <SDL_opengles.h>
#include <SDL_opengles2.h>
#include <OpenGLES/ES3/gl.h>
#include <OpenGLES/ES3/glext.h>
#define glGenVertexArrays glGenVertexArraysOES
#define glBindVertexArray glBindVertexArrayOES
#else
#ifdef __APPLE__
#define GL_GLEXT_PROTOTYPES
#endif
#include <SDL_video.h>
#include <SDL_opengl.h>
#ifndef __APPLE__
//glActiveTexture is a OpenGL 1.3 built-in function, so SDL_opengl.h defined it directly and SDL client cannot simply redefine it.
#define glActiveTexture fake_glActiveTexture
#endif
#endif

#if __IOS__ || __ANDROID__ || __EMSCRIPTEN__ || __WINRT__ || SDL_VIDEO_DRIVER_RPI
#define GLES 1
#undef FORCE_OPENGL_CORE_PROFILE
#endif

#if !defined(__APPLE__)

// I'm avoiding the use of GLEW or some extensions handler, but that
// doesn't mean you should...
PFNGLCREATESHADERPROC glCreateShader;
PFNGLSHADERSOURCEPROC glShaderSource;
PFNGLCOMPILESHADERPROC glCompileShader;
PFNGLGETSHADERIVPROC glGetShaderiv;
PFNGLGETSHADERINFOLOGPROC glGetShaderInfoLog;
PFNGLDELETESHADERPROC glDeleteShader;
PFNGLATTACHSHADERPROC glAttachShader;
PFNGLCREATEPROGRAMPROC glCreateProgram;
PFNGLLINKPROGRAMPROC glLinkProgram;
PFNGLVALIDATEPROGRAMPROC glValidateProgram;
PFNGLGETPROGRAMIVPROC glGetProgramiv;
PFNGLGETPROGRAMINFOLOGPROC glGetProgramInfoLog;
PFNGLUSEPROGRAMPROC glUseProgram;
PFNGLGENVERTEXARRAYSPROC glGenVertexArrays;
PFNGLBINDVERTEXARRAYPROC glBindVertexArray;
PFNGLGENBUFFERSPROC glGenBuffers;
PFNGLBINDBUFFERPROC glBindBuffer;
PFNGLBUFFERDATAPROC glBufferData;
PFNGLBUFFERSUBDATAPROC glBufferSubData;
PFNGLGETATTRIBLOCATIONPROC glGetAttribLocation;
PFNGLENABLEVERTEXATTRIBARRAYPROC glEnableVertexAttribArray;
PFNGLVERTEXATTRIBPOINTERPROC glVertexAttribPointer;
PFNGLUNIFORMMATRIX4FVPROC glUniformMatrix4fv;
PFNGLUNIFORM2FVPROC glUniform2fv;
PFNGLUNIFORM1IVPROC glUniform1iv;
PFNGLUNIFORM1IPROC glUniform1i;
PFNGLACTIVETEXTUREPROC glActiveTexture;
PFNGLGETUNIFORMLOCATIONPROC glGetUniformLocation;
PFNGLBINDFRAGDATALOCATIONPROC glBindFragDataLocation;
PFNGLGETSTRINGIPROC glGetStringi;

extern int initGLExtensions();
#endif

#endif /* mini_glloader_h */
