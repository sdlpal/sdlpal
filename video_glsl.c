//
//  video_glsl.c
//  Pal
//
//  Created by palxex on 27/2/18.
//

#include "main.h"

#if PAL_HAS_GLSL

#include "video_glsl.h"
#include "video.h"

#define FORCE_OPENGL_CORE_PROFILE 1
//#define SUPPORT_PARAMETER_UNIFORM 1

extern SDL_Window        *gpWindow;
extern SDL_Surface       *gpScreenReal;
extern SDL_Renderer      *gpRenderer;
extern SDL_Texture       *gpTexture;
extern SDL_Texture       *gpTouchOverlay;
extern SDL_Rect           gOverlayRect;
extern SDL_Rect           gTextureRect;

static int gRendererWidth;
static int gRendererHeight;

static uint32_t gProgramIds[]={-1,-1,-1};
static uint32_t gVAOIds[3]; // 0 for game screen; 1 for touch overlay; 2 for final blit
static uint32_t gVBOIds[3];
static uint32_t gEBOId;
static uint32_t gPassID = -1;
static int gMVPSlots[3], gTextureSizeSlots[3], gHDRSlot[3], gSRGBSlot[3];
static int manualSRGB = 0;
static int VAOSupported = 1;
static int glversion_major, glversion_minor;

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

static GLKMatrix4 gOrthoMatrixes[3];

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
uniform int sRGB;                    \r\n\
vec3 ACESFilm(vec3 x)                \r\n\
{                                    \r\n\
const float A = 2.51;            \r\n\
const float B = 0.03;            \r\n\
const float C = 2.43;            \r\n\
const float D = 0.59;            \r\n\
const float E = 0.14;            \r\n\
return (x * (A * x + B)) / (x * (C * x + D) + E); \r\n\
}                                    \r\n\
const float SRGB_ALPHA = 0.055;        \r\n\
float linear_to_srgb(float channel) {\r\n\
if(channel <= 0.0031308)        \r\n\
return 12.92 * channel;        \r\n\
else                            \r\n\
return (1.0 + SRGB_ALPHA) * pow(channel, 1.0/2.4) - SRGB_ALPHA;    \r\n\
}                                    \r\n\
vec3 rgb_to_srgb(vec3 rgb) {        \r\n\
return vec3(linear_to_srgb(rgb.r), linear_to_srgb(rgb.g), linear_to_srgb(rgb.b)    ); \r\n\
}                                    \r\n\
float srgb_to_linear(float channel) {    \r\n\
if (channel <= 0.04045)                \r\n\
return channel / 12.92;            \r\n\
else                                \r\n\
return pow((channel + SRGB_ALPHA) / (1.0 + SRGB_ALPHA), 2.4);    \r\n\
}                                    \r\n\
vec3 srgb_to_rgb(vec3 srgb) {        \r\n\
return vec3(srgb_to_linear(srgb.r),    srgb_to_linear(srgb.g),    srgb_to_linear(srgb.b));\r\n\
}\r\n\
void main()                         \r\n\
{                                   \r\n\
FragColor = vec4(srgb_to_rgb(COMPAT_TEXTURE(tex0 , v_texCoord.xy).rgb), 1.0);  \r\n\
#ifdef GL_ES                        \r\n\
FragColor.rgb = FragColor.bgr;       \r\n\
#endif                              \r\n\
vec3 color = FragColor.rgb;            \r\n\
if( HDR > 0 )                        \r\n\
color = ACESFilm(color);        \r\n\
if( sRGB > 0 )                      \r\n\
color = rgb_to_srgb(color);     \r\n\
FragColor.rgb=color; \r\n\
}";
static char *plain_glsl_frag_overlay = "\r\n\
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
void main()                         \r\n\
{                                   \r\n\
FragColor = vec4(COMPAT_TEXTURE(tex0 , v_texCoord.xy).rgb, 0.5);  \r\n\
float sat = (FragColor.r+FragColor.g+FragColor.b)/3.0; \r\n\
vec3 average = vec3(sat,sat,sat);    \r\n\
FragColor.rgb -= average*0.8;               \r\n\
}";

