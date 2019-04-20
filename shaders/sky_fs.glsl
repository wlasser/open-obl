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
    float inclination = abs(1.0f - acos(pos.y / r) / (3.14159f * 0.5f));

    // Reciprocal of lower inclination, the inclination of the lower sky color.
    const float rli = 1.0f / 0.2f;

    vec3 skyColor;
    const float skyAlpha = 1.0f;
    if (inclination <= 1.0f / rli) {
        float di = inclination * rli;
        skyColor = mix(horizonColor, lowerSkyColor, di);
    } else {
        //float di = (inclination - lowerInclination) / (1 - lowerInclination);
        float di = (inclination * rli - 1.0f) / (rli - 1.0f);
        skyColor = mix(lowerSkyColor, upperSkyColor, di);
    }

    skyColor = pow(skyColor, vec3(gamma));

    // Premultiply alpha to allow blending with the clouds
    FragColor = vec4(skyColor * skyAlpha, skyAlpha);
}
