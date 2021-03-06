#version 330 core
in vec2 TexCoord;
in vec3 FragPos;
in vec3 ViewPos;

#define MAX_LIGHTS 8

uniform sampler2D globalNormal;
uniform sampler2D vertexColor;
uniform sampler2D blendMap;

uniform sampler2D diffuse5;
uniform sampler2D normal5;

uniform sampler2D diffuse6;
uniform sampler2D normal6;

uniform sampler2D diffuse7;
uniform sampler2D normal7;

uniform sampler2D diffuse8;
uniform sampler2D normal8;

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

    // OGRE flips the y tex coordinate for some reason. One would assume that
    // the local terrain coordinates (0,0) and (1,1) corresponded to uv (0,0)
    // and (1,1) but nope, the y coordinate is flipped. This does not seem to
    // be documented, WTF OGRE.
    vec2 texCoord = vec2(TexCoord.x, 1.0f - TexCoord.y);

    // Terrain normal is given by a texture. Note that this is not the normal
    // map, it is the physics vertex normal. It is already normalized.
    vec3 normal = texture(globalNormal, texCoord).xyz;
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

    // Scale uv so textures are repeated every grid square, not every quadrant.
    vec2 uv = texCoord * 17.0f;

    vec3 vertexCol = texture(vertexColor, texCoord).xyz;

    // Undo gamma correction of textures so it is correct later
    vec3 dc5 = pow(texture(diffuse5, uv).rgb, vec3(gamma));
    vec3 dc6 = pow(texture(diffuse6, uv).rgb, vec3(gamma));
    vec3 dc7 = pow(texture(diffuse7, uv).rgb, vec3(gamma));
    vec3 dc8 = pow(texture(diffuse8, uv).rgb, vec3(gamma));

    // Alpha values of the blend maps
    vec4 fv = texture(blendMap, texCoord);

    // Blended diffuse, normal, and alpha
    vec3 diffuseColor;
    vec3 n;
    float f;

    // Note that the alpha of A⊙B is zero iff α_A = 0 and α_B = 0. Thus if the
    // base layer has nonzero alpha then every subsequent layer has nonzero
    // alpha, and the expression for the colour of A⊙B---which involves a
    // division by the alpha of A⊙B---is always valid.
    // In this pass we do not have a base layer and therefore cannot guarantee
    // that the first two layers both have nonvanishing alpha. To avoid a
    // potential division by zero then, we need to hunt for the first blend
    // layer with nonzero alpha, if one even exists, and start the blend there.
    int layer;
    if (fv.x > 0.1f) {
        f = fv.x;
        diffuseColor = f * dc5;
        n = f * texture(normal5, uv).xyz;
        layer = 0;
    } else if (fv.y > 0.1f) {
        f = fv.y;
        diffuseColor = f * dc6;
        n = f * texture(normal6, uv).xyz;
        layer = 1;
    } else if (fv.z > 0.1f) {
        f = fv.z;
        diffuseColor = f * dc7;
        n = f * texture(normal7, uv).xyz;
        layer = 2;
    } else if (fv.w > 0.1f) {
        f = fv.w;
        diffuseColor = f * dc8;
        n = f * texture(normal8, uv).xyz;
        layer = 3;
    } else {
        FragColor = vec4(0.0f);
        return;
    }

    // Blend layer6 over the previous layer
    if (layer == 0) {
        float f_new = mix(f, 1.0f, fv.y);
        if (f_new > 0.1f) {
            diffuseColor = mix(f * diffuseColor, dc6, fv.y) / f_new;
            n = mix(f * n, texture(normal6, uv).xyz, fv.y) / f_new;
            f = f_new;
        }
    }

    // Blend layer7 over the previous layers
    if (layer <= 1) {
        float f_new = mix(f, 1.0f, fv.z);
        if (f_new > 0.1f) {
            diffuseColor = mix(f * diffuseColor, dc7, fv.z) / f_new;
            n = mix(f * n, texture(normal7, uv).xyz, fv.z) / f_new;
            f = f_new;
        }
    }

    // Blend layer8 over the previous layers
    if (layer <= 2) {
        float f_new = mix(f, 1.0f, fv.w);
        if (f_new > 0.1f) {
            diffuseColor = mix(f * diffuseColor, dc8, fv.w) / f_new;
            n = mix(f * n, texture(normal8, uv).xyz, fv.w) / f_new;
            f = f_new;
        }
    }

    // If layer == 3 then there is no blending to be done since the first layer
    // was actually the last available.

    diffuseColor *= vertexCol;

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
        vec3 lightDir;
        float attenuation;
        if (lightPositionArray[i].w < 0.5f) {
            // Directional light
            lightDir = normalize(lightPositionArray[i].xyz);
            attenuation = 1.0f;
        } else {
            // Point light
            lightDir = normalize(lightPositionArray[i].xyz - FragPos);
            float lightDist = length(lightPositionArray[i].xyz - FragPos);
            vec4 attenVec = vec4(0.0f, 1.0f, lightDist, lightDist * lightDist);
            attenuation = 1.0f / dot(lightAttenuationArray[i], attenVec);
        }

        vec3 lightDiffuse = pow(lightDiffuseArray[i].rgb, vec3(gamma));
        float diff = max(dot(n, lightDir), 0.0f);
        vec3 diffuse = diff * diffuseColor;

        lighting += diffuse * lightDiffuse * attenuation * vertexCol;
    }

    vec3 fragColor = pow(min(lighting, 1.0f), vec3(1.0f / gamma));

    float distance = length(FragPos.xyz - ViewPos.xyz);
    float fog = clamp((fogParams.z - distance) * fogParams.w, 0.0f, 1.0f);
    fragColor = mix(fogColor.rgb, fragColor, fog);

    // Premultiply alpha to blend correctly over previous layer
    FragColor = vec4(fragColor * f, f);
}