char *readShaderFile(const char *fileName, GLuint type) {
    FILE *fp = UTIL_OpenRequiredFile(fileName);
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

GLuint compileShader(char* sourceOrFilename, GLuint shaderType, int is_source) {
    //DONT CHANGE
#define SHADER_TYPE(shaderType) (shaderType == GL_VERTEX_SHADER ? "VERTEX" : "FRAGMENT")
    char shaderBuffer[65536];
    const char *pBuf = shaderBuffer;
    memset(shaderBuffer,0,sizeof(shaderBuffer));
    char *source = sourceOrFilename;
    if(!is_source)
        source = readShaderFile(sourceOrFilename, shaderType);
#if !GLES
    sprintf(shaderBuffer,"%s\r\n",glversion_major>3 ? "#version 330" : "#version 110");
#else
    sprintf(shaderBuffer,"%s\r\n","#version 100");
#endif
#if SUPPORT_PARAMETER_UNIFORM
    sprintf(shaderBuffer,"%s#define PARAMETER_UNIFORM\r\n",shaderBuffer);
#endif
    sprintf(shaderBuffer,"%s#define %s\r\n%s\r\n",shaderBuffer,SHADER_TYPE(shaderType),is_source ? source : skip_version(source));
    if(!is_source)
        free((void*)source);
    
    // Create ID for shader
    GLuint result = glCreateShader(shaderType);
    // Define shader text
    glShaderSource(result, 1, &pBuf, NULL);
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
            UTIL_LogOutput(LOGLEVEL_DEBUG, "shader %s compilation error:%s\n", is_source ? "stock" : sourceOrFilename,log);
            free(log);
        }
        glDeleteShader(result);
        result = 0;
    }else
        UTIL_LogOutput(LOGLEVEL_DEBUG, "%s shader %s compilation succeed!\n", SHADER_TYPE(shaderType), is_source ? "stock" : sourceOrFilename );
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
        GLint logLen;
        glGetProgramiv(programId, GL_INFO_LOG_LENGTH, &logLen);
        if(logLen > 0) {
            char* log = (char*) malloc(logLen * sizeof(char));
            // Show any errors as appropriate
            glGetProgramInfoLog(programId, logLen, &logLen, log);
            UTIL_LogOutput(LOGLEVEL_DEBUG, "shader linkage error:%s\n",log);
            free(log);
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
    int slot = glGetAttribLocation(gProgramIds[pass], "VertexCoord");
    if(slot >= 0) {
        glEnableVertexAttribArray(slot);
        glVertexAttribPointer(slot, 4, GL_FLOAT, GL_FALSE, sizeof(struct VertexDataFormat), (GLvoid*)offsetof(struct VertexDataFormat, position));
    }else{
        UTIL_LogOutput(LOGLEVEL_DEBUG, "attrib VertexCoord not exist\n");
    }
    
    slot = glGetAttribLocation(gProgramIds[pass], "TexCoord");
    if(slot >= 0) {
        glEnableVertexAttribArray(slot);
        glVertexAttribPointer(slot, 4, GL_FLOAT, GL_FALSE, sizeof(struct VertexDataFormat), (GLvoid*)offsetof(struct VertexDataFormat, texCoord));
    }else{
        UTIL_LogOutput(LOGLEVEL_DEBUG, "attrib TexCoord not exist\n");
    }
    
    gMVPSlots[pass] = glGetUniformLocation(gProgramIds[pass], "MVPMatrix");
    if(gMVPSlots[pass] < 0)
        UTIL_LogOutput(LOGLEVEL_DEBUG, "uniform MVPMatrix not exist\n");
    
    gTextureSizeSlots[pass] = glGetUniformLocation(gProgramIds[pass], "TextureSize");
    if(gTextureSizeSlots[pass] < 0)
        UTIL_LogOutput(LOGLEVEL_DEBUG, "uniform TextureSize not exist\n");
    
    gHDRSlot[pass] = glGetUniformLocation(gProgramIds[pass], "HDR");
    if(gHDRSlot[pass] < 0)
        UTIL_LogOutput(LOGLEVEL_DEBUG, "uniform HDR not exist\n");
    
    gSRGBSlot[pass] = glGetUniformLocation(gProgramIds[pass], "sRGB");
    if(gSRGBSlot[pass] < 0)
        UTIL_LogOutput(LOGLEVEL_DEBUG, "uniform sRGB not exist\n");
}

