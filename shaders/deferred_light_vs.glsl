#version 330 core
in vec4 vertex;

uniform mat4 world;
uniform mat4 worldViewProj;

out vec4 ScreenPos;

void main() {
    vec4 pos = worldViewProj * vertex;
    gl_Position = pos;
    ScreenPos = pos;
}
