#version 330 core
in vec2 TexCoord;

uniform sampler2D diffuseMap;
uniform vec4 diffuseCol;

out vec4 FragColor;

void main() {
    FragColor = texture2D(diffuseMap, TexCoord) * diffuseCol;
}
