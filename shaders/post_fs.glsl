#version 330 core
in vec2 TexCoord;

uniform sampler2D RT;
out vec4 FragColor;

void main() {
    float gamma = 2.2f;
    FragColor = vec4(pow(texture(RT, TexCoord).rgb, vec3(1.0f / gamma)), 1.0f);
}
