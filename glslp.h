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
// glslp.h: retroarch-style shader preset parser header by palxex, 2018
//

#ifndef glslp_h
#define glslp_h

#include "main.h"

#define MAX_INDEX 26

#define MAX_TEXTURES 8
#define PREV_TEXTURES (MAX_TEXTURES-1)

#define MAX_PARAMETERS 200

enum wrap_mode {
    WRAP_REPEAT,
    WRAP_CLAMP_TO_EDGE,
    WRAP_CLAMP_TO_BORDER
};

enum scale_type {
    SCALE_SOURCE,
    SCALE_VIEWPORT,
    SCALE_ABSOLUTE
};

typedef struct tagTEXTUREUNITSLOTS {
    int texture_uniform_location;

    int texture_size_uniform_location;
    int input_size_uniform_location;

    int tex_coord_attrib_location;
    
    int output_size_uniform_location;
    int frame_direction_uniform_location;
    int frame_count_uniform_location;
    
    int texture_unit;
}pass_uniform_locations;

typedef struct tagFBOPARAM {
    bool valid;
    double width, height;
    double pow_width, pow_height;
}fbo_params;

typedef struct tagSHADERPARAM {
    //by defination
    char *shader;
    char *alias;
    bool filter_linear;
    enum wrap_mode wrap_mode;
    enum scale_type scale_type_x, scale_type_y;
    double scale_x, scale_y;
    bool mipmap_input;
    bool float_framebuffer;
    bool srgb_framebuffer;
    int frame_count_mod;
    
    //by implementation
    SDL_Texture *pass_sdl_texture;
    pass_uniform_locations self_slots;
    pass_uniform_locations orig_slots;
    pass_uniform_locations alias_slots;
    pass_uniform_locations pass_slots[MAX_INDEX];
    pass_uniform_locations prev_slots[MAX_INDEX];
    pass_uniform_locations passprev_slots[MAX_INDEX];
    fbo_params FBO;
}shader_param;

typedef struct tagTEXTUREPARAMS {
    //by defination
    char *texture_name;
    char *texture_path;
    enum wrap_mode wrap_mode;
    bool linear;
    bool mipmap;
    
    //by implementation
    SDL_Texture *sdl_texture;
    int texture_unit;
    int slots_pass[MAX_INDEX]; //corresponding every pass
}texture_param;

typedef struct tagUNIFORMPARAMS {
    //by defination
    char *parameter_name;
    char *desc;
    double value;
    double value_default;
    double minimum;
    double maximum;
    double step;
    
    //by implementation
    int uniform_ids[MAX_PARAMETERS];
}uniform_param;

typedef struct tagGLSLP {
    char *orig_filter;
    int shaders;
    shader_param *shader_params;
    int textures;
    texture_param *texture_params;
    int uniform_parameters;
    uniform_param *uniform_params;
}GLSLP;

extern GLSLP gGLSLP;

char *get_glslp_path(const char *filename);

bool parse_glslp(const char *, GLSLP *);
char *serialize_glslp(const GLSLP *);

void glslp_add_parameter(char *line, size_t len, GLSLP *);

void destroy_glslp(GLSLP *);

#endif /* glslp_h */
