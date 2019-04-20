#version 330 core
in vec2 TexCoord;
in vec3 FragPos;
in vec3 ViewPos;
in vec4 ScreenPos;

uniform vec4 ambientLightColor;

uniform sampler2D diffuse;
uniform sampler2D Tex0;
uniform sampler2D Tex1;
uniform sampler2D Tex2;

out vec4 FragColor;

void main() {
    vec4 homScreenPos = ScreenPos / ScreenPos.w;
    vec2 uv = vec2(homScreenPos.x, homScreenPos.y) * 0.5f + 0.5f;

    vec3 bump = texture(diffuse, TexCoord).xyz;

    vec4 basePos = texture(Tex0, uv);
    float baseDepth = clamp(length(FragPos - basePos.xyz) / 5.0f, 0.0f, 1.0f);

    vec2 refractedUv = uv + bump.xy * 0.025f * baseDepth;

    vec4 refractedPos = texture(Tex0, refractedUv);
    vec3 refractedCol = texture(Tex2, refractedUv).rgb * ambientLightColor.rgb;

    if (refractedPos.w < homScreenPos.z) {
        refractedPos = basePos;
        refractedCol = texture(Tex2, uv).rgb * ambientLightColor.rgb;
    }
    float depth = clamp(length(FragPos - refractedPos.xyz) / 5.0f, 0.0f, 1.0f);

    vec3 viewDir = normalize(ViewPos - FragPos);
    float nDotL = max(dot(viewDir, normalize(bump.xyz)), 0.0f);
    float facing = 1.0f - nDotL;
    float fresnelBias = 0.2f;
    float fresnelExponent = 5.0f;
    float fresnel = max(mix(pow(facing, fresnelExponent), 1.0f, fresnelBias), 0.0f);

    vec3 baseCol = vec3(0, 0.15, 0.115);
    vec3 deepBaseCol = vec3(0, 0.065, 0.075);
    vec3 deepCol = mix(refractedCol, deepBaseCol, depth);
    vec3 waterCol = mix(baseCol, deepCol, facing);

    FragColor = vec4(waterCol, 1.0f);
}
