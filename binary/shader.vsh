#version 330 core

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 color;
out vec3 vertexColor;
uniform vec2 look;

vec3 rotateVector(vec3 v, vec3 axis, float angle) {
	float c = cos(angle);
	float s = sin(angle);

	vec3 rotatedVec = v * c +
	                  cross(axis, v) * s +
	                  axis * dot(axis, v) * (1.0 - c);

	return rotatedVec;
}

const float PI = 3.14159265358979323846;

void main() {
	vec3 pos = position;

	pos = rotateVector(pos, vec3(-1.0, 0.0, 0.0), PI);

	// Rotation around Y-axis (pitch)
	pos = rotateVector(pos, vec3(0.0, -1.0, 0.0), radians(look.x));

	// Rotation around X-axis (yaw)
	pos = rotateVector(pos, vec3(-1.0, 0.0, 0.0), radians(look.y));

	gl_Position = vec4(pos, 1.0);

	vertexColor = color;
}
