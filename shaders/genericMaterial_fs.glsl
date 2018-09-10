#version 330 core
in vec3 Normal;
in vec2 TexCoord;
in vec3 FragPos;
in vec3 ViewPos;

#define MAX_LIGHTS 4

uniform sampler2D diffuseMap;
uniform vec4 lightPositionArray[MAX_LIGHTS];
uniform vec4 lightDiffuseArray[MAX_LIGHTS];

out vec4 FragColor;

void main() {
    float gamma = 2.2f;
    vec3 diffuseColor = pow(texture2D(diffuseMap, TexCoord).xyz, vec3(gamma));

    vec3 lighting = vec3(0.0f, 0.0f, 0.0f);
    vec3 norm = normalize(Normal);
    vec3 viewDir = normalize(ViewPos - FragPos);

    for (int i = 0; i < MAX_LIGHTS; ++i) {
        vec3 lightDir = normalize(lightPositionArray[i].xyz - FragPos);
        vec3 reflectDir = reflect(-lightDir, norm);

        float diff = max(dot(norm, lightDir), 0.0f);
        vec3 diffuse = diff * diffuseColor * lightDiffuseArray[i].xyz;

        float spec = pow(max(dot(viewDir, reflectDir), 0.0f), 10);
        vec3 specular = 0.5f * spec * lightDiffuseArray[i].xyz;

        lighting += (specular + diffuse);
    }

    vec3 fragColor = pow(lighting, vec3(1.0f / gamma));

    FragColor = vec4(fragColor, 1.0f);
}
