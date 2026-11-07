#version 450

// Writes palette indices into an 8-bit surface. Every mode reproduces the matching
// software routine in palcommon.c or font.c exactly.
//
// The atlas holds the index in R and "this pixel is opaque" in G.

layout(location = 0) in vec2 v_src;
layout(location = 1) flat in uvec4 v_params;

layout(location = 0) out vec4 o_index;

layout(set = 2, binding = 0) uniform sampler2D u_atlas;
layout(set = 2, binding = 1) uniform sampler2D u_screen; // copy of the target, for shadows

const uint MODE_NORMAL = 0u;
const uint MODE_SHIFT  = 1u;
const uint MODE_MONO   = 2u;
const uint MODE_GLYPH  = 3u;
const uint MODE_SHADOW = 4u;
const uint MODE_FILL   = 5u;

int toIndex(float value)
{
    return int(value * 255.0 + 0.5);
}

int shifted(int source, int shift)
{
    return clamp((source & 0x0F) + shift, 0, 0x0F);
}

void main()
{
    uint mode = v_params.x;
    int shift = int(v_params.y) - 128;
    int color = int(v_params.z);
    int result = color;

    if (mode != MODE_FILL)
    {
        vec4 texel = texelFetch(u_atlas, ivec2(v_src), 0);
        if (texel.g < 0.5)
        {
            discard;
        }

        int source = toIndex(texel.r);
        if (mode == MODE_NORMAL)
        {
            result = source;
        }
        else if (mode == MODE_SHIFT)
        {
            result = shifted(source, shift) | (source & 0xF0);
        }
        else if (mode == MODE_MONO)
        {
            result = shifted(source, shift) | (color & 0xF0);
        }
        else if (mode == MODE_SHADOW)
        {
            int below = toIndex(texelFetch(u_screen, ivec2(gl_FragCoord.xy), 0).r);
            result = (below & 0xF0) | ((below & 0x0F) >> 1);
        }
    }

    o_index = vec4(float(result) / 255.0);
}
