#version 330 core
in vec2 TexCoord;
in vec3 FragPos;

uniform sampler2D sunTexture;

out vec4 FragColor;

void main() {
    FragColor = texture(sunTexture, TexCoord).rgba;
}
