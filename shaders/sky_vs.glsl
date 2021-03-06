#version 330 core
in vec4 vertex;
in vec4 uv0;

out vec2 TexCoord;
out vec3 FragPos;

uniform mat4 world;
uniform mat4 worldViewProj;

void main() {
    gl_Position = worldViewProj * vertex;
    TexCoord = uv0.xy;
    FragPos = vec3(world * vertex);
}
