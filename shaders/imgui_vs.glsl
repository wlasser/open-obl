#version 330 core
attribute vec2 vertex;
attribute vec2 uv;
attribute vec4 colour;

out vec2 TexCoord;
out vec4 VertexCol;

uniform mat4 worldViewProj;

void main() {
    gl_Position = worldViewProj * vec4(vertex, 0.0f, 1.0f);
    VertexCol = colour;
}

