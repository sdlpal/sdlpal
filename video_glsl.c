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
// video_glsl.c: hacky SDL2 renderer that compatible of retroarch-style
// multipass shader preset by palxex, 2018
//

#include "main.h"

#if PAL_HAS_GLSL

#include "video_glsl.h"
#include "video.h"

#include "glslp.h"

#define SDL_STBIMAGE_IMPLEMENTATION
#include "SDL_stbimage.h"

#define FORCE_OPENGL_CORE_PROFILE 1
#define SUPPORT_PARAMETER_UNIFORM 1

#define MID_GLSLP "sdlpal.glslp"

extern SDL_Window        *gpWindow;
extern SDL_Surface       *gpScreenReal;
extern SDL_Renderer      *gpRenderer;
extern SDL_Texture       *gpTexture;
extern SDL_Texture       *gpTouchOverlay;
extern SDL_Rect           gOverlayRect;
extern SDL_Rect           gTextureRect;

static int gRendererWidth;
static int gRendererHeight;

static uint32_t gProgramIds[MAX_INDEX]={-1};
static uint32_t gVAOIds[MAX_INDEX];
static uint32_t gVBOIds[MAX_INDEX];
static uint32_t gEBOId;
static uint32_t gPassID = -1;
static int gMVPSlots[MAX_INDEX], gHDRSlot=-1, gSRGBSlot=-1, gTouchOverlaySlot=-1;
static int manualSRGB = 0;
static int VAOSupported = 1;
static int glversion_major, glversion_minor;
static int glslversion_major, glslversion_minor;

static SDL_Texture *origTexture;

static char *frame_prev_prefixes[MAX_TEXTURES] = {
    "",
    "Prev",
    "Prev1",
    "Prev2",
    "Prev3",
    "Prev4",
    "Prev5",
    "Prev6",
};
static SDL_Texture *framePrevTextures[MAX_TEXTURES] = {NULL};
static int frame_prev_texture_units[MAX_TEXTURES] = {-1};

static GLint frames = 0;
static bool frames_passed_limit = false;

struct AttrTexCoord
{
    GLfloat s;
    GLfloat t;
    GLfloat u;
    GLfloat v;
};
struct AttrVertexPos
{
    GLfloat x;
    GLfloat y;
    GLfloat z;
    GLfloat w;
};
struct VertexDataFormat
{
    struct AttrVertexPos position;
    struct AttrTexCoord texCoord;
};
#pragma pack(push,16)
union _GLKMatrix4
{
    struct
    {
        float m00, m01, m02, m03;
        float m10, m11, m12, m13;
        float m20, m21, m22, m23;
        float m30, m31, m32, m33;
    };
    float m[16];
};
#pragma pack(pop)
typedef union _GLKMatrix4 GLKMatrix4;

static GLKMatrix4 gOrthoMatrixes[MAX_INDEX];

GLKMatrix4 GLKMatrix4MakeOrtho(float left, float right,
                               float bottom, float top,
                               float nearZ, float farZ)
{
    float ral = right + left;
    float rsl = right - left;
    float tab = top + bottom;
    float tsb = top - bottom;
    float fan = farZ + nearZ;
    float fsn = farZ - nearZ;
    
    GLKMatrix4 m = { 2.0f / rsl, 0.0f, 0.0f, 0.0f,
        0.0f, 2.0f / tsb, 0.0f, 0.0f,
        0.0f, 0.0f, -2.0f / fsn, 0.0f,
        -ral / rsl, -tab / tsb, -fan / fsn, 1.0f };
    
    return m;
}

static unsigned next_pow2(unsigned x)
{
    x -= 1;
    x |= (x >> 1);
    x |= (x >> 2);
    x |= (x >> 4);
    x |= (x >> 8);
    x |= (x >> 16);
    
    return x + 1;
}

static char *plain_glsl_vert = "\r\n\
#if __VERSION__ >= 130              \r\n\
#define COMPAT_VARYING out          \r\n\
#define COMPAT_ATTRIBUTE in         \r\n\
#define COMPAT_TEXTURE texture      \r\n\
#else                               \r\n\
#define COMPAT_VARYING varying      \r\n\
#define COMPAT_ATTRIBUTE attribute  \r\n\
#define COMPAT_TEXTURE texture2D    \r\n\
#endif                              \r\n\
#ifdef GL_ES                        \r\n\
#define COMPAT_PRECISION mediump    \r\n\
#else                               \r\n\
#define COMPAT_PRECISION            \r\n\
#endif                              \r\n\
uniform mat4 MVPMatrix;             \r\n\
COMPAT_ATTRIBUTE vec4 VertexCoord;  \r\n\
COMPAT_ATTRIBUTE vec4 TexCoord;     \r\n\
COMPAT_VARYING vec2 v_texCoord;     \r\n\
void main()                         \r\n\
{                                   \r\n\
gl_Position = MVPMatrix * VertexCoord; \r\n\
v_texCoord = TexCoord.xy;              \r\n\
}";
static char *plain_glsl_frag = "\r\n\
#if __VERSION__ >= 130              \r\n\
#define COMPAT_VARYING in           \r\n\
#define COMPAT_TEXTURE texture      \r\n\
out vec4 FragColor;                 \r\n\
#else                               \r\n\
#define COMPAT_VARYING varying      \r\n\
#define FragColor gl_FragColor      \r\n\
#define COMPAT_TEXTURE texture2D    \r\n\
#endif                              \r\n\
#ifdef GL_ES                        \r\n\
#ifdef GL_FRAGMENT_PRECISION_HIGH   \r\n\
precision highp float;              \r\n\
#else                               \r\n\
precision mediump float;            \r\n\
#endif                              \r\n\
#define COMPAT_PRECISION mediump    \r\n\
#else                               \r\n\
#define COMPAT_PRECISION            \r\n\
#endif                              \r\n\
COMPAT_VARYING vec2 v_texCoord;     \r\n\
uniform sampler2D tex0;             \r\n\
uniform int HDR;                    \r\n\
uniform int sRGB;                   \r\n\
uniform sampler2D TouchOverlay;     \r\n\
vec3 ACESFilm(vec3 x)               \r\n\
{                                   \r\n\
const float A = 2.51;               \r\n\
const float B = 0.03;               \r\n\
const float C = 2.43;               \r\n\
const float D = 0.59;               \r\n\
const float E = 0.14;               \r\n\
return (x * (A * x + B)) / (x * (C * x + D) + E); \r\n\
}                                   \r\n\
const float SRGB_ALPHA = 0.055;     \r\n\
float linear_to_srgb(float channel) {\r\n\
if(channel <= 0.0031308)            \r\n\
return 12.92 * channel;             \r\n\
else                                \r\n\
return (1.0 + SRGB_ALPHA) * pow(channel, 1.0/2.4) - SRGB_ALPHA;    \r\n\
}                                   \r\n\
vec3 rgb_to_srgb(vec3 rgb) {        \r\n\
return vec3(linear_to_srgb(rgb.r), linear_to_srgb(rgb.g), linear_to_srgb(rgb.b)    ); \r\n\
}                                   \r\n\
float srgb_to_linear(float channel) {    \r\n\
if (channel <= 0.04045)             \r\n\
return channel / 12.92;             \r\n\
else                                \r\n\
return pow((channel + SRGB_ALPHA) / (1.0 + SRGB_ALPHA), 2.4);    \r\n\
}                                   \r\n\
vec3 srgb_to_rgb(vec3 srgb) {       \r\n\
return vec3(srgb_to_linear(srgb.r),    srgb_to_linear(srgb.g),    srgb_to_linear(srgb.b));\r\n\
}\r\n\
vec4 blend(vec4 src, vec4 dst){     \r\n\
float sat = (dst.r+dst.g+dst.b)/3.0;\r\n\
vec3 average = vec3(sat,sat,sat);   \r\n\
dst.rgb -= average*0.8;             \r\n\
dst.a=0.5;                          \r\n\
vec4 sfactor = vec4(1,1,1,1);       \r\n\
vec4 dfactor = vec4(1,1,1,1);       \r\n\
return src*sfactor+dst*dfactor;     \r\n\
}\r\n\
void main()                         \r\n\
{                                   \r\n\
vec4 srgb = COMPAT_TEXTURE(tex0 , v_texCoord.xy);   \r\n\
FragColor = vec4(srgb_to_rgb(srgb.rgb), srgb.a);  \r\n\
#ifdef GL_ES                        \r\n\
FragColor.rgb = FragColor.bgr;      \r\n\
#endif                              \r\n\
vec3 color = FragColor.rgb;         \r\n\
if( HDR > 0 )                       \r\n\
color = ACESFilm(color);            \r\n\
if( sRGB > 0 )                      \r\n\
color = rgb_to_srgb(color);         \r\n\
FragColor.rgb=color;                \r\n\
FragColor = blend(FragColor, COMPAT_TEXTURE(TouchOverlay , v_texCoord.xy));     \r\n\
}";

