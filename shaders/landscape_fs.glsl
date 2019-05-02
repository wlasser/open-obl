#version 330 core

in vec2 TexCoord;
in vec3 FragPos;
in vec3 ViewPos;

layout (location = 0) out vec2 gDepth;
layout (location = 1) out vec4 gNormalSpec;
layout (location = 2) out vec4 gAlbedo;

uniform sampler2D globalNormal;
uniform sampler2D vertexColor;
uniform sampler2D blendMap;

uniform sampler2D diffuse0;
uniform sampler2D normal0;

uniform sampler2D diffuse1;
uniform sampler2D normal1;

uniform sampler2D diffuse2;
uniform sampler2D normal2;

uniform sampler2D diffuse3;
uniform sampler2D normal3;

uniform sampler2D diffuse4;
uniform sampler2D normal4;

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
    vec3 dc0 = pow(texture(diffuse0, uv).rgb, vec3(gamma));
    vec3 dc1 = pow(texture(diffuse1, uv).rgb, vec3(gamma));
    vec3 dc2 = pow(texture(diffuse2, uv).rgb, vec3(gamma));
    vec3 dc3 = pow(texture(diffuse3, uv).rgb, vec3(gamma));
    vec3 dc4 = pow(texture(diffuse4, uv).rgb, vec3(gamma));

    float f_0 = 1.0f;
    vec4 f = texture(blendMap, texCoord);

    // Blend diffuse layers
    float f_1 = mix(f_0, 1.0f, f.x);
    vec3 dc_1 = mix(f_0 * dc0, dc1, f.x) / f_1;

    float f_2 = mix(f_1, 1.0f, f.y);
    vec3 dc_2 = mix(f_1 * dc_1, dc2, f.y) / f_2;

    float f_3 = mix(f_2, 1.0f, f.z);
    vec3 dc_3 = mix(f_2 * dc_2, dc3, f.z) / f_3;

    float f_4 = mix(f_3, 1.0f, f.a);
    vec3 dc_4 = mix(f_3 * dc_3, dc4, f.w) / f_4;

    vec3 diffuseColor = dc_4 * vertexCol;

    // Blend normal layers
    vec4 n[5];
    n[0] = texture(normal0, uv);
    n[1] = texture(normal1, uv);
    n[2] = texture(normal2, uv);
    n[3] = texture(normal3, uv);
    n[4] = texture(normal4, uv);
    for (int i = 0; i < 5; ++i) {
        n[i].w = (floor(n[i].w * 255.0) == 255 ? 0.0f : n[i].w);
    }

    vec4 n_0 = n[0];
    vec4 n_1 = mix(f_0 * n_0, n[1], f.x) / f_1;
    vec4 n_2 = mix(f_1 * n_1, n[2], f.y) / f_2;
    vec4 n_3 = mix(f_2 * n_2, n[3], f.z) / f_3;
    vec4 n_4 = mix(f_3 * n_3, n[4], f.w) / f_4;

    // Convert from dx to gl by flipping the green channel
    n_4.y = 1.0f - n_4.y;
    // Transform normal from [0, 1] -> [-1, 1]
    n_4.xyz = normalize(n_4.xyz * 2.0f - 1.0f);
    // Transform normal into world space
    gNormalSpec.xyz = normalize(TBN * n_4.xyz);
    gNormalSpec.w = n_4.w;

    gDepth.x = gl_FragCoord.z;
    gDepth.y = 0.0f;

    // TODO: Use LTEX specular. All of them seem to use 30.0f though.
    gAlbedo = vec4(diffuseColor, 30.0f);
}
