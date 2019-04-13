#version 330 core

in mat3 TBN;
in vec2 TexCoord;
in vec3 FragPos;
in vec3 ViewPos;
in vec3 VertexCol;

uniform float farDistance;
uniform float matShininess;
uniform vec3 matDiffuse;
//uniform vec3 matSpecular;

uniform sampler2D diffuseMap;
uniform sampler2D normalMap;

layout (location = 0) out vec4 gPosition;
layout (location = 1) out vec3 gNormal;
layout (location = 2) out vec4 gAlbedoSpec;

void main() {
    float gamma = 2.2f;

    // Undo gamma correction of texture so it is correct later
    vec3 diffuseColor = pow(texture(diffuseMap, TexCoord).rgb, vec3(gamma)) * VertexCol;

    vec3 normal = texture(normalMap, TexCoord).xyz;
    // Convert from dx to gl by flipping the green channel
    normal.y = 1.0f - normal.y;
    // Transform normal from [0, 1] -> [-1, 1]
    normal = normalize(normal * 2.0f - 1.0f);
    // Transform normal into world space
    gNormal = normalize(TBN * normal);

    gPosition.xyz = FragPos;
    gPosition.w = gl_FragCoord.z;

    gAlbedoSpec = vec4(diffuseColor * matDiffuse, matShininess);
}