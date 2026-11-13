#version 450

// Draws the touch overlay with a constant opacity on top of its own alpha.

layout(location = 0) in vec2 v_texCoord;
layout(location = 0) out vec4 o_color;

layout(set = 2, binding = 0) uniform sampler2D u_overlay;

layout(set = 3, binding = 0) uniform Params
{
    float u_alpha;
};

void main()
{
    vec4 color = texture(u_overlay, v_texCoord);
    o_color = vec4(color.rgb, color.a * u_alpha);
}
