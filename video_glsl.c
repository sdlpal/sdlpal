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

#include "glslp.h"

#define SDL_STBIMAGE_IMPLEMENTATION
#include "SDL_stbimage.h"

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

static uint32_t gProgramIds[MAX_INDEX]={-1};
static uint32_t gVAOIds[MAX_INDEX];
static uint32_t gVBOIds[MAX_INDEX];
static uint32_t gEBOId;
static uint32_t gPassID = -1;
static int gMVPSlots[MAX_INDEX], gHDRSlots[MAX_INDEX], gSRGBSlots[MAX_INDEX];
static int manualSRGB = 0;
static int VAOSupported = 1;
static int glversion_major, glversion_minor;

static SDL_Texture *origTexture;

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

static char *glslp_template = "\r\n\
shaders = 1     \r\n\
shader0 = %s    \r\n\
scale_type0 = absolute   \r\n\
scale_x0 = %d   \r\n\
scale_y0 = %d   \r\n\
";

char *readShaderFile(const char *fileName, GLuint type) {
    FILE *fp = UTIL_OpenRequiredFile(PAL_va(0, "%s/%s", gConfig.pszShaderPath, fileName));
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
    char *pShaderBuffer;
    char *source = sourceOrFilename;
    if(!is_source)
        source = readShaderFile(sourceOrFilename, shaderType);
    size_t sourceLen = strlen(source)*2;
    pShaderBuffer = malloc(sourceLen);
    memset(pShaderBuffer,0, sourceLen);
#if !GLES
    sprintf(pShaderBuffer,"%s\r\n",glversion_major>=3 ? "#version 330" : "#version 110");
#else
    sprintf(pShaderBuffer,"%s\r\n","#version 100");
#endif
#if SUPPORT_PARAMETER_UNIFORM
    sprintf(pShaderBuffer,"%s#define PARAMETER_UNIFORM\r\n",shaderBuffer);
#endif
    sprintf(pShaderBuffer,"%s#define %s\r\n%s\r\n",pShaderBuffer,SHADER_TYPE(shaderType),is_source ? source : skip_version(source));
    if(!is_source)
        free((void*)source);
    
    // Create ID for shader
    GLuint result = glCreateShader(shaderType);
    // Define shader text
    glShaderSource(result, 1, &pShaderBuffer, NULL);
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

void setupShaderParams(int pass, bool mainShader, bool presentShader){
    int shader = pass-2;
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
    if( mainShader )
        gGLSLP.shader_params[shader].slots.tex_coord_slot = slot;
    
    gMVPSlots[pass] = glGetUniformLocation(gProgramIds[pass], "MVPMatrix");
    if(gMVPSlots[pass] < 0)
        UTIL_LogOutput(LOGLEVEL_DEBUG, "uniform MVPMatrix not exist\n");
    
    if( mainShader ) {
        gGLSLP.shader_params[shader].slots.texture_size_slot = glGetUniformLocation(gProgramIds[pass], "TextureSize");
        if(gGLSLP.shader_params[shader].slots.texture_size_slot < 0)
            UTIL_LogOutput(LOGLEVEL_DEBUG, "uniform TextureSize not exist\n");
        
        gGLSLP.shader_params[shader].slots.output_size_slot= glGetUniformLocation(gProgramIds[pass], "OutputSize");
        if(gGLSLP.shader_params[shader].slots.output_size_slot < 0)
            UTIL_LogOutput(LOGLEVEL_DEBUG, "uniform OutputSize not exist\n");
        
        gGLSLP.shader_params[shader].slots.input_size_slot = glGetUniformLocation(gProgramIds[pass], "InputSize");
        if(gGLSLP.shader_params[shader].slots.input_size_slot < 0)
            UTIL_LogOutput(LOGLEVEL_DEBUG, "uniform InputSize not exist\n");
        
        gGLSLP.shader_params[shader].slots.frame_direction_slot = glGetUniformLocation(gProgramIds[pass], "FrameDirection");
        if(gGLSLP.shader_params[shader].slots.frame_direction_slot < 0)
            UTIL_LogOutput(LOGLEVEL_DEBUG, "uniform FrameDirection not exist\n");
        
        gGLSLP.shader_params[shader].slots.frame_count_slot = glGetUniformLocation(gProgramIds[pass],  "FrameCount");
        if(gGLSLP.shader_params[shader].slots.frame_count_slot < 0)
            UTIL_LogOutput(LOGLEVEL_DEBUG, "uniform FrameCount not exist\n");
    }
    
    if( presentShader ) {
        gHDRSlots[pass] = glGetUniformLocation(gProgramIds[pass], "HDR");
        if(gHDRSlots[pass] < 0)
            UTIL_LogOutput(LOGLEVEL_DEBUG, "uniform HDR not exist\n");
        
        gSRGBSlots[pass] = glGetUniformLocation(gProgramIds[pass], "sRGB");
        if(gSRGBSlots[pass] < 0)
            UTIL_LogOutput(LOGLEVEL_DEBUG, "uniform sRGB not exist\n");
    }
}

GLint get_gl_clamp_to_border() {
#ifdef __IOS__
    return GL_CLAMP_TO_EDGE;
#else
    return GL_CLAMP_TO_BORDER;
#endif
}

GLint get_gl_wrap_mode(enum wrap_mode mode) {
    switch (mode) {
        case WRAP_REPEAT:
            return GL_REPEAT;
        case WRAP_CLAMP_TO_EDGE:
            return GL_CLAMP_TO_EDGE;
        case WRAP_CLAMP_TO_BORDER:
            return get_gl_clamp_to_border();
        default:
            return GL_INVALID_ENUM;
    }
}

SDL_Texture *load_texture(char *name, char *path, bool filter_linear, enum wrap_mode mode) {
    SDL_Surface *surf = STBIMG_Load(PAL_va(0, "%s/%s",gConfig.pszShaderPath,path));
    if( filter_linear )
        SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "1");
    SDL_Texture *texture = SDL_CreateTextureFromSurface(gpRenderer, surf);
    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "0");
    SDL_GL_BindTexture(texture, NULL, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, get_gl_wrap_mode(mode));
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, get_gl_wrap_mode(mode));
    return texture;
}

