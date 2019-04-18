#version 330 core

in vec2 vertex;

out vec2 TexCoord;

void main() {
    vec2 pos = sign(vertex.xy);
    TexCoord = vec2(pos.x, pos.y) * 0.5f + 0.5f;

    gl_Position = vec4(pos, 0.0f, 1.0f);
}