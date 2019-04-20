#version 330 core
in vec2 TexCoord;
in vec3 FragPos;

uniform sampler2D lowerLayer;
uniform sampler2D upperLayer;
uniform vec3 lowerCloudColor;
uniform vec3 upperCloudColor;

out vec4 FragColor;

void main() {
    float gamma = 2.2f;
    vec4 lowerCloud = texture(lowerLayer, TexCoord);
    vec4 upperCloud = texture(upperLayer, TexCoord);
    float lcAlpha = lowerCloud.a;
    float ucAlpha = upperCloud.a;
    vec3 lcDiffuse = pow(lowerCloud.rgb * lowerCloudColor, vec3(gamma));
    vec3 ucDiffuse = pow(upperCloud.rgb * upperCloudColor, vec3(gamma));

    float cloudAlpha;
    vec3 cloudColor;
    if (lcAlpha < 0.01f && ucAlpha < 0.01f) {
        cloudAlpha = 0.0f;
        cloudColor = vec3(0.0f);
    } else {
        cloudAlpha = mix(ucAlpha, 1.0f, lcAlpha);
        // Premultiply alpha to allow blending with the sky
        cloudColor = mix(ucDiffuse * ucAlpha, lcDiffuse, lcAlpha);
    }

    FragColor = vec4(cloudColor, cloudAlpha);
}
