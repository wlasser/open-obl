#version 330 core
in vec2 TexCoord;

uniform sampler2D diffuseMap;

out vec4 FragColor;

void main() {
    FragColor = texture2D(diffuseMap, TexCoord);
}
