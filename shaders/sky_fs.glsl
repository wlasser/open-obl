#version 330 core
in vec2 TexCoord;
in vec3 FragPos;

uniform vec3 lowerSkyColor;
uniform vec3 upperSkyColor;
uniform vec3 horizonColor;

out vec4 FragColor;

void main() {
    float gamma = 2.2f;
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

    skyColor = pow(skyColor, vec3(gamma));

    // Premultiply alpha to allow blending with the clouds
    FragColor = vec4(skyColor * skyAlpha, skyAlpha);
}
