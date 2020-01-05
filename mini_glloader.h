/* -*- mode: c; tab-width: 4; c-basic-offset: 4; c-file-style: "linux" -*- */
//
// Copyright (c) 2011-2020, SDLPAL development team.
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
// mini_glloader.h: quick&dirty OpenGL extension loader header by palxex, 2018
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
#endif

#if __IOS__ || __ANDROID__ || __EMSCRIPTEN__ || __WINRT__ || SDL_VIDEO_DRIVER_RPI
#define GLES 1
#undef FORCE_OPENGL_CORE_PROFILE
#endif

#if !defined(__APPLE__)

//avoid manually imported glfuncs conflicts with platform-builtin ones, like emscripten
#define glCreateShader _glCreateShader
#define glShaderSource _glShaderSource
#define glCompileShader _glCompileShader
#define glGetShaderiv _glGetShaderiv
#define glGetShaderInfoLog _glGetShaderInfoLog
#define glDeleteShader _glDeleteShader
#define glAttachShader _glAttachShader
#define glCreateProgram _glCreateProgram
#define glLinkProgram _glLinkProgram
#define glValidateProgram _glValidateProgram
#define glGetProgramiv _glGetProgramiv
#define glGetProgramInfoLog _glGetProgramInfoLog
#define glUseProgram _glUseProgram
#define glGenVertexArrays _glGenVertexArrays
#define glBindVertexArray _glBindVertexArray
#define glGenBuffers _glGenBuffers
#define glBindBuffer _glBindBuffer
#define glBufferData _glBufferData
#define glBufferSubData _glBufferSubData
#define glGetAttribLocation _glGetAttribLocation
#define glEnableVertexAttribArray _glEnableVertexAttribArray
#define glVertexAttribPointer _glVertexAttribPointer
#define glUniformMatrix4fv _glUniformMatrix4fv
#define glUniform2fv _glUniform2fv
#define glUniform1iv _glUniform1iv
#define glUniform1i _glUniform1i
#define glUniform1f _glUniform1f
#define glActiveTexture _glActiveTexture
#define glGetUniformLocation _glGetUniformLocation
#define glGetStringi _glGetStringi

extern PFNGLCREATESHADERPROC glCreateShader;
extern PFNGLSHADERSOURCEPROC glShaderSource;
extern PFNGLCOMPILESHADERPROC glCompileShader;
extern PFNGLGETSHADERIVPROC glGetShaderiv;
extern PFNGLGETSHADERINFOLOGPROC glGetShaderInfoLog;
extern PFNGLDELETESHADERPROC glDeleteShader;
extern PFNGLATTACHSHADERPROC glAttachShader;
extern PFNGLCREATEPROGRAMPROC glCreateProgram;
extern PFNGLLINKPROGRAMPROC glLinkProgram;
extern PFNGLVALIDATEPROGRAMPROC glValidateProgram;
extern PFNGLGETPROGRAMIVPROC glGetProgramiv;
extern PFNGLGETPROGRAMINFOLOGPROC glGetProgramInfoLog;
extern PFNGLUSEPROGRAMPROC glUseProgram;
extern PFNGLGENVERTEXARRAYSPROC glGenVertexArrays;
extern PFNGLBINDVERTEXARRAYPROC glBindVertexArray;
extern PFNGLGENBUFFERSPROC glGenBuffers;
extern PFNGLBINDBUFFERPROC glBindBuffer;
extern PFNGLBUFFERDATAPROC glBufferData;
extern PFNGLBUFFERSUBDATAPROC glBufferSubData;
extern PFNGLGETATTRIBLOCATIONPROC glGetAttribLocation;
extern PFNGLENABLEVERTEXATTRIBARRAYPROC glEnableVertexAttribArray;
extern PFNGLVERTEXATTRIBPOINTERPROC glVertexAttribPointer;
extern PFNGLUNIFORMMATRIX4FVPROC glUniformMatrix4fv;
extern PFNGLUNIFORM2FVPROC glUniform2fv;
extern PFNGLUNIFORM1IVPROC glUniform1iv;
extern PFNGLUNIFORM1IPROC glUniform1i;
extern PFNGLUNIFORM1FPROC glUniform1f;
extern PFNGLACTIVETEXTUREPROC glActiveTexture;
extern PFNGLGETUNIFORMLOCATIONPROC glGetUniformLocation;
extern PFNGLGETSTRINGIPROC glGetStringi;

extern int initGLExtensions(int major);
#endif

#endif /* mini_glloader_h */
