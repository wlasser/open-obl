#version 330 core
in vec3 Normal;
in vec2 TexCoord;
in vec3 FragPos;
in vec3 ViewPos;

uniform sampler2D diffuseMap;
uniform vec4 lightPosition;

out vec4 FragColor;

void main() {
    vec3 lightColor = vec3(1.0f, 1.0f, 1.0f);

    vec3 lighting = vec3(0.1f, 0.1f, 0.1f);

    vec3 norm = normalize(Normal);
    vec3 lightDir = normalize(lightPosition.xyz - FragPos);
    vec3 viewDir = normalize(ViewPos - FragPos);
    vec3 reflectDir = reflect(-lightDir, norm);

    float diff = max(dot(norm, lightDir), 0.0f);
    vec3 diffuse = diff * texture2D(diffuseMap, TexCoord).xyz * lightColor;
    lighting += diffuse;

    float spec = pow(max(dot(viewDir, reflectDir), 0.0f), 10);
    vec3 specular = 0.5f * spec * lightColor;
    lighting += specular;

    FragColor = vec4(lighting, 1.0f);
}
