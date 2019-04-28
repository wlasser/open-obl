#version 330 core
in vec2 TexCoord;

uniform sampler2D RT;
out vec4 FragColor;

void main() {
    vec3 hdrCol = texture(RT, TexCoord).rgb;
    //    vec3 ldrCol = hdrCol / (vec3(1.0f) + hdrCol);
    vec3 ldrCol = hdrCol;

    float gamma = 2.2f;
    FragColor = vec4(pow(ldrCol, vec3(1.0f / gamma)), 1.0f);
}
