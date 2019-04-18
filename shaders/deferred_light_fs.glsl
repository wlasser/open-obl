#version 330 core

uniform sampler2D Tex0;
uniform sampler2D Tex1;
uniform sampler2D Tex2;

uniform vec4 lightPosition;
uniform vec4 lightDiffuseCol;
uniform vec4 lightAttenuation;

uniform vec3 ViewPos;

in vec4 ScreenPos;

out vec4 FragColor;

void main() {
    float gamma = 2.2f;

    vec4 homScreenPos = (ScreenPos / ScreenPos.w);
    vec2 uv = vec2(homScreenPos.x, homScreenPos.y) * 0.5f + 0.5f;

    vec3 worldPos = texture(Tex0, uv).xyz;
    vec3 normal = texture(Tex1, uv).xyz;
    vec4 albedoSpec = texture(Tex2, uv).rgba;

    vec3 viewDir = normalize(ViewPos - worldPos);

    float lightDist = length(lightPosition.xyz - worldPos);
    vec3 lightDir = (lightPosition.xyz - worldPos) / lightDist;
    vec3 reflectDir = reflect(-lightDir, normal);
    float atten = 1.0f / (lightAttenuation.y
    + lightAttenuation.z * lightDist
    + lightAttenuation.w * lightDist * lightDist);

    vec3 lightCol = pow(lightDiffuseCol.rgb, vec3(gamma));

    float diff = max(dot(normal, lightDir), 0.0f);
    vec3 diffCol = diff * lightCol * albedoSpec.rgb;

    vec3 halfwayDir = normalize(lightDir + viewDir);
    float spec = pow(max(dot(normal, halfwayDir), 0.0f), 8 * albedoSpec.a);
    vec3 specCol = 0.25f * spec * lightCol;

    vec3 finalCol = (specCol + diffCol) * atten;
    FragColor = vec4(finalCol, 0.0f);
}