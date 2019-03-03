#version 330 core
in vec2 TexCoord;
in vec3 FragPos;
in vec3 ViewPos;

#define MAX_LIGHTS 8

uniform sampler2D diffuseMap;
uniform sampler2D normalMap;

uniform vec4 lightPositionArray[MAX_LIGHTS];
uniform vec4 lightDiffuseArray[MAX_LIGHTS];
uniform vec4 lightAttenuationArray[MAX_LIGHTS];
uniform vec4 ambientLightColor;
uniform vec4 fogColor;
// x = density, y = start, z = end, w = 1 / (end - start)
uniform vec4 fogParams;

out vec4 FragColor;

void main() {
    float gamma = 2.2f;

    // We *don't* have to uv flip the diffuse texture because it has already
    // been compensated, but we *do* still have to flip the normal texture.
    vec2 texCoord = TexCoord.xy;

    // Gamma is already corrected
    vec3 diffuseColor = texture(diffuseMap, texCoord).rgb;

    // We're using the global normal map not any baked normals so no
    // geometric transformation is required, but we do still need to uv flip.
    vec3 n = texture(normalMap, vec2(TexCoord.x, 1.0f - TexCoord.y)).xyz;

    vec3 lighting = vec3(0.0f);

    vec3 ambient = diffuseColor * ambientLightColor.rgb;
    lighting += ambient;

    for (int i = 0; i < MAX_LIGHTS; ++i) {
        if (lightPositionArray[i].w < 0.5f) {
            // Directional light
            vec3 lightDir = normalize(lightPositionArray[i].xyz);
            vec3 lightDiffuse = pow(lightDiffuseArray[i].rgb, vec3(gamma));
            float diff = max(dot(n, lightDir), 0.0f);
            vec3 diffuse = diff * diffuseColor;

            lighting += diffuse * lightDiffuse;
        } else {
            // Point light
            vec3 lightDir = normalize(lightPositionArray[i].xyz - FragPos);
            float lightDistance = length(lightPositionArray[i].xyz - FragPos);
            float attenuation = 1.0f / (lightAttenuationArray[i].y
                + lightAttenuationArray[i].z * lightDistance
                + lightAttenuationArray[i].w * lightDistance * lightDistance);

            vec3 lightDiffuse = pow(lightDiffuseArray[i].rgb, vec3(gamma));

            float diff = max(dot(n, lightDir), 0.0f);
            vec3 diffuse = diff * diffuseColor;

            lighting += diffuse * lightDiffuse * attenuation;
        }
    }

    vec3 fragColor = pow(min(lighting, 1.0f), vec3(1.0f / gamma));

    float distance = length(FragPos.xyz - ViewPos.xyz);
    float fog = clamp((fogParams.z - distance) * fogParams.w, 0.0f, 1.0f);
    fragColor = fog * fragColor + (1.0f - fog) * fogColor.rgb;

    FragColor = vec4(fragColor, 1.0f);
}
