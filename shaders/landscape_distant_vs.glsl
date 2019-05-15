#version 330 core
in vec4 vertex;
in vec4 colour;
in vec4 uv0;

out vec2 TexCoord;
out vec3 FragPos;
out vec3 ViewPos;

uniform mat4 world;
uniform mat4 viewProj;
uniform vec4 viewPos;
uniform int gridDistantCount;

#define CELLS_PER_METER 0.01708771f

void main() {
    vec4 worldPos = world * vertex;

    vec3 cellDelta = abs(worldPos - viewPos).xyz * CELLS_PER_METER;
    float cellDeltaLInf = max(max(cellDelta.x, cellDelta.y), cellDelta.z);
    worldPos.y -= clamp(gridDistantCount - cellDeltaLInf, 0, 32);

    gl_Position = viewProj * worldPos;
    TexCoord = uv0.xy;
    FragPos = worldPos.xyz;
    ViewPos = viewPos.xyz;
}