static char *glslp_template = "\r\n\
orig_filter = %s \r\n\
shaders = 1     \r\n\
shader0 = %s    \r\n\
scale_type0 = absolute   \r\n\
scale_x0 = %d   \r\n\
scale_y0 = %d   \r\n\
filter_linear0 = %s    \r\n\
";

char *readShaderFile(const char *filename, GLuint type) {
    FILE *fp = UTIL_OpenRequiredFile(get_glslp_path(filename));
    fseek(fp,0,SEEK_END);
    long filesize = ftell(fp);
    char *buf = (char*)malloc(filesize+1);
    fseek(fp,0,SEEK_SET);
    fread(buf,filesize,1,fp);
    buf[filesize]='\0';
    return buf;
}

char *skip_version(char *src) {
    int glslVersion = -1;
    SDL_sscanf(src, "#version %d", &glslVersion);
    if( glslVersion != -1 ){
        char *eol = strstr(src, "\n");
        for(int i = 0; i < eol-src; i++)
            src[i]=' ';
    }
    return src;
}

GLuint compileShader(const char* sourceOrFilename, GLuint shaderType, int is_source) {
    //DONT CHANGE
#define SHADER_TYPE(shaderType) (shaderType == GL_VERTEX_SHADER ? "VERTEX" : "FRAGMENT")
    char *pShaderBuffer;
    char *source = (char*)sourceOrFilename;
    char *ptr = NULL;
    if(!is_source)
        source = readShaderFile(sourceOrFilename, shaderType);
    size_t sourceLen = strlen(source)*2;
    pShaderBuffer = malloc(sourceLen);
    memset(pShaderBuffer,0, sourceLen);
#if !GLES
    sprintf(pShaderBuffer,"#version %d%02d\r\n",glslversion_major, glslversion_minor);
#else
    sprintf(pShaderBuffer,"#version %d%02d %s\r\n", glslversion_major, glslversion_minor, glslversion_major >= 3 ? "es" : "");
    if( SDL_GL_ExtensionSupported("GL_OES_standard_derivatives") )
        sprintf(pShaderBuffer,"%s#extension GL_OES_standard_derivatives : enable\r\n",pShaderBuffer);
    if( SDL_GL_ExtensionSupported("GL_EXT_shader_texture_lod") )
        sprintf(pShaderBuffer,"%s#extension GL_EXT_shader_texture_lod : enable\r\n",pShaderBuffer);
    
    // should be deduced via GL_ES/GL_FRAGMENT_PRECISION_HIGH combination since both is predefined
    // but unknown why manual define is a must for WebGL2
    if( glslversion_major >= 3 )
        sprintf(pShaderBuffer,"%sprecision highp float;\r\n",pShaderBuffer);
#endif
#if SUPPORT_PARAMETER_UNIFORM
    sprintf(pShaderBuffer,"%s#define PARAMETER_UNIFORM\r\n",pShaderBuffer);
#endif
    // remove #pragma parameter from glsl, avoid glsl compiler（ I mean you, atom ) complains
    while((ptr = strstr(source, "#pragma parameter"))!= NULL) {
        char *ptrEnd = strchr(ptr, '\r');
        if( ptrEnd == NULL ) ptrEnd = strchr(ptr, '\n');
        glslp_add_parameter(ptr, ptrEnd-ptr, &gGLSLP);
        while(ptr!=ptrEnd) *ptr++=' ';
    }
    sprintf(pShaderBuffer,"%s#define %s\r\n%s\r\n",pShaderBuffer,SHADER_TYPE(shaderType),is_source ? source : skip_version(source));
    if(!is_source)
        free((void*)source);
    
    // Create ID for shader
    GLuint result = glCreateShader(shaderType);
    // Define shader text
    glShaderSource(result, 1, (const GLchar *const*)&pShaderBuffer, NULL);
    // Compile shader
    glCompileShader(result);
    
    //Check vertex shader for errors
    GLint shaderCompiled = GL_FALSE;
    glGetShaderiv( result, GL_COMPILE_STATUS, &shaderCompiled );
    if( shaderCompiled != GL_TRUE ) {
        GLint logLength;
        glGetShaderiv(result, GL_INFO_LOG_LENGTH, &logLength);
        if (logLength > 0)
        {
            GLchar *log = (GLchar*)malloc(logLength);
            glGetShaderInfoLog(result, logLength, &logLength, log);
            UTIL_LogOutput(LOGLEVEL_FATAL, "shader %s compilation error:%s\n", is_source ? "stock" : sourceOrFilename,log);
            free(log);
        }
        glDeleteShader(result);
        result = 0;
    }else
        UTIL_LogOutput(LOGLEVEL_DEBUG, "%s shader %s compilation succeed!\n", SHADER_TYPE(shaderType), is_source ? "stock" : sourceOrFilename );
    free(pShaderBuffer);
    return result;
}

