/* -*- mode: c; tab-width: 4; c-basic-offset: 4; c-file-style: "linux" -*- */
//
// Copyright (c) 2011-2019, SDLPAL development team.
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
PFNGLUNIFORM1FPROC glUniform1f;
PFNGLACTIVETEXTUREPROC glActiveTexture;
PFNGLGETUNIFORMLOCATIONPROC glGetUniformLocation;
PFNGLGETSTRINGIPROC glGetStringi;

extern int initGLExtensions(int major);
#endif

#endif /* mini_glloader_h */
