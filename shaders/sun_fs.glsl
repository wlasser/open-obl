#version 330 core
in vec2 TexCoord;
in vec3 FragPos;
in vec3 VertexCol;

uniform sampler2D sunTexture;

out vec4 FragColor;

void main() {
    float gamma = 2.2f;
    vec4 sunColor = texture(sunTexture, TexCoord).rgba;

    FragColor = vec4(pow(sunColor.rgb * VertexCol.rgb, vec3(gamma)) * sunColor.a, sunColor.a);
}
