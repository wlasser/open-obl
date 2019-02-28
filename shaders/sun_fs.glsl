#version 330 core
in vec2 TexCoord;
in vec3 FragPos;
in vec3 VertexCol;

uniform sampler2D sunTexture;

out vec4 FragColor;

void main() {
    vec4 sunColor = texture(sunTexture, TexCoord).rgba;

    FragColor = vec4(sunColor.rgb * VertexCol.rgb * sunColor.a, sunColor.a);
}
