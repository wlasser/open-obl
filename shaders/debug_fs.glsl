#version 330 core
in vec3 VertexCol;

void main() {
    gl_FragColor = vec4(VertexCol, 1.0f);
}
