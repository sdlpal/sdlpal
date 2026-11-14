#version 450

// Whole-screen effects on 8-bit surfaces. Each mode reproduces a software loop exactly:
// battle.c (background color shift, cross-fade), scene.c (wave), video.c and ending.c
// (cross-fade, screen switch) and palette.c (index replacement).

layout(location = 0) out vec4 o_index;

// Direct3D 12 pairs a pixel shader's inputs with the vertex shader's outputs by position,
// so there this shader has to take fullscreen.vert's v_texCoord first, unused as it is.
// compile_shaders.sh defines DXIL only when compiling for Direct3D 12.
#ifdef DXIL
layout(location = 0) in vec2 v_texCoord;
#endif

layout(set = 2, binding = 0) uniform sampler2D u_first;
layout(set = 2, binding = 1) uniform sampler2D u_second;

layout(set = 3, binding = 0) uniform Params
{
    int u_mode;
    int u_phase;   // which of every 6 pixels this step touches
    int u_pitch;   // the phase is counted along the surface's bytes
    int u_shift;
    int u_from;
    int u_to;
    int u_step;
    int u_waveIndex;
    ivec4 u_wave[8];
};

const int MODE_SHIFT_BACKGROUND = 0;
const int MODE_WAVE             = 1;
const int MODE_CROSS_FADE       = 2;
const int MODE_SWITCH           = 3;
const int MODE_REPLACE          = 4;

int fetch(sampler2D image, ivec2 pos)
{
    return int(texelFetch(image, pos, 0).r * 255.0 + 0.5);
}

void main()
{
    ivec2 pos = ivec2(gl_FragCoord.xy);
    int result = 0;

    if (u_mode == MODE_SHIFT_BACKGROUND)
    {
        int source = fetch(u_first, pos);
        int low = ((source & 0x0F) + u_shift) & 0xFF;
        if ((low & 0x80) != 0)
        {
            low = 0;
        }
        else if ((low & 0x70) != 0)
        {
            low = 0x0F;
        }
        result = low | (source & 0xF0);
    }
    else if (u_mode == MODE_WAVE)
    {
        int row = (u_waveIndex + pos.y) % 32;
        int offset = u_wave[row / 4][row % 4];
        result = fetch(u_first, ivec2((pos.x + offset) % 320, pos.y));
    }
    else if (u_mode == MODE_CROSS_FADE || u_mode == MODE_SWITCH)
    {
        if ((pos.y * u_pitch + pos.x) % 6 != u_phase)
        {
            discard;
        }

        result = fetch(u_first, pos);
        if (u_mode == MODE_CROSS_FADE)
        {
            int a = result;
            int b = fetch(u_second, pos);
            if (u_step != 0)
            {
                if ((a & 0x0F) > (b & 0x0F))
                {
                    b++;
                }
                else if ((a & 0x0F) < (b & 0x0F))
                {
                    b--;
                }
            }
            result = (a & 0xF0) | (b & 0x0F);
        }
    }
    else if (u_mode == MODE_REPLACE)
    {
        if (fetch(u_first, pos) != u_from)
        {
            discard;
        }
        result = u_to;
    }

    o_index = vec4(float(result) / 255.0);
}
