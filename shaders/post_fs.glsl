#version 330 core
in vec2 TexCoord;

uniform sampler2D RT;
out vec4 FragColor;

void main() {
    FragColor = vec4(TexCoord, 0.0f, 1.0f);
    float gamma = 2.2f;
    FragColor = vec4(pow(texture(RT, TexCoord).rgb, vec3(1.0f / gamma)), 1.0f);
}
