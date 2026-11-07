#version 450

// Port of shaders/reverse.glsl: inverts the colors.

layout(location = 0) in vec2 v_texCoord;
layout(location = 0) out vec4 o_color;

layout(set = 2, binding = 0) uniform sampler2D u_image;

void main()
{
    o_color = vec4(vec3(1.0) - texture(u_image, v_texCoord).rgb, 1.0);
}