GLuint compileProgram(const char* vtx, const char* frag,int is_source) {
    GLuint programId = 0;
    GLuint vtxShaderId, fragShaderId;
    programId = glCreateProgram();
    
    vtxShaderId = compileShader(vtx, GL_VERTEX_SHADER, is_source);
    
    fragShaderId = compileShader(frag, GL_FRAGMENT_SHADER, is_source);
    
    if(vtxShaderId && fragShaderId) {
        // Associate shader with program
        glAttachShader(programId, vtxShaderId);
        glAttachShader(programId, fragShaderId);
        glLinkProgram(programId);
        glValidateProgram(programId);
        
        // Check the status of the compile/link
        GLint programLinked = GL_FALSE;
        glGetProgramiv(programId, GL_LINK_STATUS, &programLinked );
        if( programLinked != GL_TRUE ) {
            GLint logLen;
            glGetProgramiv(programId, GL_INFO_LOG_LENGTH, &logLen);
            if(logLen > 0) {
                char* log = (char*) malloc(logLen * sizeof(char));
                // Show any errors as appropriate
                glGetProgramInfoLog(programId, logLen, &logLen, log);
                UTIL_LogOutput(LOGLEVEL_FATAL, "shader linkage error:%s\n",log);
                free(log);
            }
        }else
            UTIL_LogOutput(LOGLEVEL_DEBUG, "shaders linkage succeed!\n");
    }
    if(vtxShaderId) {
        glDeleteShader(vtxShaderId);
    }
    if(fragShaderId) {
        glDeleteShader(fragShaderId);
    }
    return programId;
}

void setupShaderParams(int pass){
//    glBindAttribLocation(gProgramIds[pass], 0, "TexCoord");
//    glBindAttribLocation(gProgramIds[pass], 1, "VertexCoord");
    
    int shader = pass-1;
    int slot = glGetAttribLocation(gProgramIds[pass], "VertexCoord");
    if(slot >= 0) {
        glEnableVertexAttribArray(slot);
        glVertexAttribPointer(slot, 4, GL_FLOAT, GL_FALSE, sizeof(struct VertexDataFormat), (GLvoid*)offsetof(struct VertexDataFormat, position));
    }else{
        UTIL_LogOutput(LOGLEVEL_DEBUG, "attrib VertexCoord not exist\n");
    }

    for( int i = 0; i < MAX_TEXTURES; i++ ) {
        slot = glGetAttribLocation(gProgramIds[pass], PAL_va(0,"%sTexCoord",frame_prev_prefixes[i]));
        if(slot >= 0) {
            glEnableVertexAttribArray(slot);
            glVertexAttribPointer(slot, 4, GL_FLOAT, GL_FALSE, sizeof(struct VertexDataFormat), (GLvoid*)offsetof(struct VertexDataFormat, texCoord));
        }
        if( pass > 0 && i == 0 )
            gGLSLP.shader_params[shader].self_slots.tex_coord_attrib_location = slot;
    }
    
    gMVPSlots[pass] = glGetUniformLocation(gProgramIds[pass], "MVPMatrix");
    if(gMVPSlots[pass] < 0)
        UTIL_LogOutput(LOGLEVEL_DEBUG, "uniform MVPMatrix not exist\n");
    
    if( pass > 0 ) {
        gGLSLP.shader_params[shader].self_slots.texture_size_uniform_location = glGetUniformLocation(gProgramIds[pass], "TextureSize");
        if(gGLSLP.shader_params[shader].self_slots.texture_size_uniform_location < 0)
            UTIL_LogOutput(LOGLEVEL_DEBUG, "uniform TextureSize not exist\n");
        
        gGLSLP.shader_params[shader].self_slots.output_size_uniform_location= glGetUniformLocation(gProgramIds[pass], "OutputSize");
        if(gGLSLP.shader_params[shader].self_slots.output_size_uniform_location < 0)
            UTIL_LogOutput(LOGLEVEL_DEBUG, "uniform OutputSize not exist\n");
        
        gGLSLP.shader_params[shader].self_slots.input_size_uniform_location = glGetUniformLocation(gProgramIds[pass], "InputSize");
        if(gGLSLP.shader_params[shader].self_slots.input_size_uniform_location < 0)
            UTIL_LogOutput(LOGLEVEL_DEBUG, "uniform InputSize not exist\n");
        
        gGLSLP.shader_params[shader].self_slots.frame_direction_uniform_location = glGetUniformLocation(gProgramIds[pass], "FrameDirection");
        if(gGLSLP.shader_params[shader].self_slots.frame_direction_uniform_location < 0)
            UTIL_LogOutput(LOGLEVEL_DEBUG, "uniform FrameDirection not exist\n");
        
        gGLSLP.shader_params[shader].self_slots.frame_count_uniform_location = glGetUniformLocation(gProgramIds[pass],  "FrameCount");
        if(gGLSLP.shader_params[shader].self_slots.frame_count_uniform_location < 0)
            UTIL_LogOutput(LOGLEVEL_DEBUG, "uniform FrameCount not exist\n");
    }
    
    if( pass == 0 ) {
        gHDRSlot = glGetUniformLocation(gProgramIds[pass], "HDR");
        if(gHDRSlot < 0)
            UTIL_LogOutput(LOGLEVEL_DEBUG, "uniform HDR not exist\n");
        
        gSRGBSlot = glGetUniformLocation(gProgramIds[pass], "sRGB");
        if(gSRGBSlot < 0)
            UTIL_LogOutput(LOGLEVEL_DEBUG, "uniform sRGB not exist\n");
        
        gTouchOverlaySlot = glGetUniformLocation(gProgramIds[pass], "TouchOverlay");
        if(gTouchOverlaySlot < 0)
            UTIL_LogOutput(LOGLEVEL_DEBUG, "uniform TouchOverlay not exist\n");
    }
}

GLint get_gl_clamp_to_border() {
#ifdef __IOS__
    return GL_CLAMP_TO_EDGE;
#else
    return GL_CLAMP_TO_BORDER;
#endif
}

GLint get_gl_wrap_mode(enum wrap_mode mode, enum scale_type type) {
    GLint gl_wrap_mode = GL_INVALID_ENUM;
    switch (mode) {
        case WRAP_REPEAT:
            gl_wrap_mode = GL_REPEAT;
            break;
        case WRAP_CLAMP_TO_EDGE:
            gl_wrap_mode = GL_CLAMP_TO_EDGE;
            break;
        case WRAP_CLAMP_TO_BORDER:
            gl_wrap_mode = get_gl_clamp_to_border();
            break;
        default:
            gl_wrap_mode = GL_INVALID_ENUM;
            break;
    }
    if( type == SCALE_ABSOLUTE )
        gl_wrap_mode = get_gl_clamp_to_border();
    return gl_wrap_mode;
}

SDL_Texture *load_texture(char *name, char *filename, bool filter_linear, enum wrap_mode mode, enum scale_type type) {
    SDL_Surface *surf = STBIMG_Load(get_glslp_path(filename));
    if( !surf )
        TerminateOnError("Texture %s cannot be open!", get_glslp_path(filename));
    if( filter_linear )
        SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "1");
    SDL_Texture *texture = SDL_CreateTextureFromSurface(gpRenderer, surf);
    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "0");
    SDL_GL_BindTexture(texture, NULL, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, get_gl_wrap_mode(mode, type));
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, get_gl_wrap_mode(mode, type));
    SDL_FreeSurface(surf);
    return texture;
}

