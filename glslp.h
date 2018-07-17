//
//  glslp.h
//  Pal
//
//  Created by palxex on 11/7/18.
//

#ifndef glslp_h
#define glslp_h

#include "main.h"

#define MAX_INDEX 26

#define MAX_TEXTURES 8
#define PREV_TEXTURES (MAX_TEXTURES-1)

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
    SDL_Texture *sdl_texture;
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
    char *uniform_key;
    char *uniform_value;
}uniform_param;

typedef struct tagGLSLP {
    int shaders;
    shader_param *shader_params;
    int textures;
    texture_param *texture_params;
    int uniform_parameters;
    uniform_param *uniform_params;
}GLSLP;

extern GLSLP gGLSLP;

bool parse_glslp(const char *);

void destroy_glslp();

#endif /* glslp_h */
