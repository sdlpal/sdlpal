//
//  glslp.c
//  Pal
//
//  Created by palxex on 11/7/18.
//

#include "glslp.h"

GLSLP gGLSLP;

//uniform parameter is not needed since currently SDLPal lacks runtime configuation alter method like RetroArch GUI/kind of immediate UI/at least a quake console.
//lets consider it after anyone of them introduced:)

typedef enum tagLineType {
    TOKEN_NONE,
    
    TOKEN_SHADERS,
    TOKEN_SHADER_PATH,
    TOKEN_SHADER_ALIAS,
    TOKEN_SHADER_FILTER_LINEAR,
    TOKEN_SHADER_WRAR_MODE,
    TOKEN_SHADER_SCALE_TYPE,
    TOKEN_SHADER_SCALE_TYPE_X,
    TOKEN_SHADER_SCALE_TYPE_Y,
    TOKEN_SHADER_SCALE,
    TOKEN_SHADER_SCALE_X,
    TOKEN_SHADER_SCALE_Y,
    TOKEN_SHADER_FLOAT_FRAMEBUFFER,
    TOKEN_SHADER_SRGB_FRAMEBUFFER,
    TOKEN_SHADER_MIPMAP_INPUT,
    TOKEN_SHADER_FRAME_COUNT_MOD,
    
    TOKEN_TEXTURES,
    TOKEN_TEXTURE_PATH,
    TOKEN_TEXTURE_WRAP_MODE,
    TOKEN_TEXTURE_LINEAR,
    TOKEN_TEXTURE_MIPMAP,
    
    TOKEN_END
}LineType;

static char *tokens[] = {
    "",
    
    "shaders",
    "shader%d",
    "alias%d",
    "filter_linear%d",
    "wrap_mode%d", //texture_wrap_mode which used in some glsl is not in spec, maybe obsolete?
    "scale_type%d",
    "scale_type_x%d",
    "scale_type_y%d",
    "scale%d",
    "scale_x%d",
    "scale_y%d",
    "float_framebuffer%d",
    "srgb_framebuffer%d",
    "mipmap_input%d",
    "frame_count_mod%d",

    "textures",
    "%s",
    "%s_wrap_mode",
    "%s_linear",
    "%s_mipmap",

    ""
};

static int token_conform( const char *name, LineType type ) {
    int index = -1;
    switch( type ) {
        case TOKEN_SHADERS:
        case TOKEN_TEXTURES:
            if( strcmp(name, tokens[type]) == 0 )
                index = 0;
            break;
        case TOKEN_SHADER_PATH:
        case TOKEN_SHADER_ALIAS:
        case TOKEN_SHADER_FILTER_LINEAR:
        case TOKEN_SHADER_WRAR_MODE:
        case TOKEN_SHADER_SCALE_TYPE:
        case TOKEN_SHADER_SCALE_TYPE_X:
        case TOKEN_SHADER_SCALE_TYPE_Y:
        case TOKEN_SHADER_SCALE:
        case TOKEN_SHADER_SCALE_X:
        case TOKEN_SHADER_SCALE_Y:
        case TOKEN_SHADER_FLOAT_FRAMEBUFFER:
        case TOKEN_SHADER_SRGB_FRAMEBUFFER:
        case TOKEN_SHADER_MIPMAP_INPUT:
        case TOKEN_SHADER_FRAME_COUNT_MOD:
            for( int i = 0; i < gGLSLP.shaders; i++ ) {
                if( strcmp(name, PAL_va(0, tokens[type], i) ) == 0 ) {
                    index = i;
                    break;
                }
            }
            break;
        case TOKEN_TEXTURE_PATH:
        case TOKEN_TEXTURE_WRAP_MODE:
        case TOKEN_TEXTURE_LINEAR:
        case TOKEN_TEXTURE_MIPMAP:
            for( int i = 0; i < gGLSLP.textures; i++ ) {
                texture_param *param = &gGLSLP.texture_params[i];
                if( strcmp(name, PAL_va(0, tokens[type], param->texture_name) ) == 0 ) {
                    index = i;
                    break;
                }
            }
            break;
        default:
            break;
    }
    return index;
}

char *strip_quotes(char *str) {
    char *begin = (char*)str;
    while(*begin == '"') begin++;
    size_t len = strlen(begin)-1;
    while( begin[len] == '\r' || begin[len] == '\n' || begin[len] == '"' )
        begin[len--] = '\0';
    return begin;
}

