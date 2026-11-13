#version 450

// Port of shaders/crt.glsl: public domain CRT styled scan-line shader by Timothy Lottes.

layout(location = 0) in vec2 v_texCoord;
layout(location = 0) out vec4 o_color;

layout(set = 2, binding = 0) uniform sampler2D u_image;

const float hardScan = -8.0;       // hardness of scanline: -8 soft, -16 medium
const float hardPix = -2.0;        // hardness of pixels in scanline: -2 soft, -4 hard
const vec2 warp = vec2(1.0 / 32.0, 1.0 / 24.0); // display warp
const float maskDark = 1.0;
const float maskLight = 1.5;
const vec2 res = vec2(640.0, 480.0);

float ToLinear1(float c) { return (c <= 0.04045) ? c / 12.92 : pow((c + 0.055) / 1.055, 2.4); }
vec3 ToLinear(vec3 c) { return vec3(ToLinear1(c.r), ToLinear1(c.g), ToLinear1(c.b)); }

float ToSrgb1(float c) { return (c < 0.0031308) ? c * 12.92 : 1.055 * pow(c, 0.41666) - 0.055; }
vec3 ToSrgb(vec3 c) { return vec3(ToSrgb1(c.r), ToSrgb1(c.g), ToSrgb1(c.b)); }

// Nearest emulated sample given floating point position and texel offset.
// Also zero's off screen.
vec3 Fetch(vec2 pos, vec2 off)
{
    pos = floor(pos * res + off) / res;

    // The right and bottom edges count as off screen too: the original samples them with
    // GL_CLAMP_TO_BORDER, which SDL_GPU does not have.
    if (pos.x < 0.0 || pos.y < 0.0 || pos.x >= 1.0 || pos.y >= 1.0)
    {
        return vec3(0.0);
    }

    // These positions often fall exactly between two texels, and GPUs disagree about which
    // one that picks. A small nudge makes them all agree. Of the directions tried, this one
    // stays closest to the OpenGL version.
    return ToLinear(texture(u_image, pos.xy - 0.02 / vec2(textureSize(u_image, 0)), -16.0).rgb);
}

// Distance in emulated pixels to nearest texel.
vec2 Dist(vec2 pos)
{
    pos = pos * res;
    return -((pos - floor(pos)) - vec2(0.5));
}

float Gaus(float pos, float scale)
{
    return exp2(scale * pos * pos);
}

// 3-tap Gaussian filter along horz line.
vec3 Horz3(vec2 pos, float off)
{
    vec3 b = Fetch(pos, vec2(-1.0, off));
    vec3 c = Fetch(pos, vec2( 0.0, off));
    vec3 d = Fetch(pos, vec2( 1.0, off));
    float dst = Dist(pos).x;
    float wb = Gaus(dst - 1.0, hardPix);
    float wc = Gaus(dst + 0.0, hardPix);
    float wd = Gaus(dst + 1.0, hardPix);
    return (b * wb + c * wc + d * wd) / (wb + wc + wd);
}

// 5-tap Gaussian filter along horz line.
vec3 Horz5(vec2 pos, float off)
{
    vec3 a = Fetch(pos, vec2(-2.0, off));
    vec3 b = Fetch(pos, vec2(-1.0, off));
    vec3 c = Fetch(pos, vec2( 0.0, off));
    vec3 d = Fetch(pos, vec2( 1.0, off));
    vec3 e = Fetch(pos, vec2( 2.0, off));
    float dst = Dist(pos).x;
    float wa = Gaus(dst - 2.0, hardPix);
    float wb = Gaus(dst - 1.0, hardPix);
    float wc = Gaus(dst + 0.0, hardPix);
    float wd = Gaus(dst + 1.0, hardPix);
    float we = Gaus(dst + 2.0, hardPix);
    return (a * wa + b * wb + c * wc + d * wd + e * we) / (wa + wb + wc + wd + we);
}

float Scan(vec2 pos, float off)
{
    float dst = Dist(pos).y;
    return Gaus(dst + off, hardScan);
}

// Allow nearest three lines to effect pixel.
vec3 Tri(vec2 pos)
{
    vec3 a = Horz3(pos, -1.0);
    vec3 b = Horz5(pos,  0.0);
    vec3 c = Horz3(pos,  1.0);
    float wa = Scan(pos, -1.0);
    float wb = Scan(pos,  0.0);
    float wc = Scan(pos,  1.0);
    return a * wa + b * wb + c * wc;
}

// Distortion of scanlines, and end of screen alpha.
vec2 Warp(vec2 pos)
{
    pos = pos * 2.0 - 1.0;
    pos *= vec2(1.0 + (pos.y * pos.y) * warp.x, 1.0 + (pos.x * pos.x) * warp.y);
    return pos * 0.5 + 0.5;
}

// Shadow mask. The original feeds it a position normalized to 0..1, which makes it a
// broad tint rather than a fine mask; kept so the look does not change.
vec3 Mask(vec2 pos)
{
    pos.x += pos.y * 3.0;
    vec3 mask = vec3(maskDark);
    pos.x = fract(pos.x / 6.0);
    if (pos.x < 0.333)
    {
        mask.r = maskLight;
    }
    else if (pos.x < 0.666)
    {
        mask.g = maskLight;
    }
    else
    {
        mask.b = maskLight;
    }
    return mask;
}

void main()
{
    vec3 color = Tri(Warp(v_texCoord)) * Mask(v_texCoord);
    o_color = vec4(ToSrgb(color), 1.0);
}
