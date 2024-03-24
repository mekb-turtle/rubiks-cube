#version 330 core

in vec3 fColor;
in vec2 fTexCoords;
in vec3 fPosition;
in vec3 fNormal;

out vec4 fragColor;

void main() {
		fragColor = vec4(fColor, 1.0);
}
