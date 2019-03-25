#version 330 core
in vec4 vertex;
in vec3 normal;
in vec4 uv0;// The actual uv coordinates
in vec4 uv1;// world[0]
in vec4 uv2;// world[1]
in vec4 uv3;// world[2]

out vec2 TexCoord;
out vec3 FragPos;
out vec3 ViewPos;

uniform mat4 viewProj;
uniform vec4 viewPos;

void main() {
    // Transpose of the usual world matrix
    mat4 world;
    world[0] = uv1;
    world[1] = uv2;
    world[2] = uv3;
    world[3] = vec4(0.0f, 0.0f, 0.0f, 1.0f);

    vec4 worldPos = vertex * world;
    //    vec3 worldNorm = normal * mat3(world);

    gl_Position = viewProj * worldPos;
    TexCoord = uv0.xy;
    FragPos = vec3(worldPos);
    ViewPos = viewPos.xyz;
}
