#version 330 core
in vec2 TexCoord;
in vec4 VertexCol;

uniform sampler2D diffuseMap;

out vec4 FragColor;

void main() {
    FragColor = texture2D(diffuseMap, TexCoord) * VertexCol;
}
