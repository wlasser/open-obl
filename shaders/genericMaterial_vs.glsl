#version 330 core
attribute vec4 vertex;
attribute vec3 normal;
attribute vec4 uv0;

out vec3 Normal;
out vec2 TexCoord;
out vec3 FragPos;
out vec3 ViewPos;

uniform mat4 worldViewProj;
uniform mat4 worldInverseTranspose;
uniform mat4 world;
uniform vec4 viewPos;

void main() {
    gl_Position = worldViewProj * vertex;
    Normal = vec3(worldInverseTranspose * vec4(normal, 0.0f));
    TexCoord = uv0.xy;
    FragPos = vec3(world * vertex);
    ViewPos = viewPos.xyz;
}