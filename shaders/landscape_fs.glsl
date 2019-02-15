#version 330 core
in vec2 TexCoord;
in vec3 VertexCol;

uniform sampler2D diffuseMap;

out vec4 FragColor;

void main() {
    FragColor = texture2D(diffuseMap, TexCoord * 32.0f) * vec4(VertexCol, 1.0f);
}
