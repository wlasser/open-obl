#version 330 core
in vec2 TexCoord;
in vec3 FragPos;

uniform sampler2D lowerLayer;
uniform sampler2D upperLayer;
uniform vec3 lowerSkyColor;
uniform vec3 upperSkyColor;
uniform vec3 lowerCloudColor;
uniform vec3 upperCloudColor;
uniform vec3 horizonColor;

out vec4 FragColor;

void main() {
    float lcAlpha = texture(lowerLayer, TexCoord).a;
    float ucAlpha = texture(upperLayer, TexCoord).a;
    vec3 lowerCloud = vec3(texture(lowerLayer, TexCoord).rgb * lowerCloudColor);
    vec3 upperCloud = vec3(texture(upperLayer, TexCoord).rgb * upperCloudColor);

    float cloudAlpha;
    vec3 cloudColor;
    if (lcAlpha < 0.01f && ucAlpha < 0.01f) {
        cloudAlpha = 0.0f;
        cloudColor = vec3(0.0f);
    } else {
        cloudAlpha = lcAlpha + ucAlpha * (1 - lcAlpha);
        cloudColor = (lcAlpha * lowerCloud + ucAlpha * upperCloud * (1 - lcAlpha)) / cloudAlpha;
    }

    vec3 pos = FragPos / 4000.0f;
    float r = length(pos);
    // 0 at the horizon, +1 at the upper pole, -1 at the lower pole
    float inclination = abs(1 - acos(pos.y / r) / (3.14159 / 2));

    float lowerInclination = 0.2f;
    vec3 skyColor;
    float skyAlpha = 1.0f;
    if (inclination <= lowerInclination) {
        float di = inclination / lowerInclination;
        skyColor = (1 - di) * horizonColor + di * lowerSkyColor;
    } else {
        float di = (inclination - lowerInclination) / (1 - lowerInclination);
        skyColor = (1 - di) * lowerSkyColor + di * upperSkyColor;
    }

    float a = cloudAlpha + skyAlpha * (1 - cloudAlpha);
    vec3 c = (cloudAlpha * cloudColor + skyAlpha * skyColor * (1 - cloudAlpha)) / a;

    FragColor = vec4(c, a);
}
