#version 330 core
in vec2 TexCoord;
in vec3 FragPos;
in vec3 ViewPos;

uniform sampler2D globalNormal;
uniform sampler2D blendMap0;
uniform sampler2D blendMap1;

uniform sampler2D normal0;
uniform sampler2D normal1;
uniform sampler2D normal2;
uniform sampler2D normal3;
uniform sampler2D normal4;
uniform sampler2D normal5;
uniform sampler2D normal6;
uniform sampler2D normal7;
uniform sampler2D normal8;

layout (location = 0) out vec2 gDepth;
layout (location = 1) out vec4 gNormalSpec;
//layout (location = 2) out vec4 gAlbedo;

// See landscape_fs and landscape_bake_diffuse_fs for explanatory comments.

void main() {
    vec2 texCoord = vec2(TexCoord.x, 1.0f - TexCoord.y);
    vec3 normal = texture(globalNormal, texCoord).xyz;
    vec3 tangent = vec3(1.0f, 0.0f, 0.0f);
    vec3 binormal = normalize(cross(normal, tangent));
    tangent = cross(binormal, normal);
    mat3 TBN = mat3(tangent, binormal, normal);

    vec2 uv = texCoord * 17.0f;

    vec4 n[9];
    n[0] = texture(normal0, uv);
    n[1] = texture(normal1, uv);
    n[2] = texture(normal2, uv);
    n[3] = texture(normal3, uv);
    n[4] = texture(normal4, uv);
    n[5] = texture(normal5, uv);
    n[6] = texture(normal6, uv);
    n[7] = texture(normal7, uv);
    n[8] = texture(normal8, uv);
    for (int i = 0; i < 9; ++i) {
        n[i].w = (floor(n[i].w * 255.0f) == 255 ? 0.0f : n[i].w);
    }

    float f[9];
    f[0] = 1.0f;
    vec4 blend0 = texture(blendMap0, texCoord);
    vec4 blend1 = texture(blendMap1, texCoord);
    for (int i = 0; i < 4; ++i) f[1 + i] = blend0[i];
    for (int i = 0; i < 4; ++i) f[5 + i] = blend1[i];

    for (int i = 0; i < 8; ++i) {
        float fOld = f[i + 1];
        f[i + 1] = mix(f[i], 1.0f, f[i + 1]);
        n[i + 1] = mix(f[i] * n[i], n[i + 1], fOld) / f[i + 1];
    }

    n[8].y = 1.0f - n[8].y;
    n[8].xyz = normalize(n[8].xyz * 2.0f - 1.0f);

    gNormalSpec.xyz = normalize(TBN * n[8].xyz);
    gNormalSpec.w = n[8].w;

    gDepth.x = gl_FragCoord.z;
    gDepth.y = 0.0f;
}