void GetMultiPassUniformLocations(pass_uniform_locations *pSlot, int programID, char *prefix) {
    pSlot->texture_uniform_location        = glGetUniformLocation( programID, PAL_va(0, "%sTexture",        prefix) );
    pSlot->texture_size_uniform_location   = glGetUniformLocation( programID, PAL_va(0, "%sTextureSize",    prefix) );
    pSlot->input_size_uniform_location     = glGetUniformLocation( programID, PAL_va(0, "%sInputSize",      prefix) );

    pSlot->tex_coord_attrib_location      = glGetAttribLocation ( programID, PAL_va(0, "%sTexCoord",       prefix) );
}
//void fake_glUniform1i (GLint location, GLint v0) {
//    glUniform1i(location, v0);
//}
//
//#define glUniform1i fake_glUniform1i

void SetGroupUniforms(pass_uniform_locations *pSlot, int shaderID, int texture_unit, bool is_pass) {
    glUniform1i(pSlot->texture_uniform_location, texture_unit);

    GLfloat size[2];
    if( is_pass ) {
        size[0] = (shaderID > 0) ? gGLSLP.shader_params[shaderID-1].FBO.width  : 320;
        size[1] = (shaderID > 0) ? gGLSLP.shader_params[shaderID-1].FBO.height : 200;
    }else
        size[0] = 320, size[1] = 200;
    glUniform2fv(pSlot->texture_size_uniform_location, 1, size);
    glUniform2fv(pSlot->input_size_uniform_location, 1, size);
    size[0] = is_pass ? gGLSLP.shader_params[shaderID].FBO.width  : gConfig.dwTextureWidth;
    size[1] = is_pass ? gGLSLP.shader_params[shaderID].FBO.height : gConfig.dwTextureHeight;
    glUniform2fv(pSlot->output_size_uniform_location,  1, size);
    
//    glEnableVertexAttribArray(pSlot->tex_coord_attrib_location);
//    glVertexAttribPointer(pSlot->tex_coord_attrib_location, 4, GL_FLOAT, GL_FALSE, sizeof(struct VertexDataFormat), (GLvoid*)offsetof(struct VertexDataFormat, texCoord));
}

