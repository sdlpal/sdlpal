#version 450

// One instance per sprite, glyph or filled rectangle, drawn as a 4-vertex strip.
// All coordinates are in whole pixels of the 8-bit target surface.

layout(location = 0) in ivec2 a_dst;
layout(location = 1) in uvec2 a_size;
layout(location = 2) in uvec2 a_src;
layout(location = 3) in uvec4 a_params; // mode, color shift + 128, color, unused

layout(location = 0) out vec2 v_src;
layout(location = 1) flat out uvec4 v_params;

layout(set = 1, binding = 0) uniform Target
{
    vec2 u_targetSize;
};

void main()
{
    vec2 corner = vec2(gl_VertexIndex & 1, (gl_VertexIndex >> 1) & 1);
    vec2 pos = vec2(a_dst) + corner * vec2(a_size);

    v_src = vec2(a_src) + corner * vec2(a_size);
    v_params = a_params;
    gl_Position = vec4(pos.x / u_targetSize.x * 2.0 - 1.0, 1.0 - pos.y / u_targetSize.y * 2.0, 0.0, 1.0);
}
