#version 330 core
in vec2 TexCoord;
in vec3 FragPos;

#define MAX_LIGHTS 8

uniform sampler2D globalNormal;
uniform sampler2D vertexColor;
uniform sampler2D blendMap;

uniform sampler2D diffuse0;
uniform sampler2D normal0;

uniform sampler2D diffuse1;
uniform sampler2D normal1;

uniform sampler2D diffuse2;
uniform sampler2D normal2;

uniform vec4 lightPositionArray[MAX_LIGHTS];
uniform vec4 lightDiffuseArray[MAX_LIGHTS];
uniform vec4 lightAttenuationArray[MAX_LIGHTS];
uniform vec4 ambientLightColor;

out vec4 FragColor;

void main() {
    float gamma = 2.2f;

    // Terrain normal is given by a texture. Note that this is not the normal
    // map, it is the physics vertex normal. It is already normalized.
    vec3 normal = texture2D(globalNormal, TexCoord).xyz;
    // Pick a vector linearly independent to the normal
    vec3 tangent = vec3(1.0f, 0.0f, 0.0f);
    // The binormal is now orthogonal to the normal. Still need to normalize
    // as tangent and normal are not necessarily orthonormal.
    vec3 binormal = normalize(cross(normal, tangent));
    // Recalculate the tangent to be orthogonal to both the normal and binormal,
    // giving an ONB. Since b, n orthonormal,
    // |t| = |b × n| = |b||n||sin π/2| = 1.
    tangent = cross(binormal, normal);
    mat3 TBN = mat3(tangent, binormal, normal);

    // Scale uv so textures are repeated every grid square, not every cell.
    vec2 uv = TexCoord * 32.0f;
    // Blend factor between layers
    float f = (abs(sin(uv.s)) + abs(cos(uv.t))) / sqrt(2.0f);

    vec3 vertexCol = texture2D(vertexColor, TexCoord).xyz;

    // Undo gamma correction of textures so it is correct later
    vec3 diffuseColor0 = pow(texture2D(diffuse0, uv).rgb, vec3(gamma));
    vec3 diffuseColor1 = pow(texture2D(diffuse1, uv).rgb, vec3(gamma));
    vec3 diffuseColor2 = pow(texture2D(diffuse2, uv).rgb, vec3(gamma));

    float f0 = 1.0f;
    float f1 = texture2D(blendMap, TexCoord).r;
    float f2 = texture2D(blendMap, TexCoord).g;

    // Blend diffuse layers
    float f10 = f1 + f0 * (1.0f - f1);
    vec3 diffuseColor10 = (f1 * diffuseColor1 + f0 * diffuseColor0 * (1.0f - f1)) / f10;

    float f210 = f2 + f10 * (1.0f - f2);
    vec3 diffuseColor = (f2 * diffuseColor2 + f10 * diffuseColor10 * (1.0f - f2)) / f210;

    diffuseColor *= vertexCol;

    // Blend normal layers
    vec3 n10 = (f1 * texture2D(normal1, uv).xyz + f0 * texture2D(normal0, uv).xyz * (1.0f - f1)) / f10;
    vec3 n = (f2 * texture2D(normal2, uv).xyz + f10 * n10 * (1.0f - f2)) / f210;

    // Convert from dx to gl by flipping the green channel
    n.y = 1.0f - n.y;
    // Transform normal from [0, 1] -> [-1, 1]
    n = normalize(n * 2.0f - 1.0f);
    // Transform normal into world space
    n = normalize(TBN * n);

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

            lighting += diffuse * lightDiffuse * vertexCol;
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

            lighting += diffuse * lightDiffuse * attenuation * vertexCol;
        }
    }

    vec3 fragColor = pow(min(lighting, 1.0f), vec3(1.0f / gamma));

    FragColor = vec4(fragColor, 1.0f);
}