int VIDEO_RenderTexture(SDL_Renderer * renderer, SDL_Texture * texture, const SDL_Rect * srcrect, const SDL_Rect * dstrect, int pass)
{
    GLint oldProgramId;
    GLfloat minx, miny, maxx, maxy;
    GLfloat minu, maxu, minv, maxv;
    
    GLfloat texw;
    GLfloat texh;

    int shaderID = pass-1;
    int orig_texture_unit;

    if( pass == 0 )
        frames++;
    if( !frames_passed_limit && frames > PREV_TEXTURES )
        frames_passed_limit = true;
    
    //get needed uniform locations
    if( pass >= 1 ) {
        GetMultiPassUniformLocations(&gGLSLP.shader_params[shaderID].orig_slots, gProgramIds[pass], "Orig");
        for( int i = 1; i < (frames_passed_limit ? MAX_TEXTURES : min(frames, MAX_TEXTURES)); i++ )
            GetMultiPassUniformLocations(&gGLSLP.shader_params[shaderID].prev_slots[i], gProgramIds[pass], frame_prev_prefixes[i] );
        if( pass >= 2 ){
            for( int i = 0; i < shaderID-1; i++ ) {
                GetMultiPassUniformLocations(&gGLSLP.shader_params[shaderID].pass_slots[i], gProgramIds[pass], PAL_va(0, "Pass%d", i+1) );
                GetMultiPassUniformLocations(&gGLSLP.shader_params[shaderID].pass_slots[i], gProgramIds[pass], PAL_va(0, "PassPrev%d", shaderID-i+1) );
                if( gGLSLP.shader_params[i].alias )
                    GetMultiPassUniformLocations(&gGLSLP.shader_params[shaderID].alias_slots, gProgramIds[pass], gGLSLP.shader_params[i].alias );
            }
        }
    }
    
    glActiveTexture(GL_TEXTURE0);
    SDL_GL_BindTexture(texture, &texw, &texh);
    if( shaderID >= 0 )
        gGLSLP.shader_params[shaderID].self_slots.texture_unit = 0;

    //calc texture unit:1(main texture)+glslp_textures+glsl_uniform_textures(orig,pass(1-6),prev(1-6))
    
    int texture_unit_used = 1;
    int touchoverlay_texture_slot = -1;
    if( pass == 0 ) {
        glActiveTexture(GL_TEXTURE0+texture_unit_used);
        SDL_GL_BindTexture(gpTouchOverlay, NULL, NULL);
        touchoverlay_texture_slot = texture_unit_used++;
    }
    if( pass >= 1 ) {
        //global
        if( gGLSLP.textures > 0 ) {
            for( int i = 0; i < gGLSLP.textures; i++ ) {
                glActiveTexture(GL_TEXTURE0+texture_unit_used);
                SDL_GL_BindTexture(gGLSLP.texture_params[i].sdl_texture,NULL,NULL);
                gGLSLP.texture_params[i].texture_unit = texture_unit_used++;
            }
        }
        //orig
        glActiveTexture(GL_TEXTURE0+texture_unit_used);
        SDL_GL_BindTexture(origTexture,NULL,NULL);
        orig_texture_unit = texture_unit_used++;
        //prev-prev%
        for( int i = 1; i < (frames_passed_limit ? MAX_TEXTURES : min(frames, MAX_TEXTURES)); i++ ) {
            glActiveTexture(GL_TEXTURE0+texture_unit_used);
            SDL_GL_BindTexture(framePrevTextures[i],NULL,NULL);
            frame_prev_texture_units[i] = texture_unit_used++;
        }
        if( pass >= 2 ) {
            //pass%
            for( int i = 0; i < shaderID-1; i++ ) {
                glActiveTexture(GL_TEXTURE0+texture_unit_used);
                SDL_GL_BindTexture(gGLSLP.shader_params[i].pass_sdl_texture,NULL,NULL);
                gGLSLP.shader_params[shaderID].pass_slots[i].texture_unit = texture_unit_used++;
            }
            //passprev%
            for( int i = shaderID-2; i >= 0; i-- ) {
                glActiveTexture(GL_TEXTURE0+texture_unit_used);
                SDL_GL_BindTexture(gGLSLP.shader_params[i].pass_sdl_texture,NULL,NULL);
                gGLSLP.shader_params[shaderID].passprev_slots[i].texture_unit = texture_unit_used++;
            }
        }
    }
    
    if(gProgramIds[pass] != -1) {
        glGetIntegerv(GL_CURRENT_PROGRAM,&oldProgramId);
        glUseProgram(gProgramIds[pass]);
    }

    // set uniforms
    glUniformMatrix4fv(gMVPSlots[pass], 1, GL_FALSE, gOrthoMatrixes[pass].m);
    
    if( pass == 0 ) {
#ifndef GL_ES_VERSION_3_0
        if(!manualSRGB)
            glEnable(GL_FRAMEBUFFER_SRGB);
#endif

        GLint HDR = gConfig.fEnableHDR;
        glUniform1i(gHDRSlot, HDR);
        glUniform1i(gSRGBSlot, manualSRGB);
        glUniform1i(gTouchOverlaySlot, touchoverlay_texture_slot);
    }

    //global
    if( gGLSLP.textures > 0 ) {
        for( int i = 0; i < gGLSLP.textures; i++ ) {
            glUniform1i(gGLSLP.texture_params[i].slots_pass[pass], gGLSLP.texture_params[i].texture_unit);
        }
    }
    if( pass >= 1 ) {
        //share for all retro-filter
        glUniform1i(gGLSLP.shader_params[shaderID].self_slots.frame_direction_uniform_location, 1.0); //SDLPal don't support rewinding so direction is always 1
        
        GLint frame_to_slot = frames;
        if( gGLSLP.shader_params[shaderID].frame_count_mod )
            frame_to_slot %= gGLSLP.shader_params[shaderID].frame_count_mod;
        glUniform1i(gGLSLP.shader_params[shaderID].self_slots.frame_count_uniform_location, frame_to_slot);
        
        //share for all retro-pass
        //self
        SetGroupUniforms(&gGLSLP.shader_params[shaderID].self_slots,         shaderID, 0, false );
        //orig
        SetGroupUniforms(&gGLSLP.shader_params[shaderID].orig_slots,         shaderID, orig_texture_unit, false );
        //prev-prev%
        for( int i = 1; i < (frames_passed_limit ? MAX_TEXTURES : min(frames, MAX_TEXTURES)); i++ )
            SetGroupUniforms(&gGLSLP.shader_params[shaderID].prev_slots[i],         i, frame_prev_texture_units[i], false );
        if( pass >= 2 ){
            //pass%
            for( int i = 0; i < shaderID-1; i++ )
                SetGroupUniforms(&gGLSLP.shader_params[shaderID].pass_slots[i],     i, gGLSLP.shader_params[shaderID].pass_slots[i].texture_unit, true );
            //passprev%
            for( int i = shaderID-1; i >= 0; i-- )
                SetGroupUniforms(&gGLSLP.shader_params[shaderID].passprev_slots[i], i, gGLSLP.shader_params[shaderID].pass_slots[i].texture_unit, true );
            //alias
            SetGroupUniforms(&gGLSLP.shader_params[shaderID].alias_slots,    shaderID, gGLSLP.shader_params[shaderID].alias_slots.texture_unit, false );
        }
    }
#if SUPPORT_PARAMETER_UNIFORM
    for( int i=0; i < gGLSLP.uniform_parameters; i++ ) {
        uniform_param *param = &gGLSLP.uniform_params[i];
        if( shaderID >= 0 )
            glUniform1f(param->uniform_ids[shaderID], param->value);
    }
#endif
    
    SDL_Rect _srcrect,_dstrect;
    
    int w, h;
    SDL_QueryTexture(texture, NULL, NULL, &w, &h);
    
    if(!srcrect) {
        srcrect = &_srcrect;
        _srcrect.x = 0;
        _srcrect.y = 0;
        _srcrect.w = w;
        _srcrect.h = h;
    }
    if(!dstrect) {
        dstrect = &_dstrect;
        _dstrect.x = 0;
        _dstrect.y = 0;
        _dstrect.w = gRendererWidth;
        _dstrect.h = gRendererHeight;
    }
    
    minx = dstrect->x;
    miny = dstrect->y;
    maxx = dstrect->x + dstrect->w;
    maxy = dstrect->y + dstrect->h;
    
    minu = (GLfloat) srcrect->x / srcrect->w;
    maxu = (GLfloat) (srcrect->x + srcrect->w) / srcrect->w;
    minv = (GLfloat) srcrect->y / srcrect->h;
    maxv = (GLfloat) (srcrect->y + srcrect->h) / srcrect->h;
    
    minu *= texw;
    maxu *= texw;
    minv *= texh;
    maxv *= texh;
    
    struct VertexDataFormat vData[ 4 ];
    
    //Texture coordinates
    vData[ 0 ].texCoord.s =  minu; vData[ 0 ].texCoord.t = minv; vData[ 0 ].texCoord.u = 0.0; vData[ 0 ].texCoord.v = 0.0;
    vData[ 1 ].texCoord.s =  maxu; vData[ 1 ].texCoord.t = minv; vData[ 1 ].texCoord.u = 0.0; vData[ 1 ].texCoord.v = 0.0;
    vData[ 2 ].texCoord.s =  maxu; vData[ 2 ].texCoord.t = maxv; vData[ 2 ].texCoord.u = 0.0; vData[ 2 ].texCoord.v = 0.0;
    vData[ 3 ].texCoord.s =  minu; vData[ 3 ].texCoord.t = maxv; vData[ 3 ].texCoord.u = 0.0; vData[ 3 ].texCoord.v = 0.0;
    
    //Vertex positions
    vData[ 0 ].position.x = minx; vData[ 0 ].position.y = miny; vData[ 0 ].position.z = 0.0; vData[ 0 ].position.w = 1.0;
    vData[ 1 ].position.x = maxx; vData[ 1 ].position.y = miny; vData[ 1 ].position.z = 0.0; vData[ 1 ].position.w = 1.0;
    vData[ 2 ].position.x = maxx; vData[ 2 ].position.y = maxy; vData[ 2 ].position.z = 0.0; vData[ 2 ].position.w = 1.0;
    vData[ 3 ].position.x = minx; vData[ 3 ].position.y = maxy; vData[ 3 ].position.z = 0.0; vData[ 3 ].position.w = 1.0;
    
    if(VAOSupported) glBindVertexArray(gVAOIds[pass]);
    
    //Update vertex buffer data
    glBindBuffer( GL_ARRAY_BUFFER, gVBOIds[pass] );
    glBufferSubData( GL_ARRAY_BUFFER, 0, 4 * sizeof(struct VertexDataFormat), vData );
    
    if(!VAOSupported) glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, gEBOId );
    
    //Draw quad using vertex data and index data
    glDrawElements( GL_TRIANGLE_STRIP, 4, GL_UNSIGNED_INT, NULL );
    
    if(VAOSupported) glBindVertexArray(0);
    
    if(!VAOSupported) {
        glBindBuffer( GL_ARRAY_BUFFER, 0 );
        glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, 0 );
    }
    
    if(gProgramIds[pass] != -1) {
        glUseProgram(oldProgramId);
    }
    
    return 0;
}