int VIDEO_RenderTexture(SDL_Renderer * renderer, SDL_Texture * texture, const SDL_Rect * srcrect, const SDL_Rect * dstrect, int pass)
{
    GLint oldProgramId;
    GLfloat minx, miny, maxx, maxy;
    GLfloat minu, maxu, minv, maxv;
    
    GLfloat texw;
    GLfloat texh;
    SDL_GL_BindTexture(texture, &texw, &texh);
    
#ifndef GL_ES_VERSION_3_0
    if(!manualSRGB)
        glEnable(GL_FRAMEBUFFER_SRGB);
#endif
    
    if(gProgramIds[pass] != -1) {
        glGetIntegerv(GL_CURRENT_PROGRAM,&oldProgramId);
        glUseProgram(gProgramIds[pass]);
    }
    
    // Setup MVP projection matrix uniform
    glUniformMatrix4fv(gMVPSlots[pass], 1, GL_FALSE, gOrthoMatrixes[pass].m);
    
    GLfloat textureSize[2];
    textureSize[0] = 320.0;
    textureSize[1] = 200.0;
    glUniform2fv(gTextureSizeSlots[pass], 1, textureSize);
    
    GLint HDR = gConfig.fEnableHDR;
    glUniform1iv(gHDRSlot[pass], 1, &HDR);
    
    glUniform1iv(gSRGBSlot[pass], 1, &manualSRGB);
    
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
int CORE_RenderCopy(SDL_Renderer * renderer, SDL_Texture * texture,
                    const SDL_Rect * srcrect, const SDL_Rect * dstrect)
{
    return VIDEO_RenderTexture(renderer, texture, srcrect, dstrect, gPassID);
}

SDL_Texture *VIDEO_GLSL_CreateTexture(int width, int height)
{
    gRendererWidth = width;
    gRendererHeight = height;

    double ratio = (double)width / (double)height;
    ratio *= 1.6f * (double)gConfig.dwTextureHeight / (double)gConfig.dwTextureWidth;
    
    gOrthoMatrixes[0] = GLKMatrix4MakeOrtho(0, width, height, 0, -1, 1);
    gOrthoMatrixes[1] = GLKMatrix4MakeOrtho(0, width, height, 0, -1, 1);
    gOrthoMatrixes[2] = GLKMatrix4MakeOrtho(0, width, 0, height, -1, 1);

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
        
        VIDEO_SetupTouchArea(width, height, w, h);
    }
    else
    {
        gTextureRect.x = gTextureRect.y = 0;
        gTextureRect.w = width; gTextureRect.h = height;
        VIDEO_SetupTouchArea(width, height, width, height);
    }
    
    //
    // Create texture for screen.
    //
    return SDL_CreateTexture(gpRenderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_TARGET, gConfig.dwTextureWidth, gConfig.dwTextureHeight);
}

void VIDEO_GLSL_RenderCopy()
{
    SDL_SetRenderTarget(gpRenderer, gpTexture);
    SDL_RenderClear(gpRenderer);
    SDL_Texture *screenTexture = SDL_CreateTextureFromSurface(gpRenderer, gpScreenReal);
    gPassID = 0;
    SDL_RenderCopy(gpRenderer, screenTexture, NULL, &gTextureRect);
    SDL_DestroyTexture(screenTexture);
    
    SDL_SetRenderTarget(gpRenderer, NULL);
    SDL_RenderClear(gpRenderer);
    gPassID = 2;
    SDL_RenderCopy(gpRenderer, gpTexture, NULL, NULL);
    
    if (gpTouchOverlay)
    {
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE);
        gPassID = 1;
        SDL_RenderCopy(gpRenderer, gpTouchOverlay, NULL, &gOverlayRect);
    }
    SDL_GL_SwapWindow(gpWindow);
}

