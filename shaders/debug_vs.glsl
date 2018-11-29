#version 330 core
in vec4 vertex;
in vec4 colour;

out vec3 VertexCol;

uniform mat4 worldViewProj;

void main() {
    gl_Position = worldViewProj * vertex;
    VertexCol = colour.rgb;
}
