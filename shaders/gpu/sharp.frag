#version 450

// Sharp bilinear scaling: pixels stay hard-edged, and only the seam between two of them
// is blended. At a whole-number scale no seam falls inside an output pixel, so the
// result equals nearest-neighbor; at any other scale it avoids uneven pixel widths.
// Needs a linear sampler.

layout(location = 0) in vec2 v_texCoord;
layout(location = 0) out vec4 o_color;

layout(set = 2, binding = 0) uniform sampler2D u_image;

layout(set = 3, binding = 0) uniform Params
{
    vec2 u_sourceSize;
    vec2 u_outputSize;
};

void main()
{
    vec2 prescale = max(floor(u_outputSize / u_sourceSize), vec2(1.0));
    vec2 texel = v_texCoord * u_sourceSize;
    vec2 center = fract(texel) - 0.5;
    vec2 region = 0.5 - 0.5 / prescale;
    vec2 offset = (center - clamp(center, -region, region)) * prescale + 0.5;

    o_color = vec4(texture(u_image, (floor(texel) + offset) / u_sourceSize).rgb, 1.0);
}
