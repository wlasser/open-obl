#version 330 core

#define POINT_LIGHT 0
#define DIRECTIONAL_LIGHT 1
#define SPOT_LIGHT 2

uniform sampler2D Tex0;
uniform sampler2D Tex1;
uniform sampler2D Tex2;

#if LIGHT_TYPE == POINT_LIGHT
uniform vec4 lightPosition;
#elif LIGHT_TYPE == DIRECTIONAL_LIGHT
uniform vec4 lightDirection;
#endif
uniform vec4 lightDiffuseCol;
uniform vec4 lightAttenuation;

uniform vec3 ViewPos;

#if LIGHT_TYPE == DIRECTIONAL_LIGHT
in vec2 TexCoord;
#else
in vec4 ScreenPos;
#endif

out vec4 FragColor;

void main() {
    float gamma = 2.2f;

    #if LIGHT_TYPE == DIRECTIONAL_LIGHT
    vec2 uv = TexCoord;
    #else
    vec4 homScreenPos = (ScreenPos / ScreenPos.w);
    vec2 uv = homScreenPos.xy * 0.5f + 0.5f;
    #endif

    vec3 normal = texture(Tex1, uv).xyz;

    // Don't try to light things without normals, like the sky.
    if (dot(normal, normal) < 0.4f) {
        FragColor = vec4(0.0f);
        return;
    }

    vec3 worldPos = texture(Tex0, uv).xyz;
    vec4 albedoSpec = texture(Tex2, uv).rgba;

    vec3 viewDir = normalize(ViewPos - worldPos);

    #if LIGHT_TYPE == DIRECTIONAL_LIGHT
    vec3 lightDir = lightDirection.xyz;
    float atten = 1.0f;
    #else
    float lightDist = length(lightPosition.xyz - worldPos);
    vec3 lightDir = (lightPosition.xyz - worldPos) / lightDist;
    float atten = 1.0f / (lightAttenuation.y
    + lightAttenuation.z * lightDist
    + lightAttenuation.w * lightDist * lightDist);
    #endif

    vec3 reflectDir = reflect(-lightDir, normal);

    vec3 lightCol = pow(lightDiffuseCol.rgb, vec3(gamma));

    float diff = max(dot(normal, lightDir), 0.0f);
    vec3 diffCol = diff * lightCol * albedoSpec.rgb;

    vec3 halfwayDir = normalize(lightDir + viewDir);
    float spec = pow(max(dot(normal, halfwayDir), 0.0f), 8 * albedoSpec.a);
    vec3 specCol = 0.25f * spec * lightCol;

    vec3 finalCol = (specCol + diffCol) * atten;
    FragColor = vec4(finalCol, 0.0f);
}
