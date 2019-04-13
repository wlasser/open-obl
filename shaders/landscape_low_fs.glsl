#version 330 core
in vec2 TexCoord;
in vec3 FragPos;
in vec3 ViewPos;

uniform sampler2D diffuseMap;
uniform sampler2D normalMap;

layout (location = 0) out vec4 gPosition;
layout (location = 1) out vec3 gNormal;
layout (location = 2) out vec4 gAlbedoSpec;

void main() {
    float gamma = 2.2f;

    // We *don't* have to uv flip the diffuse texture because it has already
    // been compensated, but we *do* still have to flip the normal texture.
    vec2 texCoord = TexCoord.xy;

    // Gamma is already corrected
    vec3 diffuseColor = texture(diffuseMap, texCoord).rgb;

    // We're using the global normal map not any baked normals so no
    // geometric transformation is required, but we do still need to uv flip.
    gNormal = texture(normalMap, vec2(TexCoord.x, 1.0f - TexCoord.y)).xyz;

    gPosition.xyz = FragPos;
    gPosition.w = gl_FragCoord.z;

    gAlbedoSpec = vec4(diffuseColor, 0.0f);
}
