#version 330 core
in vec2 TexCoord;
in vec3 FragPos;
in vec3 ViewPos;

uniform sampler2D diffuse;

out vec4 FragColor;

void main() {
    vec3 fragColor = texture(diffuse, TexCoord).rgb;
    FragColor = vec4(fragColor, 0.7f);
}