//remove all fixed pipeline call in RenderCopy
#define SDL_RenderCopy CORE_RenderCopy
PAL_FORCE_INLINE int CORE_RenderCopy(SDL_Renderer * renderer, SDL_Texture * texture,
                    const SDL_Rect * srcrect, const SDL_Rect * dstrect)
{
#if SDL_VERSION_ATLEAST(2,0,10)
    // hack for 2.0.10, manually call glViewport for replaced SDL_RenderCopy.
    int w,h;
    SDL_GetRendererOutputSize(renderer, &w, &h);
    glViewport(0, 0, w, h);
#endif
    return VIDEO_RenderTexture(renderer, texture, srcrect, dstrect, gPassID);
}
SDL_Texture *VIDEO_GLSL_CreateTexture(int width, int height)
{
    gRendererWidth = width;
    gRendererHeight = height;

    double ratio = (double)width / (double)height;
    ratio *= 1.6f * (double)gConfig.dwTextureHeight / (double)gConfig.dwTextureWidth;
    
    for( int i=0; i<MAX_INDEX; i++)
        gOrthoMatrixes[i] = GLKMatrix4MakeOrtho(0, width, 0, height, -1, 1);
    //hack!
    if( strstr(gConfig.pszShader,"metacrt.glslp") == NULL)
    gOrthoMatrixes[0] = GLKMatrix4MakeOrtho(0, width, height, 0, -1, 1);

    //
    // Check whether to keep the aspect ratio
    //
    if (gConfig.fKeepAspectRatio && fabs(ratio - 1.6f) > FLT_EPSILON)
    {
        if (ratio > 1.6f)
        {
            ratio = (float)height / gConfig.dwTextureHeight;
        }
        else
        {
            ratio = (float)width / gConfig.dwTextureWidth;
        }
        
        WORD w = (WORD)(ratio * gConfig.dwTextureWidth) & ~0x3;
        WORD h = (WORD)(ratio * gConfig.dwTextureHeight) & ~0x3;
        gTextureRect.x = (width - w) / 2;
        gTextureRect.y = (height - h) / 2;
        gTextureRect.w = w; gTextureRect.h = h;
    }
    else
    {
        gTextureRect.x = gTextureRect.y = 0;
        gTextureRect.w = width; gTextureRect.h = height;
    }
    
    // in GLSL now touch is fullscreen forever
    VIDEO_SetupTouchArea(width, height, width, height);

    for( int i = 0; i < gGLSLP.shaders; i++ ) {
        shader_param *param = &gGLSLP.shader_params[i];
        shader_param *param_next_pass = &gGLSLP.shader_params[i+1];
        if( i== gGLSLP.shaders - 1 )
            param_next_pass = NULL;
        
        if( param->pass_sdl_texture )
            SDL_DestroyTexture( param->pass_sdl_texture );
        
        switch (param->scale_type_x) {
            case SCALE_SOURCE:
                param->FBO.width = (i == 0 ? 320 : gGLSLP.shader_params[i-1].FBO.width) * param->scale_x;
                break;
            case SCALE_VIEWPORT:
                param->FBO.width = gConfig.dwTextureWidth * param->scale_x;
                break;
            case SCALE_ABSOLUTE:
                param->FBO.width = param->scale_x;
                break;
        }
        switch (param->scale_type_y) {
            case SCALE_SOURCE:
                param->FBO.height = (i == 0 ? 200 : gGLSLP.shader_params[i-1].FBO.height) * param->scale_y;
                break;
            case SCALE_VIEWPORT:
                param->FBO.height = gConfig.dwTextureHeight * param->scale_y;
                break;
            case SCALE_ABSOLUTE:
                param->FBO.height = param->scale_y;
                break;
        }
        param->FBO.pow_width = next_pow2(param->FBO.width);
        param->FBO.pow_height = next_pow2(param->FBO.height);
        
        if( param_next_pass && param_next_pass->filter_linear )
            SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "1");
        param->pass_sdl_texture = SDL_CreateTexture(gpRenderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_TARGET, param->FBO.pow_width, param->FBO.pow_height);
        SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "0");
        SDL_GL_BindTexture(param->pass_sdl_texture, NULL, NULL);
        if( param_next_pass ) {
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, get_gl_wrap_mode(param_next_pass->wrap_mode, param_next_pass->scale_type_x));
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, get_gl_wrap_mode(param_next_pass->wrap_mode, param_next_pass->scale_type_y));
        }
        //lacks parameter processing:
        //srgb   - already srgb,    need figure what retroarch is
        //float  - already float32, need figure what retroarch is
        //mipmap - do we really need it?
    }
    
    //
    // Recreate textures
    //
    for( int i = 0; i < MAX_TEXTURES; i++ ) {
        if( framePrevTextures[i] )
            SDL_DestroyTexture(framePrevTextures[i]);
        framePrevTextures[i] = SDL_CreateTexture(gpRenderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_TARGET, gConfig.dwTextureWidth, gConfig.dwTextureHeight);
    }
    return framePrevTextures[0];
}

void VIDEO_GLSL_RenderCopy()
{
    gpTexture = framePrevTextures[0]; //...
    if( gpTexture == NULL )
        return;
    
    if( gGLSLP.shader_params[0].filter_linear)
        SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "1");
    origTexture = SDL_CreateTextureFromSurface(gpRenderer, gpScreenReal);
    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "0");
    SDL_GL_BindTexture(origTexture, NULL, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, get_gl_wrap_mode(gGLSLP.shader_params[0].wrap_mode, gGLSLP.shader_params[0].scale_type_x));
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, get_gl_wrap_mode(gGLSLP.shader_params[0].wrap_mode, gGLSLP.shader_params[0].scale_type_y));

    SDL_Texture *prevTexture = origTexture;
    gPassID = 0;

    for( int i = 0; i < gGLSLP.shaders - 1; i++ ) {
        SDL_SetRenderTarget(gpRenderer, gGLSLP.shader_params[i].pass_sdl_texture);
        SDL_RenderClear(gpRenderer);
        gPassID++;
        SDL_RenderCopy(gpRenderer, prevTexture, NULL, NULL);
        prevTexture = gGLSLP.shader_params[i].pass_sdl_texture;
    }

    SDL_SetRenderTarget(gpRenderer, gpTexture);
    SDL_RenderClear(gpRenderer);
    gPassID++;
    SDL_RenderCopy(gpRenderer, prevTexture, NULL, &gTextureRect);
    SDL_DestroyTexture(origTexture);
    
    SDL_SetRenderTarget(gpRenderer, NULL);
    SDL_RenderClear(gpRenderer);
    gPassID = 0;
    SDL_RenderCopy(gpRenderer, gpTexture, NULL, NULL);
    
    SDL_GL_SwapWindow(gpWindow);
    
    prevTexture = framePrevTextures[PREV_TEXTURES];
    for( int i = PREV_TEXTURES; i > 0; i-- )
        framePrevTextures[i] = framePrevTextures[i-1];
    framePrevTextures[0] = prevTexture;
    
    gpTexture = NULL; //prevent its deleted when resize...-_-|||
}

const char *get_gl_profile(int flags) {
    switch (flags) {
        case SDL_GL_CONTEXT_PROFILE_ES:
            return "ES";
        case SDL_GL_CONTEXT_PROFILE_COMPATIBILITY:
            return "Compatible";
        case SDL_GL_CONTEXT_PROFILE_CORE:
            return "Core";
        default:
            return "Default";
    }
}

int get_SDL_GLAttribute(SDL_GLattr attr) {
    int orig_value;
    SDL_GL_GetAttribute(attr, &orig_value);
    return orig_value;
}

void VIDEO_GLSL_Init() {
    int orig_major, orig_minor, orig_profile, orig_srgb;
    SDL_GL_GetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, &orig_major);
    SDL_GL_GetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, &orig_minor);
    SDL_GL_GetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, &orig_profile);
    SDL_GL_GetAttribute(SDL_GL_FRAMEBUFFER_SRGB_CAPABLE, &orig_srgb);
