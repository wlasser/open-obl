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

uniform mat4 worldViewProj;
uniform mat4 worldInverseTranspose;
uniform mat4 world;
uniform vec4 viewPos;
uniform mat4x3 worldMatrixArray[100];

void main() {
    vec3 newVertex = vec3(0.0f, 0.0f, 0.0f);
    vec3 newNormal = vec3(0.0f, 0.0f, 0.0f);
    vec3 newTangent = vec3(0.0f, 0.0f, 0.0f);
    vec3 newBinormal = vec3(0.0f, 0.0f, 0.0f);
    for (int i = 0; i < 4; ++i) {
        int index = int(blendIndices[i]);
        float weight = blendWeights[i];
        newVertex += blendWeights[i] * worldMatrixArray[index] * vertex;
        newNormal += blendWeights[i] * worldMatrixArray[index] * vec4(normal, 0.0f);
        newTangent += blendWeights[i] * worldMatrixArray[index] * vec4(tangent, 0.0f);
        newBinormal += blendWeights[i] * worldMatrixArray[index] * vec4(binormal, 0.0f);
    }
    gl_Position = worldViewProj * vec4(newVertex, 1.0f);
    vec3 N = vec3(worldInverseTranspose * vec4(newNormal, 0.0f));
    vec3 B = vec3(worldInverseTranspose * vec4(newBinormal, 0.0f));
    vec3 T = vec3(worldInverseTranspose * vec4(newTangent, 0.0f));
    TBN = mat3(T, B, N);
    TexCoord = uv0.xy;
    FragPos = vec3(world * vertex);
    ViewPos = viewPos.xyz;
    VertexCol = colour.rgb;
}
