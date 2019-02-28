#version 330 core
in vec2 TexCoord;
in vec3 FragPos;

uniform sampler2D lowerLayer;
uniform sampler2D upperLayer;
uniform vec3 lowerCloudColor;
uniform vec3 upperCloudColor;

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

    // Premultiply alpha to allow belding with the sky
    FragColor = vec4(cloudColor * cloudAlpha, cloudAlpha);
}
