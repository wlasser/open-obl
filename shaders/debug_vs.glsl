#version 330 core
attribute vec4 vertex;
attribute vec4 colour;

out vec3 VertexCol;

uniform mat4 worldViewProj;

void main() {
    gl_Position = worldViewProj * vertex;
    VertexCol = colour.rgb;
}
