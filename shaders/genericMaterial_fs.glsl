#version 330 core
in mat3 TBN;
in vec2 TexCoord;
in vec3 FragPos;
in vec3 ViewPos;

#define MAX_LIGHTS 8

uniform sampler2D diffuseMap;
uniform sampler2D normalMap;
uniform vec4 lightPositionArray[MAX_LIGHTS];
uniform vec4 lightDiffuseArray[MAX_LIGHTS];

uniform float matShininess;
uniform vec3 matDiffuse;
uniform vec3 matSpecular;

out vec4 FragColor;

void main() {
    float gamma = 2.2f;

    // Undo gamma correction of texture so it is correct later
    vec3 diffuseColor = pow(texture2D(diffuseMap, TexCoord).xyz, vec3(gamma));

    vec3 normal = texture2D(normalMap, TexCoord).xyz;
    // Convert from dx to gl by flipping the green channel
    normal.y = 1.0f - normal.y;
    // Transform normal from [0, 1] -> [-1, 1]
    normal = normalize(normal * 2.0f - 1.0f);
    // Transform normal into world space
    normal = normalize(TBN * normal);

    vec3 lighting = vec3(0.0f, 0.0f, 0.0f);
    vec3 viewDir = normalize(ViewPos - FragPos);

    vec3 ambient = diffuseColor * vec3(0.1f, 0.1f, 0.1f);

    for (int i = 0; i < MAX_LIGHTS; ++i) {
        vec3 lightDir = normalize(lightPositionArray[i].xyz - FragPos);
        vec3 reflectDir = reflect(-lightDir, normal);

        float lightDistance = length(lightPositionArray[i].xyz - FragPos);
        float attenuation = 1.0f / (1.0f + 0.0014f * lightDistance
            + 0.000007f * lightDistance * lightDistance);

        float diff = max(dot(normal, lightDir), 0.0f);
        vec3 diffuse = diff * diffuseColor * matDiffuse;

        vec3 halfwayDir = normalize(lightDir + viewDir);
        float spec = pow(max(dot(normal, halfwayDir), 0.0f), 8 * matShininess);
        vec3 specular = 0.25f * spec * matSpecular;

        lighting += (specular + diffuse) * lightDiffuseArray[i].xyz * attenuation;
    }

    vec3 fragColor = pow(min(ambient + lighting, 1.0f), vec3(1.0f / gamma));

    FragColor = vec4(fragColor, 1.0f);
}
