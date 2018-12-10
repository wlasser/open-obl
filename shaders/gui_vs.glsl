#version 330 core
in vec2 vertex;
in vec4 colour;
in vec4 uv0;

out vec2 TexCoord;
out vec4 VertexCol;

uniform mat4 worldViewProj;

void main() {
    gl_Position = worldViewProj * vec4(vertex, 0.0f, 1.0f);
    VertexCol = colour;
    TexCoord = uv0.xy;
}