#if GLES
    SDL_SetHint( SDL_HINT_RENDER_DRIVER, "opengles2");
#   if SDL_VIDEO_OPENGL_EGL && (SDL_VIDEO_DRIVER_EMSCRIPTEN || SDL_VIDEO_DRIVER_WINRT)
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
#   endif
#else
    SDL_SetHint( SDL_HINT_RENDER_DRIVER, "opengl");
#   if FORCE_OPENGL_CORE_PROFILE
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
#   endif
#endif
    
#if SDL_VIDEO_DRIVER_RPI || SDL_VIDEO_DRIVER_EMSCRIPTEN || SDL_VIDEO_DRIVER_WINRT || SDL_VIDEO_DRIVER_ANDROID
    manualSRGB = 1;
#else
    //
    // iOS need this line to enable built-in color correction
    // WebGL/WinRT/RaspberryPI will not crash with sRGB capable, but will not work with it too.
    // after several tests, Android which version below Nougat completely unable to initial video with sRGB framebuffer capability requested, and MAY CRASH on extension detection;
    // but Oreo behavior changed.
    //
    SDL_GL_SetAttribute(SDL_GL_FRAMEBUFFER_SRGB_CAPABLE, 1);
#endif
    
    Uint32 flags = PAL_VIDEO_INIT_FLAGS | (gConfig.fFullScreen ? SDL_WINDOW_BORDERLESS : 0) | SDL_WINDOW_OPENGL;
    
    UTIL_LogOutput(LOGLEVEL_DEBUG, "requesting to create window with flags: %s %s profile latest available, %s based sRGB gamma correction \n", SDL_GetHint( SDL_HINT_RENDER_DRIVER ),  get_gl_profile(get_SDL_GLAttribute(SDL_GL_CONTEXT_PROFILE_MASK)), manualSRGB ? "shader" : "framebuffer_sRGB" );
    gpWindow = SDL_CreateWindow("Pal", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, gConfig.dwScreenWidth, gConfig.dwScreenHeight, flags);
    if (gpWindow == NULL) {
        UTIL_LogOutput(LOGLEVEL_DEBUG, "failed to create window with ordered flags! %s\n", SDL_GetError());
        UTIL_LogOutput(LOGLEVEL_DEBUG, "reverting to: OpenGL %s profile %d.%d, %s based sRGB gamma correction \n", get_gl_profile(orig_profile), orig_major, orig_minor, "shader" );
        
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, orig_major);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, orig_minor);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK,  orig_profile);
        SDL_GL_SetAttribute(SDL_GL_FRAMEBUFFER_SRGB_CAPABLE, orig_srgb);
        manualSRGB = 1;
        gpWindow = SDL_CreateWindow("Pal", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, gConfig.dwScreenWidth, gConfig.dwScreenHeight, flags);
    }
}

static void dump_preset() {
    FILE *fp = UTIL_OpenFileForMode(MID_GLSLP, "w");
    if(fp) {
        char *content = serialize_glslp(&gGLSLP);
        fputs( content, fp );
        free(content);
        fclose(fp);
    }
}

