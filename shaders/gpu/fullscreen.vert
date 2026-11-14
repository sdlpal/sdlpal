#version 450

// Covers the whole target with one triangle; needs no vertex buffer.
// SDL_GPU uses +Y up clip space and a top-left texture origin on every backend.

layout(location = 0) out vec2 v_texCoord;

void main()
{
    vec2 uv = vec2((gl_VertexIndex << 1) & 2, gl_VertexIndex & 2);
    v_texCoord = uv;
    gl_Position = vec4(uv.x * 2.0 - 1.0, 1.0 - uv.y * 2.0, 0.0, 1.0);
}
