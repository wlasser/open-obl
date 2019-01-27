#version 330 core
in vec4 vertex;
in vec4 blendIndices;
in vec4 blendWeights;
in vec3 normal;
in vec4 colour;
in vec4 uv0;
in vec3 tangent;
in vec3 binormal;

out mat3 TBN;
out vec2 TexCoord;
out vec3 FragPos;
out vec3 ViewPos;
out vec3 VertexCol;

uniform mat4 viewProj;
uniform mat4 worldInverseTranspose;
uniform mat4 world;
uniform vec4 viewPos;
uniform mat4x3 worldMatrixArray[100];

void main() {
    vec3 blendVertex = vec3(0.0f);
    vec3 blendNormal = vec3(0.0f);
    vec3 blendTangent = vec3(0.0f);
    vec3 blendBinormal = vec3(0.0f);

    for (int i = 0; i < 4; ++i) {
        int index = int(blendIndices[i]);
        float weight = blendWeights[i];
        blendVertex += weight * worldMatrixArray[index] * vertex;

        mat3 worldRotMatrix = mat3(worldMatrixArray[index]);
        blendNormal += weight * worldRotMatrix * normal;
        blendTangent += weight * worldRotMatrix * tangent;
        blendBinormal += weight * worldRotMatrix * binormal;
    }

    gl_Position = viewProj * vec4(blendVertex, 1.0f);

    vec3 N = vec3(worldInverseTranspose * vec4(blendNormal, 0.0f));
    vec3 B = vec3(worldInverseTranspose * vec4(blendBinormal, 0.0f));
    vec3 T = vec3(worldInverseTranspose * vec4(blendTangent, 0.0f));
    TBN = mat3(T, B, N);

    TexCoord = uv0.xy;
    FragPos = vec3(world * vertex);
    ViewPos = viewPos.xyz;
    VertexCol = colour.rgb;
}
