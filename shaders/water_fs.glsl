#version 330 core
in vec2 TexCoord;
in vec3 FragPos;
in vec3 ViewPos;
in vec4 ScreenPos;

uniform vec4 ambientLightColor;
uniform vec4 sunlightColor;
uniform vec4 sunlightDirection;

uniform sampler2D diffuse;
uniform sampler2D Tex0;
uniform sampler2D Tex1;
uniform sampler2D Tex2;

out vec4 FragColor;

void main() {
    vec4 homScreenPos = ScreenPos / ScreenPos.w;
    vec2 uv = homScreenPos.xy * 0.5f + 0.5f;

    vec3 lightCol = pow(sunlightColor.rgb, vec3(2.2f));
    vec3 lightDir = normalize(sunlightDirection.xyz);

    vec3 shallowCol = vec3(2.0f, 21.0f, 30.0f) / 255.0f * ambientLightColor.rgb;
    vec3 deepCol = vec3(32.0f, 46.0f, 53.0f) / 255.0f * ambientLightColor.rgb;
    vec3 reflectCol = vec3(31.0f, 68.0f, 75.0f) / 255.0f * ambientLightColor.rgb;

    float fresnelAmount = 0.15f;
    float reflectivityAmount = 0.9f;

    // Swizzle to make blue component the up component.
    vec3 normal = normalize(texture(diffuse, TexCoord).xzy);

    vec2 refractedUv = uv + normal.xz * 0.01f;

    vec4 bedPos = texture(Tex0, refractedUv);
    vec3 bedNormal = texture(Tex1, refractedUv).xyz;
    vec3 bedAlbedo = texture(Tex2, refractedUv).rgb;

    if (bedPos.w < homScreenPos.z) {
        bedPos = texture(Tex0, uv);
        bedNormal = texture(Tex1, uv).xyz;
        bedAlbedo = texture(Tex2, uv).rgb;
    }

    // Apply sunlight diffuse to underwater objects.
    float diff = max(dot(bedNormal, lightDir), 0.0f);
    vec3 bedCol = bedAlbedo.rgb * (ambientLightColor.rgb + diff * lightCol.rgb);

    float depth = length(FragPos - bedPos.xyz);

    float ior = 1.3333f;
    float R0 = pow((1.0f - ior) / (1.0f + ior), 2);

    vec3 viewDir = normalize(ViewPos - FragPos);
    vec3 halfwayDir = normalize(lightDir + viewDir);
    float nDotH = max(dot(normal, halfwayDir), 0.0f);
    float R = mix(pow(1.0f - nDotH, 5.0f), 1.0f, R0);

    float spec = pow(nDotH, 80.0f);
    vec3 specCol = fresnelAmount * R * reflectCol
    + 0.5f * reflectivityAmount * spec * lightCol;

    float distScale = clamp(2.0f / depth, 0.0f, 1.0f);
    vec3 deepWaterCol = mix(deepCol, bedCol, distScale);
    //    vec3 waterCol = mix(deepWaterCol, shallowCol, nDotH);
    //    vec3 depthAdjustedCol = mix(waterCol, bedCol, distScale);

    vec3 finalCol = deepWaterCol + specCol;
    FragColor = vec4(finalCol, 1.0f);
}
