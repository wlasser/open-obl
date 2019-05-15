#version 330 core
in vec2 TexCoord;
in vec3 FragPos;
in vec3 ViewPos;

uniform sampler2D diffuseMap;
uniform sampler2D normalMap;

layout (location = 0) out vec2 gDepth;
layout (location = 1) out vec4 gNormalSpec;
layout (location = 2) out vec4 gAlbedo;

void main() {
    float gamma = 2.2f;

    vec2 texCoord = TexCoord.xy;

    vec3 albedo = pow(texture(diffuseMap, texCoord).rgb, vec3(gamma));

    gNormalSpec.xyz = normalize(texture(normalMap, texCoord).xyz);
    gNormalSpec.w = 0.0f;

    gDepth.x = gl_FragCoord.z;
    gDepth.y = 0.0f;

    gAlbedo = vec4(albedo, 30.0f);
}