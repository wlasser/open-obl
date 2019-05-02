#version 330 core

in mat3 TBN;
in vec2 TexCoord;
in vec3 FragPos;
in vec3 VertexCol;

uniform float farDistance;
uniform float matShininess;
uniform vec3 matDiffuse;
//uniform vec3 matSpecular;

uniform sampler2D diffuseMap;
uniform sampler2D normalMap;

layout (location = 0) out vec2 gDepth;
layout (location = 1) out vec4 gNormalSpec;
layout (location = 2) out vec4 gAlbedo;

void main() {
    float gamma = 2.2f;

    // Convert texture to linear space
    vec3 diffuseColor = pow(texture(diffuseMap, TexCoord).rgb, vec3(gamma)) * VertexCol;

    vec4 normalSpec = texture(normalMap, TexCoord);
    vec3 normal = normalSpec.xyz;
    // Convert from dx to gl by flipping the green channel
    normal.y = 1.0f - normal.y;
    // Transform normal from [0, 1] -> [-1, 1]
    normal = normalize(normal * 2.0f - 1.0f);
    // Transform normal into world space
    gNormalSpec.xyz = TBN * normal;
    gNormalSpec.w = floor(normalSpec.w * 255.0f) == 255 ? 0.0f : normalSpec.w;

    gDepth.x = gl_FragCoord.z;
    gDepth.y = 0.0f;

    gAlbedo = vec4(diffuseColor * matDiffuse, matShininess);
}