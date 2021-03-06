#version 330 core
in mat3 TBN;
in vec2 TexCoord;
in vec3 FragPos;
in vec3 VertexCol;

#define MAX_LIGHTS 8

uniform sampler2D diffuseMap;
uniform sampler2D normalMap;
uniform vec4 lightPositionArray[MAX_LIGHTS];
uniform vec4 lightDiffuseArray[MAX_LIGHTS];
uniform vec4 lightAttenuationArray[MAX_LIGHTS];
uniform vec4 ambientLightColor;
uniform vec3 viewPos;

uniform float matShininess;
uniform vec3 matDiffuse;
uniform vec3 matSpecular;

out vec4 FragColor;

void main() {
    float gamma = 2.2f;

    // Undo gamma correction of texture so it is correct later
    vec3 albedo = pow(texture(diffuseMap, TexCoord).xyz, vec3(gamma)) * VertexCol;
    float alpha = texture(diffuseMap, TexCoord).a;

    vec4 normalSpec = texture(normalMap, TexCoord);
    vec3 normal = normalSpec.xyz;
    float specular = floor(normalSpec.w * 255.0) == 255 ? 0.0f : normalSpec.w;

    // Convert from dx to gl by flipping the green channel
    normal.y = 1.0f - normal.y;
    // Transform normal from [0, 1] -> [-1, 1]
    normal = normalize(normal * 2.0f - 1.0f);
    // Transform normal into world space
    normal = normalize(TBN * normal);

    vec3 lighting = vec3(0.0f, 0.0f, 0.0f);
    vec3 viewDir = normalize(viewPos - FragPos);

    vec3 ambientCol = albedo * ambientLightColor.rgb;

    for (int i = 0; i < MAX_LIGHTS; ++i) {
        if (lightPositionArray[i].w < 0.5f) {
            // Directional light
            vec3 lightDir = normalize(lightPositionArray[i].xyz);
            vec3 reflectDir = reflect(-lightDir, normal);
            vec3 lightCol = pow(lightDiffuseArray[i].rgb, vec3(gamma));

            float diff = max(dot(normal, lightDir), 0.0f);
            vec3 diffCol = diff * albedo * matDiffuse;

            vec3 halfwayDir = normalize(lightDir + viewDir);
            float spec = pow(max(dot(normal, halfwayDir), 0.0f), 8 * matShininess);
            vec3 specCol = spec * specular * matSpecular;

            lighting += (specCol + diffCol) * lightCol * VertexCol;
        } else {
            // Point light
            vec3 lightDir = normalize(lightPositionArray[i].xyz - FragPos);
            vec3 reflectDir = reflect(-lightDir, normal);
            float lightDistance = length(lightPositionArray[i].xyz - FragPos);
            float attenuation = 1.0f / (lightAttenuationArray[i].y
            + lightAttenuationArray[i].z * lightDistance
            + lightAttenuationArray[i].w * lightDistance * lightDistance);

            vec3 lightCol = pow(lightDiffuseArray[i].rgb, vec3(gamma));

            float diff = max(dot(normal, lightDir), 0.0f);
            vec3 diffCol = diff * albedo * matDiffuse;

            vec3 halfwayDir = normalize(lightDir + viewDir);
            float spec = pow(max(dot(normal, halfwayDir), 0.0f), 8 * matShininess);
            vec3 specCol = spec * specular * matSpecular;

            lighting += (specCol + diffCol) * lightCol * VertexCol * attenuation;
        }
    }

    vec3 fragColor = ambientCol + lighting;

    FragColor = vec4(fragColor, alpha);
}
