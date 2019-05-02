#version 330 core
in vec2 TexCoord;

uniform sampler2D RT;
uniform sampler2D Tex0;
uniform float nearClipDist;
uniform float farClipDist;
uniform vec4 fogColor;
// x = density, y = start, z = end, w = 1 / (end - start)
uniform vec4 fogParams;

out vec4 FragColor;

void main() {
    vec3 hdrCol = texture(RT, TexCoord).rgb;
    //    vec3 ldrCol = hdrCol / (vec3(1.0f) + hdrCol);
    vec3 ldrCol = hdrCol;

    float gamma = 2.2f;

    float depth = texture(Tex0, TexCoord).x;
    float distance = nearClipDist * farClipDist / (farClipDist - depth * (farClipDist - nearClipDist));
    float fog = clamp((fogParams.z - distance) * fogParams.w, 0.0f, 1.0f);
    ldrCol = mix(pow(fogColor.rgb, vec3(gamma)), ldrCol, fog);

    vec3 finalCol = pow(ldrCol, vec3(1.0f / gamma));

    FragColor = vec4(finalCol, 1.0f);
}
