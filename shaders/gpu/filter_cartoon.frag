#version 450

// Port of shaders/cartoon/cartoon.glsl: the second pass of the cartoon filter.

layout(location = 0) in vec2 v_texCoord;
layout(location = 0) out vec4 o_color;

layout(set = 2, binding = 0) uniform sampler2D u_image;

layout(set = 3, binding = 0) uniform Params
{
    vec2 u_inputSize;
};

const float cl = 9.0;
const float max_outline = 0.70;

void main()
{
    float dx = 1.0 / u_inputSize.x / 2.0;
    float dy = 1.0 / u_inputSize.y / 2.0;

    // At some window sizes these positions fall exactly between two texels, and GPUs disagree
    // about which one that picks. A small nudge makes them all agree. Of the directions tried,
    // this one stays closest to the OpenGL version.
    vec2 uv = v_texCoord + 0.02 / vec2(textureSize(u_image, 0));

    vec3 c00 = texture(u_image, uv + vec2(-dx, -dy)).xyz;
    vec3 c10 = texture(u_image, uv + vec2(  0, -dy)).xyz;
    vec3 c20 = texture(u_image, uv + vec2( dx, -dy)).xyz;
    vec3 c01 = texture(u_image, uv + vec2(-dx,   0)).xyz;
    vec3 c11 = texture(u_image, uv).xyz;
    vec3 c21 = texture(u_image, uv + vec2( dx,   0)).xyz;
    vec3 c02 = texture(u_image, uv + vec2(-dx,  dy)).xyz;
    vec3 c12 = texture(u_image, uv + vec2(  0,  dy)).xyz;
    vec3 c22 = texture(u_image, uv + vec2( dx,  dy)).xyz;

    vec3 dt = vec3(1.0);
    float d1 = dot(abs(c00 - c22), dt) + 0.001;
    float d2 = dot(abs(c20 - c02), dt) + 0.001;
    float hl = dot(abs(c01 - c21), dt) + 0.001;
    float vl = dot(abs(c10 - c12), dt) + 0.001;

    float d = max(max(d1, d2), max(hl, vl)) / (length(0.20 * (c11 + c10 + c01 + c12 + c21)) + 0.5);
    d = max(d - 0.15, 0.0) * 1.25;

    float l = length(c11);
    l = l * cl;
    float f = fract(l);
    f = pow(f, 5.0);
    l = (floor(l) + f) / cl;

    c11 = (1.1 - min(d, max_outline)) * normalize(c11) * l;
    o_color = vec4(c11, 1.0);
}