void GetMultiPassUniformLocations(texture_unit_slots *pSlot, int programID, char *prefix) {
    pSlot->texture_slot        = glGetUniformLocation( programID, PAL_va(0, "%sTexture",        prefix) );
    pSlot->texture_size_slot   = glGetUniformLocation( programID, PAL_va(0, "%sTextureSize",    prefix) );
    pSlot->input_size_slot     = glGetUniformLocation( programID, PAL_va(0, "%sInputSize",      prefix) );
    pSlot->tex_coord_slot      = glGetAttribLocation ( programID, PAL_va(0, "%sTexCoord",       prefix) );
}
void SetMultiPassUniforms(texture_unit_slots *pSlot, int shaderID) {
    shader_param *param = &gGLSLP.shader_params[shaderID];
    GLfloat size[2];

    glUniform1i(pSlot->texture_slot, param[shaderID].texture_unit);
    glUniform2fv(pSlot->texture_size_slot, 1, size);
    glUniform2fv(pSlot->input_size_slot, 1, size);
    
    //lacks texcoord attrib; do not know how to calc it
}

int VIDEO_RenderTexture(SDL_Renderer * renderer, SDL_Texture * texture, const SDL_Rect * srcrect, const SDL_Rect * dstrect, int pass)
{
    GLint oldProgramId;
    GLfloat minx, miny, maxx, maxy;
    GLfloat minu, maxu, minv, maxv;
    
    GLfloat texw;
    GLfloat texh;
    GLfloat size[2];

    int shaderID = pass-2;
    static char *prev_names[PREV_TEXTURES] = {
        "PREV",
        "PREV1",
        "PREV2",
        "PREV3",
        "PREV4",
        "PREV5",
        "PREV6",
    };

    //get needed uniform locations
    if( pass >= 2 ) {
        GetMultiPassUniformLocations(&gGLSLP.shader_params[shaderID].orig_slots, gProgramIds[pass], "Orig");
        if( pass >= 3 ){
            for( int i = 0; i < shaderID-1; i++ ) {
                if( shaderID - i < PREV_TEXTURES )
                    GetMultiPassUniformLocations(&gGLSLP.shader_params[shaderID].prev_slots[i], gProgramIds[pass], prev_names[shaderID - i] );
                GetMultiPassUniformLocations(&gGLSLP.shader_params[shaderID].pass_slots[i], gProgramIds[pass], PAL_va(0, "Pass%d", i+1) );
                GetMultiPassUniformLocations(&gGLSLP.shader_params[shaderID].pass_slots[i], gProgramIds[pass], PAL_va(0, "PassPrev%d", shaderID-i+1) );
                if( gGLSLP.shader_params[i].alias )
                    GetMultiPassUniformLocations(&gGLSLP.shader_params[shaderID].alias_slots, gProgramIds[pass], gGLSLP.shader_params[i].alias );
            }
        }
    }
    

    glActiveTexture(GL_TEXTURE0);
    SDL_GL_BindTexture(texture, &texw, &texh);

    if(gGLSLP.shader_params[gGLSLP.shaders-1].scale_type_x == SCALE_ABSOLUTE)
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, get_gl_clamp_to_border());
    if(gGLSLP.shader_params[gGLSLP.shaders-1].scale_type_y == SCALE_ABSOLUTE)
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, get_gl_clamp_to_border());
    
    //calc texture unit:1(main texture)+glslp_textures+glsl_uniform_textures(orig,pass(1-6),prev(1-6))
    
    int texture_unit_used = 1;
    if( pass >= 2 ) {
        //global
        if( gGLSLP.textures > 0 ) {
            for( int i = 0; i < gGLSLP.textures; i++ ) {
                glActiveTexture(GL_TEXTURE0+texture_unit_used++);
                SDL_GL_BindTexture(gGLSLP.texture_params[i].sdl_texture,NULL,NULL);
            }
        }
        //orig
        glActiveTexture(GL_TEXTURE0+texture_unit_used++);
        SDL_GL_BindTexture(origTexture,NULL,NULL);
        if( pass >= 3 ) {
            //prev-prev%
            for( int i = shaderID-1; i >= 0; i-- ) {
                glActiveTexture(GL_TEXTURE0+texture_unit_used++);
                SDL_GL_BindTexture(gGLSLP.shader_params[i].sdl_texture,NULL,NULL);
                gGLSLP.shader_params[i].texture_unit = texture_unit_used-1;
            }
            //pass%
            for( int i = 0; i < shaderID-1; i++ ) {
                glActiveTexture(GL_TEXTURE0+texture_unit_used++);
                SDL_GL_BindTexture(gGLSLP.shader_params[i].sdl_texture,NULL,NULL);
                gGLSLP.shader_params[i].texture_unit = texture_unit_used-1;
            }
            //passprev%
            for( int i = shaderID-1; i >= 0; i-- ) {
                glActiveTexture(GL_TEXTURE0+texture_unit_used++);
                SDL_GL_BindTexture(gGLSLP.shader_params[i].sdl_texture,NULL,NULL);
                gGLSLP.shader_params[i].texture_unit = texture_unit_used-1;
            }
        }
    }

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
    
    GLint HDR = gConfig.fEnableHDR;
    glUniform1i(gHDRSlots[pass], HDR);
    glUniform1i(gSRGBSlots[pass], manualSRGB);

    // tell shaders texture slots
    texture_unit_used = 1;
    //global
    if( gGLSLP.textures > 0 ) {
        for( int i = 0; i < gGLSLP.textures; i++ ) {
            glUniform1i(gGLSLP.texture_params[i].slots[pass], texture_unit_used++);
        }
    }
    if( pass >= 2 ) {
        //orig
        glUniform1i(gGLSLP.shader_params[shaderID].orig_slots.texture_slot, texture_unit_used++);
        if( pass >= 3 ){
            //prev-prev%
            for( int i = shaderID-1; i >= 0; i-- ) {
                SetMultiPassUniforms(&gGLSLP.shader_params[shaderID].prev_slots[i], i );
            }
            //pass%
            for( int i = 0; i < shaderID-1; i++ ) {
                SetMultiPassUniforms(&gGLSLP.shader_params[shaderID].pass_slots[i], i );
            }
            //passprev%
            for( int i = shaderID-1; i >= 0; i-- ) {
                SetMultiPassUniforms(&gGLSLP.shader_params[shaderID].passprev_slots[i], i );
            }
            //lacks aliases
        }
        
        int src_pow = next_pow2(max(320,200));
        size[0] = src_pow;
        size[1] = src_pow;
        glUniform2fv(gGLSLP.shader_params[shaderID].orig_slots.texture_size_slot, 1, size);
        
        size[0] = shaderID > 0 ? gGLSLP.shader_params[shaderID-1].FBO.width  : 320;
        size[1] = shaderID > 0 ? gGLSLP.shader_params[shaderID-1].FBO.height : 200;
        glUniform2fv(gGLSLP.shader_params[shaderID].slots.texture_size_slot, 1, size);
        glUniform2fv(gGLSLP.shader_params[shaderID].slots.input_size_slot,   1, size);
        size[0] = gGLSLP.shader_params[shaderID].FBO.width;
        size[1] = gGLSLP.shader_params[shaderID].FBO.height;
        glUniform2fv(gGLSLP.shader_params[shaderID].slots.output_size_slot,  1, size);

        glUniform1i(gGLSLP.shader_params[shaderID].slots.frame_direction_slot, 1.0);
        //SDLPal don't support rewinding so direction is always 1

        gGLSLP.shader_params[shaderID].frame_count = gGLSLP.shader_params[shaderID].frame_count+1;
        if( gGLSLP.shader_params[shaderID].frame_count_mod )
            gGLSLP.shader_params[shaderID].frame_count %= gGLSLP.shader_params[shaderID].frame_count_mod;
        glUniform1i(gGLSLP.shader_params[shaderID].slots.frame_count_slot, gGLSLP.shader_params[shaderID].frame_count);
    }
    
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
    
    for( int i=0; i<MAX_INDEX; i++)
        gOrthoMatrixes[i] = GLKMatrix4MakeOrtho(0, width, 0, height, -1, 1);
    gOrthoMatrixes[0] = GLKMatrix4MakeOrtho(0, width, height, 0, -1, 1);
    gOrthoMatrixes[1] = GLKMatrix4MakeOrtho(0, width, height, 0, -1, 1);

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
    
    int viewport_pow = next_pow2(max(gConfig.dwTextureWidth,gConfig.dwTextureHeight));

    for( int i = 0; i < gGLSLP.shaders; i++ ) {
        shader_param *param = &gGLSLP.shader_params[i];
        if( param->sdl_texture )
            SDL_DestroyTexture( param->sdl_texture );
        
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
        
        if( param->filter_linear )
            SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "1");
        param->sdl_texture = SDL_CreateTexture(gpRenderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_TARGET, param->FBO.pow_width, param->FBO.pow_height);
        SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "0");
        SDL_GL_BindTexture(param->sdl_texture, NULL, NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, get_gl_wrap_mode(param->wrap_mode));
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, get_gl_wrap_mode(param->wrap_mode));
        //lacks parameter processing:
        //srgb
        //float
        //mipmap
    }
    
    //
    // Create texture for screen.
    //
    return SDL_CreateTexture(gpRenderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_TARGET, viewport_pow, viewport_pow);
}

