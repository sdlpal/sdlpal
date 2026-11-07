#version 450

// Turns the 8-bit indexed game screen into RGB, applying any palette fade in progress.
// Each fade reproduces the matching loop in palette.c exactly, in closed form.

layout(location = 0) in vec2 v_texCoord;
layout(location = 0) out vec4 o_color;

layout(set = 2, binding = 0) uniform sampler2D u_index;
layout(set = 2, binding = 1) uniform sampler2D u_palette;
layout(set = 2, binding = 2) uniform sampler2D u_paletteTo;

layout(set = 3, binding = 0) uniform Params
{
    int u_mode;
    int u_step;
    int u_color;
    int u_unused;
};

const int FADE_NONE       = 0;
const int FADE_BRIGHTNESS = 1;
const int FADE_BLEND      = 2;
const int FADE_FROM_COLOR = 3;
const int FADE_TO_COLOR   = 4;
const int FADE_TO_RED     = 5;

ivec3 entry(sampler2D palette, int index)
{
    return ivec3(texelFetch(palette, ivec2(index, 0), 0).rgb * 255.0 + 0.5);
}

ivec3 toward(ivec3 from, ivec3 to, int limit)
{
    return from + clamp(to - from, ivec3(-limit), ivec3(limit));
}

void main()
{
    int index = int(texture(u_index, v_texCoord).r * 255.0 + 0.5);
    ivec3 color = entry(u_palette, index);

    if (u_mode == FADE_BRIGHTNESS)
    {
        color = (color * u_step) >> 6;
    }
    else if (u_mode == FADE_BLEND)
    {
        color = (color * (31 - u_step) + entry(u_paletteTo, index) * u_step) / 31;
    }
    else if (u_mode == FADE_FROM_COLOR)
    {
        color = toward(entry(u_palette, u_color), color, 4 * u_step);
    }
    else if (u_mode == FADE_TO_COLOR)
    {
        color = toward(color, entry(u_palette, u_color), 4 * u_step);
    }
    else if (u_mode == FADE_TO_RED && index != 0x4F)
    {
        int red = (color.r + color.g + color.b) / 4 + 64;
        color = toward(color, ivec3(red, 0, 0), 8 * u_step);
    }

    o_color = vec4(vec3(color) / 255.0, 1.0);
}