void VIDEO_GLSL_Init() {
    int orig_major, orig_minor, orig_profile, orig_srgb;
    SDL_GL_GetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, &orig_major);
    SDL_GL_GetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, &orig_minor);
    SDL_GL_GetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, &orig_profile);
    SDL_GL_GetAttribute(SDL_GL_FRAMEBUFFER_SRGB_CAPABLE, &orig_srgb);
#if GLES
    SDL_SetHint( SDL_HINT_RENDER_DRIVER, "opengles2");
#   if SDL_VIDEO_OPENGL_EGL
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
#   endif
#else
    SDL_SetHint( SDL_HINT_RENDER_DRIVER, "opengl");
#   if FORCE_OPENGL_CORE_PROFILE
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
#   endif
#endif
    //
    // iOS need this line to enable built-in color correction
    // WebGL/WinRT/RaspberryPI will not crash with sRGB capable, but will not work with it too.
    // after several tests, Android which version below Nougat completely unable to initial video with sRGB framebuffer capability requested, and MAY CRASH on extension detection;
    // but Oreo behavior changed.
    //
    SDL_GL_SetAttribute(SDL_GL_FRAMEBUFFER_SRGB_CAPABLE, 1);
    
#if SDL_VIDEO_DRIVER_RPI || SDL_VIDEO_DRIVER_EMSCRIPTEN || SDL_VIDEO_DRIVER_WINRT || SDL_VIDEO_DRIVER_ANDROID
    manualSRGB = 1;
#endif
    
    Uint32 flags = PAL_VIDEO_INIT_FLAGS | (gConfig.fFullScreen ? SDL_WINDOW_BORDERLESS : 0) | SDL_WINDOW_OPENGL;
    gpWindow = SDL_CreateWindow("Pal", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, gConfig.dwScreenWidth, gConfig.dwScreenHeight, flags);
    if (gpWindow == NULL) {
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, orig_major);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, orig_minor);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK,  orig_profile);
        SDL_GL_SetAttribute(SDL_GL_FRAMEBUFFER_SRGB_CAPABLE, orig_srgb);
        manualSRGB = 1;
        gpWindow = SDL_CreateWindow("Pal", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, gConfig.dwScreenWidth, gConfig.dwScreenHeight, flags);
    }
}

