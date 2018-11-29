#version 330 core
in vec3 VertexCol;
out vec4 FragColor;

void main() {
    FragColor = vec4(VertexCol, 1.0f);
}