static BOOL
line_tokenize(
    const char * line,
    int * sLength,
    LineType *pType,
    int *pIndex,
    char *pValue
)
{
    //
    // Skip leading spaces
    //
    while (*line && isspace(*line)) line++;
    
    //
    // Skip comments
    //
    if (*line && *line != '#')
    {
        const char *ptr;
        if ((ptr = strchr(line, '=')) != NULL)
        {
            char *end = (char*)ptr++;
            
            //
            // Skip tailing spaces
            //
            while (end > line && isspace(end[-1])) end--;
            char *name = (char*)line, *value = end;
            while (isspace(*value) || *value == '=' ) value++;
            *end='\0';
            size_t len = strlen(value)-1;
            while( isspace(value[len]) )
                value[len--] = '\0';

            len = end - line;
            
            if( len > 0 ) {
                for( LineType i = TOKEN_NONE+1; i < TOKEN_END; i++ ) {
                    int index = -1;
                    if( (index = token_conform(name, i) ) != -1 ) {
                        *pType = i;
                        *pIndex = index;
                        strcpy(pValue, strip_quotes(value));
                        return TRUE;
                    }
                }
            }
        }
    }
    return FALSE;
}

void clear_shader_slots(shader_param *param) {
    memset(&param->slots, -1, sizeof(texture_unit_slots) );
    memset(&param->orig_slots, -1, sizeof(texture_unit_slots) );
    memset(&param->pass_slots, -1, sizeof(texture_unit_slots)*MAX_INDEX );
    memset(&param->prev_slots, -1, sizeof(texture_unit_slots)*MAX_INDEX );
    memset(&param->passprev_slots, -1, sizeof(texture_unit_slots)*MAX_INDEX );
}

int parse_textures(char *line, char *values) {
    char *begin = strip_quotes(line),*pos;
    int i = 0;
    while( (pos = strchr(begin, ';')) != NULL ) {
        strncpy(values+FILENAME_MAX*(i++), begin, pos-begin);
        begin = pos+1;
    }
    if(strlen(begin))
        strcpy(values+FILENAME_MAX*(i++), begin);
    return i;
}

enum scale_type get_scale_type(const char *value) {
    if( SDL_strcasecmp(value, "SOURCE") == 0 )
        return SCALE_SOURCE;
    else if( SDL_strcasecmp(value, "VIEWPORT") == 0 )
        return SCALE_VIEWPORT;
    else
        return SCALE_ABSOLUTE;
}

enum wrap_mode get_wrap_mode(const char *value) {
    if( SDL_strcasecmp(value, "REPEAT") == 0 )
        return WRAP_REPEAT;
    else if( SDL_strcasecmp(value, "CLAMP_TO_EDGE") == 0 )
        return WRAP_CLAMP_TO_EDGE;
    else
        return WRAP_CLAMP_TO_BORDER;
}

char *basename(const char *filename) {
    char *pos = NULL;
    for( int i=0;i<strlen(PAL_PATH_SEPARATORS);i++)
        if( (pos = strrchr(filename,PAL_PATH_SEPARATORS[i])) != NULL )
            *pos='\0';
    return (char*)filename;
}