void VIDEO_GLSL_Setup() {
    SDL_GetRendererOutputSize(gpRenderer, &gRendererWidth, &gRendererHeight);

    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, gConfig.pszScaleQuality);
    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "0");
    SDL_RendererInfo rendererInfo;
    SDL_GetRendererInfo(gpRenderer, &rendererInfo);
    
    UTIL_LogOutput(LOGLEVEL_DEBUG, "render info:%s\n",rendererInfo.name);
    if(!strncmp(rendererInfo.name, "opengl", 6)) {
#     ifndef __APPLE__
        // If you want to use GLEW or some other GL extension handler, do it here!
        if (!initGLExtensions()) {
            UTIL_LogOutput(LOGLEVEL_DEBUG,  "Couldn't init GL extensions!\n" );
            SDL_Quit();
            exit(-1);
        }
#     endif
    }
    char *glversion = (char*)glGetString(GL_VERSION);
    UTIL_LogOutput(LOGLEVEL_DEBUG, "GL_VERSION:%s\n",glversion);
    UTIL_LogOutput(LOGLEVEL_DEBUG, "GL_SHADING_LANGUAGE_VERSION:%s\n",glGetString(GL_SHADING_LANGUAGE_VERSION));
    UTIL_LogOutput(LOGLEVEL_DEBUG, "GL_RENDERER:%s\n",glGetString(GL_RENDERER));
    GLint maxTextureSize, maxDrawBuffers, maxColorAttachments;
    glGetIntegerv(GL_MAX_TEXTURE_SIZE, &maxTextureSize);
    glGetIntegerv(GL_MAX_DRAW_BUFFERS, &maxDrawBuffers);
    glGetIntegerv(GL_MAX_COLOR_ATTACHMENTS, &maxColorAttachments);
    UTIL_LogOutput(LOGLEVEL_DEBUG, "GL_MAX_TEXTURE_SIZE:%d\n",maxTextureSize);
    UTIL_LogOutput(LOGLEVEL_DEBUG, "GL_MAX_DRAW_BUFFERS:%d\n",maxDrawBuffers);
    UTIL_LogOutput(LOGLEVEL_DEBUG, "GL_MAX_COLOR_ATTACHMENTS:%d\n",maxColorAttachments);
    SDL_sscanf(glversion, "%d.%d", &glversion_major, &glversion_minor);
    if( glversion_major >= 3 ) {
        GLint n, i;
        glGetIntegerv(GL_NUM_EXTENSIONS, &n);
        for (i = 0; i < n; i++) {
            UTIL_LogOutput(LOGLEVEL_DEBUG, "extension %d:%s\n", i, glGetStringi(GL_EXTENSIONS, i));
        }
    }else
        UTIL_LogOutput(LOGLEVEL_DEBUG, "GL_EXTENSIONS:%s\n",glGetString(GL_EXTENSIONS));
    
    // iOS native GLES supports VAO extension
#if GLES && !defined(__APPLE__)
    if(!strncmp(glversion, "OpenGL ES", 9)) {
        SDL_sscanf(glversion, "OpenGL ES %d.%d", &glversion_major, &glversion_minor);
        if( glversion_major <= 2)
            VAOSupported = 0;
    }
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
        glGenVertexArrays(3, gVAOIds);
        glBindVertexArray(gVAOIds[0]);
    }
    
    UTIL_LogSetPrelude("[PASS 1] ");
    gProgramIds[0] = compileProgram(gConfig.pszShader,gConfig.pszShader, 0);
    
    //Create VBO
    glGenBuffers( 3, gVBOIds );
    glBindBuffer( GL_ARRAY_BUFFER, gVBOIds[0] );
    glBufferData( GL_ARRAY_BUFFER, 4 * sizeof(struct VertexDataFormat), vData, GL_DYNAMIC_DRAW );
    
    //Create IBO
    glGenBuffers( 1, &gEBOId );
    glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, gEBOId );
    glBufferData( GL_ELEMENT_ARRAY_BUFFER, 4 * sizeof(GLuint), iData, GL_DYNAMIC_DRAW );
    
    setupShaderParams(0);
    
    if(VAOSupported) glBindVertexArray(gVAOIds[1]);
    glBindBuffer( GL_ARRAY_BUFFER, gVBOIds[1] );
    glBufferData( GL_ARRAY_BUFFER, 4 * sizeof(struct VertexDataFormat), vData, GL_DYNAMIC_DRAW );
    glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, gEBOId );
    UTIL_LogSetPrelude("[PASS 2] ");
    gProgramIds[1] = compileProgram(plain_glsl_vert, plain_glsl_frag_overlay, 1);
    setupShaderParams(1);
    
    if(VAOSupported) glBindVertexArray(gVAOIds[2]);
    glBindBuffer( GL_ARRAY_BUFFER, gVBOIds[2] );
    glBufferData( GL_ARRAY_BUFFER, 4 * sizeof(struct VertexDataFormat), vData, GL_DYNAMIC_DRAW );
    glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, gEBOId );
    UTIL_LogSetPrelude("[PASS 3] ");
    gProgramIds[2] = compileProgram(plain_glsl_vert, plain_glsl_frag, 1);
    setupShaderParams(2);
    
    UTIL_LogSetPrelude(NULL);
    
    if(VAOSupported) glBindVertexArray(0);
}
#endif
