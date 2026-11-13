#version 450

// Port of shaders/cartoon/AA.glsl: the first pass of the cartoon filter.

layout(location = 0) in vec2 v_texCoord;
layout(location = 0) out vec4 o_color;

layout(set = 2, binding = 0) uniform sampler2D u_image;

layout(set = 3, binding = 0) uniform Params
{
    vec2 u_inputSize;
};

void main()
{
    float dx = 1.0 / u_inputSize.x;
    float dy = 1.0 / u_inputSize.y;

    vec3 c00 = texture(u_image, v_texCoord + vec2(-dx, -dy)).xyz;
    vec3 c10 = texture(u_image, v_texCoord + vec2(  0, -dy)).xyz;
    vec3 c20 = texture(u_image, v_texCoord + vec2( dx, -dy)).xyz;
    vec3 c01 = texture(u_image, v_texCoord + vec2(-dx,   0)).xyz;
    vec3 c11 = texture(u_image, v_texCoord).xyz;
    vec3 c21 = texture(u_image, v_texCoord + vec2( dx,   0)).xyz;
    vec3 c02 = texture(u_image, v_texCoord + vec2(-dx,  dy)).xyz;
    vec3 c12 = texture(u_image, v_texCoord + vec2(  0,  dy)).xyz;
    vec3 c22 = texture(u_image, v_texCoord + vec2( dx,  dy)).xyz;

    vec3 dt = vec3(1.0);
    float d1 = dot(abs(c00 - c22), dt) + 0.001;
    float d2 = dot(abs(c20 - c02), dt) + 0.001;
    float hl = dot(abs(c01 - c21), dt) + 0.001;
    float vl = dot(abs(c10 - c12), dt) + 0.001;

    float md = d1 + d2;
    float mc = hl + vl;
    hl *= md;
    vl *= md;
    d1 *= mc;
    d2 *= mc;

    float ww = d1 + d2 + hl + vl;

    o_color = vec4((hl * (c10 + c12) + vl * (c01 + c21) + d1 * (c20 + c02) + d2 * (c00 + c22) + ww * c11) / (3.0 * ww), 1.0);
}