bool parse_glslp(const char *filename) {
    destroy_glslp();
    
    FILE *fp = UTIL_OpenFileAtPathForMode(gConfig.pszShaderPath, filename, "r");
    char *basedir = strdup(basename(filename));

    if (fp)
    {
        PAL_LARGE char buf[512];
        while (fgets(buf, 512, fp) != NULL)
        {
            LineType lineType;
            int index;
            char value[512];
            int slen;
            if (line_tokenize(buf, &slen, &lineType, &index, value))
            {
                shader_param  *s_param = &gGLSLP.shader_params[index];
                texture_param *t_param = &gGLSLP.texture_params[index];
                switch( lineType ) {
                    case TOKEN_SHADERS: {
                        gGLSLP.shaders = SDL_atoi(value);
                        gGLSLP.shader_params = malloc(gGLSLP.shaders*sizeof(shader_param));
                        memset(gGLSLP.shader_params, 0, gGLSLP.shaders*sizeof(shader_param));
                        for( int i = 0; i < gGLSLP.shaders; i++ ) {
                            shader_param *param = &gGLSLP.shader_params[i];
                            clear_shader_slots(param);
                        }
                        break;
                    }
                    case TOKEN_SHADER_PATH:
                        s_param->shader = strdup(PAL_va(0,"%s/%s",basedir,value));
                        break;
                    case TOKEN_SHADER_ALIAS:
                        s_param->alias = strdup(value);
                        break;
                    case TOKEN_SHADER_FILTER_LINEAR:
                        s_param->filter_linear = SDL_strcasecmp(value, "true") == 0;
                        break;
                    case TOKEN_SHADER_WRAR_MODE:
                        s_param->wrap_mode = SDL_strcasecmp(value, "true") == 0;
                        break;
                    case TOKEN_SHADER_SCALE_TYPE:
                        s_param->scale_type_x = s_param->scale_type_y = get_scale_type(value);
                        break;
                    case TOKEN_SHADER_SCALE_TYPE_X:
                        s_param->scale_type_x = get_scale_type(value);
                        break;
                    case TOKEN_SHADER_SCALE_TYPE_Y:
                        s_param->scale_type_y = get_scale_type(value);
                        break;
                    case TOKEN_SHADER_SCALE:
                        s_param->scale_x = s_param->scale_y = SDL_atof(value);
                        break;
                    case TOKEN_SHADER_SCALE_X:
                        s_param->scale_x = SDL_atof(value);
                        break;
                    case TOKEN_SHADER_SCALE_Y:
                        s_param->scale_y = SDL_atof(value);
                        break;
                    case TOKEN_SHADER_FLOAT_FRAMEBUFFER:
                        s_param->float_framebuffer = SDL_strcasecmp(value, "true") == 0;
                        break;
                    case TOKEN_SHADER_SRGB_FRAMEBUFFER:
                        s_param->srgb_framebuffer = SDL_strcasecmp(value, "true") == 0;
                        break;
                    case TOKEN_SHADER_MIPMAP_INPUT:
                        s_param->mipmap_input = SDL_strcasecmp(value, "true") == 0;
                        break;
                    case TOKEN_SHADER_FRAME_COUNT_MOD:
                        s_param->frame_count_mod = SDL_atoi(value);
                        break;
                    case TOKEN_TEXTURES: {
                        char texture_names[MAX_TEXTURES*FILENAME_MAX];
                        memset(texture_names,0,sizeof(texture_names));
                        gGLSLP.textures = parse_textures(value, texture_names);
                        gGLSLP.texture_params = malloc(gGLSLP.textures*sizeof(texture_param));
                        memset(gGLSLP.texture_params, 0, gGLSLP.textures*sizeof(texture_param));
                        for( int i = 0; i < gGLSLP.textures; i++ ) {
                            texture_param *param = &gGLSLP.texture_params[i];
                            memset(param->slots,-1,sizeof(param->slots));
                            param->texture_name = strdup((char*)&texture_names+i*FILENAME_MAX);
                            param->linear = true;
                            param->mipmap = false;
                        }
                        break;
                    }
                    case TOKEN_TEXTURE_PATH:
                        t_param->texture_path = strdup(PAL_va(0,"%s/%s",basedir,value));
                        break;
                    case TOKEN_TEXTURE_WRAP_MODE:
                        t_param->wrap_mode = get_wrap_mode(value);
                        break;
                    case TOKEN_TEXTURE_LINEAR:
                        t_param->linear = SDL_strcasecmp(value, "true") == 0;
                        break;
                    case TOKEN_TEXTURE_MIPMAP:
                        t_param->mipmap = SDL_strcasecmp(value, "true") == 0;
                        break;
                    default:
                        break;
                }
            }
        }
    }
    free(basedir);
    
    return true;
}

void destroy_glslp() {
    free(gGLSLP.shader_params);
    for( int i = 0; i < gGLSLP.shaders; i++ ) {
        shader_param *param = &gGLSLP.shader_params[i];
        if(param->shader)
            free(param->shader);
        if(param->alias)
            free(param->alias);
        if(param->sdl_texture )
            SDL_DestroyTexture( gGLSLP.shader_params[i].sdl_texture );
    }
    for( int i=0; i<gGLSLP.textures; i++ ) {
        texture_param *param = &gGLSLP.texture_params[i];
        if(param->texture_name)
            free(param->texture_name);
        if(param->texture_path)
            free(param->texture_path);
        if(param->sdl_texture )
            SDL_DestroyTexture(gGLSLP.texture_params[i].sdl_texture);
    }
    free(gGLSLP.texture_params);
    free(gGLSLP.uniform_params);
    memset(&gGLSLP, 0, sizeof(GLSLP));
}
