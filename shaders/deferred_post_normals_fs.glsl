#version 330 core
uniform sampler2D Tex0;
uniform sampler2D Tex1;
uniform sampler2D Tex2;

in vec2 TexCoord;

out vec4 FragColor;

void main() {
    vec3 a1 = texture(Tex1, TexCoord).xyz;

    FragColor = vec4(a1 + 1.0f, 0.0f) * 0.5f;
}