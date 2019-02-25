#version 330 core
in vec2 TexCoord;
in vec3 FragPos;

uniform sampler2D lowerLayer;
uniform sampler2D upperLayer;
uniform vec3 lowerSkyColor;
uniform vec3 upperSkyColor;
uniform vec3 lowerCloudColor;
uniform vec3 upperCloudColor;

out vec4 FragColor;

void main() {
    float lcAlpha = 0.5f;
    float ucAlpha = 0.5f;
    vec3 lowerCloud = vec3(texture(lowerLayer, TexCoord).rgb * lowerCloudColor);
    vec3 upperCloud = vec3(texture(lowerLayer, TexCoord).rgb * upperCloudColor);

    float cAlpha = ucAlpha + lcAlpha * (1 - ucAlpha);
    vec3 fragColor = (ucAlpha * upperCloud + lcAlpha * lowerCloud * (1 - ucAlpha)) / cAlpha;
    FragColor = vec4(fragColor, cAlpha);
}
