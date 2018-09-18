#version 330 core
attribute vec4 vertex;
attribute vec3 normal;
attribute vec4 colour;
attribute vec4 uv0;
attribute vec3 tangent;
attribute vec3 binormal;

out mat3 TBN;
out vec2 TexCoord;
out vec3 FragPos;
out vec3 ViewPos;
out vec3 VertexCol;

uniform mat4 worldViewProj;
uniform mat4 worldInverseTranspose;
uniform mat4 world;
uniform vec4 viewPos;

void main() {
    gl_Position = worldViewProj * vertex;
    vec3 N = vec3(worldInverseTranspose * vec4(normal, 0.0f));
    vec3 B = vec3(worldInverseTranspose * vec4(binormal, 0.0f));
    vec3 T = vec3(worldInverseTranspose * vec4(tangent, 0.0f));
    TBN = mat3(T, B, N);
    TexCoord = uv0.xy;
    FragPos = vec3(world * vertex);
    ViewPos = viewPos.xyz;
    VertexCol = colour.rgb;
}