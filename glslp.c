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
// glslp.c: retroarch-style shader preset parser by palxex, 2018
//

#include "main.h"

#if PAL_HAS_GLSL
#include "glslp.h"

GLSLP gGLSLP;
char glslp_commonbuf[MAX_PARAMETERS*PAL_MAX_PATH];

//uniform parameter is not needed since currently SDLPal lacks runtime configuation alter method like RetroArch GUI/kind of immediate UI/at least a quake console.
//lets consider it after anyone of them introduced:)

typedef enum tagLineType {
    TOKEN_NONE,
    
    TOKEN_ORIGFILTER,
    
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
    
    TOKEN_PARAMETERS,
    TOKEN_PARAMETER_NAME,
    
    TOKEN_END
}LineType;

static char *tokens[] = {
    "",
    
    "orig_filter",
    
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
    
    "parameters",
    "%s",

    ""
};

static int token_conform( const char *name, LineType type, GLSLP *pGLSLP ) {
    int index = -1;
    switch( type ) {
        case TOKEN_ORIGFILTER:
            if( SDL_strcasecmp(name, tokens[type]) == 0 )
                index = 0;
            break;
        case TOKEN_SHADERS:
        case TOKEN_TEXTURES:
        case TOKEN_PARAMETERS:
            if( SDL_strcasecmp(name, tokens[type]) == 0 )
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
            for( int i = 0; i < pGLSLP->shaders; i++ ) {
                if( SDL_strcasecmp(name, PAL_va(0, tokens[type], i) ) == 0 ) {
                    index = i;
                    break;
                }
            }
            break;
        case TOKEN_TEXTURE_PATH:
        case TOKEN_TEXTURE_WRAP_MODE:
        case TOKEN_TEXTURE_LINEAR:
        case TOKEN_TEXTURE_MIPMAP:
            for( int i = 0; i < pGLSLP->textures; i++ ) {
                texture_param *param = &pGLSLP->texture_params[i];
                if( SDL_strcasecmp(name, PAL_va(0, tokens[type], param->texture_name) ) == 0 ) {
                    index = i;
                    break;
                }
            }
            break;
        case TOKEN_PARAMETER_NAME:
            for( int i = 0; i < pGLSLP->uniform_parameters; i++ ) {
                uniform_param *param = &pGLSLP->uniform_params[i];
                if( SDL_strcasecmp(name, PAL_va(0, tokens[type], param->parameter_name) ) == 0 ) {
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

static char *strip_quotes(char *str) {
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
    char *pValue,
    GLSLP *pGLSLP
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
                    if( (index = token_conform(name, i, pGLSLP) ) != -1 ) {
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

static void clear_shader_slots(shader_param *param) {
    memset(&param->self_slots, -1, sizeof(pass_uniform_locations) );
    memset(&param->orig_slots, -1, sizeof(pass_uniform_locations) );
    memset(&param->prev_slots, -1, sizeof(pass_uniform_locations)*MAX_INDEX );
    memset(&param->pass_slots, -1, sizeof(pass_uniform_locations)*MAX_INDEX );
    memset(&param->passprev_slots, -1, sizeof(pass_uniform_locations)*MAX_INDEX );
    memset(&param->alias_slots, -1, sizeof(pass_uniform_locations) );
}

static char *glslp_pack_texturenames(const GLSLP *gGLSLP) {
    memset(glslp_commonbuf,0,sizeof(glslp_commonbuf));
    for(int i = 0; i < gGLSLP->textures; i++ ) {
        if( i != 0 ) strcat(glslp_commonbuf, ";");
        strcat(glslp_commonbuf, gGLSLP->texture_params[i].texture_name);
    }
    return glslp_commonbuf;
}

char *glslp_pack_parameters(const GLSLP *gGLSLP) {
    memset(glslp_commonbuf,0,sizeof(glslp_commonbuf));
    for(int i = 0; i < gGLSLP->uniform_parameters; i++ ) {
        if( i != 0 ) strcat(glslp_commonbuf, ";");
        strcat(glslp_commonbuf, gGLSLP->uniform_params[i].parameter_name);
    }
    return glslp_commonbuf;
}

static int split_with(char *line, char seperator) {
    memset(glslp_commonbuf,0,sizeof(glslp_commonbuf));
    char *begin = strip_quotes(line),*pos;
    int i = 0;
    while( (pos = strchr(begin, seperator)) != NULL ) {
        while(*pos==' ') pos++;
        strncpy(glslp_commonbuf+PAL_MAX_PATH*(i++), begin, pos-begin);
        begin = pos+1;
    }
    if(strlen(begin)) {
        while(*begin==' ') begin++;
        strcpy(glslp_commonbuf+PAL_MAX_PATH*(i++), begin);
    }
    return i;
}

static enum scale_type string_to_scale_type(const char *value) {
    if( SDL_strcasecmp(value, "SOURCE") == 0 )
        return SCALE_SOURCE;
    else if( SDL_strcasecmp(value, "VIEWPORT") == 0 )
        return SCALE_VIEWPORT;
    else
        return SCALE_ABSOLUTE;
}
static char * scale_type_to_string(enum scale_type type) {
    char *value=NULL;
    switch (type) {
        case SCALE_SOURCE:
            value="SOURCE";
            break;
        case SCALE_VIEWPORT:
            value="VIEWPORT";
            break;
        case SCALE_ABSOLUTE:
            value="ABSOLUTE";
            break;
    }
    return value;
}

static enum wrap_mode string_to_wrap_mode(const char *value) {
    if( SDL_strcasecmp(value, "REPEAT") == 0 )
        return WRAP_REPEAT;
    else if( SDL_strcasecmp(value, "CLAMP_TO_EDGE") == 0 )
        return WRAP_CLAMP_TO_EDGE;
    else
        return WRAP_CLAMP_TO_BORDER;
}
static char * wrap_mode_to_string(enum wrap_mode type) {
    char *value=NULL;
    switch (type) {
        case WRAP_REPEAT:
            value="REPEAT";
            break;
        case WRAP_CLAMP_TO_EDGE:
            value="CLAMP_TO_EDGE";
            break;
        case WRAP_CLAMP_TO_BORDER:
            value="CLAMP_TO_BORDER";
            break;
    }
    return value;
}

static char *GLSLP_basename(const char *filename) {
    char *pos = NULL;
    int broked = 0;
    for( int i=0;i<strlen(PAL_PATH_SEPARATORS);i++)
        if( (pos = strrchr(filename,PAL_PATH_SEPARATORS[i])) != NULL )
            *pos='\0', broked = 1;
    if( !broked )
        sprintf((char*)filename, "./");
    return (char*)filename;
}

static char *GLSLP_reflow(char *path) {
	char *ptr;
	while ((ptr = strstr(path, "..")) != NULL) {
		char *dup = strdup(path);
		dup[ptr - path - 1] = '\0';
		dup = GLSLP_basename(dup);
		sprintf(path, "%s/%s", dup, ptr + 3);
		free(dup);
	}
	while ((ptr = strstr(path, "/")) != NULL)
		*ptr = PAL_NATIVE_PATH_SEPARATOR[0];
	return path;
}

char *get_glslp_path(const char *filename) {
    char *path = (char*)filename;
    if( !UTIL_IsAbsolutePath(filename) )
        path = PAL_va(0, "%s%s%s", gConfig.pszShaderPath, PAL_NATIVE_PATH_SEPARATOR, filename);
#if __WINRT__
    //seems M$ decided fobidden parent referencing. avoid sandbox escaping?
	GLSLP_reflow(path);
#endif
    return path;
}

bool parse_glslp(const char *filename, GLSLP *pGLSLP) {
    destroy_glslp(pGLSLP);
    
    FILE *fp = UTIL_OpenRequiredFile(filename);
    char *basedir = GLSLP_basename(strdup(filename));

    if (fp)
    {
        PAL_LARGE char buf[512];
        while (fgets(buf, 512, fp) != NULL)
        {
            LineType lineType;
            int index;
            char value[512];
            int slen;
            if (line_tokenize(buf, &slen, &lineType, &index, value, pGLSLP))
            {
                shader_param  *s_param = &pGLSLP->shader_params[index];
                texture_param *t_param = &pGLSLP->texture_params[index];
                uniform_param *u_param = &pGLSLP->uniform_params[index];
                switch( lineType ) {
                    case TOKEN_ORIGFILTER:
                        pGLSLP->orig_filter=strdup(value);
                        break;
                    case TOKEN_SHADERS: {
                        pGLSLP->shaders = SDL_atoi(value);
                        pGLSLP->shader_params = UTIL_calloc(pGLSLP->shaders, sizeof(shader_param));
                        for( int i = 0; i < pGLSLP->shaders; i++ ) {
                            shader_param *param = &pGLSLP->shader_params[i];
                            clear_shader_slots(param);
                            param->scale_type_x = param->scale_type_y = SCALE_SOURCE;
                            param->scale_x = param->scale_y = 1.0f;
                            param->wrap_mode = WRAP_CLAMP_TO_EDGE;
                            param->filter_linear = true;
                        }
                        break;
                    }
                    case TOKEN_SHADER_PATH:
                        s_param->shader = strdup((UTIL_IsAbsolutePath(value) || strcmp(basedir, "./") == 0)? value : PAL_va(0,"%s/%s",basedir,value));
                        break;
                    case TOKEN_SHADER_ALIAS:
                        s_param->alias = strdup(value);
                        break;
                    case TOKEN_SHADER_FILTER_LINEAR:
                        s_param->filter_linear = SDL_strcasecmp(value, "true") == 0;
                        break;
                    case TOKEN_SHADER_WRAR_MODE:
                        s_param->wrap_mode = string_to_wrap_mode(value);
                        break;
                    case TOKEN_SHADER_SCALE_TYPE:
                        s_param->scale_type_x = s_param->scale_type_y = string_to_scale_type(value);
                        break;
                    case TOKEN_SHADER_SCALE_TYPE_X:
                        s_param->scale_type_x = string_to_scale_type(value);
                        break;
                    case TOKEN_SHADER_SCALE_TYPE_Y:
                        s_param->scale_type_y = string_to_scale_type(value);
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
                        pGLSLP->textures = split_with(value, ';');
                        if( pGLSLP->textures > 0 ) {
                            pGLSLP->texture_params = UTIL_calloc(pGLSLP->textures, sizeof(texture_param));
                            for( int i = 0; i < pGLSLP->textures; i++ ) {
                                texture_param *param = &pGLSLP->texture_params[i];
                                memset(param->slots_pass,-1,sizeof(param->slots_pass));
                                param->texture_name = strdup((char*)&glslp_commonbuf+i*PAL_MAX_PATH);
                                param->linear = true;
                                param->mipmap = false;
                            }
                        }
                        break;
                    }
                    case TOKEN_TEXTURE_PATH:
                        t_param->texture_path = strdup((UTIL_IsAbsolutePath(value) || strcmp(basedir, "./") == 0) ? value : PAL_va(0,"%s%s%s",basedir,PAL_NATIVE_PATH_SEPARATOR,value));
                        break;
                    case TOKEN_TEXTURE_WRAP_MODE:
                        t_param->wrap_mode = string_to_wrap_mode(value);
                        break;
                    case TOKEN_TEXTURE_LINEAR:
                        t_param->linear = SDL_strcasecmp(value, "true") == 0;
                        break;
                    case TOKEN_TEXTURE_MIPMAP:
                        t_param->mipmap = SDL_strcasecmp(value, "true") == 0;
                        break;
                    case TOKEN_PARAMETERS: {
                        pGLSLP->uniform_parameters = split_with(value, ';');
                        if( pGLSLP->uniform_parameters > 0 ) {
                            pGLSLP->uniform_params = UTIL_calloc(pGLSLP->uniform_parameters, sizeof(uniform_param));
                            for( int i = 0; i < pGLSLP->uniform_parameters; i++ ) {
                                uniform_param *param = &pGLSLP->uniform_params[i];
                                memset(param->uniform_ids,-1,sizeof(param->uniform_ids));
                                param->parameter_name = strdup((char*)&glslp_commonbuf+i*PAL_MAX_PATH);
                            }
                        }
                        break;
                    }
                    case TOKEN_PARAMETER_NAME:
                        u_param->value = SDL_atof(value);
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

char *serialize_glslp(const GLSLP *pGLSLP){
    char *output = UTIL_calloc( 1, 65535 );
    sprintf(output, "%s\r\n%s = %s", output, tokens[TOKEN_ORIGFILTER], pGLSLP->orig_filter ? pGLSLP->orig_filter : gConfig.pszShader);
    sprintf(output, "%s\r\n\r\n%s = %d", output, tokens[TOKEN_SHADERS], pGLSLP->shaders);
    for( int i = 0; i < pGLSLP->shaders; i++ ) {
        shader_param *param = &pGLSLP->shader_params[i];
        sprintf(output, "%s\r\n%s = %s", output, PAL_va(0, tokens[TOKEN_SHADER_PATH], i),               param->shader );
        sprintf(output, "%s\r\n%s = %s", output, PAL_va(0, tokens[TOKEN_SHADER_ALIAS], i),              param->alias ? param->alias : "");
        sprintf(output, "%s\r\n%s = %s", output, PAL_va(0, tokens[TOKEN_SHADER_FILTER_LINEAR], i),      param->filter_linear ? "true" : "false");
        sprintf(output, "%s\r\n%s = %s", output, PAL_va(0, tokens[TOKEN_SHADER_WRAR_MODE], i),          wrap_mode_to_string(param->wrap_mode));
        sprintf(output, "%s\r\n%s = %s", output, PAL_va(0, tokens[TOKEN_SHADER_SCALE_TYPE_X], i),       scale_type_to_string(param->scale_type_x));
        sprintf(output, "%s\r\n%s = %s", output, PAL_va(0, tokens[TOKEN_SHADER_SCALE_TYPE_Y], i),       scale_type_to_string(param->scale_type_y));
        sprintf(output, "%s\r\n%s = %.1f", output, PAL_va(0, tokens[TOKEN_SHADER_SCALE_X], i),          param->scale_x );
        sprintf(output, "%s\r\n%s = %.1f", output, PAL_va(0, tokens[TOKEN_SHADER_SCALE_Y], i),          param->scale_y );
        sprintf(output, "%s\r\n%s = %s", output, PAL_va(0, tokens[TOKEN_SHADER_FLOAT_FRAMEBUFFER], i),  param->float_framebuffer ? "true" : "false");
        sprintf(output, "%s\r\n%s = %s", output, PAL_va(0, tokens[TOKEN_SHADER_SRGB_FRAMEBUFFER], i),   param->srgb_framebuffer ? "true" : "false");
        sprintf(output, "%s\r\n%s = %s", output, PAL_va(0, tokens[TOKEN_SHADER_MIPMAP_INPUT], i),       param->mipmap_input ? "true" : "false");
        sprintf(output, "%s\r\n%s = %d", output, PAL_va(0, tokens[TOKEN_SHADER_FRAME_COUNT_MOD], i),    param->frame_count_mod);
    }
    sprintf(output, "%s\r\n\r\n%s = \"%s\"", output, tokens[TOKEN_TEXTURES], glslp_pack_texturenames(pGLSLP));
    for( int i=0; i<pGLSLP->textures; i++ ) {
        texture_param *param = &pGLSLP->texture_params[i];
        sprintf(output, "%s\r\n%s = %s", output, PAL_va(0, tokens[TOKEN_TEXTURE_PATH], param->texture_name),        param->texture_path);
        sprintf(output, "%s\r\n%s = %s", output, PAL_va(0, tokens[TOKEN_TEXTURE_LINEAR], param->texture_name),      param->linear ? "true" : "false");
        sprintf(output, "%s\r\n%s = %s", output, PAL_va(0, tokens[TOKEN_TEXTURE_MIPMAP], param->texture_name),      param->mipmap ? "true" : "false");
        sprintf(output, "%s\r\n%s = %s", output, PAL_va(0, tokens[TOKEN_TEXTURE_WRAP_MODE], param->texture_name),   wrap_mode_to_string(param->wrap_mode));
    }
    sprintf(output, "%s\r\n\r\n%s = \"%s\"", output, tokens[TOKEN_PARAMETERS], glslp_pack_parameters(pGLSLP));
    for( int i=0; i<pGLSLP->uniform_parameters; i++ ) {
        uniform_param *param = &pGLSLP->uniform_params[i];
        sprintf(output, "%s\r\n%s = \"%.6f\"", output, param->parameter_name, param->value);
    }
    return output;
}

void glslp_add_parameter(char *line, size_t len, GLSLP *pGLSLP) {
    uniform_param tempParam;
    tempParam.parameter_name = UTIL_calloc(1, PAL_MAX_PATH);
    tempParam.desc = UTIL_calloc(1, PAL_MAX_PATH);
    int found = -1, nfound = 0;
    sscanf(line, "#pragma parameter %63s \"%63[^\"]\" %lf %lf %lf %lf", tempParam.parameter_name, tempParam.desc, &tempParam.value_default, &tempParam.minimum, &tempParam.maximum, &tempParam.step);
    UTIL_LogOutput(LOGLEVEL_INFO, "Found #pragma parameter %s (%s) %.6f %.6f %.6f %.6f\n", tempParam.desc, tempParam.parameter_name, tempParam.value_default, tempParam.minimum, tempParam.maximum, tempParam.step);
    for( int i = 0; i < pGLSLP->uniform_parameters; i++ ) {
        uniform_param *param = &pGLSLP->uniform_params[i];
        if( param->parameter_name && strncmp( param->parameter_name, tempParam.parameter_name, strlen(tempParam.parameter_name) ) == 0 ) {
            found = i;
        }
    }
    if( found == -1 ) {
        nfound = 1;
        found = pGLSLP->uniform_parameters++;
        pGLSLP->uniform_params = realloc(pGLSLP->uniform_params, pGLSLP->uniform_parameters * sizeof(uniform_param));
    }
    uniform_param *param = &pGLSLP->uniform_params[found];
    param->value_default = tempParam.value_default;
    param->minimum = tempParam.minimum;
    param->maximum = tempParam.maximum;
    param->step = tempParam.step;
    
    if( nfound ) {
        param->parameter_name = tempParam.parameter_name;
        param->desc = tempParam.desc;
        memset(param->uniform_ids,-1,sizeof(param->uniform_ids));
        param->value = param->value_default;
    }else {
        UTIL_LogOutput(LOGLEVEL_INFO, "#pragma parameter %s already has value %.6f\n", param->parameter_name, param->value);
        free(tempParam.parameter_name);
        free(tempParam.desc);
    }
}

void destroy_glslp(GLSLP *pGLSLP) {
    if(pGLSLP->orig_filter)
        free(pGLSLP->orig_filter);
    for( int i = 0; i < pGLSLP->shaders; i++ ) {
        shader_param *param = &pGLSLP->shader_params[i];
        if(param->shader)
            free(param->shader);
        if(param->alias)
            free(param->alias);
        if(param->pass_sdl_texture )
            SDL_DestroyTexture( pGLSLP->shader_params[i].pass_sdl_texture );
    }
	free(pGLSLP->shader_params);
    for( int i=0; i<pGLSLP->textures; i++ ) {
        texture_param *param = &pGLSLP->texture_params[i];
        if(param->texture_name)
            free(param->texture_name);
        if(param->texture_path)
            free(param->texture_path);
        if(param->sdl_texture )
            SDL_DestroyTexture(pGLSLP->texture_params[i].sdl_texture);
    }
    free(pGLSLP->texture_params);
    for( int i=0; i<pGLSLP->uniform_parameters; i++ ) {
        uniform_param *param = &pGLSLP->uniform_params[i];
        if(param->parameter_name)
            free(param->parameter_name);
        if(param->desc)
            free(param->desc);
    }
    free(pGLSLP->uniform_params);
    memset(pGLSLP, 0, sizeof(GLSLP));
}

#endif // PAL_HAS_GLSL
