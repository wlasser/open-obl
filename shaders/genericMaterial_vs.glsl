#version 330 core
in vec4 vertex;
in vec3 normal;
in vec4 colour;
in vec4 uv0;
in vec3 tangent;
in vec3 binormal;

out mat3 TBN;
out vec2 TexCoord;
out vec3 FragPos;
out vec3 VertexCol;

uniform mat4 worldViewProj;
uniform mat4 worldInverseTranspose;
uniform mat4 world;

void main() {
    gl_Position = worldViewProj * vertex;
    vec3 T = normalize(vec3(worldInverseTranspose * vec4(tangent, 0.0f)));
    vec3 B = normalize(vec3(worldInverseTranspose * vec4(binormal, 0.0f)));
    vec3 N = normalize(vec3(worldInverseTranspose * vec4(normal, 0.0f)));
    TBN = mat3(T, B, N);
    TexCoord = uv0.xy;
    FragPos = vec3(world * vertex);
    VertexCol = colour.rgb;
}