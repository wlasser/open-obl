#version 330 core
in vec2 TexCoord;
in vec3 FragPos;
in vec3 ViewPos;

uniform int numBlendMaps;
uniform sampler2D vertexColor;
uniform sampler2D blendMap0;
uniform sampler2D blendMap1;

uniform sampler2D diffuse0;
uniform sampler2D diffuse1;
uniform sampler2D diffuse2;
uniform sampler2D diffuse3;
uniform sampler2D diffuse4;
uniform sampler2D diffuse5;
uniform sampler2D diffuse6;
uniform sampler2D diffuse7;
uniform sampler2D diffuse8;

out vec4 FragColor;

void main () {
    float gamma = 2.2f;

    // See landscape_fs.glsl
    vec2 texCoord = vec2(TexCoord.x, 1.0f - TexCoord.y);
    vec2 uv = texCoord * 17.0f;

    // Undo gamma correction of textures so it is correct later
    vec3 dc[9];
    dc[0] = pow(texture(diffuse0, uv).rgb, vec3(gamma));
    dc[1] = pow(texture(diffuse1, uv).rgb, vec3(gamma));
    dc[2] = pow(texture(diffuse2, uv).rgb, vec3(gamma));
    dc[3] = pow(texture(diffuse3, uv).rgb, vec3(gamma));
    dc[4] = pow(texture(diffuse4, uv).rgb, vec3(gamma));
    dc[5] = pow(texture(diffuse5, uv).rgb, vec3(gamma));
    dc[6] = pow(texture(diffuse6, uv).rgb, vec3(gamma));
    dc[7] = pow(texture(diffuse7, uv).rgb, vec3(gamma));
    dc[8] = pow(texture(diffuse8, uv).rgb, vec3(gamma));

    // Blend factors
    float f[9];
    f[0] = 1.0f;
    for (int i = 0; i < 4; ++i) f[1 + i] = texture(blendMap0, texCoord)[i];
    if (numBlendMaps > 1) {
        for (int i = 0; i < 4; ++i) f[5 + i] = texture(blendMap1, texCoord)[i];
    } else {
        for (int i = 0; i < 4; ++i) f[5 + i] = 0.0f;
    }

    // Blend the diffuse layers
    float f_[9];
    vec3 dc_[9];

    f_[0] = f[0];
    dc_[0] = dc[0];

    for (int i = 0; i < 8; ++i) {
        f_[i + 1] = f[i + 1] + f_[i] * (1.0f - f[i + 1]);
        dc_[i + 1] = (f[i + 1] * dc[i + 1] + f_[i] * dc_[i] * (1.0f - f[i + 1])) / f_[i + 1];
    }

    dc_[8] *= texture(vertexColor, texCoord).rgb;
    // Leave the gamma, no point correcting it then immediately undoing it in
    // the lod shader
    FragColor = vec4(dc_[8], 1.0f);
}