void VIDEO_GLSL_Setup() {
    SDL_GetRendererOutputSize(gpRenderer, &gRendererWidth, &gRendererHeight);
    SDL_RendererInfo rendererInfo;
    SDL_GetRendererInfo(gpRenderer, &rendererInfo);
    
    for( int i = 0; i < MAX_TEXTURES; i++ )
        frame_prev_texture_units[i] = -1;
    
    UTIL_LogOutput(LOGLEVEL_DEBUG, "render info:%s\n",rendererInfo.name);

    char *glversion = (char*)glGetString(GL_VERSION);
    char *glslversion = (char*)glGetString(GL_SHADING_LANGUAGE_VERSION);
    SDL_sscanf(glversion, "%d.%d", &glversion_major, &glversion_minor);
    if(!strncmp(rendererInfo.name, "opengl", 6)) {
#     ifndef __APPLE__
        if (!initGLExtensions(glversion_major))
            UTIL_LogOutput(LOGLEVEL_FATAL,  "Couldn't init GL extensions!\n" );
#     endif
	}
	else
		UTIL_LogOutput(LOGLEVEL_FATAL, "OpenGL initial failed, check your code!\n");
    UTIL_LogOutput(LOGLEVEL_DEBUG, "GL_VENDOR:%s\n",glGetString(GL_VENDOR));
    UTIL_LogOutput(LOGLEVEL_DEBUG, "GL_VERSION:%s\n",glversion);
    UTIL_LogOutput(LOGLEVEL_DEBUG, "GL_SHADING_LANGUAGE_VERSION:%s\n",glslversion);
    UTIL_LogOutput(LOGLEVEL_DEBUG, "GL_RENDERER:%s\n",glGetString(GL_RENDERER));
    GLint maxTextureSize, maxDrawBuffers, maxColorAttachments;
    glGetIntegerv(GL_MAX_TEXTURE_SIZE, &maxTextureSize);
    glGetIntegerv(GL_MAX_DRAW_BUFFERS, &maxDrawBuffers);
    glGetIntegerv(GL_MAX_COLOR_ATTACHMENTS, &maxColorAttachments);
    UTIL_LogOutput(LOGLEVEL_DEBUG, "GL_MAX_TEXTURE_SIZE:%d\n",maxTextureSize);
    UTIL_LogOutput(LOGLEVEL_DEBUG, "GL_MAX_DRAW_BUFFERS:%d\n",maxDrawBuffers);
    UTIL_LogOutput(LOGLEVEL_DEBUG, "GL_MAX_COLOR_ATTACHMENTS:%d\n",maxColorAttachments);
    if( glversion_major >= 3 ) {
        GLint n, i;
        glGetIntegerv(GL_NUM_EXTENSIONS, &n);
        for (i = 0; i < n; i++) {
            UTIL_LogOutput(LOGLEVEL_DEBUG, "extension %d:%s\n", i, glGetStringi(GL_EXTENSIONS, i));
        }
    }else
        UTIL_LogOutput(LOGLEVEL_DEBUG, "GL_EXTENSIONS:%s\n",glGetString(GL_EXTENSIONS));
    SDL_sscanf(glslversion, "%d.%d", &glslversion_major, &glslversion_minor);
    
    // iOS native GLES supports VAO extension
#if GLES
#if !defined(__APPLE__)
    if(!strncmp(glversion, "OpenGL ES", 9)) {
        SDL_sscanf(glversion, "OpenGL ES %d.%d", &glversion_major, &glversion_minor);
        if( glversion_major <= 2)
            VAOSupported = 0;
    }
#endif
    if(!strncmp(glslversion, "OpenGL ES GLSL ES", 17)) {
        SDL_sscanf(glslversion, "OpenGL ES GLSL ES %d.%d", &glslversion_major, &glslversion_minor);
    }
#ifdef __EMSCRIPTEN__
    // EDGE even on GLES3 does not support VAO. Since hard to detect, disabled totally
    VAOSupported = 0;
#endif
#endif
    
    struct VertexDataFormat vData[ 4 ];
    GLuint iData[ 4 ];
    //Set rendering indices
    iData[ 0 ] = 0;
    iData[ 1 ] = 1;
    iData[ 2 ] = 3;
    iData[ 3 ] = 2;
    
    if(VAOSupported) {
        // Initialize vertex array object
        glGenVertexArrays(MAX_INDEX, gVAOIds);
    }
    
    //Create VBO
    glGenBuffers( MAX_INDEX, gVBOIds );
    
    //Create IBO
    glGenBuffers( 1, &gEBOId );
    glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, gEBOId );
    glBufferData( GL_ELEMENT_ARRAY_BUFFER, 4 * sizeof(GLuint), iData, GL_STATIC_DRAW );
    
    memset(&gMVPSlots,              -1, sizeof(gMVPSlots));
    
    int id = 0;
    
    if(VAOSupported) glBindVertexArray(gVAOIds[id]);
    glBindBuffer( GL_ARRAY_BUFFER, gVBOIds[id] );
    glBufferData( GL_ARRAY_BUFFER, 4 * sizeof(struct VertexDataFormat), vData, GL_DYNAMIC_DRAW );
    glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, gEBOId );
    UTIL_LogSetPrelude("[PASS 1] ");
    gProgramIds[id] = compileProgram(plain_glsl_vert, plain_glsl_frag, 1);
    setupShaderParams(id++);
    
    UTIL_LogSetPrelude(NULL);

    GLSLP tempGLSLP;
    memset(&tempGLSLP,0,sizeof(GLSLP));
    if( access(PAL_va(0,"%s%s%s",gConfig.pszGamePath, PAL_NATIVE_PATH_SEPARATOR,MID_GLSLP), 0) == 0 && parse_glslp(MID_GLSLP,&tempGLSLP) && tempGLSLP.orig_filter && strcmp( tempGLSLP.orig_filter, gConfig.pszShader ) == 0 ) {
        //same file, not needed to parse again
        memcpy(&gGLSLP,&tempGLSLP,sizeof(GLSLP));
        UTIL_LogOutput(LOGLEVEL_DEBUG, "[PASS 2] load parametered filter preset\n");
    }else{
        char *origGLSL = NULL;
        if( SDL_strcasecmp( strrchr(gConfig.pszShader, '.'), ".glsl") == 0 ) {
            UTIL_LogOutput(LOGLEVEL_DEBUG, "[PASS 2] loading %s\n", gConfig.pszShader);
            FILE *fp = UTIL_OpenFileForMode(MID_GLSLP, "w");
            fputs( PAL_va( 0, glslp_template, gConfig.pszShader, gConfig.pszShader, gConfig.dwTextureWidth, gConfig.dwTextureHeight, "false" ), fp );
            fclose(fp);
            origGLSL = gConfig.pszShader;
            gConfig.pszShader = strdup(MID_GLSLP);
        }else{
            UTIL_LogOutput(LOGLEVEL_DEBUG, "[PASS 2] going to parse %s\n", gConfig.pszShader);
        }
        parse_glslp(gConfig.pszShader,&gGLSLP);
        if( origGLSL ) {
            free(gConfig.pszShader);
            gConfig.pszShader = origGLSL;
        }
        assert(gGLSLP.shaders > 0);
    }
    
    for( int i = 0; i < gGLSLP.shaders; i++ ) {
        if(VAOSupported) glBindVertexArray(gVAOIds[id+i]);
        glBindBuffer( GL_ARRAY_BUFFER, gVBOIds[id+i] );
        glBufferData( GL_ARRAY_BUFFER, 4 * sizeof(struct VertexDataFormat), vData, GL_DYNAMIC_DRAW );
        glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, gEBOId );
        UTIL_LogSetPrelude(PAL_va(0,"[PASS 2.%d] ",i+1));
        gProgramIds[id+i] = compileProgram(gGLSLP.shader_params[i].shader, gGLSLP.shader_params[i].shader, 0);
        setupShaderParams(id+i);
    }
    // for debugging usage
    dump_preset();
    for( int i = 0; i < gGLSLP.textures; i++ ) {
        texture_param *param = &gGLSLP.texture_params[i];
        char *texture_name = param->texture_name;
        gGLSLP.texture_params[i].sdl_texture = load_texture(texture_name, param->texture_path, param->linear, param->wrap_mode, SCALE_SOURCE);
        for( int j = 0; j < gGLSLP.shaders; j++ )
            gGLSLP.texture_params[i].slots_pass[id+j] = glGetUniformLocation(gProgramIds[id+j], texture_name);
    }
    for( int i = 0; i < gGLSLP.uniform_parameters; i++ ) {
        uniform_param *param = &gGLSLP.uniform_params[i];
        for( int j = 0; j < gGLSLP.shaders; j++ )
            param->uniform_ids[j] = glGetUniformLocation(gProgramIds[id+j], param->parameter_name);
    }
    Filter_StepParamSlot(0);

#if !__IOS__
    // in case of GL2/GLES2(except iOS), the LACK of the belowing snippit makes keepaspectratio a mess.
    // Unsure what happened.
    if( glversion_major <= 2 ) {
        id=0;
        UTIL_LogSetPrelude("[PASS 1] ");
        glBindBuffer( GL_ARRAY_BUFFER, gVBOIds[id] );
        glBufferData( GL_ARRAY_BUFFER, 4 * sizeof(struct VertexDataFormat), vData, GL_DYNAMIC_DRAW );
        glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, gEBOId );
        setupShaderParams(id);
    }
#endif
    
    if(VAOSupported) glBindVertexArray(0);

    UTIL_LogSetPrelude(NULL);
}

void VIDEO_GLSL_Destroy() {
    // for modified parameters
    if( gGLSLP.shaders > 0 )
        dump_preset();
    
    destroy_glslp(&gGLSLP);
    for( int i = 0; i < MAX_TEXTURES; i++ )
        if( framePrevTextures[i] )
            SDL_DestroyTexture(framePrevTextures[i]);
    memset(framePrevTextures,0,sizeof(framePrevTextures));
    gpTexture = NULL;
}

static int slot = 0;
#define CLAMP(x,a,b) (min(max(x,a),b))
void Filter_StepParamSlot(int step) {
    if( !gConfig.fEnableGLSL || gGLSLP.uniform_parameters <= 0 )
        return;
    slot = (gGLSLP.uniform_parameters + slot + step) % gGLSLP.uniform_parameters;
    uniform_param *param = &gGLSLP.uniform_params[slot];
    UTIL_LogOutput(LOGLEVEL_INFO, "[PARAM] slot:%s cur:%.2f range:[%.2f,%.2f]\n", param->parameter_name, param->value, param->minimum, param->maximum);
}
void Filter_StepCurrentParam(int step) {
    if( !gConfig.fEnableGLSL || gGLSLP.uniform_parameters <= 0 )
        return;
    uniform_param *param = &gGLSLP.uniform_params[slot];
    param->value = CLAMP( param->value + step * param->step, param->minimum, param->maximum);
    UTIL_LogOutput(LOGLEVEL_INFO, "[PARAM] slot:%s cur:%.2f range:[%.2f,%.2f]\n", param->parameter_name, param->value, param->minimum, param->maximum);
}
#endif