void VIDEO_GLSL_RenderCopy()
{
    origTexture = SDL_CreateTextureFromSurface(gpRenderer, gpScreenReal);
    SDL_Texture *prevTexture = origTexture;
    gPassID = 1;

    for( int i = 0; i < gGLSLP.shaders - 1; i++ ) {
        SDL_SetRenderTarget(gpRenderer, gGLSLP.shader_params[i].sdl_texture);
        SDL_RenderClear(gpRenderer);
        gPassID++;
        SDL_RenderCopy(gpRenderer, prevTexture, NULL, NULL);
        prevTexture = gGLSLP.shader_params[i].sdl_texture;
    }

    SDL_SetRenderTarget(gpRenderer, gpTexture);
    SDL_RenderClear(gpRenderer);
    gPassID++;
    SDL_RenderCopy(gpRenderer, prevTexture, NULL, &gTextureRect);
    SDL_DestroyTexture(origTexture);
    
    SDL_SetRenderTarget(gpRenderer, NULL);
    SDL_RenderClear(gpRenderer);
    gPassID = 1;
    SDL_RenderCopy(gpRenderer, gpTexture, NULL, NULL);
    
    if (gpTouchOverlay)
    {
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE);
        gPassID = 0;
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
        glGenVertexArrays(MAX_INDEX, gVAOIds);
    }
    
    //Create VBO
    glGenBuffers( MAX_INDEX, gVBOIds );
    
    //Create IBO
    glGenBuffers( 1, &gEBOId );
    glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, gEBOId );
    glBufferData( GL_ELEMENT_ARRAY_BUFFER, 4 * sizeof(GLuint), iData, GL_DYNAMIC_DRAW );
    
    memset(&gMVPSlots,              -1, sizeof(gMVPSlots));
    memset(&gHDRSlots,              -1, sizeof(gHDRSlots));
    memset(&gSRGBSlots,             -1, sizeof(gSRGBSlots));
    
    int id = 0;
    
    if(VAOSupported) glBindVertexArray(gVAOIds[id]);
    glBindBuffer( GL_ARRAY_BUFFER, gVBOIds[id] );
    glBufferData( GL_ARRAY_BUFFER, 4 * sizeof(struct VertexDataFormat), vData, GL_DYNAMIC_DRAW );
    glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, gEBOId );
    UTIL_LogSetPrelude("[PASS 1] ");
    gProgramIds[id] = compileProgram(plain_glsl_vert, plain_glsl_frag_overlay, 1);
    setupShaderParams(id++,false,false);
    
    if(VAOSupported) glBindVertexArray(gVAOIds[id]);
    glBindBuffer( GL_ARRAY_BUFFER, gVBOIds[id] );
    glBufferData( GL_ARRAY_BUFFER, 4 * sizeof(struct VertexDataFormat), vData, GL_DYNAMIC_DRAW );
    glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, gEBOId );
    UTIL_LogSetPrelude("[PASS 2] ");
    gProgramIds[id] = compileProgram(plain_glsl_vert, plain_glsl_frag, 1);
    setupShaderParams(id++,false,true);
    
    if( strcmp( strrchr(gConfig.pszShader, '.'), ".glslp") != 0 ) {
        char *tempFile = "sdlpal.glslp";
        FILE *fp = UTIL_OpenFileAtPathForMode(gConfig.pszShaderPath, tempFile, "wt"); //follow retroarch spec, this folder needs to be writable
        fputs( PAL_va( 0, glslp_template, gConfig.pszShader, gConfig.dwTextureWidth, gConfig.dwTextureHeight ), fp );
        fclose(fp);
        gConfig.pszShader = strdup(tempFile);
    }
    parse_glslp(gConfig.pszShader);
    assert(gGLSLP.shaders > 0);
    for( int i = 0; i < gGLSLP.shaders; i++ ) {
        if(VAOSupported) glBindVertexArray(gVAOIds[id+i]);
        glBindBuffer( GL_ARRAY_BUFFER, gVBOIds[id+i] );
        glBufferData( GL_ARRAY_BUFFER, 4 * sizeof(struct VertexDataFormat), vData, GL_DYNAMIC_DRAW );
        glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, gEBOId );
        UTIL_LogSetPrelude(PAL_va(0,"[PASS 3.%d] ",i+1));
        gProgramIds[id+i] = compileProgram(gGLSLP.shader_params[i].shader, gGLSLP.shader_params[i].shader, 0);
        setupShaderParams(id+i,true,false);
    }
    for( int i = 0; i < gGLSLP.textures; i++ ) {
        texture_param *param = &gGLSLP.texture_params[i];
        char *texture_name = param->texture_name;
        gGLSLP.texture_params[i].sdl_texture = load_texture(texture_name, param->texture_path, param->linear, param->wrap_mode);
        for( int j = 0; j < gGLSLP.shaders; j++ )
            gGLSLP.texture_params[i].slots[id+j] = glGetUniformLocation(gProgramIds[id+j], texture_name);
    }

    UTIL_LogSetPrelude(NULL);
    
    if(VAOSupported) glBindVertexArray(0);
}

void VIDEO_GLSL_Destroy() {
    destroy_glslp();
}

#endif
