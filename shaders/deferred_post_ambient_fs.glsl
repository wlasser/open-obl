#version 330 core
uniform sampler2D Tex0;
uniform sampler2D Tex1;
uniform sampler2D Tex2;

uniform vec4 ambientLightColor;

in vec2 TexCoord;

out vec4 FragColor;

void main() {
    float gamma = 2.2f;

    float depth = texture(Tex0, TexCoord).w;
    // Sufficiently small depth buffer mean unwritten, so must be sky, which
    // we shouldn't write over.
    if (depth < 1e-4f) discard;

    vec3 a2 = texture(Tex2, TexCoord).rgb;

    vec3 diffuseColor = a2 * ambientLightColor.rgb;

    vec3 col = pow(diffuseColor, 1.0f / vec3(gamma));
    FragColor = vec4(col, 1.0f);
    gl_FragDepth = depth;
}