#version 330 core
in vec2 TexCoord;
in vec4 VertexCol;

uniform sampler2D diffuseMap;

void main() {
    gl_FragColor = vec4(1.0f, 1.0f, 1.0f, texture2D(diffuseMap, TexCoord)) * VertexCol;
}